////////////////////////////////
//~ bgz: video: the player
//
// See video.h. Nothing here knows what decodes: that is video_decoder.h.
//
// A thread decodes ahead of what is shown, into a few frames that wait in a
// queue. The main thread, once a frame (VID_Update), moves the clock on and
// takes from the queue the frame whose time has come:
//
//    decode thread                         main thread
//    VID_DecoderNextFrame --> [queue] --> VID_Update --> texture
//
// The queue is all the two share, and it is under the mutex. The decode
// thread writes a frame into the queue's next free place without the mutex
// - nobody else touches a free place - and takes it only to say the frame
// is there.
//
// A seek is asked for by number (seek_serial). The decode thread, when it
// sees a number that is not the one it is on, empties the queue, seeks the
// decoder and throws away what it decodes from before the time asked for.

#define _VID_QUEUE_CAP 4

typedef struct _VID_Frame _VID_Frame;
struct _VID_Frame {
   U8* pixels; // the video's size, RGBA: on the video's arena
   F64 time_s;
};

struct VID_Video {
   Arena*          arena;
   String8         path;
   VID_Decoder*    decoder;
   VID_DecoderInfo info;
   R_Handle        texture;

   Thread  thread;
   Mutex   mutex;
   CondVar wake_decode; // room in the queue, a seek, or quit

   //- bgz: under the mutex
   _VID_Frame queue[_VID_QUEUE_CAP];
   U64        queue_first;
   U64        queue_count;
   B32        quit;
   U64        seek_serial; // goes up with every seek asked for
   F64        seek_time_s;
   B32        at_end; // the decoder has no more frames, since the last seek

   //- bgz: the main thread's
   B32 playing;
   F64 position_s;
   U64 updated_us;  // when VID_Update last ran. 0: not yet
   B32 need_frame;  // show the next frame decoded, whatever its time: the first, and after a seek
   B32 ended;       // stopped at its end: playing starts it over
};

internal B32
VID_NameIsVideo(String8 name) {
   B32 result = 0;

   String8 extensions[] = {Str8Lit("mp4"), Str8Lit("m4v"), Str8Lit("mov"), Str8Lit("mkv"), Str8Lit("webm"),
                           Str8Lit("avi"), Str8Lit("wmv"), Str8Lit("flv"), Str8Lit("mpg"), Str8Lit("mpeg"),
                           Str8Lit("ts"), Str8Lit("m2ts"), Str8Lit("mts"), Str8Lit("3gp"), Str8Lit("ogv"),
                           Str8Lit("vob")};

   String8 extension = Str8SkipLastDot(name);
   for (U64 i = 0; i < ArrayCount(extensions); i += 1) {
      if (Str8Match(extension, extensions[i], StringMatchFlag_CaseInsensitive)) {
         result = 1;
         break;
      }
   }

   return result;
}

internal void
_VID_DecodeThread(void* p) {
   VID_Video* video = (VID_Video*)p;
   SetThreadNameF("video decode");

   U64 serial = 0;      // the seek this thread is on
   F64 skip_before = 0; // frames from before this are thrown away

   for (;;) {
      //- bgz: wait until there is something to do
      B32 do_seek = 0;
      F64 seek_time_s = 0;
      U8* pixels = 0;
      MutexTake(video->mutex);
      while (!video->quit && video->seek_serial == serial && (video->queue_count == _VID_QUEUE_CAP || video->at_end)) {
         CondVarWait(video->wake_decode, video->mutex, max_U64);
      }
      B32 quit = video->quit;
      if (!quit) {
         if (video->seek_serial != serial) {
            serial = video->seek_serial;
            seek_time_s = video->seek_time_s;
            do_seek = 1;
            video->queue_count = 0;
            video->at_end = 0;
         }
         // The next free place. It stays free, and this thread's, until the
         // count goes up below: the main thread only takes from the front.
         pixels = video->queue[(video->queue_first + video->queue_count) % _VID_QUEUE_CAP].pixels;
      }
      MutexDrop(video->mutex);
      if (quit) {
         break;
      }

      //- bgz: decode one frame
      if (do_seek) {
         VID_DecoderSeek(video->decoder, seek_time_s);
         skip_before = seek_time_s;
      }
      F64 time_s = 0;
      B32 got_frame = VID_DecoderNextFrame(video->decoder, pixels, &time_s);

      // Half a frame's leeway: the frame that is showing at the time asked
      // for began a little before it.
      F64 frame_s = (video->info.frames_per_second > 0) ? 1.0 / video->info.frames_per_second : 0.04;
      if (got_frame && time_s + frame_s * 0.5 < skip_before) {
         continue;
      }

      //- bgz: say it is there - unless a seek was asked for meanwhile
      MutexTake(video->mutex);
      if (video->seek_serial == serial) {
         if (got_frame) {
            // A seek the main thread asked for and this thread has not seen
            // would have changed seek_serial, so the place is still the one
            // written to.
            video->queue[(video->queue_first + video->queue_count) % _VID_QUEUE_CAP].time_s = time_s;
            video->queue_count += 1;
         } else {
            video->at_end = 1;
         }
      }
      MutexDrop(video->mutex);
   }
}

internal VID_Video*
VID_Open(String8 path) {
   VID_Video* result = 0;

   Arena*          arena = ArenaAlloc();
   VID_DecoderInfo info = {0};
   VID_Decoder*    decoder = VID_DecoderOpen(arena, path, VID_MAX_DIM, &info);
   if (decoder != 0 && (info.size.x <= 0 || info.size.y <= 0)) {
      LogUserErrorF("%S has no picture to show\n", path);
      VID_DecoderClose(decoder);
      decoder = 0;
   }

   if (decoder == 0) {
      ArenaRelease(arena);
   } else {
      VID_Video* video = PushArray(arena, VID_Video, 1);
      video->arena = arena;
      video->path = PushStr8Copy(arena, path);
      video->decoder = decoder;
      video->info = info;

      U64 frame_size = (U64)info.size.x * (U64)info.size.y * 4;
      for (U64 i = 0; i < _VID_QUEUE_CAP; i += 1) {
         video->queue[i].pixels = PushArrayNoZero(arena, U8, frame_size);
      }

      // Black, not see-through, until the first frame is in. The first
      // place in the queue is borrowed for it: the thread is not started yet.
      U8* black = video->queue[0].pixels;
      MemoryZero(black, frame_size);
      for (U64 i = 3; i < frame_size; i += 4) {
         black[i] = 255;
      }
      video->texture = Core_R_Tex2DAlloc(R_ResourceKind_Dynamic, info.size, R_Tex2DFormat_RGBA8, black);

      video->need_frame = 1;
      video->mutex = MutexAlloc();
      video->wake_decode = CondVarAlloc();
      video->thread = ThreadLaunch(_VID_DecodeThread, video);

      LogInfoF("Video %S: %d x %d, %.1f s, %.2f frames a second\n", path, info.size.x, info.size.y, info.duration_s,
               info.frames_per_second);
      result = video;
   }

   return result;
}

internal void
VID_Close(VID_Video* video) {
   if (video == 0) {
      return;
   }

   MutexTake(video->mutex);
   video->quit = 1;
   MutexDrop(video->mutex);
   CondVarBroadcast(video->wake_decode);
   ThreadJoin(video->thread, max_U64);

   VID_DecoderClose(video->decoder);
   Core_R_Tex2DRelease(video->texture);
   CondVarRelease(video->wake_decode);
   MutexRelease(video->mutex);
   ArenaRelease(video->arena);
}

internal R_Handle
VID_Texture(VID_Video* video) {
   return video->texture;
}

internal Vec2S32
VID_Size(VID_Video* video) {
   return video->info.size;
}

internal F64
VID_Duration(VID_Video* video) {
   return video->info.duration_s;
}

internal F64
VID_Position(VID_Video* video) {
   return video->position_s;
}

internal B32
VID_IsPlaying(VID_Video* video) {
   return video->playing;
}

internal void
VID_Seek(VID_Video* video, F64 time_s) {
   if (video->info.duration_s > 0) {
      time_s = Min(time_s, video->info.duration_s);
   }
   time_s = Max(time_s, 0.0);

   MutexTake(video->mutex);
   video->seek_serial += 1;
   video->seek_time_s = time_s;
   // What waits is from before the seek: never shown.
   video->queue_count = 0;
   video->at_end = 0;
   MutexDrop(video->mutex);
   CondVarBroadcast(video->wake_decode);

   video->position_s = time_s;
   video->need_frame = 1;
   video->ended = 0;
}

internal void
VID_SetPlaying(VID_Video* video, B32 playing) {
   if (playing && video->ended) {
      VID_Seek(video, 0);
   }
   video->playing = playing;
}

internal void
VID_Update(VID_Video* video) {
   U64 now_us = NowTimeUS();
   F64 dt_s = (video->updated_us != 0) ? (F64)(now_us - video->updated_us) / 1000000.0 : 0;
   video->updated_us = now_us;
   // A long gap - the window dragged, the program held up - is not played
   // through at once.
   dt_s = Min(dt_s, 0.1);

   MutexTake(video->mutex);

   // The clock stands still while there is nothing to show: waiting for the
   // first frame after a seek, or for a decoder that can't keep up. A video
   // too heavy for the machine plays slowly rather than in jumps.
   B32 is_waiting = (video->need_frame || (video->queue_count == 0 && !video->at_end));
   if (video->playing && !is_waiting) {
      video->position_s += dt_s;
   }

   // The latest frame whose time has come. Ones before it were due and are
   // passed over.
   U8* due = 0;
   while (video->queue_count != 0) {
      _VID_Frame* frame = &video->queue[video->queue_first];
      if (!video->need_frame && frame->time_s > video->position_s) {
         break;
      }
      due = frame->pixels;
      if (video->need_frame) {
         video->need_frame = 0;
         // From the frame there is: a seek lands on a frame, not between two.
         video->position_s = Max(video->position_s, frame->time_s);
      }
      video->queue_first = (video->queue_first + 1) % _VID_QUEUE_CAP;
      video->queue_count -= 1;
   }
   if (due != 0) {
      // Before the mutex is let go: the place is free for the decode thread
      // to write over as soon as it is.
      Core_R_FillTex2DRegion(video->texture, R2S32(V2S32(0, 0), video->info.size), due);
   }

   if (video->queue_count == 0 && video->at_end) {
      // Nothing waiting and nothing to come.
      video->need_frame = 0;
      if (video->playing) {
         video->playing = 0;
         video->ended = 1;
      }
   }

   MutexDrop(video->mutex);
   if (due != 0) {
      CondVarBroadcast(video->wake_decode);
   }
}

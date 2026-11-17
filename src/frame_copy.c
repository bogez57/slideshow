////////////////////////////////
//~ bgz: files copied into the deck's folder, in the background
//
// A video dropped on the edit screen belongs in the deck's folder, with the
// deck's other pictures, and a video can be very large. So it is not waited
// for:
//
//  1. The slide gets its picture line at once, naming the file as it will
//     be in the deck's folder.
//  2. Until the file is there, the deck is told that its path stands for
//     the original's (DECK_SetPathAlias), and the video plays from where it
//     was dropped from.
//  3. A thread copies the file, a piece at a time, under another name
//     (name.copying), and gives it its real name when it is whole: a copy
//     that is cut short never looks like the video.
//  4. When the copy is whole and the video is not playing, the slide is
//     moved over to it, at the point the video was at. A video that is
//     playing is left to play: it is moved over when it is paused or ends.
//
// A copy that fails leaves the video playing from where it is, and says so.
// Closing the program while one is still going asks once, as unsaved changes
// do: the picture line would name a file that is not there.

#define _COPY_PIECE_SIZE MB(4)

struct COPY_Job {
   COPY_Job* next;
   Arena*    arena;
   String8   src; // the original
   String8   dst; // in the deck's folder
   String8   tmp; // what it is called there until it is whole
   Thread    thread;
   Mutex     mutex;

   //- bgz: under the mutex
   B32 cancel; // asked to stop
   B32 done;   // the thread is through, one way or the other
   B32 ok;     // dst is there, whole

   //- bgz: the main thread's
   B32 said_waiting; // that the copy is whole and waits for the video to stop
};

internal void
_COPY_Thread(void* p) {
   COPY_Job* job = (COPY_Job*)p;
   SetThreadNameF("file copy");

   Arena* arena = ArenaAlloc();
   // Shared: the video is played from the original while it is copied, and
   // the original is not ours to lock.
   File   in = FileOpen(AccessFlag_Read | AccessFlag_ShareRead | AccessFlag_ShareWrite, job->src);
   File   out = FileOpen(AccessFlag_Write, job->tmp);
   B32    ok = (!FileMatch(in, FileZero()) && !FileMatch(out, FileZero()));
   B32    cancelled = 0;

   if (ok) {
      U64 size = PropertiesFromFile(in).size;
      for (U64 off = 0; off < size && ok;) {
         MutexTake(job->mutex);
         cancelled = job->cancel;
         MutexDrop(job->mutex);
         if (cancelled) {
            ok = 0;
            break;
         }

         ArenaClear(arena);
         String8 piece = StringFromFileRange(arena, in, R1U64(off, Min(off + _COPY_PIECE_SIZE, size)));
         if (piece.size == 0 || FileWrite(out, R1U64(off, off + piece.size), piece.str) != piece.size) {
            ok = 0;
         }
         off += piece.size;
      }
   }

   if (!FileMatch(in, FileZero())) {
      FileClose(in);
   }
   if (!FileMatch(out, FileZero())) {
      FileClose(out);
   }
   if (ok) {
      ok = ReplaceFilePath(job->dst, job->tmp);
   }
   if (!ok) {
      DeleteFileAtPath(job->tmp);
   }
   ArenaRelease(arena);

   MutexTake(job->mutex);
   job->ok = ok;
   job->done = 1;
   MutexDrop(job->mutex);
}

// Whether dst is being copied to already.
internal B32
COPY_IsCopyingTo(App_State* app_state, String8 dst) {
   B32 result = 0;
   for (COPY_Job* job = app_state->first_copy; job != 0; job = job->next) {
      if (Str8Match(job->dst, dst, StringMatchFlag_CaseInsensitive)) {
         result = 1;
         break;
      }
   }
   return result;
}

// Starts copying src to dst, and has dst stand for src in the deck until
// the copy has taken over.
internal void
COPY_Start(App_State* app_state, String8 src, String8 dst) {
   Arena*    arena = ArenaAlloc();
   COPY_Job* job = PushArray(arena, COPY_Job, 1);
   job->arena = arena;
   job->src = PushStr8Copy(arena, src);
   job->dst = PushStr8Copy(arena, dst);
   job->tmp = PushStr8F(arena, "%S.copying", dst);
   job->mutex = MutexAlloc();

   DECK_SetPathAlias(job->dst, job->src);

   job->next = app_state->first_copy;
   app_state->first_copy = job;
   job->thread = ThreadLaunch(_COPY_Thread, job);
   LogInfoF("Copying %S to %S\n", src, dst);
}

// Whether a copy is still being made.
internal B32
COPY_IsBusy(App_State* app_state) {
   B32 result = 0;
   for (COPY_Job* job = app_state->first_copy; job != 0; job = job->next) {
      MutexTake(job->mutex);
      B32 done = job->done;
      MutexDrop(job->mutex);
      if (!done) {
         result = 1;
         break;
      }
   }
   return result;
}

internal void
_COPY_Release(COPY_Job* job) {
   ThreadJoin(job->thread, max_U64);
   MutexRelease(job->mutex);
   ArenaRelease(job->arena);
}

// Once a frame: a copy that is through is taken over to, if its video is not
// playing, or given up on.
internal void
COPY_Update(App_State* app_state) {
   DECK_Deck* deck = app_state->deck;

   COPY_Job** link = &app_state->first_copy;
   for (COPY_Job* job = *link; job != 0; job = *link) {
      MutexTake(job->mutex);
      B32 done = job->done;
      B32 ok = job->ok;
      MutexDrop(job->mutex);

      B32 is_finished = 0; // nothing more to do for it
      if (done && !ok) {
         // The alias stays: the video goes on playing from where it is, for
         // as long as the program runs.
         LogUserErrorF("Could not copy %S to %S\n", job->src, job->dst);
         FR_Notice(app_state, Str8Lit("Could not copy the video into the deck's folder. It plays from the original."));
         is_finished = 1;
      } else if (done) {
         B32 is_playing = 0;
         for (U64 i = 0; i < deck->slide_count; i += 1) {
            DECK_Slide* slide = &deck->slides[i];
            if (slide->video != 0 && VID_IsPlaying(slide->video) &&
                Str8Match(slide->image_path, job->dst, StringMatchFlag_CaseInsensitive)) {
               is_playing = 1;
               break;
            }
         }

         if (is_playing) {
            if (!job->said_waiting) {
               job->said_waiting = 1;
               LogInfoF("%S is copied. The slide moves over to it when its video stops\n", job->dst);
            }
         } else {
            // From here on the path is its own. Every slide showing the
            // file is opened again, on the copy, where it was.
            DECK_ClearPathAlias(job->dst);
            for (U64 i = 0; i < deck->slide_count; i += 1) {
               DECK_Slide* slide = &deck->slides[i];
               // One that could not be opened from the original is tried
               // again too, now that the file is where the slide says.
               if ((slide->video != 0 || slide->load_failed) &&
                   Str8Match(slide->image_path, job->dst, StringMatchFlag_CaseInsensitive)) {
                  F64 position_s = (slide->video != 0) ? VID_Position(slide->video) : 0;
                  DECK_ReopenImage(deck, i);
                  if (slide->video != 0 && position_s > 0) {
                     VID_Seek(slide->video, position_s);
                  }
               }
            }
            LogInfoF("%S is copied, and used from now on\n", job->dst);
            FR_Notice(app_state, Str8Lit("Video copied into the deck's folder"));
            app_state->pictures_scanned_us = 0; // the list of pictures, if open, is out of date
            is_finished = 1;
         }
      }

      if (is_finished) {
         *link = job->next;
         _COPY_Release(job);
      } else {
         link = &job->next;
      }
   }
}

// At exit: every copy still going is stopped, and what it had written is
// deleted.
internal void
COPY_CancelAll(App_State* app_state) {
   for (COPY_Job* job = app_state->first_copy, *next = 0; job != 0; job = next) {
      next = job->next;
      MutexTake(job->mutex);
      job->cancel = 1;
      MutexDrop(job->mutex);
      _COPY_Release(job);
   }
   app_state->first_copy = 0;
}

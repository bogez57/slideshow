////////////////////////////////
//~ bgz: video: the decoder, done with FFmpeg
//
// The five functions of video_decoder.h. This is the only file in the
// program that includes FFmpeg's headers or calls it: to decode with
// something else, write another file with the same five functions and
// include that one in main.c instead.
//
// FFmpeg is src\third_party\ffmpeg: its headers, the import libraries the
// program links against, and the DLLs, which build.bat copies next to the
// exe. It is the LGPL build, used as it comes: nothing of it is compiled
// into the program.
//
//    libavformat   opens the file and hands out its packets
//    libavcodec    decodes the video stream's packets into frames
//    libswscale    turns a frame, in whatever form the codec has it, into
//                  RGBA at the size asked for
//    libavutil     what the three share
//
// Decoding is on the processor, on as many threads as FFmpeg sees fit. The
// graphics card's decoders are not used.
//
// FFmpeg's memory is its own (malloc): its libraries are DLLs with no way
// to be given an arena. Everything it allocates here is let go of in
// VID_DecoderClose.

// FFmpeg's headers are found through -Isrc\third_party\ffmpeg\include
// (build.bat): they include each other from there. They have fields named
// internal and global, which here are words for static: put aside while the
// headers are read.
#pragma push_macro("internal")
#pragma push_macro("global")
#undef internal
#undef global
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Weverything"
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/error.h"
#include "libswscale/swscale.h"
#if defined(__clang__)
# pragma clang diagnostic pop
#endif
#pragma pop_macro("global")
#pragma pop_macro("internal")

struct VID_Decoder {
   String8          path; // to say which file, when something goes wrong
   AVFormatContext* format;
   AVCodecContext*  codec;
   int              stream_idx;
   AVPacket*        packet;
   AVFrame*         frame;
   struct SwsContext* scale;

   Vec2S32 size;         // of the frames given out
   F64     seconds_per_tick; // the stream's time base
   S64     start_ticks;  // the time of the stream's first frame, which is given out as 0
   F64     frame_s;      // how long a frame lasts, if the file says. Else 0
   F64     last_time_s;  // of the last frame given out
   B32     is_draining;  // the file's packets are all read: the codec is giving up what it holds
};

// What FFmpeg has to say about one of its error codes.
internal String8
_VID_FFmpegError(Arena* arena, int code) {
   char text[AV_ERROR_MAX_STRING_SIZE] = {0};
   av_strerror(code, text, sizeof(text));
   String8 result = PushStr8Copy(arena, Str8CString(text));
   return result;
}

internal void
VID_DecoderClose(VID_Decoder* decoder) {
   if (decoder == 0) {
      return;
   }
   // Each of these takes a pointer to nothing in its stride.
   sws_freeContext(decoder->scale);
   av_frame_free(&decoder->frame);
   av_packet_free(&decoder->packet);
   avcodec_free_context(&decoder->codec);
   avformat_close_input(&decoder->format);
}

internal VID_Decoder*
VID_DecoderOpen(Arena* arena, String8 path, S32 max_dim, VID_DecoderInfo* info_out) {
   VID_Decoder* decoder = PushArray(arena, VID_Decoder, 1);
   decoder->path = PushStr8Copy(arena, path);
   decoder->stream_idx = -1;

   // Only what goes wrong, to the console. Why a file can't be played is
   // logged below, for the program's own log.
   av_log_set_level(AV_LOG_ERROR);

   B32  is_open = 0;
   Temp scratch = ScratchBegin(&arena, 1);

   // FFmpeg takes the path as UTF-8, ended with a 0, on Windows too.
   String8 path_z = PushStr8Copy(scratch.arena, path);

   const AVCodec* codec = 0;
   int            error = avformat_open_input(&decoder->format, (char*)path_z.str, 0, 0);
   if (error < 0) {
      LogUserErrorF("Could not open %S as a video: %S\n", path, _VID_FFmpegError(scratch.arena, error));
   } else if ((error = avformat_find_stream_info(decoder->format, 0)) < 0) {
      LogUserErrorF("Could not read what is in %S: %S\n", path, _VID_FFmpegError(scratch.arena, error));
   } else if ((decoder->stream_idx = av_find_best_stream(decoder->format, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0 ||
              codec == 0) {
      LogUserErrorF("%S has no video in it that can be decoded\n", path);
   } else {
      AVStream* stream = decoder->format->streams[decoder->stream_idx];

      decoder->codec = avcodec_alloc_context3(codec);
      if (decoder->codec == 0) {
         LogUserErrorF("Out of memory opening %S\n", path);
      } else if ((error = avcodec_parameters_to_context(decoder->codec, stream->codecpar)) < 0) {
         LogUserErrorF("Could not set up the decoder for %S: %S\n", path, _VID_FFmpegError(scratch.arena, error));
      } else {
         // 0: as many threads as it sees fit.
         decoder->codec->thread_count = 0;
         error = avcodec_open2(decoder->codec, codec, 0);
         if (error < 0) {
            LogUserErrorF("Could not start the %s decoder for %S: %S\n", codec->name, path,
                          _VID_FFmpegError(scratch.arena, error));
         } else if (decoder->codec->width <= 0 || decoder->codec->height <= 0) {
            LogUserErrorF("%S does not say how big its picture is\n", path);
         } else {
            decoder->packet = av_packet_alloc();
            decoder->frame = av_frame_alloc();
            is_open = (decoder->packet != 0 && decoder->frame != 0);
            if (!is_open) {
               LogUserErrorF("Out of memory opening %S\n", path);
            }
         }
      }

      if (is_open) {
         //- bgz: how big the frames come out
         //
         // Pixels that are not square are made so: the width is the one
         // changed. Then the whole is made smaller if it is over max_dim.
         F64        width = (F64)decoder->codec->width;
         F64        height = (F64)decoder->codec->height;
         AVRational pixel_shape = av_guess_sample_aspect_ratio(decoder->format, stream, 0);
         if (pixel_shape.num > 0 && pixel_shape.den > 0) {
            width = width * (F64)pixel_shape.num / (F64)pixel_shape.den;
         }
         F64 longest = Max(width, height);
         if (max_dim > 0 && longest > (F64)max_dim) {
            width = width * (F64)max_dim / longest;
            height = height * (F64)max_dim / longest;
         }
         decoder->size = V2S32(Max((S32)(width + 0.5), 1), Max((S32)(height + 0.5), 1));

         //- bgz: time
         decoder->seconds_per_tick = av_q2d(stream->time_base);
         decoder->start_ticks = (stream->start_time != AV_NOPTS_VALUE) ? stream->start_time : 0;

         AVRational rate = av_guess_frame_rate(decoder->format, stream, 0);
         F64        frames_per_second = (rate.num > 0 && rate.den > 0) ? av_q2d(rate) : 0;
         decoder->frame_s = (frames_per_second > 0) ? 1.0 / frames_per_second : 0;

         F64 duration_s = 0;
         if (stream->duration != AV_NOPTS_VALUE && stream->duration > 0) {
            duration_s = (F64)stream->duration * decoder->seconds_per_tick;
         } else if (decoder->format->duration != AV_NOPTS_VALUE && decoder->format->duration > 0) {
            duration_s = (F64)decoder->format->duration / (F64)AV_TIME_BASE;
         }

         info_out->size = decoder->size;
         info_out->duration_s = duration_s;
         info_out->frames_per_second = frames_per_second;
      }
   }

   ScratchEnd(scratch);

   if (!is_open) {
      VID_DecoderClose(decoder);
      decoder = 0;
   }
   return decoder;
}

internal B32
VID_DecoderNextFrame(VID_Decoder* decoder, U8* pixels, F64* time_s_out) {
   B32 result = 0;

   // The codec is asked for a frame. When it wants more of the file first,
   // it is fed the next packet of the video stream and asked again - several
   // packets can go in before a frame comes out, and several frames can come
   // out of one.
   for (;;) {
      int error = avcodec_receive_frame(decoder->codec, decoder->frame);
      if (error == 0) {
         result = 1;
         break;
      }
      if (error != AVERROR(EAGAIN)) {
         // AVERROR_EOF: the end. Anything else: it can't go on.
         if (error != AVERROR_EOF) {
            Temp scratch = ScratchBegin(0, 0);
            LogUserErrorF("%S stopped decoding: %S\n", decoder->path, _VID_FFmpegError(scratch.arena, error));
            ScratchEnd(scratch);
         }
         break;
      }
      if (decoder->is_draining) {
         break; // asked for more after being told there is none
      }

      error = av_read_frame(decoder->format, decoder->packet);
      if (error < 0) {
         // The end of the file - or it can't be read further, which is the
         // end of what can be shown. A packet of nothing tells the codec to
         // give up the frames it is holding.
         decoder->is_draining = 1;
         avcodec_send_packet(decoder->codec, 0);
      } else {
         if (decoder->packet->stream_index == decoder->stream_idx) {
            // A packet it can't take is passed over: one bad one does not
            // end the video.
            avcodec_send_packet(decoder->codec, decoder->packet);
         }
         av_packet_unref(decoder->packet);
      }
   }

   if (result) {
      AVFrame* frame = decoder->frame;

      //- bgz: when it is shown
      S64 ticks = frame->best_effort_timestamp;
      F64 time_s = 0;
      if (ticks != AV_NOPTS_VALUE) {
         time_s = (F64)(ticks - decoder->start_ticks) * decoder->seconds_per_tick;
      } else {
         // The file does not say: a frame after the last.
         time_s = decoder->last_time_s + ((decoder->frame_s > 0) ? decoder->frame_s : 0.04);
      }
      time_s = Max(time_s, 0.0);
      decoder->last_time_s = time_s;
      *time_s_out = time_s;

      //- bgz: to RGBA, at the size given out
      //
      // The context is kept from frame to frame, and made again only if the
      // frames change size or form part way through, as some files' do.
      decoder->scale = sws_getCachedContext(decoder->scale, frame->width, frame->height, (enum AVPixelFormat)frame->format,
                                            decoder->size.x, decoder->size.y, AV_PIX_FMT_RGBA, SWS_BILINEAR, 0, 0, 0);
      if (decoder->scale == 0) {
         LogUserErrorF("%S has frames in a form that can't be shown\n", decoder->path);
         result = 0;
      } else {
         uint8_t* dst_planes[4] = {pixels, 0, 0, 0};
         int      dst_strides[4] = {decoder->size.x * 4, 0, 0, 0};
         sws_scale(decoder->scale, (const uint8_t* const*)frame->data, frame->linesize, 0, frame->height, dst_planes,
                   dst_strides);
      }
      av_frame_unref(frame);
   }

   return result;
}

internal void
VID_DecoderSeek(VID_Decoder* decoder, F64 time_s) {
   S64 ticks = decoder->start_ticks;
   if (decoder->seconds_per_tick > 0) {
      ticks += (S64)(time_s / decoder->seconds_per_tick);
   }

   // To the last frame that can be decoded on its own at or before the
   // time: what is between it and the time is decoded and thrown away by
   // the player.
   int error = av_seek_frame(decoder->format, decoder->stream_idx, ticks, AVSEEK_FLAG_BACKWARD);
   if (error < 0) {
      // Some files can't be sought in. From the start is all that is left.
      error = av_seek_frame(decoder->format, decoder->stream_idx, decoder->start_ticks, AVSEEK_FLAG_BACKWARD);
   }
   // What the codec was holding is from where it was.
   avcodec_flush_buffers(decoder->codec);
   decoder->is_draining = 0;
   decoder->last_time_s = time_s;
}

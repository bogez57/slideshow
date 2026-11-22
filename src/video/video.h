#pragma once

////////////////////////////////
//~ bgz: video
//
// A video file, played into a texture: what a slide draws where a picture
// would go, with its sound through the mixer (src\audio).
//
// The layer is in two parts, so that what decodes can be changed without
// anything else knowing (video.md has the whole of it):
//
//    video.h, video.c       the player. A thread that decodes ahead, a few
//                           frames waiting, a clock, and the texture. Knows
//                           nothing of file formats or codecs.
//    video_decoder.h        what the player asks of a decoder: open, next
//                           frame, next sound, seek, close.
//    video_ffmpeg.c         those, done with FFmpeg. The only file
//                           that knows FFmpeg is there.
//
// The rest of the program uses only what is declared in this file.
//
// All of it is called from the main thread. Times are in seconds from the
// video's start.

// The longest side a video is decoded at. A larger one is made smaller as
// it is decoded: a slide never shows more, and a frame of 4K is 33 MB to
// convert, keep and send to the renderer 60 times a second.
#define VID_MAX_DIM 1920

typedef struct Video_Video Video_Video;

// Whether a file name ends in an extension that is a video's.
B32 Video_NameIsVideo(String8 name);

// Opens the file and starts decoding it. The video is paused at its start,
// and its first frame is in the texture by the first or second Video_Update.
// 0 if it could not be opened, and why is logged.
Video_Video* Video_Open(String8 path);
void       Video_Close(Video_Video* video);

// Black until the first frame is in. As long as the video is open.
R_Handle Video_Texture(Video_Video* video);
Vec2S32  Video_Size(Video_Video* video); // of the texture, in pixels

// 0 if the file does not say.
F64 Video_Duration(Video_Video* video);
F64 Video_Position(Video_Video* video);

// Playing a video that is at its end starts it over.
B32  Video_IsPlaying(Video_Video* video);
void Video_SetPlaying(Video_Video* video, B32 playing);
void Video_Seek(Video_Video* video, F64 time_s);
// Whether it has played to its end and stopped there.
B32  Video_HasEnded(Video_Video* video);

// No sound from this one. It keeps its time by its sound all the same.
void Video_SetMuted(Video_Video* video, B32 muted);

// Once a frame: moves the clock on and puts the frame that is due in the
// texture. A video that reaches its end stops there, on its last frame.
void Video_Update(Video_Video* video);

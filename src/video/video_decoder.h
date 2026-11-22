#pragma once

////////////////////////////////
//~ bgz: video: what the player asks of a decoder
//
// The player (video.c) is written against these functions and nothing
// else. A file that defines them is a decoder: video_ffmpeg.c is the one
// there is. To decode with something else, write another such file and
// include it at the end of video.c in place of that one.
//
// A decoder is used by one thread at a time: the main thread opens and
// closes it, and between the two only the player's decode thread calls it.
// It does not have to be fast to seek or exact about it - the player throws
// away frames from before the time it asked for.

typedef struct Video_Decoder Video_Decoder; // the decoder's own

typedef struct Video_DecoderInfo Video_DecoderInfo;
struct Video_DecoderInfo {
   Vec2S32 size;              // of the frames Video_DecoderNextFrame writes
   F64     duration_s;        // 0: not known
   F64     frames_per_second; // 0: not known
   B32     has_sound;         // Video_DecoderNextSound has something to give
};

// Opens the file. Frames come out no longer than max_dim on their longer
// side, made smaller if the video's own are larger. Sound, if the file has
// any and sound_rate is not 0, comes out as two channels at sound_rate
// frames a second, whatever the file's own is. What the decoder itself needs
// for as long as it is open may be pushed on arena. 0 if the file could not
// be opened as a video, and why is logged (LogUserErrorF).
Video_Decoder* Video_DecoderOpen(Arena* arena, String8 path, S32 max_dim, S32 sound_rate, Video_DecoderInfo* info_out);
void         Video_DecoderClose(Video_Decoder* decoder);

// Decodes the next frame into pixels: info.size.x * info.size.y * 4 bytes,
// rows from the top, each pixel red, green, blue, alpha. time_s_out gets when
// it is shown, in seconds from the video's start. 0 at the end of the video,
// or when it can't be decoded further.
B32 Video_DecoderNextFrame(Video_Decoder* decoder, U8* pixels, F64* time_s_out);

// Decodes the next of the video's sound into samples: up to frame_cap
// frames, each a left and a right F32. Returns how many, and time_s_out gets
// when the first of them is heard, on the same clock as the frames' times. 0
// at the end of the sound, or if there is none.
//
// Frames and sound are asked for separately and in any order: the decoder
// keeps what it has read of the one while it is asked for the other.
U64 Video_DecoderNextSound(Video_Decoder* decoder, F32* samples, U64 frame_cap, F64* time_s_out);

// The next frame and the next sound decoded will be at or before time_s - as near before
// it as the decoder can seek without decoding.
void Video_DecoderSeek(Video_Decoder* decoder, F64 time_s);

# video

A video file played into a texture, which a slide draws where a picture would
go, with its sound played through the mixer in `src\audio`.

## The parts

```
the app (deck, frame_video.c, presenter)
        |  uses only video.h
        v
video.h / video.c          the player: decode thread, frame queue, sound ring, clock, texture
        |  uses only video_decoder.h
        v
video_decoder.h            what a decoder is: open, next frame, next sound, seek, close
        ^
        |  defines them
video_ffmpeg.c             the decoder there is, done with FFmpeg
```

- **`video.h`** is all the rest of the program sees: `VID_Open`, `VID_Close`,
  `VID_Texture`, `VID_Size`, `VID_Duration`, `VID_Position`, `VID_IsPlaying`,
  `VID_SetPlaying`, `VID_Seek`, `VID_Update`, `VID_NameIsVideo`. Only bgz types.
- **`video.c`** is the player. It knows nothing of files or codecs. A thread
  decodes a few frames ahead into a queue; `VID_Update`, once a frame on the
  main thread, moves the clock on and copies the frame that is due into the
  texture.
- **`video_decoder.h`** is the seam: `VID_DecoderOpen`, `VID_DecoderClose`,
  `VID_DecoderNextFrame`, `VID_DecoderNextSound`, `VID_DecoderSeek`, and the `VID_DecoderInfo` that open
  fills in. Plain, one call at a time, no threads of its own to answer for.
- **`video_ffmpeg.c`** is those functions done with FFmpeg. It is the only
  file that includes FFmpeg's headers.

## Changing the decoder

1. Write `video_<other>.c` that defines the functions in `video_decoder.h`.
2. In `src\main.c`, include it in place of `video/video_ffmpeg.c`.
3. In `build.bat`, take out the `ffmpeg` block and what it adds to the
   include path and the link line, and add what the new one needs.

Nothing else changes: the player, the deck and the screens don't know which
decoder is there.

## How a slide gets a video

A slide's picture line with a video file in it, `![](clip.mp4)`, makes the
slide's picture a video. The deck (`deck.c`) opens a `VID_Video` for it where
it would decode an image, and gives the slide's `IMG_Image` the player's
texture, so the view draws a video exactly as it draws a picture. When it
plays and its controls are in `src\frame_video.c`.

## Sound and the clock

The decoder gives sound as two channels of `F32` at the mixer's rate. The
player keeps a couple of seconds of it in a ring and has a voice in the mixer
(`AUD_VoiceAdd`) that plays from the ring.

A video with sound keeps time by its sound: the clock is "what is being heard
now", worked out from what the audio thread last took from the ring and when
that reaches the speakers. Frames are shown when the clock gets to them.
Picture follows sound, so they can't drift apart. A file with no sound, or
whose sound ends before its picture, goes by the computer's clock.

## FFmpeg

`src\third_party\ffmpeg` has FFmpeg as it is given out by the BtbN builds
(`ffmpeg-master-latest-win64-lgpl-shared`, version `N-128959-g4dbee3b241`,
2026-11-16): `include`, `lib` (import libraries) and `bin` (the DLLs, which
`build.bat` copies next to the exe). Only what playing needs is there:
avformat, avcodec, avutil, swscale and swresample.

It is the LGPL build, used as separate DLLs and not changed. `LICENSE.txt` is
its licence. A program that ships with these DLLs has to say that it uses
FFmpeg and under which licence, and let the DLLs be replaced.

## The graphics card

`video_ffmpeg.c` decodes on the graphics card where the card has a decoder
for the stream (H.264 and H.265 on nearly every card), through D3D11VA, and
on the processor where it has not. The decoded frame is brought back to the
program's memory and goes the same way as before, so nothing outside that
one file knows which of the two did the work. The log says which, once per
video. `_VID_USE_GRAPHICS_CARD` at the top of the file turns it off.

## Not done yet

- A frame decoded on the card is copied back to memory and then up again as
  a texture. Drawing it straight from the card would save both copies, and
  would tie the decoder to the renderer.
- A video turned on its side by its file's rotation mark (some phone videos)
  is shown as stored, not turned upright.

# video

A video file played into a texture, which a slide draws where a picture would
go, with its sound played through the mixer in `src\audio`.

## The parts

```
the app (deck, slide_video\slide_video.c, presenter)
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

- **`video.h`** is all the rest of the program sees: `Video_Open`, `Video_Close`,
  `Video_Texture`, `Video_Size`, `Video_Duration`, `Video_Position`, `Video_IsPlaying`,
  `Video_SetPlaying`, `Video_Seek`, `Video_Update`, `Video_NameIsVideo`. Only bgz types.
- **`video.c`** is the player. It knows nothing of files or codecs. A thread
  decodes a few frames ahead into a queue; `Video_Update`, once a frame on the
  main thread, moves the clock on and copies the frame that is due into the
  texture.
- **`video_decoder.h`** is the seam: `Video_DecoderOpen`, `Video_DecoderClose`,
  `Video_DecoderNextFrame`, `Video_DecoderNextSound`, `Video_DecoderSeek`, and the `Video_DecoderInfo` that open
  fills in. Plain, one call at a time, no threads of its own to answer for.
- **`video_ffmpeg.c`** is those functions done with FFmpeg. It is the only
  file that includes FFmpeg's headers.

## Changing the decoder

1. Write `video_<other>.c` that defines the functions in `video_decoder.h`.
2. At the end of `video.c`, include it in place of `video_ffmpeg.c`.
3. In `build.bat`, take out the `ffmpeg` block and what it adds to the
   include path and the link line, and add what the new one needs.

Nothing else changes: the player, the deck and the screens don't know which
decoder is there.

## How a slide gets a video

A slide's picture line with a video file in it, `![](clip.mp4)`, makes the
slide's picture a video. The deck (`deck.c`) opens a `Video_Video` for it where
it would decode an image, and gives the slide's `Image_Image` the player's
texture, so the view draws a video exactly as it draws a picture. When it
plays and its controls are in `src\slide_video\slide_video.c`.

## Sound and the clock

The decoder gives sound as two channels of `F32` at the mixer's rate. The
player keeps a couple of seconds of it in a ring and has a voice in the mixer
(`Audio_VoiceAdd`) that plays from the ring.

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
one file knows which of the two did the work. `_VID_USE_GRAPHICS_CARD` at the
top of the file turns it off.

Which of the two it is, is found out when the video is opened, by decoding
its first frame there, and the log says it once per video:

```
Video ...\65_x265.mp4: decoded on the graphics card (hevc, d3d11va)
Video ...\clip.avi: decoded on the processor (mpeg4): the graphics card has no decoder for it
```

On the card the codec has one thread; if FFmpeg finds the card has no
decoder for that form of the stream (a 4:2:2 H.264, say) the codec is opened
again on the processor with its threads. A card that fails part way through
a file is given up on and the processor goes on from the same frame and the
same sound sample.

The frame's colour range and matrix, as the file gives them, are passed to
libswscale on both paths, so a full range video looks the same on the card
and on the processor, and an HD video marked BT.709 is converted as BT.709.

Measured on a Ryzen 9 9900X with an RTX 5060 (process CPU while playing, in
cores; no frames dropped either way unless said):

| file                               | processor | card |
|------------------------------------|-----------|------|
| H.264 1080p24                      | 0.17-0.21 | 0.11-0.21 |
| H.265 1080p24 (8 and 10 bit)       | 0.16-0.26 | 0.09-0.22 |
| H.264 4K60, 90 Mb/s                | 0.57-0.62, one run dropped 105 frames | 0.41-0.44 |
| H.265 4K60 10 bit, 60 Mb/s         | 1.81-1.85 | 0.47-0.53 |

Memory is lower on the card too (4K60 H.265: about 960 MB working set on the
processor, 360 MB on the card), since the processor's threads each hold
frames. Opening a video on the card takes about 50 ms on the main thread
(20 ms for the device, 25 ms for the first frame), against 4-10 ms on the
processor.

## Not done yet

- A frame decoded on the card is copied back to memory and then up again as
  a texture. Drawing it straight from the card would save both copies, and
  would tie the decoder to the renderer.
- Each video opens a device of its own on the default graphics card: two
  4K videos open at once hold about 630 MB of the card's memory. It is
  Windows' default card, the one the program draws with too (a 4K60 H.265
  keeps its video decoder about 12% busy); on a machine with two, the other
  is not tried.
- What the decode thread logs is lost, because the log is the main
  thread's: a card that fails part way through, or a file that stops
  decoding, falls back or stops without a word in the log.
- A video turned on its side by its file's rotation mark (some phone videos)
  is shown as stored, not turned upright.

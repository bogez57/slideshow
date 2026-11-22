# slide_video

A slide's video: when it plays, and its strip of controls - play and pause, the bar, the time and the sound. Presenting, a slide's videos start when it comes up and stop when it goes; editing, they wait until played by hand. Its keys (K, J, L, M) and how a slide's @video line changes this are at the top of slide_video.c. It knows nothing of decoding: it uses the video layer's header only.

## State

- `App_State`'s `video_auto_slide`: the slide whose video was started because the presentation came to it. Written by `SlideVideo_Update`.
- Whether a video plays and where it is are the video's own (src\video), changed by the ops below.
- When one of the video's keys was last pressed, for how long the strip shows, is frame.c's (`_frame_state.video_key_us`).

## Functions and ops

- `SlideVideo_BuildStripHits`, `SlideVideo_BuildStripLook`: the strip as UI boxes - the ones clicked, which push ops, and the ones drawn.
- `SlideVideo_SlideIsPlaying`: whether any of a slide's videos is playing.
- `SlideVideo_Keys`: the video's keys while presenting. Pushes ops.
- `SlideVideo_Update`: once a frame, from `_FR_UpdateAppState`, starts and stops videos as slides come and go.
- Ops: `SlideVideo_ToggleSlide` and `SlideVideo_StepSlide` (a slide's videos together), `SlideVideo_TogglePicture` and `SlideVideo_SeekPicture` (one video, by its slide and which picture it is), `SlideVideo_ToggleMuted`.
- `SVID_STRIP_SECONDS`: how long the presentation shows the strip after a key.

## Used by

- frame.c: the keys, the update, and the strip over the presentation.
- edit: the strip of a video in the preview, which takes clicks.
- presenter: its play and pause button.

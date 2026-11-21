# slide_video

A slide's video: when it plays, and its strip of controls - play and pause, the bar, the time and the sound. Presenting, a slide's videos start when it comes up and stop when it goes; editing, they wait until played by hand. Its keys (K, J, L, M) and how a slide's @video line changes this are at the top of slide_video.c. It knows nothing of decoding: it uses the video layer's header only.

## State

- `App_State`'s `video_auto_slide`: the slide whose video was started because the presentation came to it.
- `video_key_us`, written by the presentation (frame.c): when one of the video's keys was last pressed, for how long the strip shows.

## Functions

- `SVID_BuildStripHits`, `SVID_BuildStripLook`: the strip as UI boxes - the ones clicked, and the ones drawn.
- `SVID_SlideIsPlaying`, `SVID_ToggleSlide`: a slide's videos together.
- `SVID_Keys`: the video's keys while presenting.
- `SVID_Update`: once a frame, starts and stops videos as slides come and go.
- `SVID_STRIP_SECONDS`: how long the presentation shows the strip after a key.

## Used by

- frame.c: the keys, the update, and the strip over the presentation.
- edit: the strip of a video in the preview, which takes clicks.
- presenter: its play and pause button.

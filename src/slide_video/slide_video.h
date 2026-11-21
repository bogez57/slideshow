////////////////////////////////
//~ bgz: slide_video
//
// A slide's video: when it plays, and its controls.
// What it is and how it works is at the top of slide_video.c; here is what the
// rest of the program calls.

#ifndef SLIDE_VIDEO_H
#define SLIDE_VIDEO_H

// A slide's video: slide_video\slide_video.c.
//
// Its strip of controls, for a video drawn at video_rect, as UI boxes: the
// boxes clicked, which are not drawn (SVID_BuildStripHits: 1 if the mouse
// is over the strip), and the ones drawn (SVID_BuildStripLook), lit where
// the first are hovered. id tells a slide's videos apart. The presentation
// builds only the second, with interactive 0: its clicks are for the slides,
// and the strip is only lit by where the mouse is.
internal B32  SVID_BuildStripHits(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, U64 id);
internal void SVID_BuildStripLook(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, U64 id, B32 interactive);
// A slide's videos together: whether any is playing, and all paused if one
// is, else all played.
internal B32  SVID_SlideIsPlaying(DECK_Deck* deck, U64 slide_idx);
internal void SVID_ToggleSlide(DECK_Deck* deck, U64 slide_idx);
internal B32  SVID_Keys(App_State* app_state);
internal void SVID_Update(App_State* app_state);

#endif // SLIDE_VIDEO_H

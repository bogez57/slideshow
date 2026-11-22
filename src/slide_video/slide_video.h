#pragma once

////////////////////////////////
//~ bgz: slide_video
//
// A slide's video: when it plays, and its controls.
// What it is and how it works is at the top of slide_video.c; here is what the
// rest of the program calls.

typedef struct App_State App_State;

// How long the presentation shows the strip after a key of the video's.
#define SVID_STRIP_SECONDS 2.5f

// Its strip of controls, for a video drawn at video_rect, as UI boxes: the
// boxes clicked, which are not drawn (SlideVideo_BuildStripHits: 1 if the mouse
// is over the strip), and the ones drawn (SlideVideo_BuildStripLook), lit where
// the first are hovered. id tells a slide's videos apart. The presentation
// builds only the second, with interactive 0: its clicks are for the slides,
// and the strip is only lit by where the mouse is.
B32  SlideVideo_BuildStripHits(const App_State* app_state, Video_Video* video, Rng2F32 video_rect, F32 ui, U64 id);
void SlideVideo_BuildStripLook(const App_State* app_state, Video_Video* video, Rng2F32 video_rect, F32 ui, U64 id, B32 interactive);
// Whether any of a slide's videos is playing.
B32 SlideVideo_SlideIsPlaying(Deck_Deck* deck, U64 slide_idx);
// Presenting: the keys of the videos of the slide showing. Returns whether
// one was pressed.
B32 SlideVideo_Keys(const App_State* app_state);
// Once a frame, from _FR_UpdateAppState (frame.c).
void SlideVideo_Update(App_State* app_state);

//- bgz: state ops, pushed with StateOp_<Name>(ops)

// A slide's videos together: all paused if one is playing, else all played.
typedef struct SlideVideo_ToggleSlideOpParams SlideVideo_ToggleSlideOpParams;
struct SlideVideo_ToggleSlideOpParams {
   U64 slide_idx;
};
SlideVideo_ToggleSlideOpParams* StateOp_SlideVideo_ToggleSlide(StateOp_List* ops);

// A slide's videos together, back (under 0) or on by seconds.
typedef struct SlideVideo_StepSlideOpParams SlideVideo_StepSlideOpParams;
struct SlideVideo_StepSlideOpParams {
   U64 slide_idx;
   F64 seconds;
};
SlideVideo_StepSlideOpParams* StateOp_SlideVideo_StepSlide(StateOp_List* ops);

// All of the program's sound, off and on.
void StateOp_SlideVideo_ToggleMuted(StateOp_List* ops);

// One video, the slide's picture picture_idx: played and paused, and taken to
// part of its length, 0 to 1.
typedef struct SlideVideo_TogglePictureOpParams SlideVideo_TogglePictureOpParams;
struct SlideVideo_TogglePictureOpParams {
   U64 slide_idx;
   U64 picture_idx;
};
SlideVideo_TogglePictureOpParams* StateOp_SlideVideo_TogglePicture(StateOp_List* ops);

typedef struct SlideVideo_SeekPictureOpParams SlideVideo_SeekPictureOpParams;
struct SlideVideo_SeekPictureOpParams {
   U64 slide_idx;
   U64 picture_idx;
   F64 part;
};
SlideVideo_SeekPictureOpParams* StateOp_SlideVideo_SeekPicture(StateOp_List* ops);

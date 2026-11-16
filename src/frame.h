////////////////////////////////
//~ bgz: frame
//
// One frame's work, for both screens: the presentation (frame.c) and the edit
// screen (frame_edit.c). What the two share is declared here.

#ifndef FRAME_H
#define FRAME_H

#define FR_FADE_SECONDS 0.25f
#define FR_PLAY_SECONDS 4.0f

// How long a notice stays up.
#define FR_NOTICE_SECONDS 4.0f

// The window's background, and a slide's.
#define FR_COLOR_BACKGROUND V4F32(0.05f, 0.05f, 0.06f, 1.0f)

// Everything between the renderer's begin and end of a frame (main.c): this
// frame's input, then the screen that is up.
internal void FrameWork(App_State* app_state);

// Opens the deck at path - a folder, an image or a text file - in place of
// the one that is open. Refused, with a notice, when the open deck has
// unsaved changes, or when path has nothing to show and a deck is open.
internal void FR_OpenPath(App_State* app_state, String8 path);

// Makes the deck again from the editor's text if that has changed, and reads
// the file again if something else has written it. Nothing for a folder of
// images.
internal void FR_SyncDeck(App_State* app_state);

// notice must last: a literal.
internal void FR_Notice(App_State* app_state, String8 notice);

// The notice, if one is up now, else an empty string.
internal String8 FR_CurrentNotice(App_State* app_state);

// Quits - unless there are unsaved changes that have not been warned about,
// which it warns about instead.
internal void FR_RequestQuit(App_State* app_state);

//- bgz: the edit screen (frame_edit.c)
internal void EDIT_Input(App_State* app_state, Vec2F32 window_dim);
internal void EDIT_Draw(App_State* app_state, Vec2F32 window_dim);

#endif // FRAME_H

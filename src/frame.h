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

// Everything between the renderer's begin and end of a frame (main.c): this
// frame's input, then the screen that is up.
internal void FrameWork(App_State* app_state);

// Opens the deck at path - a folder, an image or a text file - in place of
// the one that is open, and remembers it as the deck opened last. Refused,
// with a notice, when the open deck has unsaved changes, or when path has
// nothing to show and a deck is open: 0 then.
internal B32 FR_OpenPath(App_State* app_state, String8 path);

// path as a whole path, from the drive, with \ between its parts: one that
// is not whole already is taken from the folder the program was started in.
internal String8 FR_WholePath(Arena* arena, String8 path);

// The program's own folder for what it keeps from run to run - the log, the
// decks opened lately: local\ beside the program. Made if it is not there.
internal String8 FR_LocalFolder(Arena* arena);

// Starts the log's file for this run, keeping the last run's beside it.
internal void FR_OpenLogFile(App_State* app_state);

// Writes what a log scope gathered to the log's file and the console: what
// went wrong marked !!!, warnings ???, the rest >>>.
internal void FR_WriteLogs(App_State* app_state, LogScopeResult logs);

// Reads the decks opened lately from their file beside the program. Once,
// at the start.
internal void FR_LoadRecents(App_State* app_state);

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

//- bgz: sizes of what is not a slide
//
// The screens' sizes are for a window 900 pixels tall, and grow with a taller
// one - a large or dense screen - up to twice, but do not shrink with a
// shorter: how many pixels a unit is.
internal F32 FR_UIScale(Vec2F32 window_dim);

// The UI layer's font size for text px pixels tall, as the hand-drawn text
// of that size (TXT_Draw) is drawn: the font cache's points, whole ones.
internal F32 FR_UIFontSize(F32 px);

//- bgz: the edit screen (frame_edit.c)
//
// Input for the keys and the hand-drawn parts: the text, the preview, the
// small slides. EDIT_Build builds the UI parts - the Format panel, the list
// of pictures, the status line - in FrameWork's UI build. Not interactive:
// drawn, but taking no clicks or wheel, as when the open screen is over it.
internal void EDIT_Input(App_State* app_state, Vec2F32 window_dim);
internal void EDIT_Build(App_State* app_state, Vec2F32 window_dim, B32 interactive);
internal void EDIT_Draw(App_State* app_state, Vec2F32 window_dim);

//- bgz: the open screen (frame_open.c)
//
// Input for its keys; OPEN_Build builds the panel, in FrameWork's UI build,
// and takes its clicks and wheel.
internal void OPEN_Show(App_State* app_state);
internal void OPEN_Input(App_State* app_state, Vec2F32 window_dim);
internal void OPEN_Build(App_State* app_state, Vec2F32 window_dim);

//- bgz: the presenter's window (frame_presenter.c)

// Opens the window if it is closed and closes it if it is open. Between
// frames only (main.c).
// Files copied into the deck's folder in the background: frame_copy.c.
internal B32  COPY_IsCopyingTo(App_State* app_state, String8 dst);
internal void COPY_Start(App_State* app_state, String8 src, String8 dst);
internal B32  COPY_IsBusy(App_State* app_state);
internal void COPY_Update(App_State* app_state);
internal void COPY_CancelAll(App_State* app_state);

// A slide's video: frame_video.c.
internal void FR_VideoDrawStrip(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, Vec2F32 mouse);
internal B32  FR_VideoClickStrip(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, Vec2F32 pos);
// Whether pos is on the strip FR_VideoDrawStrip draws for a video at video_rect.
internal B32  FR_VideoStripContains(App_State* app_state, Rng2F32 video_rect, F32 ui, Vec2F32 pos);
// A slide's videos together: whether any is playing, and all paused if one
// is, else all played.
internal B32  FR_VideoSlideIsPlaying(DECK_Deck* deck, U64 slide_idx);
internal void FR_VideoToggleSlide(DECK_Deck* deck, U64 slide_idx);
internal B32  FR_VideoKeys(App_State* app_state);
internal void FR_VideoUpdate(App_State* app_state);

// The edit screen's picture and body, moved and resized with the mouse:
// frame_place.c. PLACE_Input before EDIT_Input's own clicks, PLACE_Draw over
// the preview's slide: 1 if the mouse is on something that can be moved.
internal void PLACE_Input(App_State* app_state, F32 ui);
internal B32  PLACE_Draw(App_State* app_state, F32 ui, Vec2F32 mouse);

internal void PRES_Toggle(App_State* app_state);

// Takes the window's events off this frame's: its close, and its keys, which
// become the main window's so that either window drives the slides.
internal void PRES_Input(App_State* app_state);

// Draws the window. Inside its own Core_R_WindowBeginFrame and
// Core_R_WindowEndFrame, after FrameWork.
internal void PRES_Draw(App_State* app_state);

#endif // FRAME_H

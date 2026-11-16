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

//- bgz: the edit screen (frame_edit.c)
internal void EDIT_Input(App_State* app_state, Vec2F32 window_dim);
internal void EDIT_Draw(App_State* app_state, Vec2F32 window_dim);

//- bgz: the open screen (frame_open.c)
internal void OPEN_Show(App_State* app_state);
internal void OPEN_Input(App_State* app_state, Vec2F32 window_dim);
internal void OPEN_Draw(App_State* app_state, Vec2F32 window_dim);

#endif // FRAME_H

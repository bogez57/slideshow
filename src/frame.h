#pragma once

////////////////////////////////
//~ bgz: frame
//
// One frame's work, for both screens: the presentation (frame.c) and the edit
// screen (edit\edit.c). What the two share is declared here.

#define FR_FADE_SECONDS 0.25f
#define FR_PLAY_SECONDS 4.0f

// How long a notice stays up.
#define FR_NOTICE_SECONDS 4.0f

// Everything between the renderer's begin and end of a frame (main.c): this
// frame's input, then the screen that is up. It runs the queue of state ops
// (_FR_UpdateAppState).
void FrameWork(App_State* rw_app_state);

// Opens the deck at path - a folder, an image or a text file - in place of
// the one that is open, and remembers it as the deck opened last. Refused,
// with a notice, when the open deck has unsaved changes, or when path has
// nothing to show and a deck is open: 0 then. It changes app state: called
// from ops (App_OpenPath, Open_NewDeck), and by main.c at the start.
B32 FR_OpenPath(App_State* app_state, String8 path);

// path as a whole path, from the drive, with \ between its parts: one that
// is not whole already is taken from the folder the program was started in.
String8 FR_WholePath(Arena* arena, String8 path);

// The program's own folder for what it keeps from run to run - the log, the
// decks opened lately: local\ beside the program. Made if it is not there.
String8 FR_LocalFolder(Arena* arena);

// Starts the log's file for this run, keeping the last run's beside it.
void FR_OpenLogFile(void);
void FR_CloseLogFile(void);

// Writes what a log scope gathered to the log's file and the console: what
// went wrong marked !!!, warnings ???, the rest >>>.
void FR_WriteLogs(LogScopeResult logs);

// Reads the decks opened lately from their file beside the program. Once,
// at the start.
void FR_LoadRecents(App_State* app_state);

// Makes the deck again from the editor's text if that has changed, and reads
// the file again if something else has written it. Nothing for a folder of
// images. It changes app state: _FR_UpdateAppState calls it after the ops,
// and an op that needs the deck of the text as it has left it.
void FR_SyncDeck(App_State* app_state);

// Whether the slide showing is still fading in over the one before it.
B32 FR_IsFading(const App_State* app_state);

// The notice (App_Notice), if one is up now, else an empty string.
String8 FR_CurrentNotice(const App_State* app_state);

//- bgz: sizes of what is not a slide
//
// The screens' sizes are for a window 900 pixels tall, and grow with a taller
// one - a large or dense screen - up to twice, but do not shrink with a
// shorter: how many pixels a unit is.
F32 FR_UIScale(Vec2F32 window_dim);

// The UI layer's font size for text px pixels tall, as the hand-drawn text
// of that size (Text_Draw) is drawn: the font cache's points, whole ones.
F32 FR_UIFontSize(F32 px);

//- bgz: boxes at a place of their own (frame.c)
//
// FR_BuildRectBox: a UI box at rect, in window pixels, under the parent
// pushed - the UI's root, or a box built with FR_BuildRectBox. Its signal is
// from rect, this frame's, and not from where the box was laid out last
// frame. FR_BuildEdge: a box that draws only an outline thick pixels wide,
// around rect exactly, in color.
UI_Box* FR_BuildRectBox(UI_BoxFlags flags, Rng2F32 rect, char* fmt, ...);
UI_Box* FR_BuildEdge(Rng2F32 rect, F32 thick, Vec4F32 color, char* fmt, ...);
// Between the two, the boxes those build are kept inside rect: only what of
// each is in it is built, and nothing of one that is all outside. For the
// pieces of a board (edit\edit.c), which can be anywhere, past the preview.
void FR_BeginBuildClip(Rng2F32 rect);
void FR_EndBuildClip(void);

//- bgz: a window's UI build (frame.c)
//
// Each window has a UI_State of its own: the main window's is
// app_state->ui_state, the presenter's app_state->presenter_ui_state.
// FR_BeginUIBuild selects ui_state and begins its build with this frame's
// events of that window - the slideshow's theme, icons and fonts, and
// last_frame_us for its animations - until UI_EndBuild. FR_DrawUITree draws
// a state's laid-out tree into the draw bucket that is open.
void FR_BeginUIBuild(const App_State* app_state, UI_State* ui_state, WM_Window window, U64* last_frame_us);
void FR_DrawUITree(UI_State* ui_state);

// A line of words at top_left, as Text_Draw would put it, px tall, in color
// (frame.c).
void FR_BuildWords(FNT_Tag font, F32 px, Vec2F32 top_left, Vec4F32 color, String8 words, char* fmt, ...);

// What the UI's signal counts for a notch of the wheel (UI_SignalFromBox's
// scroll, with the slideshow's UI_AnimationInfo).
#define FR_UI_SCROLL_PER_NOTCH 4.0f

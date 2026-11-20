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

//- bgz: boxes at a place of their own (frame.c)
//
// FR_BuildRectBox: a UI box at rect, in window pixels, under the parent
// pushed - the UI's root, or a box built with FR_BuildRectBox. Its signal is
// from rect, this frame's, and not from where the box was laid out last
// frame. FR_BuildEdge: a box that draws only an outline thick pixels wide,
// around rect exactly, in color.
internal UI_Box* FR_BuildRectBox(UI_BoxFlags flags, Rng2F32 rect, char* fmt, ...);
internal UI_Box* FR_BuildEdge(Rng2F32 rect, F32 thick, Vec4F32 color, char* fmt, ...);

//- bgz: a window's UI build (frame.c)
//
// Each window has a UI_State of its own: the main window's is
// app_state->ui_state, the presenter's app_state->presenter_ui_state.
// FR_BeginUIBuild selects ui_state and begins its build with this frame's
// events of that window - the slideshow's theme, icons and fonts, and
// last_frame_us for its animations - until UI_EndBuild. FR_DrawUITree draws
// a state's laid-out tree into the draw bucket that is open.
internal void FR_BeginUIBuild(App_State* app_state, UI_State* ui_state, WM_Window window, U64* last_frame_us);
internal void FR_DrawUITree(UI_State* ui_state);

// A line of words at top_left, as TXT_Draw would put it, px tall, in color
// (frame.c).
internal void FR_BuildWords(FNT_Tag font, F32 px, Vec2F32 top_left, Vec4F32 color, String8 words, char* fmt, ...);

//- bgz: the edit screen (frame_edit.c)
//
// EDIT_Input: the keys, the editor's among them. EDIT_Build builds the whole
// screen in FrameWork's UI build: the text (the editor draws it, and has its
// mouse), the Format panel, the list of pictures, the status line, and the
// preview. Not interactive: drawn, but taking no clicks or wheel, as when the
// open screen is over it.
internal void EDIT_Input(App_State* app_state, Vec2F32 window_dim);
internal void EDIT_Build(App_State* app_state, Vec2F32 window_dim, B32 interactive);

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
//
// Its strip of controls, for a video drawn at video_rect, as UI boxes: the
// boxes clicked, which are not drawn (FR_VideoBuildStripHits: 1 if the mouse
// is over the strip), and the ones drawn (FR_VideoBuildStripLook), lit where
// the first are hovered. id tells a slide's videos apart. The presentation
// builds only the second, with interactive 0: its clicks are for the slides,
// and the strip is only lit by where the mouse is.
internal B32  FR_VideoBuildStripHits(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, U64 id);
internal void FR_VideoBuildStripLook(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, U64 id, B32 interactive);
// A slide's videos together: whether any is playing, and all paused if one
// is, else all played.
internal B32  FR_VideoSlideIsPlaying(DECK_Deck* deck, U64 slide_idx);
internal void FR_VideoToggleSlide(DECK_Deck* deck, U64 slide_idx);
internal B32  FR_VideoKeys(App_State* app_state);
internal void FR_VideoUpdate(App_State* app_state);

// What the UI's signal counts for a notch of the wheel (UI_SignalFromBox's
// scroll, with the slideshow's UI_AnimationInfo).
#define FR_UI_SCROLL_PER_NOTCH 4.0f

// The edit screen's preview: a press on the slide at pos, as a click - to
// the letter of the words there, or the picture's name (frame_edit.c).
internal void EDIT_PreviewClick(App_State* app_state, Vec2F32 pos, B32 keeps_mark);

// The edit screen's picture and body, moved and resized with the mouse:
// frame_place.c. All in the edit screen's UI build, around its slide:
// PLACE_Keys before the edit screen's own keys (Esc gives up a drag, as
// PLACE_GiveUp does, which F5 calls too),
// PLACE_BeforeLayout before the slide is laid out (a drag goes with the
// mouse), PLACE_BuildCorners and PLACE_BuildHits after, for the boxes that
// are clicked - the corners' handles first, which are over a video's strip,
// and the rest after the strip - 1 if what they did changed the text, the
// cursor or a scroll; and PLACE_BuildLook last, for what is drawn: 1 if the
// mouse is on something that can be moved.
typedef struct PLACE_Hover PLACE_Hover;
struct PLACE_Hover {
   B32            is_over;   // over one of the slide's boxes
   B32            has_piece; // and that box is of a piece that can be moved
   DECK_PlaceKind kind;
   U64            idx;
   U32            corner;    // the corner's handle it is over, or 0
   S64            hit_idx;   // the hit of the preview it is over, or -1
};
internal void PLACE_GiveUp(App_State* app_state);
internal void PLACE_Keys(App_State* app_state);
internal void PLACE_BeforeLayout(App_State* app_state, F32 ui);
internal B32  PLACE_BuildCorners(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover);
internal B32  PLACE_BuildHits(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover);
internal B32  PLACE_BuildLook(App_State* app_state, F32 ui, PLACE_Hover* hover);

// The edit screen's words, given a size and a colour: frame_look.c. All of
// them are about the piece the cursor is in - the title, or a block of the
// body. LOOK_Input is the keys. direction: over 0 larger, under 0 smaller, 0
// the usual size again. named_idx: one of deck.h's colours with names, or
// DECK_NAMED_COLOR_COUNT for the theme's again. LOOK_Describe is what the
// piece has now, in words, for the Format panel, LOOK_ColorIsNow whether
// its colour is that one, and LOOK_SizeIsUsual whether it has no @size.
internal void    LOOK_Input(App_State* app_state);
internal void    LOOK_StepSize(App_State* app_state, S32 direction);
internal void    LOOK_SetColor(App_State* app_state, U64 named_idx);
internal String8 LOOK_Describe(Arena* arena, App_State* app_state, B32 is_color);
internal B32     LOOK_ColorIsNow(App_State* app_state, U64 named_idx);
internal B32     LOOK_SizeIsUsual(App_State* app_state);

// The edit screen's words, typed into on the slide: frame_caret.c.
// CARET_Click: a press at pos in the preview - 1 if it was in the title's
// words or a block's, and the cursor has gone to the letter there
// (keeps_mark: selecting from where it was). CARET_ScrollBlock: a block's
// words moved delta notches down. CARET_DragTo: the mouse held from a press
// at a piece's words, now at pos - what is between is selected, 1 if that
// changed the cursor or a scroll. CARET_SelectWord and CARET_SelectLine: a
// double and a triple click there.
//
// Around the preview's slide as it is laid out (frame_edit.c):
// CARET_BeforeDraw, before VIEW_DrawSlide, puts on the slide how far down
// each block's words it shows; CARET_Follow, after it, moves the words to
// follow the caret and keeps them to where they end - 1 if it moved any, and
// the slide is to be laid out again; CARET_KeepRuns keeps the runs, for
// clicks and the caret.
//
// Then UI boxes: CARET_BeforeLayout moves a scroll bar held by the mouse
// before the slide is laid out; CARET_BuildBars builds the scroll bars of
// blocks with more words than room, and CARET_BuildRooms the boxes of the
// rooms the words have - a click there goes to the nearest letter - both 1
// if what was done to them changed the cursor or a scroll; CARET_BuildLook,
// last, the box of the piece the cursor is in, the selection and the caret:
// 1 if it built that box.
internal B32  CARET_Click(App_State* app_state, Vec2F32 pos, B32 keeps_mark);
internal void CARET_ScrollBlock(App_State* app_state, U64 block_idx, F32 delta);
internal B32  CARET_DragTo(App_State* app_state, DECK_PlaceKind kind, U64 idx, Vec2F32 pos);
internal void CARET_SelectWord(App_State* app_state);
internal void CARET_SelectLine(App_State* app_state, DECK_PlaceKind kind, U64 idx);
internal void CARET_BeforeDraw(App_State* app_state);
internal B32  CARET_Follow(App_State* app_state, VIEW_HitList* hits);
internal void CARET_KeepRuns(App_State* app_state, VIEW_HitList* hits);
internal void CARET_BeforeLayout(App_State* app_state);
internal B32  CARET_BuildBars(App_State* app_state, F32 ui, B32 interactive);
internal B32  CARET_BuildRooms(App_State* app_state, B32 interactive);
internal B32  CARET_BuildLook(App_State* app_state, F32 ui);
// Presenting: a block with more words than room is moved through by the
// wheel over it, as far as it was in the edit screen when the slide came
// up. CARET_PresentAfterDraw, after CARET_BeforeDraw and the slide's
// VIEW_DrawSlide, takes the scroll off the slide and keeps its boxes;
// CARET_PresentWheel is the wheel at pos, delta notches down - 1 if it was
// over such a block.
internal void CARET_PresentAfterDraw(App_State* app_state, VIEW_HitList* hits);
internal B32  CARET_PresentWheel(App_State* app_state, Vec2F32 pos, F32 delta);
// Another drawing of a slide - the one fading out, the presenter window's -
// with its blocks as far down their words as where it is shown, or was
// when it was left; CARET_AfterDrawOf takes that off again.
internal void CARET_BeforeDrawOf(App_State* app_state, U64 slide_idx);
internal void CARET_AfterDrawOf(App_State* app_state, U64 slide_idx);

internal void PRES_Toggle(App_State* app_state);

// Takes the window's events off this frame's: its close, and its keys, which
// become the main window's so that either window drives the slides. Its
// mouse is left for its own UI (PRES_Draw).
internal void PRES_Input(App_State* app_state);

// Builds the window's UI, with its own UI_State, and draws it. Inside its own
// Core_R_WindowBeginFrame and Core_R_WindowEndFrame, after FrameWork.
internal void PRES_Draw(App_State* app_state);

#endif // FRAME_H

#pragma once

////////////////////////////////
//~ bgz: caret
//
// The edit screen: typing into the slide.
// What it is and how it works is at the top of caret.c; here is what the
// rest of the program calls.

typedef struct App_State App_State;

// The caret's own, which is not app state, written with no state op: how far
// down their words the blocks are scrolled, and what the mouse holds.
typedef struct Caret_View Caret_View;
struct Caret_View {
   // How far down its words each block of the slide showing shows, in page
   // units (Deck_Box's scroll): by block for the ones with an @place line,
   // and one for the rest, which share the body's room. scroll_slide is the
   // slide they are of: another starts from 0. The seen two are the cursor
   // and the text the room last followed the caret for.
   F32 scroll[DECK_MAX_BOXES];
   F32 body_scroll;
   U64 scroll_slide;
   U64 seen_cursor;
   U64 seen_edit_count;
   // The same of the slide shown before scroll_slide, as it was left: drawn
   // so while it fades out.
   F32 left_scroll[DECK_MAX_BOXES];
   F32 left_body_scroll;
   U64 left_slide;

   // A scroll bar of the preview's slide with the mouse's button down on
   // it: the block whose it is, how far under the top of the bar's thumb the
   // mouse took hold, and the UI box pressed - the thumb or the track -
   // which is held for as long as the UI has it active.
   B32    bar_drag;
   U64    bar_block;
   F32    bar_grab;
   UI_Key bar_key;

   // Presenting: the boxes of the slide showing, as last drawn, for the
   // wheel over a block with more words than room (Caret_PresentWheel).
   View_Box present_boxes[VIEW_MAX_BOXES];
   U64      present_box_count;
   Rng2F32  present_page;

   // Esc has taken the box around the piece the cursor is in off the
   // preview. It is back when the cursor is anywhere but
   // focus_hidden_cursor, or a piece is clicked.
   B32 focus_hidden;
   U64 focus_hidden_cursor;
};
global Caret_View g_caret_view;

// Caret_Click: a press at pos in the preview - 1 if it was in the title's
// words or a block's, and the cursor has gone to the letter there
// (keeps_mark: selecting from where it was). It changes the cursor: for ops
// only (Caret_PressRoom, and edit's Edit_PreviewClick). Caret_ScrollBlock: a
// block's words moved delta notches down. Caret_ScrollToTop: every block
// from its top again. Caret_DragTo: the mouse held from a press at a piece's
// words, now at pos - what is between is selected (it pushes
// Caret_DragSelect), 1 if a block's words were moved on under the mouse.
//
// Around the preview's slide as it is laid out (edit\edit.c):
// Caret_BeforeDraw, before View_DrawSlide, puts on the slide how far down
// each block's words it shows; Caret_Follow, after it, moves the words to
// follow the caret and keeps them to where they end - 1 if it moved any, and
// the slide is to be laid out again; Caret_KeepRuns keeps the runs, for
// clicks and the caret.
//
// Then UI boxes: Caret_BeforeLayout moves a scroll bar held by the mouse
// before the slide is laid out; Caret_BuildBars builds the scroll bars of
// blocks with more words than room, and Caret_BuildRooms the boxes of the
// rooms the words have - a click there goes to the nearest letter - both 1
// if what was done to them changed a scroll or was a press; Caret_BuildLook,
// last, the box of the piece the cursor is in, the selection and the caret:
// 1 if it built that box.
//
// None of these changes app state: a scroll is the caret's own
// (g_caret_view), and what is done to the cursor is pushed as a state op.
B32  Caret_Click(App_State* app_state, Vec2F32 pos, B32 keeps_mark);
void Caret_ScrollBlock(const App_State* app_state, U64 block_idx, F32 delta);
void Caret_ScrollToTop(void);
B32  Caret_DragTo(const App_State* app_state, Deck_PlaceKind kind, U64 idx, Vec2F32 pos);
void Caret_BeforeDraw(const App_State* app_state);
B32  Caret_Follow(const App_State* app_state, View_HitList* hits);
void Caret_KeepRuns(View_HitList* hits);
void Caret_BeforeLayout(const App_State* app_state);
B32  Caret_BuildBars(const App_State* app_state, F32 ui, B32 interactive);
B32  Caret_BuildRooms(const App_State* app_state, F32 ui, B32 interactive);
B32  Caret_BuildLook(const App_State* app_state, F32 ui);
// Presenting: a block with more words than room is moved through by the
// wheel over it, as far as it was in the edit screen when the slide came
// up. Caret_PresentAfterDraw, after Caret_BeforeDraw and the slide's
// View_DrawSlide, takes the scroll off the slide and keeps its boxes;
// Caret_PresentWheel is the wheel at pos, delta notches down - 1 if it was
// over such a block.
void Caret_PresentAfterDraw(const App_State* app_state, View_HitList* hits);
B32  Caret_PresentWheel(const App_State* app_state, Vec2F32 pos, F32 delta);
// Another drawing of a slide - the one fading out, the presenter window's -
// with its blocks as far down their words as where it is shown, or was
// when it was left; Caret_AfterDrawOf takes that off again.
void Caret_BeforeDrawOf(const App_State* app_state, U64 slide_idx);
void Caret_AfterDrawOf(const App_State* app_state, U64 slide_idx);

//- bgz: state ops, pushed with StateOp_<Name>(ops)

// A press at pos in the room of the title's or a block's words: the cursor
// to the nearest letter (keeps_mark: selecting from where it was).
typedef struct Caret_PressRoomOpParams Caret_PressRoomOpParams;
struct Caret_PressRoomOpParams {
   Vec2F32 pos;
   B32     keeps_mark;
};
Caret_PressRoomOpParams* StateOp_Caret_PressRoom(StateOp_List* ops);

// The mouse held from a press at the words of kind and idx, now at pos: the
// cursor to the letter there, the mark left. whole_lines: only of the lines
// that show whole.
typedef struct Caret_DragSelectOpParams Caret_DragSelectOpParams;
struct Caret_DragSelectOpParams {
   Deck_PlaceKind kind;
   U64            idx;
   Vec2F32        pos;
   B32            whole_lines;
};
Caret_DragSelectOpParams* StateOp_Caret_DragSelect(StateOp_List* ops);

// A double click: the word the cursor is in or beside is selected. A triple
// click: the words of the line the cursor is in, as the slide has them.
void StateOp_Caret_SelectWord(StateOp_List* ops);
typedef struct Caret_SelectLineOpParams Caret_SelectLineOpParams;
struct Caret_SelectLineOpParams {
   Deck_PlaceKind kind;
   U64            idx;
};
Caret_SelectLineOpParams* StateOp_Caret_SelectLine(StateOp_List* ops);
////////////////////////////////
//~ bgz: caret
//
// The edit screen: typing into the slide.
// What it is and how it works is at the top of caret.c; here is what the
// rest of the program calls.

#ifndef CARET_H
#define CARET_H

// The edit screen's words, typed into on the slide: caret\caret.c.
// CARET_Click: a press at pos in the preview - 1 if it was in the title's
// words or a block's, and the cursor has gone to the letter there
// (keeps_mark: selecting from where it was). CARET_ScrollBlock: a block's
// words moved delta notches down. CARET_DragTo: the mouse held from a press
// at a piece's words, now at pos - what is between is selected, 1 if that
// changed the cursor or a scroll. CARET_SelectWord and CARET_SelectLine: a
// double and a triple click there.
//
// Around the preview's slide as it is laid out (edit\edit.c):
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
internal B32  CARET_BuildRooms(App_State* app_state, F32 ui, B32 interactive);
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

#endif // CARET_H

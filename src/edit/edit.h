#pragma once

////////////////////////////////
//~ bgz: edit
//
// The edit screen. What it is and how it works is at the top of edit.c; here
// is what the rest of the program calls.

typedef struct App_State App_State;

// EDIT_Input: the keys, the editor's among them. EDIT_Build builds the whole
// screen in FrameWork's UI build: the text (the editor draws it, and has its
// mouse), the Format panel, the list of pictures, the status line, and the
// preview. Not interactive: drawn, but taking no clicks or wheel, as when the
// open screen is over it.
void EDIT_Input(App_State* app_state, Vec2F32 window_dim);
void EDIT_Build(App_State* app_state, Vec2F32 window_dim, B32 interactive);

// The preview: a press on the slide at pos, as a click - to the letter of the
// words there, or the picture's name.
void EDIT_PreviewClick(App_State* app_state, Vec2F32 pos, B32 keeps_mark);

// Which of the preview's pieces (app_state->preview_hits) a point is on, or
// -1: where pieces are over each other, the one drawn last.
S64 EDIT_PreviewHitAt(App_State* app_state, Vec2F32 pos);

// Writes a line that is about one piece of a slide - an @place, @size or
// @color line, for the idx-th picture or block of the body, or the title:
// over the one there is (old_line, with has_line), or as a new line next to
// the piece's own. EDIT_TakeOutLine takes such a line out of the text, with
// its line ending.
void EDIT_PutLineFor(ED_Editor* editor, DECK_Slide* slide, DECK_PlaceKind kind, U64 idx, B32 has_line, Rng1U64 old_line,
                     String8 line);
void EDIT_TakeOutLine(ED_Editor* editor, Rng1U64 line);

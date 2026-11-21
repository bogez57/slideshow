#pragma once

////////////////////////////////
//~ bgz: place
//
// The edit screen: a slide's pictures, title and words, moved and resized
// with the mouse.
// What it is and how it works is at the top of place.c; here is what the
// rest of the program calls.

typedef struct App_State App_State;

// A press on one of the preview's pictures, its title or its body, and what
// has come of it: app_state->place_drag.
typedef enum PLACE_DragState {
   PLACE_DragState_None,
   PLACE_DragState_Pressed, // the button is down on it, and the mouse has not gone far: a click, so far
   PLACE_DragState_Dragging,
   // The button went down right at the words of the title or of a block:
   // nothing is moved, the words from there to the mouse are selected
   // (CARET_DragTo, caret\caret.c).
   PLACE_DragState_Selecting,
} PLACE_DragState;

typedef struct PLACE_Drag PLACE_Drag;
struct PLACE_Drag {
   PLACE_DragState state;
   DECK_PlaceKind  kind;
   U64             idx;         // which of the slide's pictures, or of its body's blocks
   U32             corner;      // 0: all of it is moved. 1 to 4: that corner is - top left, top right, bottom left, bottom right. 5 to 8: that side is - left, right, top, bottom
   U64             slide_idx;
   U64             edit_count;  // the editor's when the button went down: a change to the text ends it
   Vec2F32         press_mouse;
   Rng2F32         start;       // where it was then, in parts of the page
   Rng2F32         now;         // where the mouse has it
   // With Shift down: what it has been lined up with, across and down, as
   // parts of the page - lines drawn over the slide while it is.
   B32             has_guide_x;
   B32             has_guide_y;
   F32             guide_x;
   F32             guide_y;
   // The other box each line is an edge or the middle of, outlined with it:
   // the body's box is larger than its words, and a line through empty space
   // says nothing until its box shows. None for the page's own lines.
   B32             has_guide_box_x;
   B32             has_guide_box_y;
   Rng2F32         guide_box_x;
   Rng2F32         guide_box_y;
   DECK_Place      before;      // the slide's place of that kind as the text has it, to go back to
   // Selecting: the hit of the preview pressed (a line of words), whose box
   // is the UI's active one, or -1 for a piece's box or the room beside the
   // words. A line moved out of its block's room by the drag has no hit,
   // and its box is built without one (PLACE_BuildHits).
   S64             select_hit;
};

// All in the edit screen's UI build, around its slide:
// PLACE_Keys before the edit screen's own keys (Esc gives up a drag, as
// PLACE_GiveUp does, which F5 calls too),
// PLACE_BeforeLayout before the slide is laid out (a drag goes with the
// mouse), PLACE_BuildCorners and PLACE_BuildHits after, for the boxes that
// are clicked - the corners' handles first, which are over a video's strip,
// and the rest after the strip - 1 if what they did changed the text, the
// cursor or a scroll; and PLACE_BuildLook last, for what is drawn: 1 if the
// mouse is on something that can be moved.
//
// Selecting the words on the slide by the mouse, for any box a press at them
// can be on - a line of words, a piece's box, the room beside the words
// (CARET_BuildRooms) - with its signal. PLACE_IsAtWords: whether pos is
// right at the words of kind and idx, where the mouse is the bar text is
// typed at. PLACE_PressWords: a press on such a box, 1 if it was there - the
// cursor has gone to the letter, or a double or triple click has selected
// the word or the line. PLACE_SelectHeld: the button held since, which
// selects to the mouse - 1 if that changed the cursor or a scroll.
typedef struct PLACE_Hover PLACE_Hover;
struct PLACE_Hover {
   B32            is_over;   // over one of the slide's boxes
   B32            has_piece; // and that box is of a piece that can be moved
   DECK_PlaceKind kind;
   U64            idx;
   U32            corner;    // the corner's handle it is over, or 0
   S64            hit_idx;   // the hit of the preview it is over, or -1
};
void PLACE_GiveUp(App_State* app_state);
void PLACE_Keys(App_State* app_state);
void PLACE_BeforeLayout(App_State* app_state, F32 ui);
B32  PLACE_BuildCorners(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover);
B32  PLACE_BuildHits(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover);
B32  PLACE_BuildLook(App_State* app_state, F32 ui, PLACE_Hover* hover);
B32  PLACE_IsAtWords(App_State* app_state, DECK_PlaceKind kind, U64 idx, Vec2F32 pos);
B32  PLACE_PressWords(App_State* app_state, UI_Signal signal, DECK_PlaceKind kind, U64 idx, F32 ui);
B32  PLACE_SelectHeld(App_State* app_state, UI_Signal signal);

// Whether the slide's boxes, as laid out, are of the text as it is.
B32 PLACE_BoxesAreGood(App_State* app_state);

// Which box a point is on: 0 for none. Where boxes are over each other, a
// point on a line of the body's words is on the block that line is in, one on
// the title's words on the title, and any other on the picture drawn last -
// the one on top - then the title, then the block drawn last.
VIEW_Box* PLACE_BoxAt(App_State* app_state, Vec2F32 pos);

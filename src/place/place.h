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
// has come of it: g_place_view.drag.
typedef enum Place_DragState {
   Place_DragState_None,
   Place_DragState_Pressed, // the button is down on it, and the mouse has not gone far: a click, so far
   Place_DragState_Dragging,
   // The button went down right at the words of the title or of a block:
   // nothing is moved, the words from there to the mouse are selected
   // (Caret_DragTo, caret\caret.c).
   Place_DragState_Selecting,
   // The button has come up on a piece dragged: it stays drawn where it was
   // left until its @place line is written, by Place_WriteLine at the start
   // of the next frame.
   Place_DragState_Dropped,
} Place_DragState;

typedef struct Place_Drag Place_Drag;
struct Place_Drag {
   Place_DragState state;
   Deck_PlaceKind  kind;
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
   Deck_Place      before;      // the slide's place of that kind as the text has it: kept while the slide is drawn with the mouse's (Place_PutDrag)
   B32             is_put;      // the slide has the mouse's place on it now, between Place_PutDrag and Place_TakeDrag
   // Selecting: the hit of the preview pressed (a line of words), whose box
   // is the UI's active one, or -1 for a piece's box or the room beside the
   // words. A line moved out of its block's room by the drag has no hit,
   // and its box is built without one (Place_BuildHits).
   S64             select_hit;
};

// The mouse's own on the slide, which is not app state, written with no
// state op.
typedef struct Place_View Place_View;
struct Place_View {
   // The picture, the title or the body being moved or resized with the
   // mouse. While it is, the slide is drawn with the place the mouse makes
   // without its being in the text (Place_PutDrag): the @place line is
   // written when the button comes up.
   Place_Drag drag;
   // The last press at the words on the slide, and which of a double or a
   // triple click it was - 1, 2 or 3; 0 after a press anywhere else there
   // (Place_PressWords).
   Vec2F32 click_mouse;
   U32     click_count;
};
global Place_View g_place_view;

// All in the edit screen's UI build, around its slide:
// Place_Keys before the edit screen's own keys (Esc gives up a drag, as
// Place_GiveUp does, which F5 calls too),
// Place_BeforeLayout before the slide is laid out (a drag goes with the
// mouse), Place_BuildCorners and Place_BuildHits after, for the boxes that
// are clicked - the corners' handles first, which are over a video's strip,
// and the rest after the strip - 1 if what they did changed the text, the
// cursor or a scroll; and Place_BuildLook last, for what is drawn: 1 if the
// mouse is on something that can be moved.
//
// Selecting the words on the slide by the mouse, for any box a press at them
// can be on - a line of words, a piece's box, the room beside the words
// (Caret_BuildRooms) - with its signal. Place_IsAtWords: whether pos is
// right at the words of kind and idx, where the mouse is the bar text is
// typed at. Place_PressWords: a press on such a box, 1 if it was there - the
// cursor has gone to the letter, or a double or triple click has selected
// the word or the line. Place_SelectHeld: the button held since, which
// selects to the mouse - 1 if that changed the cursor or a scroll.
typedef struct Place_Hover Place_Hover;
struct Place_Hover {
   B32            is_over;   // over one of the slide's boxes
   B32            has_piece; // and that box is of a piece that can be moved
   Deck_PlaceKind kind;
   U64            idx;
   U32            corner;    // the corner's handle it is over, or 0
   S64            hit_idx;   // the hit of the preview it is over, or -1
};
//
// None of these changes app state: the drag is the mouse's own
// (g_place_view), and what comes of it is pushed as a state op.
// Place_PutDrag and Place_TakeDrag go around every drawing of the slide a
// piece is being dragged on - View_DrawSlide between them - so that it is
// drawn where the mouse has it: the first puts that place on the slide, the
// second takes it off again.
void Place_GiveUp(void);
void Place_Keys(const App_State* app_state);
void Place_BeforeLayout(const App_State* app_state, F32 ui);
B32  Place_BuildCorners(const App_State* app_state, F32 ui, B32 interactive, Place_Hover* hover);
B32  Place_BuildHits(const App_State* app_state, F32 ui, B32 interactive, Place_Hover* hover);
B32  Place_BuildLook(const App_State* app_state, F32 ui, Place_Hover* hover);
B32  Place_IsAtWords(Deck_PlaceKind kind, U64 idx, Vec2F32 pos);
B32  Place_PressWords(const App_State* app_state, UI_Signal signal, Deck_PlaceKind kind, U64 idx, F32 ui);
B32  Place_SelectHeld(const App_State* app_state, UI_Signal signal);
void Place_PutDrag(const App_State* app_state, U64 slide_idx);
void Place_TakeDrag(const App_State* app_state, U64 slide_idx);

// Whether the slide's boxes, as laid out, are of the text as it is.
B32 Place_BoxesAreGood(const App_State* app_state);

// Which box a point is on: 0 for none. Where boxes are over each other, a
// point on a line of the body's words is on the block that line is in, one on
// the title's words on the title, and any other on the picture drawn last -
// the one on top - then the title, then the block drawn last.
View_Box* Place_BoxAt(const App_State* app_state, Vec2F32 pos);

//- bgz: state ops, pushed with StateOp_<Name>(ops)

// A piece dropped after a drag: its @place line written, with rect in parts
// of the page. edit_count: the editor's when the drag began.
typedef struct Place_WriteLineOpParams Place_WriteLineOpParams;
struct Place_WriteLineOpParams {
   U64            slide_idx;
   Deck_PlaceKind kind;
   U64            idx;
   U64            edit_count;
   Rng2F32        rect;
};
Place_WriteLineOpParams* StateOp_Place_WriteLine(StateOp_List* ops);

// A piece of the slide showing put back where the layout has it: its @place
// line taken out.
typedef struct Place_RemoveLineOpParams Place_RemoveLineOpParams;
struct Place_RemoveLineOpParams {
   Deck_PlaceKind kind;
   U64            idx;
};
Place_RemoveLineOpParams* StateOp_Place_RemoveLine(StateOp_List* ops);
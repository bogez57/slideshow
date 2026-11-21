////////////////////////////////
//~ bgz: place
//
// The edit screen: a slide's pictures, title and words, moved and resized
// with the mouse.
// What it is and how it works is at the top of place.c; here is what the
// rest of the program calls.

#ifndef PLACE_H
#define PLACE_H

// The edit screen's picture and body, moved and resized with the mouse:
// place\place.c. All in the edit screen's UI build, around its slide:
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
internal void PLACE_GiveUp(App_State* app_state);
internal void PLACE_Keys(App_State* app_state);
internal void PLACE_BeforeLayout(App_State* app_state, F32 ui);
internal B32  PLACE_BuildCorners(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover);
internal B32  PLACE_BuildHits(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover);
internal B32  PLACE_BuildLook(App_State* app_state, F32 ui, PLACE_Hover* hover);
internal B32  PLACE_IsAtWords(App_State* app_state, DECK_PlaceKind kind, U64 idx, Vec2F32 pos);
internal B32  PLACE_PressWords(App_State* app_state, UI_Signal signal, DECK_PlaceKind kind, U64 idx, F32 ui);
internal B32  PLACE_SelectHeld(App_State* app_state, UI_Signal signal);

#endif // PLACE_H

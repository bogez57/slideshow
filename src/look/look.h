#pragma once

////////////////////////////////
//~ bgz: look
//
// The edit screen: the size and the colour of a slide's words. What it is and
// how it works is at the top of look.c; here is what the rest of the program
// calls.

typedef struct App_State App_State;

// All of them are about the piece the cursor is in - the title, or a block of
// the body. LOOK_Input is the keys. direction: over 0 larger, under 0
// smaller, 0 the usual size again. named_idx: one of deck.h's colours with
// names, or DECK_NAMED_COLOR_COUNT for the theme's again. LOOK_Describe is
// what the piece has now, in words, for the Format panel, LOOK_ColorIsNow
// whether its colour is that one, and LOOK_SizeIsUsual whether it has no
// @size.
void    LOOK_Input(App_State* app_state);
void    LOOK_StepSize(App_State* app_state, S32 direction);
void    LOOK_SetColor(App_State* app_state, U64 named_idx);
String8 LOOK_Describe(Arena* arena, App_State* app_state, B32 is_color);
B32     LOOK_ColorIsNow(App_State* app_state, U64 named_idx);
B32     LOOK_SizeIsUsual(App_State* app_state);

// The piece the cursor is in: 0 if there is none whose words can be sized -
// no slides, or a slide with neither a title nor a body. The deck has to be
// of the text as it is: FR_SyncDeck first.
B32 LOOK_Target(App_State* app_state, U64* slide_idx_out, DECK_PlaceKind* kind_out, U64* idx_out);

// How a slide's piece's words look: the title's, or the idx-th block's.
DECK_Look* LOOK_Of(DECK_Slide* slide, DECK_PlaceKind kind, U64 idx);

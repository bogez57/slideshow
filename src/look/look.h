////////////////////////////////
//~ bgz: look
//
// The edit screen: the size and the colour of a slide's words.
// What it is and how it works is at the top of look.c; here is what the
// rest of the program calls.

#ifndef LOOK_H
#define LOOK_H

// The edit screen's words, given a size and a colour: look\look.c. All of
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

#endif // LOOK_H

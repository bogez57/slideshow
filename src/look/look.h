#pragma once

////////////////////////////////
//~ bgz: look
//
// The edit screen: the size, the colour and the side of a slide's words. What it is and
// how it works is at the top of look.c; here is what the rest of the program
// calls.

typedef struct App_State App_State;

// All of them are about the piece the cursor is in - the title, or a block of
// the body. Or, with words selected in a block that are not of all its
// lines, about the lines the selection is in, each its own: what is written
// is then "@size line", "@color line" and "@align line" lines above them
// (deck.h), and not the block's. Look_Input is the keys, which push the ops
// below. Look_Describe is what the piece has now, in words, for the Format
// panel, Look_ColorIsNow whether its colour is that one, and
// Look_SizeIsUsual whether it has no @size.
void    Look_Input(const App_State* app_state, StateOp_List* ops);
String8 Look_Describe(Arena* arena, const App_State* app_state, B32 is_color);
B32     Look_ColorIsNow(const App_State* app_state, U64 named_idx);
B32     Look_SizeIsUsual(const App_State* app_state);
// align: a Deck_Align, or Deck_Align_COUNT for none of its own - as the
// slide's layout has it.
String8 Look_DescribeAlign(Arena* arena, const App_State* app_state);
B32     Look_AlignIsNow(const App_State* app_state, U64 align);

// The piece the cursor is in: 0 if there is none whose words can be sized -
// no slides, or a slide with neither a title nor a body. The deck has to be
// of the text as it is.
B32 Look_Target(const App_State* app_state, U64* slide_idx_out, Deck_PlaceKind* kind_out, U64* idx_out);

//- bgz: state ops, pushed with StateOp_<Name>(ops)

// direction: over 0 larger, under 0 smaller, 0 the usual size again. Of the
// piece the cursor is in - or, with has_piece, of the preview slide's piece
// of kind and idx (Ctrl and the wheel over it, place\place.c), and with
// with_picked of every piece picked whole that has words (edit\edit.c).
typedef struct Look_StepSizeOpParams Look_StepSizeOpParams;
struct Look_StepSizeOpParams {
   S32            direction;
   B32            has_piece;
   Deck_PlaceKind kind;
   U64            idx;
   B32            with_picked;
};
Look_StepSizeOpParams* StateOp_Look_StepSize(StateOp_List* ops);

// named_idx: one of deck.h's colours with names, or DECK_NAMED_COLOR_COUNT
// for the theme's again. Of the piece the cursor is in, or the lines
// selected - or, with has_piece, of the preview slide's piece of kind and
// idx, and with with_picked of every piece picked whole that has words.
typedef struct Look_SetColorOpParams Look_SetColorOpParams;
struct Look_SetColorOpParams {
   U64            named_idx;
   B32            has_piece;
   Deck_PlaceKind kind;
   U64            idx;
   B32            with_picked;
};
Look_SetColorOpParams* StateOp_Look_SetColor(StateOp_List* ops);

// align: a Deck_Align, or Deck_Align_COUNT for as the slide's layout has
// it: the @align line taken out. Of the piece the cursor is in, or the
// lines selected - or, with has_piece, of the preview slide's piece of kind
// and idx, and with with_picked of every piece picked whole that has words.
typedef struct Look_SetAlignOpParams Look_SetAlignOpParams;
struct Look_SetAlignOpParams {
   U64            align;
   B32            has_piece;
   Deck_PlaceKind kind;
   U64            idx;
   B32            with_picked;
};
Look_SetAlignOpParams* StateOp_Look_SetAlign(StateOp_List* ops);

// How a slide's piece's words look: the title's, or the idx-th block's.
Deck_Look* Look_Of(Deck_Slide* slide, Deck_PlaceKind kind, U64 idx);

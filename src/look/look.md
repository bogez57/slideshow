# look

The edit screen: the size and the colour of a slide's words - the title's, or a block of the body's, whichever the cursor is in. Ctrl+] and Ctrl+[ make them larger and smaller; the Format panel's @size and @color lines have the rest to click. What comes of it is an `@size` or `@color` line in the deck's text. The keys are at the top of look.c.

## State

- None of its own: what it does is in the deck's text.

## Functions

- `LOOK_Input`: the keys.
- `LOOK_StepSize`, `LOOK_SetColor`: larger, smaller or usual; a colour with a name or the theme's.
- `LOOK_Describe`, `LOOK_ColorIsNow`, `LOOK_SizeIsUsual`: what the piece has now, for the Format panel.
- `LOOK_Target`: the piece the cursor is in.
- `LOOK_Of`: how a slide's piece's words look.

## Used by

- edit: its keys, and the Format panel's size and colour rows.
- caret: the piece the cursor is in, and its words' look, for the caret.

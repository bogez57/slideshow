# look

The edit screen: the size and the colour of a slide's words - the title's, or a block of the body's, whichever the cursor is in. Ctrl+] and Ctrl+[ make them larger and smaller; the Format panel's @size and @color lines have the rest to click. What comes of it is an `@size` or `@color` line in the deck's text. The keys are at the top of look.c.

## State

- None of its own: what it does is in the deck's text.

## Functions

- `Look_Input`: the keys. Pushes `Look_StepSize`. Ctrl and the wheel over a piece of the preview push it too (place), for that piece - or for every piece picked whole, over one of them - as one step to undo.
- Ops: `Look_StepSize`, `Look_SetColor`: larger, smaller or usual; a colour with a name or the theme's. Each writes or takes out a line of the text.
- `Look_Describe`, `Look_ColorIsNow`, `Look_SizeIsUsual`: what the piece has now, for the Format panel.
- `Look_Target`: the piece the cursor is in.
- `Look_Of`: how a slide's piece's words look.

## Used by

- edit: its keys, and the Format panel's size and colour rows.
- caret: the piece the cursor is in, and its words' look, for the caret.

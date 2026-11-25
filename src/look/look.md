# look

The edit screen: the size, the colour and the side of a slide's words - the title's, or a block of the body's, whichever the cursor is in. Ctrl+] and Ctrl+[ make them larger and smaller, Ctrl+Shift+L, E and R put them against the left, in the middle and against the right; the Format panel's @size, @color and @align lines have the rest to click. What comes of it is an `@size`, `@color` or `@align` line in the deck's text. With words selected in a block that are not of all its lines, it is of the lines the selection is in, each its own: `@size line 1.5` above each. The keys are at the top of look.c.

## State

- None of its own: what it does is in the deck's text.

## Functions

- `Look_Input`: the keys. Pushes `Look_StepSize` and `Look_SetAlign`. Ctrl and the wheel over a piece of the preview push it too (place), for that piece - or for every piece picked whole, over one of them - as one step to undo.
- Ops: `Look_StepSize`, `Look_SetColor`, `Look_SetAlign`: larger, smaller or usual; a colour with a name or the theme's; a side or the layout's. Each writes or takes out a line of the text - the piece's the cursor is in, the lines' selected, or with `has_piece` a piece's of the preview and all picked with it.
- `Look_Describe`, `Look_DescribeAlign`, `Look_ColorIsNow`, `Look_AlignIsNow`, `Look_SizeIsUsual`: what the piece, or the first of the lines selected, has now, for the Format panel and the right-click menu.
- `Look_Target`: the piece the cursor is in.
- `Look_Of`: how a slide's piece's words look.

## Used by

- edit: its keys, the Format panel's size, colour and align rows, and the right-click menu's.
- caret: the piece the cursor is in, and its words' look, for the caret.

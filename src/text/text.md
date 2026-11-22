# text

Drawing and measuring text, on top of bgz's font cache and draw layers. Sizes are in pixels, rounded the same way by every function so measuring and drawing agree. How positions and sizes are meant is in text.h.

## State

- None of its own. The fonts it opens (`Text_Fonts`) are kept in `App_State` as `fonts`.

## Functions

- `Text_OpenFonts`: the regular, bold and mono fonts - the program's own in `data\fonts`, else Windows'.
- `Text_LineHeight`, `Text_Width`, `Text_Baseline`, `Text_CapHeight`: measuring.
- `Text_Draw`: one line of text.
- `Text_Wrap`: a string cut into lines no wider than a width.
- `TXT_RASTER_FLAGS`: how the font cache draws the letters, for the UI layer to draw its text the same way.

## Used by

- main.c: opens the fonts at the start.
- view: the slides' words.
- editor: the text pane.
- frame.c, edit, caret, open, presenter, slide_video: the hand-drawn parts of the screens, and measuring for the UI's boxes.

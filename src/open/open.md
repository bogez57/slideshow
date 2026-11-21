# open

The open screen: a panel over whichever screen is up, to pick a deck without leaving the program. Its lines are the decks opened lately, then one folder: the folder above, Windows' own open-file window, a new text deck or the folder's pictures as a deck, and the folders and text decks in it. Its picture and keys are at the top of open.c.

## State

In `App_State`:

- `show_open`: the panel is up.
- `open_arena`, `open_folder`, `open_rows`, `open_row_count` (`OPEN_Row`): the folder shown and its lines.
- `open_scroll`, `open_scanned_us`: where the lines are scrolled to, and when the folder was last read.

## Functions

- `OPEN_Show`: puts the panel up, in the folder of the deck that is open.
- `OPEN_Input`: its keys.
- `OPEN_Build`: the panel, in FrameWork's UI build, with its clicks and wheel.

## Used by

- frame.c: Ctrl+O, and its input and build while it is up.
- main.c: the first run, with no deck opened before.

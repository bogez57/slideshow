# open

The open screen: a panel over whichever screen is up, to pick a deck without leaving the program. Its lines are the decks opened lately, then one folder: the folder above, the text decks and the folders in it, and last what else can be done there - a new text deck, the folder's pictures as a deck, Windows' own open-file window. The panel is as tall as its lines, up to the window's height. Its picture and keys are at the top of open.c.

## State

In `App_State`, changed by the ops below:

- `show_open`: the panel is up.
- `open_arena`, `open_folder`: the folder shown.

The panel's own, `_open_view` in open.c, written with no op:

- `rows`, `row_count` (`Open_Row`), `arena`: the lines, made again from the folder every second while the panel is built.
- `scroll`, `scanned_us`: where the lines are scrolled to, and when the folder was last read.

## Functions and ops

- `Open_Input`: its keys. Pushes ops.
- `Open_Build`: the panel, in FrameWork's UI build, with its clicks and wheel. Reads the folder into the rows, and pushes ops.
- Ops: `Open_Show` (puts the panel up, in the folder of the deck that is open), `Open_Close`, `Open_GoToFolder`, `Open_NewDeck` (makes a text deck and opens it). Opening a deck is app_state.c's `App_OpenPath`.

## Used by

- frame.c: Ctrl+O, and its input and build while it is up.
- main.c: the first run, with no deck opened before.

# vim

The edit screen's keys in the text, as the editor project's vim has them: command, insert and visual mode, the keys that move the cursor, and the ones that cut, copy and paste. Not all of vim: the keys that are used. They are listed at the top of vim.c.

In insert mode the keys and typing are the editor's own. In command and visual mode vim's keys come first, and the editor has what is left: undo and redo, select all, copy, cut and paste, with Ctrl.

A click on the preview's slide, or on a line of the Format panel, is insert mode: what it puts the cursor in, or selects, is there to be typed.

## State

In `App_State`, changed by ops only:

- `vim` (`Vim_State`, vim.h): `mode`, `selects_lines` (visual mode by whole lines), and `pending`, `pending_size`: the start of a command of more than one key (d, y, g).

On the editor, set whenever the mode is: `block_cursor` and `selects_lines` (editor.h), which is how the mode is drawn, and how a selection of whole lines is taken.

## Functions and ops

- `Vim_ModeName`, `Vim_ModeTag`: the mode's name and its colour's tag, for the status line.
- `Vim_WantsEsc`: whether Esc is vim's this press.
- For ops only: `Vim_EditorOpened` (a deck's text opened: command mode), `Vim_StartTyping` (insert mode), `Vim_AfterTextMouse` (the mouse in the text: a drag in command mode is visual mode, a click in visual mode leaves it).
- Ops other layers push: `Vim_KeyInput` (the text's keys and typing, from what is left of the frame's events: vim's by the mode, then `Editor_Input`), `Vim_Escape` (back to command mode).

## Used by

- edit: its keys in the text, Esc, the mouse in the text, the status line, and insert mode for what a click puts in the text.
- caret: insert mode for a click in the slide's words.
- frame.c: command mode when a deck's text opens.

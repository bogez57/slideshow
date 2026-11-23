# editor

A plain text editor for one file: the text, the cursor and selection, undo, and the box it is typed into and drawn in. What the text means is not its business. Its line endings, wrapping and memory are described at the top of editor.h; its keys are in editor.c.

## State

- An `Editor_Editor` is the caller's: `App_State`'s `editor`, for a deck from a text file. It has two arenas of its own, the text's and the undo history's, released by `Editor_Close`.

## Functions

- The file: `Editor_Open`, `Editor_Close`, `Editor_Save`, `Editor_Reload`, `Editor_HasUnsavedChanges`, `Editor_FileHasChanged`.
- The text: `Editor_Text`, `Editor_ReplaceRange`, `Editor_Insert`, `Editor_InsertLine`, `Editor_WrapSelection`, `Editor_SetCursor`, `Editor_JoinUndoSince` (several changes undone as one: a move of several pieces, a size for several).
- Its box: `Editor_Input` (keys), `Editor_Press`, `Editor_Drag`, `Editor_Wheel`, `Editor_Draw`.

## Used by

- edit: the text pane - drawn in its box, given its keys and mouse - and every change the screen makes to the text.
- frame.c: opens and closes it with a text deck, reads its text into the deck, and reads the file again when something else writes it.
- caret, look, place: the text, the cursor, and lines written for a piece of a slide.
- open: unsaved changes, before another deck is opened.

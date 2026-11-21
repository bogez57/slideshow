# editor

A plain text editor for one file: the text, the cursor and selection, undo, and the box it is typed into and drawn in. What the text means is not its business. Its line endings, wrapping and memory are described at the top of editor.h; its keys are in editor.c.

## State

- An `ED_Editor` is the caller's: `App_State`'s `editor`, for a deck from a text file. It has two arenas of its own, the text's and the undo history's, released by `ED_Close`.

## Functions

- The file: `ED_Open`, `ED_Close`, `ED_Save`, `ED_Reload`, `ED_HasUnsavedChanges`, `ED_FileHasChanged`.
- The text: `ED_Text`, `ED_ReplaceRange`, `ED_Insert`, `ED_InsertLine`, `ED_WrapSelection`, `ED_SetCursor`.
- Its box: `ED_Input` (keys), `ED_Press`, `ED_Drag`, `ED_Wheel`, `ED_Draw`.

## Used by

- edit: the text pane - drawn in its box, given its keys and mouse - and every change the screen makes to the text.
- frame.c: opens and closes it with a text deck, reads its text into the deck, and reads the file again when something else writes it.
- caret, look, place: the text, the cursor, and lines written for a piece of a slide.
- open: unsaved changes, before another deck is opened.

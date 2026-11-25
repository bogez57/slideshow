# edit

The edit screen, what a text deck opens on: its text on the left, in the editor, the Format panel and the status line under it, and on the right a preview of the slide the cursor is on, made again from the text at every change, with every slide small under it. Its picture of the screen and its keys are at the top of edit.c.

The preview is typed into (caret), its pieces moved and resized with the mouse (place), and its words given a size and a colour (look): edit lays out the slide and builds those layers' boxes around it.

## State

In `App_State`, changed by ops only:

- `is_editing`: the edit screen is up, not the presentation.
- `hide_text`: the left side - the text, the Format panel, the status line - is put away, and the preview has the whole window. A notice is then over the top of the preview.
- `show_format`, `show_pictures`: the Format panel is open, and the list of pictures in its place.
- `editor`, `deck`, `slide_idx`: the text, the deck made from it and the slide the cursor is on. `_FR_UpdateAppState` (frame.c) makes the deck again and finds the slide after the ops have run, so the screen is always built from a deck that is of the text as it is.

The screen's own, `g_edit_view` (`Edit_View`, edit.h), written with no op:

- `hits`, `hit_count`, `hits_arena`, `hits_edit_count`, `runs`, `run_count`, `boxes`, `box_count`, `page`: where the preview's pieces were laid out this frame, and for which text. caret, place and look read them.
- `is_board`, `board`, `board_base`, `board_at`, `board_zoom`, `board_panning`, `board_pan_mouse`, `board_seen_cursor`, `board_seen_edit_count`, `board_seen_slide_idx`, `board_file`, `board_file_size`, `board_saved_at`, `board_saved_zoom`: a board showing (an @canvas slide): the part of the window it is drawn in, and where the window is over it - the wheel moves it, Ctrl and the wheel zoom, the middle button drags it (`_Edit_BoardInput`), it goes to the cursor when what is done in the text is somewhere that does not show (`_Edit_BoardFollow`), and where it was left is written to board.txt in the program's own folder, for the deck worked on last. For a slide that is no board, `board` is its page. caret and place read `is_board` and `board`.
- `split_x`, `split_format`: where the two dividers were dragged to - the edge between the left side and the preview, and the Format panel's top edge. 0: where the layout puts it itself.
- `format_scroll`: where the Format panel's lines are scrolled to.
- `pictures`, `pictures_arena`, `pictures_scanned_us`, `pictures_scroll`: the names in the list of pictures, and where it is scrolled to.
- `is_drawing`, `draw_is_box`, `draw_picks`, `draw_from`, `draw_to`: a box being drawn with the mouse on the slide's open space: it picks the pieces it touches, or with Ctrl is a text box.
- `menu_open`, `menu_pos`, `menu_has_piece`, `menu_has_text`, `menu_kind`, `menu_idx`: the right-click menu on the preview's slide, and what it was opened on: a piece, words that are selected, or open space.
- `clip_arena`, `clip`, `clip_count`, `clip_text`, `clip_slide_idx`, `clip_paste_count`: the pieces copied whole (`Edit_ClipPiece`), and the text put on the system's clipboard for them. Ctrl+V pastes them as pieces while the clipboard still has that text.
- `has_new_box`, `new_box_edit_count`, `new_box_range`: a text box just drawn on the slide's open space, while nothing has been done to it. Left so, it is taken out again.
- `picked_count`, `picked_kinds`, `picked_idxs`, `picked_slide_idx`, `picked_edit_count`, `picked_cursor`, `picked_mark`: the pieces of the slide picked whole - a picture clicked, or the title or a block clicked away from its words; Ctrl+click adds one or takes it out, Shift+click picks all of the kind of the first - for as long as the text, the cursor and the slide are as the click left them. Delete takes them out, a drag of one moves them all, Ctrl and the wheel sizes their words.

## Functions and ops

- `Edit_Input`: the keys of the list under an @ line (complete), then the screen's keys, then the text's (`Vim_KeyInput`, vim: vim's keys by its mode, then the editor's). Pushes ops.
- `Edit_Build`: the whole screen, in FrameWork's UI build. Lays the preview's slide out into `g_edit_view`, and pushes ops for clicks.
- `Edit_OpenMenu`: a right-click on the preview's slide opens its menu there - copy, delete, front and back, lining up - each line pushing the op its key does. On a piece with words it has their size, a row of colours and a row of sides too. On words that are selected it is about them instead, and picks nothing: bold, code, and a size, a colour and a side for the lines the selection is in (look). place and caret call it for a right-click on a piece. On words selected it also has their lines made bullets, inner bullets, a heading or plain (`Edit_SetLineKind`). On open space it has "Add a picture or video" - the system's file window, and the file put where the menu was opened (`Edit_DropPicture` with a place) - and the slide's own: a new slide, a copy of this one, this one deleted or made a board (`Edit_ChangeSlide`), and each layout by name (`Edit_SetLayout`).
- `Edit_PreviewHitAt`: which piece of the preview a point is on.
- `Edit_PieceIsPicked`, `Edit_PickedIsGood`: whether a piece of the preview's slide is picked whole, or any is. caret leaves out the caret in one that is.
- `Edit_KeepPicked`: after an op moved or sized the pieces picked, they stay picked. For ops only (place, look).
- `Edit_PutLineFor`, `Edit_TakeOutLine`: write or take out a line about one piece of a slide (@place, @size, @color). They change the text: called from ops only.
- Ops, edit.c's own: `Edit_TextPress`, `Edit_TextDrag`, `Edit_TextWheel` (the mouse in the text), `Edit_ToggleFormat`, `Edit_ToggleText` (Alt+[), `Edit_ClosePictures`, `Edit_Deselect`, `Edit_Wrap` (bold, code), `Edit_Save`, `Edit_Present`, `Edit_LiftPieces` (Ctrl+Shift+N: the pieces picked to a slide of their own at the deck's end), `Edit_MoveSlide`, `Edit_AddSlide`, `Edit_GoToSlide`, `Edit_PutPicture`, `Edit_FormatRow`, `Edit_AddTextBox` (a box drawn on the slide where nothing is: a block of its own there, as large as drawn, `@box` and `@place text`, with "Text" selected), `Edit_CopyPieces` (Ctrl+C, Ctrl+X with pieces picked), `Edit_PastePieces` (Ctrl+V: each a block or a picture of its own, with its @place line, at the end of the slide, as one step to undo, and picked after), `Edit_DuplicatePieces` (Ctrl+D), `Edit_OrderPieces` (Ctrl+Shift+] and [: the pieces picked moved to the end of the slide's lines, or its start, which is what is over what), `Edit_PickRect` (Ctrl and a box drawn on open space: what it touches picked), `Edit_DeletePiece` (Delete with a piece picked: all of its lines out, as one step to undo; a title leaves `---` in its place).
- Ops other layers push too: `Edit_DropPicture` (complete: a file picked from the list under an @ line), `Edit_PreviewClick` (a press on the preview's slide: the cursor to the letter, or the picture's name selected; with `picks`, the piece picked whole as well; with `adds` (Ctrl) or `picks_all` (Shift), the pieces picked changed and the cursor left where it is).

## Used by

- frame.c: its input and build, while the edit screen is up.
- place, look: the lines their ops write, the preview's layout, and the click on a piece. place keeps a piece picked when it moves it.
- caret: the preview's layout, and whether a piece is picked.
- copy: marks the list of pictures out of date.
- complete: `Edit_DropPicture`, for a picture or video picked.

# edit

The edit screen, what a text deck opens on: its text on the left, in the editor, the Format panel and the status line under it, and on the right a preview of the slide the cursor is on, made again from the text at every change, with every slide small under it. Its picture of the screen and its keys are at the top of edit.c.

The preview is typed into (caret), its pieces moved and resized with the mouse (place), and its words given a size and a colour (look): edit lays out the slide and builds those layers' boxes around it.

## State

In `App_State`, changed by ops only:

- `is_editing`: the edit screen is up, not the presentation.
- `show_format`, `show_pictures`: the Format panel is open, and the list of pictures in its place.
- `editor`, `deck`, `slide_idx`: the text, the deck made from it and the slide the cursor is on. `_FR_UpdateAppState` (frame.c) makes the deck again and finds the slide after the ops have run, so the screen is always built from a deck that is of the text as it is.

The screen's own, `g_edit_view` (`Edit_View`, edit.h), written with no op:

- `hits`, `hit_count`, `hits_arena`, `hits_edit_count`, `runs`, `run_count`, `boxes`, `box_count`, `page`: where the preview's pieces were laid out this frame, and for which text. caret, place and look read them.
- `format_scroll`: where the Format panel's lines are scrolled to.
- `pictures`, `pictures_arena`, `pictures_scanned_us`, `pictures_scroll`: the names in the list of pictures, and where it is scrolled to.

## Functions and ops

- `Edit_Input`: the screen's keys, then the editor's. Pushes ops.
- `Edit_Build`: the whole screen, in FrameWork's UI build. Lays the preview's slide out into `g_edit_view`, and pushes ops for clicks.
- `Edit_PreviewHitAt`: which piece of the preview a point is on.
- `Edit_PutLineFor`, `Edit_TakeOutLine`: write or take out a line about one piece of a slide (@place, @size, @color). They change the text: called from ops only.
- Ops, edit.c's own: `Edit_KeyInput` (the editor's keys and typing, from what is left of the frame's events), `Edit_TextPress`, `Edit_TextDrag`, `Edit_TextWheel` (the mouse in the text), `Edit_ToggleFormat`, `Edit_ClosePictures`, `Edit_Deselect`, `Edit_Wrap` (bold, code), `Edit_Save`, `Edit_Present`, `Edit_MoveSlide`, `Edit_AddSlide`, `Edit_GoToSlide`, `Edit_DropPicture`, `Edit_PutPicture`, `Edit_FormatRow`.
- Op other layers push too: `Edit_PreviewClick` (a press on the preview's slide: the cursor to the letter, or the picture's name selected).

## Used by

- frame.c: its input and build, while the edit screen is up.
- place, look: the lines their ops write, the preview's layout, and the click on a piece.
- caret: the preview's layout.
- copy: marks the list of pictures out of date.

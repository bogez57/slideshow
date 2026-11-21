# edit

The edit screen, what a text deck opens on: its text on the left, in the editor, the Format panel and the status line under it, and on the right a preview of the slide the cursor is on, made again from the text at every change, with every slide small under it. Its picture of the screen and its keys are at the top of edit.c.

The preview is typed into (caret), its pieces moved and resized with the mouse (place), and its words given a size and a colour (look): edit lays out the slide and builds those layers' boxes around it.

## State

In `App_State`:

- `is_editing`: the edit screen is up, not the presentation.
- `show_format`, `format_scroll`: the Format panel.
- `show_pictures`, `pictures_arena`, `pictures`, `pictures_scanned_us`, `pictures_scroll`: the list of pictures in the deck's folder.
- `preview_hits_arena`, `preview_hits`, `preview_hit_count`, `preview_hits_edit_count`, `preview_boxes`, `preview_box_count`, `preview_page`: where the preview's pieces were laid out this frame, and for which text.
- `preview_focus_hidden`, `preview_focus_hidden_cursor`: Esc has taken the box around the piece the cursor is in off the preview.

## Functions

- `EDIT_Input`: the screen's keys, then the editor's.
- `EDIT_Build`: the whole screen, in FrameWork's UI build.
- `EDIT_PreviewClick`, `EDIT_PreviewHitAt`: a press on the preview's slide, and which piece a point is on.
- `EDIT_PutLineFor`, `EDIT_TakeOutLine`: write or take out a line about one piece of a slide (@place, @size, @color).

## Used by

- frame.c: its input and build, while the edit screen is up.
- place, look: the lines they write, and the piece under the mouse.

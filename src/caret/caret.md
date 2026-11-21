# caret

The edit screen: typing into the slide. A click on the preview's title or a block of words puts the cursor at the letter clicked, and a caret shows there; the keys are the editor's, as ever, since the slide's words and the text pane are the same text with the one cursor. A block with more words than room shows the part the caret is in, with a scroll bar, and the wheel moves through it - presenting too. The ways of the mouse are at the top of caret.c.

## State

In `App_State`:

- `preview_runs`, `preview_run_count`: the runs of letters of the preview's slide, as laid out this frame.
- `caret_scroll`, `caret_body_scroll`, `caret_scroll_slide`, `caret_seen_cursor`, `caret_seen_edit_count`: how far down its words each block of the slide showing is, and what the room last followed.
- `caret_left_scroll`, `caret_left_body_scroll`, `caret_left_slide`: the same, of the slide shown before, as it was left.
- `caret_bar_drag`, `caret_bar_block`, `caret_bar_grab`, `caret_bar_key`: a scroll bar held by the mouse.
- `present_boxes`, `present_box_count`, `present_page`: presenting, the boxes of the slide showing, for the wheel.

## Functions

- `CARET_Click`, `CARET_DragTo`, `CARET_SelectWord`, `CARET_SelectLine`, `CARET_ScrollBlock`: the mouse on the words.
- `CARET_BeforeDraw`, `CARET_Follow`, `CARET_KeepRuns`: around the preview's slide as it is laid out.
- `CARET_BeforeLayout`, `CARET_BuildBars`, `CARET_BuildRooms`, `CARET_BuildLook`: its UI boxes.
- `CARET_PresentAfterDraw`, `CARET_PresentWheel`: presenting.
- `CARET_BeforeDrawOf`, `CARET_AfterDrawOf`: another drawing of a slide, with its blocks where they were.

## Used by

- edit: around the preview, and a click on it.
- place: selecting with the mouse.
- frame.c: presenting, and the slide fading out.
- presenter: its drawings of the slides.

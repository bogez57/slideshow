# caret

The edit screen: typing into the slide. A click on the preview's title or a block of words puts the cursor at the letter clicked, and a caret shows there; the keys are the editor's, as ever, since the slide's words and the text pane are the same text with the one cursor. A block with more words than room shows the part the caret is in, with a scroll bar, and the wheel moves through it - presenting too. The ways of the mouse are at the top of caret.c.

## State

None in `App_State`. The caret's own, `g_caret_view` (`Caret_View`, caret.h), written with no op:

- `scroll`, `body_scroll`, `scroll_slide`, `seen_cursor`, `seen_edit_count`: how far down its words each block of the slide showing is, and what the room last followed.
- `left_scroll`, `left_body_scroll`, `left_slide`: the same, of the slide shown before, as it was left.
- `bar_drag`, `bar_block`, `bar_grab`, `bar_key`: a scroll bar held by the mouse.
- `focus_hidden`, `focus_hidden_cursor`: Esc has taken the box around the piece the cursor is in off the preview.
- `present_boxes`, `present_box_count`, `present_page`: presenting, the boxes of the slide showing, for the wheel.

The runs of letters it reads are edit's (`g_edit_view.runs`). A scroll is put on the slide only while it is drawn (`Caret_BeforeDraw`, `Caret_AfterDrawOf`).

## Functions

- `Caret_DragTo`, `Caret_ScrollBlock`, `Caret_ScrollToTop`: the mouse on the words, and their scroll.
- `Caret_Click`: the cursor to the letter at a point. It changes the cursor: called from ops only.
- Ops: `Caret_PressRoom` (a press in the room of the words), `Caret_DragSelect` (selecting by dragging), `Caret_SelectWord`, `Caret_SelectLine` (a double and a triple click).
- `Caret_BeforeDraw`, `Caret_Follow`, `Caret_KeepRuns`: around the preview's slide as it is laid out.
- `Caret_BeforeLayout`, `Caret_BuildBars`, `Caret_BuildRooms`, `Caret_BuildLook`: its UI boxes.
- `Caret_PresentAfterDraw`, `Caret_PresentWheel`: presenting.
- `Caret_BeforeDrawOf`, `Caret_AfterDrawOf`: another drawing of a slide, with its blocks where they were.

## Used by

- edit: around the preview, and a click on it.
- place: selecting with the mouse.
- frame.c: presenting, and the slide fading out.
- presenter: its drawings of the slides.

# place

The edit screen: a slide's pictures, title and blocks of words, moved and resized with the mouse in the preview, lined up with Shift, and put back with a right-click. What comes of it is an `@place` line in the deck's text, written when the button comes up. Pressing right at the words selects them instead. The mouse's ways and the boxes it is built from are at the top of place.c.

## State

None in `App_State`. The mouse's own, `g_place_view` (`Place_View`, place.h), written with no op:

- `drag` (`Place_Drag`): the press on a piece and what has come of it - a click so far, a drag, selecting words, or dropped and waiting for its line - with the guides Shift lines up with.
- `click_mouse`, `click_count`: the last press at the words, and whether it was a double or a triple click.

A drag changes nothing of the deck. The slide is given the place the mouse makes only while it is drawn: `Place_PutDrag` before `View_DrawSlide` and `Place_TakeDrag` after, in edit's layout and draws.

## Functions and ops

- `Place_Keys`, `Place_GiveUp`: Esc, and F5, give up a drag.
- `Place_BeforeLayout`: moves a drag with the mouse before the slide is laid out. When the button comes up it pushes `Place_WriteLine`, and the piece stays drawn where it was dropped until that has run.
- `Place_PutDrag`, `Place_TakeDrag`: around a drawing of the slide.
- Ops: `Place_WriteLine` (the dropped piece's `@place` line), `Place_RemoveLine` (a right-click: the line taken out). Clicks push edit's `Edit_PreviewClick` and caret's `Caret_SelectWord`, `Caret_SelectLine`.
- `Place_BuildCorners`, `Place_BuildHits`: the boxes that are clicked, after the slide is laid out.
- `Place_BuildLook`: what is drawn - outlines, handles, guides.
- `Place_IsAtWords`, `Place_PressWords`, `Place_SelectHeld`: selecting words with the mouse.
- `Place_BoxesAreGood`, `Place_BoxAt`: whether the preview's boxes are of the text as it is, and which one a point is on.

## Used by

- edit: builds its boxes around the preview's slide, and its keys.
- caret: selecting with the mouse, and the box a point is on.
- look: whether the preview's boxes can be read.

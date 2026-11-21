# place

The edit screen: a slide's pictures, title and blocks of words, moved and resized with the mouse in the preview, lined up with Shift, and put back with a right-click. What comes of it is an `@place` line in the deck's text, written when the button comes up. Pressing right at the words selects them instead. The mouse's ways and the boxes it is built from are at the top of place.c.

## State

In `App_State`:

- `place_drag` (`PLACE_Drag`): the press on a piece and what has come of it - a click so far, a drag, or selecting words - with the guides Shift lines up with.
- `place_click_mouse`, `place_click_count`: the last press at the words, and whether it was a double or a triple click.

## Functions

- `PLACE_Keys`, `PLACE_GiveUp`: Esc, and F5, give up a drag.
- `PLACE_BeforeLayout`: moves a drag with the mouse before the slide is laid out.
- `PLACE_BuildCorners`, `PLACE_BuildHits`: the boxes that are clicked, after the slide is laid out.
- `PLACE_BuildLook`: what is drawn - outlines, handles, guides.
- `PLACE_IsAtWords`, `PLACE_PressWords`, `PLACE_SelectHeld`: selecting words with the mouse.
- `PLACE_BoxesAreGood`, `PLACE_BoxAt`: whether the preview's boxes are of the text as it is, and which one a point is on.

## Used by

- edit: builds its boxes around the preview's slide, and its keys.
- caret: selecting with the mouse, and the box a point is on.
- look: whether the preview's boxes can be read.

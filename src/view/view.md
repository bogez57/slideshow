# view

Draws one slide into a rect: lays out its title, body and pictures on a page of `VIEW_PAGE_W` by `VIEW_PAGE_H` units, scaled to fit, and issues the draw calls. How a slide is laid out is at the top of view.h.

## State

- None. A slide is laid out again every time it is drawn.

## Functions

- `View_DrawSlide`: lays out and draws a slide. Given a `View_HitList`, it also says where each piece went (`View_Hit`, `View_Box`) and where its letters are (`View_Run`), for clicks and the caret.
- `View_ColorsFromTheme`: a theme's colours.
- `View_ScaleFromDim`, `View_PageRectFromRect`: where the page goes in a rect, and how large.
- `View_DrawImageFit`: a picture as large as fits, its shape kept.
- `VIEW_MARGIN_X`, `VIEW_MARGIN_TOP`, `VIEW_MARGIN_BOTTOM`: the page's margins.

## Used by

- frame.c: the presentation.
- edit: the preview, and the small slides.
- presenter: the slide showing and the next one.
- place, caret, look: the boxes and runs of the preview's slide; place lines pieces up with the margins.

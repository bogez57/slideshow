////////////////////////////////
//~ bgz: view
//
// Draws one slide into a rect - the whole window when presenting, the preview
// when editing: lays out its title, body and picture, and issues the draw
// calls. Nothing is kept between frames - a slide is laid out again every
// time it is drawn.
//
// A slide is designed on a page of VIEW_PAGE_W by VIEW_PAGE_H units, which is
// scaled to the largest size that fits the rect with its shape kept, in the
// middle. Every size in view.c is in those units. The page has no edge of its
// own: what is behind the rect shows around it.
//
// A slide that is only a picture is not put on the page: the picture fills
// the rect.

#ifndef VIEW_H
#define VIEW_H

#define VIEW_PAGE_W 1280.0f
#define VIEW_PAGE_H 720.0f

// How many pixels a page unit is in a rect of this size.
internal F32 VIEW_ScaleFromDim(Vec2F32 dim);

// The largest rect of the page's shape that fits in rect, in the middle.
internal Rng2F32 VIEW_PageRectFromRect(Rng2F32 rect);

// The image as large as fits in rect with its shape kept, in the middle of
// it. alpha is how much of it shows over what is already drawn.
internal void VIEW_DrawImageFit(IMG_Image* image, Rng2F32 rect, F32 alpha);

// image is the slide's picture, or 0 to draw the slide without it: loading is
// the caller's business (DECK_ImageFromSlide). alpha is as for
// VIEW_DrawImageFit, for the whole slide. Line lists for wrapped text go on
// arena, and are not needed after the call. Nothing is drawn outside rect.
//
// A picture that could not be loaded (slide->load_failed) is drawn as a box
// with its file's name in it.
internal void VIEW_DrawSlide(Arena* arena, TXT_Fonts* fonts, DECK_Slide* slide, IMG_Image* image, Rng2F32 rect, F32 alpha);

#endif // VIEW_H

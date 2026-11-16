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
//
// The deck's theme gives the colours, and each slide's layout says where its
// picture goes - beside the words on either side, or behind them - or puts
// every line in the middle.

#ifndef VIEW_H
#define VIEW_H

#define VIEW_PAGE_W 1280.0f
#define VIEW_PAGE_H 720.0f

// A theme's colours. The background is the caller's to draw, behind the
// slide and around it: a slide is drawn over it, at any alpha.
typedef struct VIEW_Colors VIEW_Colors;
struct VIEW_Colors {
   Vec4F32 background;
   Vec4F32 title;
   Vec4F32 body;
   Vec4F32 accent; // the bar under the title, headings, bullets' dots
};

internal VIEW_Colors VIEW_ColorsFromTheme(DECK_Theme theme);

// How many pixels a page unit is in a rect of this size.
internal F32 VIEW_ScaleFromDim(Vec2F32 dim);

// The largest rect of the page's shape that fits in rect, in the middle.
internal Rng2F32 VIEW_PageRectFromRect(Rng2F32 rect);

// The image as large as fits in rect with its shape kept, in the middle of
// it. alpha is how much of it shows over what is already drawn.
internal void VIEW_DrawImageFit(IMG_Image* image, Rng2F32 rect, F32 alpha);

// Where a piece of a slide was drawn, and where in the deck's text it is
// from: what a click on the slide is a click on.
typedef struct VIEW_Hit VIEW_Hit;
struct VIEW_Hit {
   VIEW_Hit* next;
   Rng2F32   rect;
   Rng1U64   text_range;
};

typedef struct VIEW_HitList VIEW_HitList;
struct VIEW_HitList {
   VIEW_Hit* first;
   VIEW_Hit* last;
   U64       count;
};

// image is the slide's picture, or 0 to draw the slide without it: loading is
// the caller's business (DECK_ImageFromSlide). alpha is as for
// VIEW_DrawImageFit, for the whole slide. Line lists for wrapped text go on
// arena, and are not needed after the call. Nothing is drawn outside rect.
//
// A picture that could not be loaded (slide->load_failed) is drawn as a box
// with its file's name in it.
//
// hits, if not 0, is given a hit for the title, for each line of the body
// and for the picture, on arena.
//
// theme is the deck's, for the colours. The slide's own layout
// (slide->layout) says where its picture and words go.
internal void VIEW_DrawSlide(Arena* arena, TXT_Fonts* fonts, DECK_Theme theme, DECK_Slide* slide, IMG_Image* image,
                             Rng2F32 rect, F32 alpha, VIEW_HitList* hits);

#endif // VIEW_H

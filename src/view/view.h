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
// pictures go - beside the words on either side, or the first behind them -
// or puts every line in the middle. Several pictures share the room the
// layout has for them, as a grid. An @place line puts one picture, the
// title or a block of the body where it says instead. The body's blocks
// (@box) that are not placed are one under another, where the body is.
// Words are never made smaller to fit: a block's that do not fit its room
// are not shown, and DECK_Box's scroll says how far down them it shows.
// @size and @color lines (DECK_Look) give the title's words, or a block's,
// a size and a colour of their own.

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

// Where something an @place line can move was put. In pixels. A picture's
// is the picture as drawn, a block of the body's as tall as its words, the
// title's as wide as its words. As many as a slide can have: each picture,
// each block and the title.
#define VIEW_MAX_BOXES (DECK_MAX_PICTURES + DECK_MAX_BOXES + 1)
typedef struct VIEW_Box VIEW_Box;
struct VIEW_Box {
   DECK_PlaceKind kind;
   U64            idx; // which of the slide's pictures, or blocks of its body (DECK_Box). 0 for the title
   Rng2F32        rect;
   // The part of their usual size its words were drawn at: an @size line's,
   // or 1. 0 for a picture.
   F32 text_size;
   // The room its words have: they wrap at its width, and those under its
   // bottom are not shown. A placed block's or title's is its place. A
   // block's that is not placed is the body's width, down to where the
   // next block starts - or, for the last of them, the body's bottom. A
   // picture's is rect.
   Rng2F32 room;
   // The y under its last word, as drawn: under room's bottom when there
   // are more words than room. 0 for a picture.
   F32 words_max_y;
};

// A run of letters as drawn: one line of a wrapped title or heading, or
// one piece of a line of the body (a word, or a part of one drawn alike).
// With these, a letter of the deck's text can be found on the slide, and a
// point on the slide in the text (frame_caret.c). Runs of words that are
// not shown - under the bottom of their block's room - are here too.
typedef struct VIEW_Run VIEW_Run;
struct VIEW_Run {
   VIEW_Run*      next;
   Rng2F32        rect;       // the letters across, the line they are on down
   Rng1U64        text_range; // where they are in the deck's text. Empty for a line with no words yet
   FNT_Tag        font;       // what they were drawn with: TXT_Width with these
   F32            px;         // measures them as drawn
   DECK_PlaceKind kind;       // whose they are: the title's, or a block's
   U64            idx;        // which block
};

typedef struct VIEW_HitList VIEW_HitList;
struct VIEW_HitList {
   VIEW_Hit* first;
   VIEW_Hit* last;
   U64       count;

   VIEW_Run* first_run;
   VIEW_Run* last_run;
   U64       run_count;

   // Where the slide's title, each block of its body and each of its
   // pictures were put: what an @place line would move. None for a picture
   // behind the words.
   VIEW_Box boxes[VIEW_MAX_BOXES];
   U64      box_count;
   Rng2F32  page; // the page the slide was drawn on
};

// The slide is drawn with whichever of its pictures are loaded: loading is
// the caller's business (DECK_LoadSlide). alpha is as for
// VIEW_DrawImageFit, for the whole slide. Line lists for wrapped text go on
// arena, and are not needed after the call. Nothing is drawn outside rect.
//
// A picture that could not be loaded (slide->load_failed) is drawn as a box
// with its file's name in it.
//
// hits, if not 0, is given a hit for the title, for each line of the body
// and for each picture, on arena, the boxes of what can be moved, and the
// runs of letters.
//
// theme is the deck's, for the colours. The slide's own layout
// (slide->layout) says where its picture and words go, and its @place lines
// (slide->places, and the pictures' and blocks' own) put any of them
// somewhere else.
internal void VIEW_DrawSlide(Arena* arena, TXT_Fonts* fonts, DECK_Theme theme, DECK_Slide* slide, Rng2F32 rect, F32 alpha,
                             VIEW_HitList* hits);

#endif // VIEW_H

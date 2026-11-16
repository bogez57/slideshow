////////////////////////////////
//~ bgz: deck
//
// The slides, in the order they are shown. A deck is opened from one of:
//
//  - A folder of images. Every image file directly in it is a slide, in order
//    of file name - slide2 before slide10. Opening one image file opens its
//    folder, starting at that image.
//
//  - A text file (.md or .txt) that says what is on each slide - the format
//    is below. A folder with a slides.md in it opens as that file. The
//    program edits these: the deck is then made from the editor's text
//    (DECK_OpenText), again at every change.
//
// A slide's image is loaded the first time it is asked for, not when the deck
// is opened, and only the slides around the one showing are kept: a folder of
// photos is far more than fits in video memory at once.
//
//- bgz: the text format
//
// A small part of Markdown, read a line at a time:
//
//    # Title            starts a new slide, with this title
//                       (the space after the # or ## can be left out)
//    ---                starts a new slide with no title
//    ## Heading         a heading inside the slide
//    - point            a bullet. Two more spaces in front for each level in
//    * point            (up to DECK_MAX_BULLET_LEVEL)
//    ![](picture.png)   the slide's picture, found from the text file's folder.
//                       A slide has one: the first
//    @theme light       the whole deck's colours: dark (as with no such line),
//                       light, blue or paper. Anywhere in the file
//    @layout center     how the slide it is in is laid out: picture-right (as
//                       with no such line), picture-left, picture-behind (the
//                       picture fills the slide, behind the words) or center
//                       (every line in the middle, bullets without their dots)
//    @note Say this     a note for whoever presents, on the slide it is in: not
//                       drawn on the slide, shown beside it on request
//    anything else      a paragraph. Each line is its own
//    **words**          in a paragraph or a bullet: drawn bold
//    `words`            in a paragraph or a bullet: drawn as code (view.c)
//    (an empty line)    a little more room before what follows
//
// A slide with only a title shows it large, in the middle. A slide with only
// a picture shows it filling the window, as a folder of images does.

#ifndef DECK_H
#define DECK_H

#define DECK_MAX_BULLET_LEVEL 3

// The colours of a whole deck. The names, as written after @theme, are in
// deck.c (DECK_ThemeName), in this order.
typedef enum DECK_Theme {
   DECK_Theme_Dark,
   DECK_Theme_Light,
   DECK_Theme_Blue,
   DECK_Theme_Paper,
   DECK_Theme_COUNT,
} DECK_Theme;

// How one slide is laid out. The names, as written after @layout, are in
// deck.c (DECK_LayoutName), in this order.
typedef enum DECK_Layout {
   DECK_Layout_PictureRight, // the body on the left, the picture on the right
   DECK_Layout_PictureLeft,
   DECK_Layout_PictureBehind, // the picture fills the slide, behind the words
   DECK_Layout_Center,        // every line in the middle
   DECK_Layout_COUNT,
} DECK_Layout;

typedef enum DECK_ItemKind {
   DECK_ItemKind_Text,
   DECK_ItemKind_Bullet,
   DECK_ItemKind_Heading,
} DECK_ItemKind;

// One line of a slide's body.
typedef struct DECK_Item DECK_Item;
struct DECK_Item {
   DECK_Item*    next;
   DECK_ItemKind kind;
   String8       text;
   U32           level;      // bullets: 0 is outermost
   B32           gap_before; // an empty line came before it
   Rng1U64       text_range; // where its words are in the deck's text. Empty, at the line's end, for no words
};

typedef struct DECK_Slide DECK_Slide;
struct DECK_Slide {
   String8   image_path;  // empty: the slide has no picture
   IMG_Image image;       // zero until loaded
   B32       load_failed; // tried once and could not be: not tried again

   // Where in the deck's text the slide begins: the start of its first line.
   // 0 in a folder of images.
   U64 text_pos;

   // Where in the deck's text the title's words are, and the picture's
   // path: as DECK_Item's text_range. Zero for a slide without the one or
   // the other, and in a folder of images.
   Rng1U64 title_range;
   Rng1U64 image_range;

   // From the slide's @layout line, which is at layout_line in the deck's
   // text (the line without its ending) when has_layout_line.
   DECK_Layout layout;
   B32         has_layout_line;
   Rng1U64     layout_line;

   // The slide's @note lines, in order: for whoever presents, not on the
   // slide.
   String8List notes;

   // Empty and zero for a slide that is only a picture.
   String8    title;
   DECK_Item* first_item;
   DECK_Item* last_item;
   U64        item_count;
};

typedef struct DECK_Deck DECK_Deck;
struct DECK_Deck {
   Arena*      arena;  // the deck's own: everything it points to is on it
   String8     folder; // of the images, or that the text file is in
   DECK_Slide* slides;
   U64         slide_count;

   // For a deck from a text file: the file. Empty for a folder of images.
   String8 file;

   // From the file's first @theme line, which is at theme_line in the text
   // (the line without its ending) when has_theme_line.
   DECK_Theme theme;
   B32        has_theme_line;
   Rng1U64    theme_line;
};

// As written after @theme and @layout.
internal String8 DECK_ThemeName(DECK_Theme theme);
internal String8 DECK_LayoutName(DECK_Layout layout);

// Never 0. A folder that is missing, or has no images in it, gives a deck
// with no slides.
internal DECK_Deck* DECK_OpenFolder(String8 folder);

// Reads a deck from a text file. Never 0: a file that is missing or has
// nothing in it gives a deck with no slides.
internal DECK_Deck* DECK_OpenFile(String8 file);

// A deck from text that is already in memory - the editor's, which may not be
// what is in the file. The deck keeps its own copy. Line endings must be \n
// alone. file is where the text is from: pictures are found from its folder.
internal DECK_Deck* DECK_OpenText(String8 text, String8 file);

// The slide a position in the deck's text is on: the last that begins at or
// before it. 0 if there are no slides.
internal U64 DECK_SlideFromTextPos(DECK_Deck* deck, U64 text_pos);

// Moves the pictures src has loaded over to the slides of dst that show the
// same file, so that a deck read again does not decode them again. src is
// left with none of the ones moved. A file src could not load is marked as
// not loadable in dst too.
internal void DECK_TakeImages(DECK_Deck* dst, DECK_Deck* src);

// Opens whichever kind of deck path is: a text file, a folder, or one image
// file. An image opens the folder it is in, and first_slide_out is given its
// slide's index, which is where to start. For the others that is 0.
internal DECK_Deck* DECK_OpenPath(String8 path, U64* first_slide_out);

// Whether there is anything on the slide but its picture.
internal B32 DECK_SlideHasText(DECK_Slide* slide);

// The names of the image files in a folder and in the folders directly inside
// it, as a slide's ![](...) line would name them from a text file there:
// "hills.png", "pictures/bars.jpg". In order of name, on arena.
internal String8Array DECK_PictureNamesFromFolder(Arena* arena, String8 folder);

// Releases every image the deck has loaded, and the deck itself.
internal void DECK_Close(DECK_Deck* deck);

// The image of a slide, loaded now if it is not yet. 0 if there is no such
// slide, it has no picture, or the file could not be loaded. Slides more than one away from this
// one have their images released.
internal IMG_Image* DECK_ImageFromSlide(DECK_Deck* deck, U64 slide_idx);

// The same, but releases nothing: for a second look at a slide close to the
// one DECK_ImageFromSlide was last asked for (the presenter's next slide).
internal IMG_Image* DECK_LoadImageOfSlide(DECK_Deck* deck, U64 slide_idx);

// The image of a slide only if it is loaded already, else 0. Loads nothing
// and releases nothing.
internal IMG_Image* DECK_LoadedImageFromSlide(DECK_Deck* deck, U64 slide_idx);

#endif // DECK_H

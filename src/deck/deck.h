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
//    ![](picture.png)   a picture on the slide, found from the text file's
//                       folder. A slide can have several, up to
//                       DECK_MAX_PICTURES: they share the room pictures have
//    ![](clip.mp4)      the same line with a video file: the picture is the
//                       video, playing (src\video)
//    @theme light       the whole deck's colours: dark (as with no such line),
//                       light, blue or paper. Anywhere in the file
//    @layout center     how the slide it is in is laid out: picture-right (as
//                       with no such line), picture-left, picture-behind (the
//                       picture fills the slide, behind the words) or center
//                       (every line in the middle, bullets without their dots)
//    @video loop mute   how the video of the slide it is in plays, any of:
//                       loop (starts over at its end), mute (no sound from
//                       this one), manual (waits for K, not starting when
//                       the slide comes up)
//    @place picture 0.55 0.2 0.4 0.6
//                       where on the slide a picture - or video - goes, in
//                       place of where the layout would put it: its left,
//                       top, width and height, each as a part of the page's
//                       width or height (0.5 is half). The picture is as
//                       large as fits in that with its shape kept. Which
//                       picture: the one whose line is the last above this
//                       one on the slide - or the slide's first, with none
//                       above
//    @place text 0.05 0.3 0.5 0.6
//                       the same for the slide's body: every line under the
//                       title - or, on a slide with @box lines, the block
//                       of it the line is in. The words wrap at its width,
//                       and are made smaller if they do not fit its height
//    @box               the lines of the body under it, on this slide, are
//                       another block of words: one that @place text, under
//                       it, puts somewhere of its own. Up to DECK_MAX_BOXES
//                       to a slide. Blocks that are not placed are one under
//                       another, where the body is
//    @place title 0.05 0.05 0.6 0.2
//                       the same for the title, which wraps at the width.
//                       The edit screen writes all three, from the mouse
//    @size title 1.5    how large the title's words are, as so many times
//                       their usual size (0.5 is half)
//    @size text 0.8     the same for the body's words - on a slide with
//                       @box lines, those of the block the line is in.
//                       Words given a size are not made smaller to fit
//    @color title red   the colour of the title's words: one of the names
//                       in deck.c (DECK_NamedColorFromIdx) - white, black,
//                       gray, red, orange, yellow, green, blue, purple,
//                       pink - or #rrggbb, as a web page has it. In place
//                       of the theme's
//    @color text #ffcc00
//                       the same for the body's words, or the block's. The
//                       edit screen writes these four too (frame_look.c)
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

// How many pictures a slide can have: lines past these are passed over.
#define DECK_MAX_PICTURES 8

// How many blocks of words a slide's body can be in: an @box line past
// these starts nothing, and its lines go on in the last.
#define DECK_MAX_BOXES 8

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

// What of a slide an @place line can put somewhere. The names, as written
// after @place, are in deck.c (DECK_PlaceName), in this order.
typedef enum DECK_PlaceKind {
   DECK_PlaceKind_Picture, // or video: one of the slide's (DECK_Picture's place)
   DECK_PlaceKind_Text,    // the body: every line under the title
   DECK_PlaceKind_Title,
   DECK_PlaceKind_COUNT,
} DECK_PlaceKind;

// From an @place line.
typedef struct DECK_Place DECK_Place;
struct DECK_Place {
   B32     is_set;
   Rng2F32 rect; // in parts of the page: 0,0 its top left, 1,1 its bottom right
   // Where in the deck's text the line is (without its ending) when
   // has_line: is_set without it is a place the edit screen is trying out
   // (frame_edit.c), not yet written.
   B32     has_line;
   Rng1U64 line;
};

// The smallest a placed thing can be, as a part of the page.
#define DECK_PLACE_MIN 0.04f

// How the words of a title, or of a block of the body, look where the
// deck's text says so: from @size and @color lines. Without them, the size
// is view.c's and the colour the theme's.
typedef struct DECK_Look DECK_Look;
struct DECK_Look {
   B32     has_size;
   F32     size; // times the usual size
   B32     has_color;
   Vec4F32 color;
   // Where in the deck's text each line is (without its ending), when there
   // is one: for the edit screen to write over.
   Rng1U64 size_line;
   Rng1U64 color_line;
};

// The smallest and the largest an @size line can make words.
#define DECK_SIZE_MIN 0.25f
#define DECK_SIZE_MAX 4.0f

// A colour an @color line can ask for by name.
typedef struct DECK_NamedColor DECK_NamedColor;
struct DECK_NamedColor {
   String8 name;
   Vec4F32 color;
};
#define DECK_NAMED_COLOR_COUNT 10

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

// One block of a slide's body: its items are item_count of the slide's,
// from first_item on.
typedef struct DECK_Box DECK_Box;
struct DECK_Box {
   DECK_Item* first_item;
   U64        item_count;
   DECK_Place place; // from its @place text line
   DECK_Look  look;  // from its @size text and @color text lines
   // Where in the deck's text the block begins: the start of its @box line
   // - or, for a slide's first block, where the slide begins.
   U64 text_pos;
};

// One of a slide's pictures.
typedef struct DECK_Picture DECK_Picture;
struct DECK_Picture {
   String8   path;        // of the file
   IMG_Image image;       // zero until loaded
   B32       load_failed; // tried once and could not be: not tried again

   // A picture that is a video file (VID_NameIsVideo) is played, not
   // decoded once: video is its player, open for as long as image is
   // loaded, and image is the player's texture - so a video is drawn as a
   // picture is, by code that does not know the difference.
   VID_Video* video;

   // Where in the deck's text the path is: as DECK_Item's text_range. Zero
   // in a folder of images.
   Rng1U64 range;

   // From its @place line.
   DECK_Place place;

   // For one of a deck's spares only: when it lost its slide.
   U64 spared_us;
};

typedef struct DECK_Slide DECK_Slide;
struct DECK_Slide {
   // The slide's pictures, in the order of their lines. On the deck's arena.
   // Lines past DECK_MAX_PICTURES are counted in pictures_passed_over, so
   // the edit screen can say they are not shown.
   DECK_Picture* pictures;
   U64           picture_count;
   U64           pictures_passed_over;

   // Where in the deck's text the slide begins: the start of its first line.
   // 0 in a folder of images.
   U64 text_pos;

   // Where in the deck's text the title's words are: as DECK_Item's
   // text_range. Zero for a slide without one, and in a folder of images.
   Rng1U64 title_range;

   // From the slide's @layout line, which is at layout_line in the deck's
   // text (the line without its ending) when has_layout_line.
   DECK_Layout layout;
   B32         has_layout_line;
   Rng1U64     layout_line;

   // From the slide's @place lines, by DECK_PlaceKind. The pictures' are
   // their own (DECK_Picture's place) and so are the body's blocks'
   // (DECK_Box's): the ones here for those are not used.
   DECK_Place places[DECK_PlaceKind_COUNT];

   // From the slide's @size title and @color title lines. The body's
   // blocks have their own (DECK_Box's look).
   DECK_Look title_look;

   // The body's blocks: all of it is the first, until an @box line. A
   // block with no items is one being written. 0 of them in a folder of
   // images.
   DECK_Box boxes[DECK_MAX_BOXES];
   U64      box_count;

   // From the slide's @video line: how its video plays (frame_video.c).
   B32 video_loop;   // starts over when it reaches its end
   B32 video_mute;   // no sound from it
   B32 video_manual; // does not start when the slide comes up

   // The slide's @note lines, in order: for whoever presents, not on the
   // slide.
   String8List notes;

   // Empty and zero for a slide that is only a picture.
   String8    title;
   DECK_Item* first_item;
   DECK_Item* last_item;
   U64        item_count;
};

// A deck's spares (below): how many at most, and for how long.
#define DECK_SPARE_MAX     8
#define DECK_SPARE_SECONDS 5.0

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

   // Pictures that were loaded for slides that are gone, kept while a slide
   // of the deck still shows their file (DECK_TakeImages). Typing over a
   // title line takes its slide away for a key or two: when the slide is
   // back it gets its picture back, and a video goes on from where it was.
   // Only path, image, video and spared_us are set. At most DECK_SPARE_MAX.
   DECK_Picture* spares;
   U64           spare_count;
};

// As written after @theme and @layout.
internal String8 DECK_ThemeName(DECK_Theme theme);
internal String8 DECK_LayoutName(DECK_Layout layout);
// As written after @place.
internal String8 DECK_PlaceName(DECK_PlaceKind kind);

// The colours with names, idx under DECK_NAMED_COLOR_COUNT.
internal DECK_NamedColor DECK_NamedColorFromIdx(U64 idx);
// The colour an @color line's word is: a name, or #rrggbb (or #rgb). 0 if
// it is neither.
internal B32 DECK_ColorFromString(String8 string, Vec4F32* color_out);

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

// Moves the pictures src has loaded over to the pictures of dst's slides that show the
// same file, so that a deck read again does not decode them again. src is
// left with none of the ones moved. A file src could not load is marked as
// not loadable in dst too. What no slide of dst takes, of a file a slide of
// dst still shows, goes to dst's spares.
internal void DECK_TakeImages(DECK_Deck* dst, DECK_Deck* src);

// Opens whichever kind of deck path is: a text file, a folder, or one image
// file. An image opens the folder it is in, and first_slide_out is given its
// slide's index, which is where to start. For the others that is 0.
internal DECK_Deck* DECK_OpenPath(String8 path, U64* first_slide_out);

// Whether there is anything on the slide but its picture.
internal B32 DECK_SlideHasText(DECK_Slide* slide);

// The names of the image and video files in a folder and in the folders directly inside
// it, as a slide's ![](...) line would name them from a text file there:
// "hills.png", "pictures/bars.jpg". In order of name, on arena.
internal String8Array DECK_PictureNamesFromFolder(Arena* arena, String8 folder);

// Releases every image the deck has loaded, and the deck itself.
internal void DECK_Close(DECK_Deck* deck);

// Loads the pictures of a slide that are not loaded yet, and releases those
// of slides more than one away from it: for the slide that is showing. A
// slide is drawn with whichever of its pictures are loaded (VIEW_DrawSlide).
internal void DECK_LoadSlide(DECK_Deck* deck, U64 slide_idx);

// The same, but releases nothing: for a second look at a slide close to the
// one DECK_LoadSlide was last asked for (the presenter's next slide).
internal void DECK_LoadSlideOnly(DECK_Deck* deck, U64 slide_idx);

// The first of a slide's videos that is open - as long as its image is
// loaded - else 0: the one whose time and state stand for the slide's.
internal VID_Video* DECK_VideoFromSlide(DECK_Deck* deck, U64 slide_idx);
// Every open video of a slide, into videos, which has room for
// DECK_MAX_PICTURES. Returns how many.
internal U64 DECK_VideosFromSlide(DECK_Deck* deck, U64 slide_idx, VID_Video** videos);
// Whether any of the slide's pictures is a video file, open or not. Not one
// that could not be opened.
internal B32 DECK_SlideHasVideo(DECK_Deck* deck, U64 slide_idx);
// Whether all of them that are video files are open: none still to load.
internal B32 DECK_SlideVideosAreOpen(DECK_Deck* deck, U64 slide_idx);
// Once a frame: every open video's VID_Update, and spares kept for longer
// than DECK_SPARE_SECONDS released.
internal void DECK_UpdateVideos(DECK_Deck* deck);

// Lets go of a picture and loads it again: for a file that has changed, or
// whose path now leads somewhere else (below).
internal void DECK_ReopenPicture(DECK_Picture* picture);

// From now on, a slide whose picture is at path is shown the file at
// stands_for instead - until DECK_ClearPathAlias(path). For a file that is
// on its way to path and not there yet (frame_copy.c). For every deck: a
// deck read again from its text keeps showing the same file. Slides that
// have the picture loaded already are not changed: DECK_ReopenImage does
// that.
internal void DECK_SetPathAlias(String8 path, String8 stands_for);
internal void DECK_ClearPathAlias(String8 path);

#endif // DECK_H

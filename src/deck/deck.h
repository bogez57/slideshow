#pragma once

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
//    (Deck_OpenText), again at every change.
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
//                       video, playing (src\video). A .gif is played too,
//                       over and over, with no sound and no controls
//    @slide             starts a new slide, as --- does. A # Title under it is
//                       that slide's title, and not one more slide. An
//                       @color, @size or @place line between the two that
//                       does not say whose is the title's:
//                          @slide
//                          @color red
//                          # Title
//    @canvas            starts a board: a slide with no edge, to put ideas
//                       down on before there are slides. Its blocks and
//                       pictures can be placed anywhere, off the page too,
//                       and a block is as tall as its words. Nothing in it
//                       starts a slide but ---, @slide or another @canvas:
//                       a # line is a heading there. Not shown when
//                       presenting
//                       Without an @slide: lines about the slide or its
//                       title - @place, @size, @color, @align, @layout,
//                       @video - right above a # Title, no empty line
//                       between, are that slide's too. What a line is about
//                       is under it:
//                          @place title 0.05 0.1 0.6 0.2
//                          # Title
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
//                       picture: the one whose line is right under this
//                       one. With no picture's line right under it, the
//                       one whose line is the last above it on the slide -
//                       or the slide's first, with none above
//    @place text 0.05 0.3 0.5 0.6
//                       the same for the slide's body: every line under the
//                       title - or, on a slide with @box lines, the block
//                       of it the line is in. The words wrap at its width,
//                       and stay the size they are: those that do not fit
//                       its height are not shown
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
//    @size 0.8          the same for the body's words - on a slide with
//                       @box lines, those of the block the line is in.
//                       Whose words need not be said: a line that does not
//                       say title is the block's. "@size text 0.8" is the
//                       same, and so it is for @color and @place
//    @color title red   the colour of the title's words: one of the names
//                       in deck.c (Deck_NamedColorFromIdx) - white, black,
//                       gray, red, orange, yellow, green, blue, purple,
//                       pink - or #rrggbb, as a web page has it. In place
//                       of the theme's
//    @color #ffcc00     the same for the body's words, or the block's. The
//                       edit screen writes these four too (look\look.c)
//                       In a block, under its first words, an @size, @color
//                       or @align line is for the one line under it, and
//                       not the block: the lines after that one are as the
//                       block is again
//                          @size 0.8
//                          small, as all the block is
//                          @size 2
//                          @color red
//                          this one line large and red
//                          small again
//                       With "line" it is for the one line under it
//                       wherever it is, at the block's top too:
//                       "@color line red" above a block's first line is
//                       that line's, and not the block's
//    @align center      which side the lines of the block it is in are
//                       against: left, center or right. Centered or to the
//                       right, bullets are lines like any other, without
//                       their dots. "@align title right" is the title's, as
//                       with @size and @color. Without the line: as the
//                       slide's layout has it
//    @note Say this     a note for whoever presents, on the slide it is in: not
//                       drawn on the slide, shown beside it on request
//    anything else      a paragraph. Each line is its own
//    **words**          in a paragraph or a bullet: drawn bold
//    `words`            in a paragraph or a bullet: drawn as code (view.c)
//    {red:words}        in a paragraph or a bullet: those words in a colour
//                       of their own - a name, or #rrggbb. {1.5:words} is
//                       a size, as a part of the line's, and {red 1.5:words}
//                       both. Anything else between { and : is not a mark,
//                       and is drawn as written (Deck_SpanOpenAt)
//    (an empty line)    a little more room before what follows
//
// What is over what, where a slide's pieces overlap: the order of their
// lines. A picture or a block of words is drawn over those whose lines are
// before its own, so the last in the text is on top. The picture of the
// picture-behind layout is under everything, and the title over everything.
//
// A slide with only a title shows it large, in the middle. A slide with only
// a picture shows it filling the window, as a folder of images does.

#define DECK_MAX_BULLET_LEVEL 3

// How many pictures a slide can have: lines past these are passed over.
#define DECK_MAX_PICTURES 24

// How many blocks of words a slide's body can be in: an @box line past
// these starts nothing, and its lines go on in the last.
#define DECK_MAX_BOXES 48

// The colours of a whole deck. The names, as written after @theme, are in
// deck.c (Deck_ThemeName), in this order.
typedef enum Deck_Theme {
   Deck_Theme_Dark,
   Deck_Theme_Light,
   Deck_Theme_Blue,
   Deck_Theme_Paper,
   Deck_Theme_COUNT,
} Deck_Theme;

// How one slide is laid out. The names, as written after @layout, are in
// deck.c (Deck_LayoutName), in this order.
typedef enum Deck_Layout {
   Deck_Layout_PictureRight, // the body on the left, the picture on the right
   Deck_Layout_PictureLeft,
   Deck_Layout_PictureBehind, // the picture fills the slide, behind the words
   Deck_Layout_Center,        // every line in the middle
   Deck_Layout_COUNT,
} Deck_Layout;

// What of a slide an @place line can put somewhere. The names, as written
// after @place, are in deck.c (Deck_PlaceName), in this order.
typedef enum Deck_PlaceKind {
   Deck_PlaceKind_Picture, // or video: one of the slide's (Deck_Picture's place)
   Deck_PlaceKind_Text,    // the body: every line under the title
   Deck_PlaceKind_Title,
   Deck_PlaceKind_COUNT,
} Deck_PlaceKind;

// From an @place line.
typedef struct Deck_Place Deck_Place;
struct Deck_Place {
   B32     is_set;
   Rng2F32 rect; // in parts of the page: 0,0 its top left, 1,1 its bottom right
   // Where in the deck's text the line is (without its ending) when
   // has_line: is_set without it is a place the edit screen is trying out
   // (edit\edit.c), not yet written.
   B32     has_line;
   Rng1U64 line;
};

// How far from the page, in pages, a board's pieces can be placed.
#define DECK_CANVAS_REACH 40.0f

// The smallest a placed thing can be, as a part of the page.
#define DECK_PLACE_MIN 0.04f

// How the words of a title, or of a block of the body, look where the
// deck's text says so: from @size and @color lines. Without them, the size
// is view.c's and the colour the theme's.
// Where each line of some words is, across the room they have. The names,
// as written after @align, are in deck.c (Deck_AlignName), in this order.
typedef enum Deck_Align {
   Deck_Align_Left,
   Deck_Align_Center,
   Deck_Align_Right,
   Deck_Align_COUNT,
} Deck_Align;

typedef struct Deck_Look Deck_Look;
struct Deck_Look {
   B32     has_size;
   F32     size; // times the usual size
   B32     has_color;
   Vec4F32 color;
   // Where in the deck's text each line is (without its ending), when there
   // is one: for the edit screen to write over.
   Rng1U64 size_line;
   Rng1U64 color_line;

   // From an @align line: which side its lines are against, or the middle.
   // Without one, as the slide's layout has it (Deck_AlignOf).
   B32        has_align;
   Deck_Align align;
   Rng1U64    align_line;
};

// The smallest and the largest an @size line can make words.
#define DECK_SIZE_MIN 0.25f
#define DECK_SIZE_MAX 4.0f

// A colour an @color line can ask for by name.
typedef struct Deck_NamedColor Deck_NamedColor;
struct Deck_NamedColor {
   String8 name;
   Vec4F32 color;
};
#define DECK_NAMED_COLOR_COUNT 10

typedef enum Deck_ItemKind {
   Deck_ItemKind_Text,
   Deck_ItemKind_Bullet,
   Deck_ItemKind_Heading,
} Deck_ItemKind;

// One line of a slide's body.
typedef struct Deck_Item Deck_Item;
struct Deck_Item {
   Deck_Item*    next;
   Deck_ItemKind kind;
   String8       text;
   U32           level;      // bullets: 0 is outermost
   B32           gap_before; // an empty line came before it
   Rng1U64       text_range; // where its words are in the deck's text. Empty, at the line's end, for no words
   // Its own, from @size, @color and @align lines above it in the block,
   // under the block's first words: for this line only. What it does not
   // have is the block's.
   Deck_Look     look;
};

// One block of a slide's body: its items are item_count of the slide's,
// from first_item on.
typedef struct Deck_Box Deck_Box;
struct Deck_Box {
   Deck_Item* first_item;
   U64        item_count;
   Deck_Place place; // from its @place text line
   Deck_Look  look;  // from its @size text and @color text lines
   // Where in the deck's text the block begins: the start of its @box line
   // - or, for a slide's first block, where the slide begins.
   U64 text_pos;
   // How far up its words are moved in its place, in page units: a block
   // with more words than its place has room for shows this far down them.
   // Not from the text: the edit screen's, while it is typed in
   // (caret\caret.c), and 0 when presenting. For a block with an @place
   // line: the others move together, by the slide's body_scroll.
   F32 scroll;
};

// One of a slide's pictures.
typedef struct Deck_Picture Deck_Picture;
struct Deck_Picture {
   String8   path;        // of the file
   Image_Image image;       // zero until loaded
   B32       load_failed; // tried once and could not be: not tried again

   // A picture that is a video file (Video_NameIsVideo) is played, not
   // decoded once: video is its player, open for as long as image is
   // loaded, and image is the player's texture - so a video is drawn as a
   // picture is, by code that does not know the difference.
   Video_Video* video;

   // Where in the deck's text the path is: as Deck_Item's text_range. Zero
   // in a folder of images.
   Rng1U64 range;

   // From its @place line.
   Deck_Place place;

   // For one of a deck's spares only: when it lost its slide.
   U64 spared_us;
};

typedef struct Deck_Slide Deck_Slide;
struct Deck_Slide {
   // The slide's pictures, in the order of their lines. On the deck's arena.
   // Lines past DECK_MAX_PICTURES are counted in pictures_passed_over, so
   // the edit screen can say they are not shown.
   Deck_Picture* pictures;
   U64           picture_count;
   U64           pictures_passed_over;

   // Where in the deck's text the slide begins: the start of its first line.
   // 0 in a folder of images.
   U64 text_pos;

   // Begun by an @canvas line: a board, not a slide to show (the format,
   // above).
   B32 is_canvas;

   // Where in the deck's text the title's words are: as Deck_Item's
   // text_range. Zero for a slide without one, and in a folder of images.
   Rng1U64 title_range;

   // From the slide's @layout line, which is at layout_line in the deck's
   // text (the line without its ending) when has_layout_line.
   Deck_Layout layout;
   B32         has_layout_line;
   Rng1U64     layout_line;

   // From the slide's @place lines, by Deck_PlaceKind. The pictures' are
   // their own (Deck_Picture's place) and so are the body's blocks'
   // (Deck_Box's): the ones here for those are not used.
   Deck_Place places[Deck_PlaceKind_COUNT];

   // From the slide's @size title and @color title lines. The body's
   // blocks have their own (Deck_Box's look).
   Deck_Look title_look;

   // The body's blocks: all of it is the first, until an @box line. A
   // block with no items is one being written. 0 of them in a folder of
   // images.
   Deck_Box boxes[DECK_MAX_BOXES];
   U64      box_count;
   // As Deck_Box's scroll, for the blocks without an @place line, which
   // share the body's room.
   F32 body_scroll;

   // From the slide's @video line: how its video plays (slide_video\slide_video.c).
   B32 video_loop;   // starts over when it reaches its end
   B32 video_mute;   // no sound from it
   B32 video_manual; // does not start when the slide comes up

   // The slide's @note lines, in order: for whoever presents, not on the
   // slide.
   String8List notes;

   // Empty and zero for a slide that is only a picture.
   String8    title;
   Deck_Item* first_item;
   Deck_Item* last_item;
   U64        item_count;
};

// A deck's spares (below): how many at most, and for how long.
#define DECK_SPARE_MAX     8
#define DECK_SPARE_SECONDS 5.0

typedef struct Deck_Deck Deck_Deck;
struct Deck_Deck {
   Arena*      arena;  // the deck's own: everything it points to is on it
   String8     folder; // of the images, or that the text file is in
   Deck_Slide* slides;
   U64         slide_count;

   // For a deck from a text file: the file. Empty for a folder of images.
   String8 file;

   // From the file's first @theme line, which is at theme_line in the text
   // (the line without its ending) when has_theme_line.
   Deck_Theme theme;
   B32        has_theme_line;
   Rng1U64    theme_line;

   // Pictures that were loaded for slides that are gone, kept while a slide
   // of the deck still shows their file (Deck_TakeImages). Typing over a
   // title line takes its slide away for a key or two: when the slide is
   // back it gets its picture back, and a video goes on from where it was.
   // Only path, image, video and spared_us are set. At most DECK_SPARE_MAX.
   Deck_Picture* spares;
   U64           spare_count;
};

// As written after @theme and @layout.
String8 Deck_ThemeName(Deck_Theme theme);
String8 Deck_LayoutName(Deck_Layout layout);
// As written after @align.
String8 Deck_AlignName(Deck_Align align);

// How a title's or a block's lines are set across their room: its @align
// line's, or with none, in the middle on a slide whose layout is center and
// against the left on any other.
Deck_Align Deck_AlignOf(Deck_Slide* slide, Deck_Look* look);

// As written after @place.
String8 Deck_PlaceName(Deck_PlaceKind kind);

// The colours with names, idx under DECK_NAMED_COLOR_COUNT.
Deck_NamedColor Deck_NamedColorFromIdx(U64 idx);
// The colour an @color line's word is: a name, or #rrggbb (or #rgb). 0 if
// it is neither.
B32 Deck_ColorFromString(String8 string, Vec4F32* color_out);

// Words with a look of their own inside a line: "{red 1.5:these}".
typedef struct Deck_Span Deck_Span;
struct Deck_Span {
   B32     has_color;
   Vec4F32 color;
   String8 color_name; // as written, a part of the text asked about
   B32     has_size;
   F32     size; // a part of the line's size
};
// How long the mark that opens a span at text[at] is - "{red 1.5:" - and
// what it says, or 0 if there is none there: not a {, nothing or something
// other than a colour and a size before the :, or no } after it on the line.
U64 Deck_SpanOpenAt(String8 text, U64 at, Deck_Span* span_out);

// Never 0. A folder that is missing, or has no images in it, gives a deck
// with no slides.
Deck_Deck* Deck_OpenFolder(String8 folder);

// Reads a deck from a text file. Never 0: a file that is missing or has
// nothing in it gives a deck with no slides.
Deck_Deck* Deck_OpenFile(String8 file);

// A deck from text that is already in memory - the editor's, which may not be
// what is in the file. The deck keeps its own copy. Line endings must be \n
// alone. file is where the text is from: pictures are found from its folder.
Deck_Deck* Deck_OpenText(String8 text, String8 file);

// The nearest slide to show from slide_idx on, itself too, going forward
// (dir over 0) or back: not a board. slide_count if there is none that way.
U64 Deck_SlideToShow(Deck_Deck* deck, U64 slide_idx, S32 dir);

// Which of the slides that are shown slide_idx is, from 1 - or, for a
// board, the one after it would be - and into count_out how many are shown.
U64 Deck_ShownNumber(Deck_Deck* deck, U64 slide_idx, U64* count_out);

// The slide a position in the deck's text is on: the last that begins at or
// before it. 0 if there are no slides.
U64 Deck_SlideFromTextPos(Deck_Deck* deck, U64 text_pos);

// Moves the pictures src has loaded over to the pictures of dst's slides that show the
// same file, so that a deck read again does not decode them again. src is
// left with none of the ones moved. A file src could not load is marked as
// not loadable in dst too. What no slide of dst takes, of a file a slide of
// dst still shows, goes to dst's spares.
void Deck_TakeImages(Deck_Deck* dst, Deck_Deck* src);

// Opens whichever kind of deck path is: a text file, a folder, or one image
// file. An image opens the folder it is in, and first_slide_out is given its
// slide's index, which is where to start. For the others that is 0.
Deck_Deck* Deck_OpenPath(String8 path, U64* first_slide_out);

// Whether there is anything on the slide but its picture.
B32 Deck_SlideHasText(Deck_Slide* slide);

// The names of the image and video files in a folder and in the folders directly inside
// it, as a slide's ![](...) line would name them from a text file there:
// "hills.png", "pictures/bars.jpg". In order of name, on arena.
String8Array Deck_PictureNamesFromFolder(Arena* arena, String8 folder);

// Whether a file name ends in an extension a text deck can have: .md or
// .txt.
B32 Deck_NameIsTextDeck(String8 name);

// Sorts names in place, as a folder's slides are shown: by name, ignoring
// case, with runs of digits in order of their number (2 before 10).
void Deck_SortNames(String8Array names);

// Releases every image the deck has loaded, and the deck itself.
void Deck_Close(Deck_Deck* deck);

// Loads the pictures of a slide that are not loaded yet, and releases those
// of slides more than one away from it: for the slide that is showing. A
// slide is drawn with whichever of its pictures are loaded (View_DrawSlide).
void Deck_LoadSlide(Deck_Deck* deck, U64 slide_idx);

// The same, but releases nothing: for a second look at a slide close to the
// one Deck_LoadSlide was last asked for (the presenter's next slide).
void Deck_LoadSlideOnly(Deck_Deck* deck, U64 slide_idx);

// The first of a slide's videos that is open - as long as its image is
// loaded - else 0: the one whose time and state stand for the slide's.
Video_Video* Deck_VideoFromSlide(Deck_Deck* deck, U64 slide_idx);
// Every open video of a slide, into videos, which has room for
// DECK_MAX_PICTURES. Returns how many.
U64 Deck_VideosFromSlide(Deck_Deck* deck, U64 slide_idx, Video_Video** videos);
// Whether any of the slide's pictures is a video file, open or not. Not one
// that could not be opened.
B32 Deck_SlideHasVideo(Deck_Deck* deck, U64 slide_idx);
// Whether all of them that are video files are open: none still to load.
B32 Deck_SlideVideosAreOpen(Deck_Deck* deck, U64 slide_idx);
// Once a frame: every open video's Video_Update, and spares kept for longer
// than DECK_SPARE_SECONDS released.
void Deck_UpdateVideos(Deck_Deck* deck);

// Lets go of a picture and loads it again: for a file that has changed, or
// whose path now leads somewhere else (below).
void Deck_ReopenPicture(Deck_Picture* picture);

// From now on, a slide whose picture is at path is shown the file at
// stands_for instead - until Deck_ClearPathAlias(path). For a file that is
// on its way to path and not there yet (copy\copy.c). For every deck: a
// deck read again from its text keeps showing the same file. Slides that
// have the picture loaded already are not changed: Deck_ReopenPicture does
// that.
void Deck_SetPathAlias(String8 path, String8 stands_for);
void Deck_ClearPathAlias(String8 path);

////////////////////////////////
//~ bgz: the edit screen
//
// What a text deck opens on: its text on the left, in an editor, and on the
// right the slide the cursor is on, drawn as it will be presented and made
// again from the text at every change.
//
//    +---------------------------+------------------------------+
//    |                           |                              |
//    |  the text (editor.h)      |     +------------------+     |
//    |                           |     |  the slide the   |     |
//    |                           |     |  cursor is on    |     |
//    +---------------------------+     +------------------+     |
//    |  Format: what can be      |        Slide 3 of 9          |
//    |  written, a line each     |                              |
//    +---------------------------+   [1] [2] [3] [4] [5] ...    |
//    |  status                   |   every slide, small         |
//    +---------------------------+------------------------------+
//
// The Format panel is how the format is learned without leaving the program:
// every kind of line there is, as it is written, with what it does. Clicking
// one writes it into the text at the cursor, ready to be typed over. Its
// last two lines change something instead: the deck's theme and the layout
// of the slide the cursor is on, each to the next there is, by writing the
// @theme or @layout line.
//
// Clicking the picture line there opens, in the panel's place, the list of
// the pictures in the deck's folder, which scrolls: clicking one puts it on
// the slide. Dropping an image file on the window does the same, copying the
// file into the deck's folder first if it is not there.
//
// The Format panel, the list of pictures, the status line and the whole
// preview - the slide, everything over it, the caption, the small slides and
// the + - are built with the UI layer (Edit_Build, called from FrameWork's UI
// build), and take their own clicks and wheel there: see "the preview, in
// the UI layer" below. The slides are drawn by the boxes they are in (the
// view, as a box's custom draw), and so is the text (the editor, editor.h,
// which is also given the text box's press, drag and wheel). The keys are
// Edit_Input's, before the build: the screen's own, then the editor's.
//
// The slide in the preview is typed into: a click on its title or on a
// block of its words puts the cursor at the letter clicked, with a caret
// there on the slide, and a thin blue box around the room the words have
// (caret\caret.c). A click on a picture puts the cursor on its line, with
// the file's name selected, to be typed over, and a box around the picture.
// What the mouse is over is lit.
//
// The row of small slides under the preview is the whole deck: clicking one
// moves the cursor to that slide's first line, and the + at its end starts a
// new slide after the one showing. Alt+Left and Alt+Right move the slide
// showing a place earlier or later in the deck.
//
//    present          F5 (and F5 or Esc comes back)
//    save             Ctrl+S
//    format panel     F1 opens and closes it
//    more room        drag the edge between the text and the preview, or
//                     the top edge of the Format panel. A double click
//                     puts it back
//    the left side    Alt+[ puts the text, the Format panel and the status
//                     line away, and the preview has the window. Again, or
//                     F1, brings them back
//    fullscreen       F11 or Alt+Return. Esc leaves it
//    the block        a line around the lines of the text that are the
//                     block of words the cursor is in: an @box begins the
//                     next
//    @ lines          type @ in the text: a list of what can be written,
//                     Tab or Return to take one (complete\complete.c)
//    the text's keys  as vim has them, in three modes (vim\vim.c). Esc is
//                     back to command mode
//    deselect         Esc: nothing selected, and no box in the preview
//    pictures         click the picture line under Format. Esc closes the list,
//                     the wheel or the scroll bar moves through it
//    edit a piece     click it in the preview, and type: the caret is on
//                     the slide, at the letter clicked (caret\caret.c)
//    move a piece     drag the picture or the body in the preview, or a
//                     corner of it to resize it (place\place.c)
//    the menu         right-click a piece, or the slide where nothing is:
//                     what can be done there, each with its key beside it.
//                     On words selected: bold and code, a size and a
//                     colour for them, a side for their lines, and their
//                     lines made bullets, a heading or plain. Where
//                     nothing is: the slide's own - a new one, a copy,
//                     deleted, made a board, and its layout
//    which side       Ctrl+Shift+L, E, R: the words against the left, in
//                     the middle, against the right (look\look.c)
//    add a text box   double-click the slide where nothing is: a box there.
//                     Or Ctrl and a drag, for one as large as the mouse
//                     makes it. Its words are selected, to be typed over. Left as it is, it goes again when something
//                     else is clicked
//    take a piece out click a picture, or the title or a block away from its
//                     words - where it is taken hold of - and press Delete
//                     or Backspace. Ctrl+Z brings it back
//    pick several     Ctrl+click adds a piece to those picked, or takes it
//                     out. Shift+click, with one picked, picks every piece
//                     of its kind - all the pictures, or all the blocks.
//                     Delete takes them all out, and a drag of one moves
//                     them all
//    copy pieces      with pieces picked, Ctrl+C copies them and Ctrl+X cuts
//                     them; Ctrl+V pastes them on the slide showing, picked.
//                     Ctrl+D puts a copy a little down and right
//    pick by a box    a drag on the slide where nothing is: every piece the
//                     box touches is picked, in place of those that were -
//                     or with them, with Shift held
//    front and back   Ctrl+Shift+] brings the pieces picked in front of the
//                     slide's other pictures, or other blocks, and
//                     Ctrl+Shift+[ sends them behind
//    line up, space   Ctrl+Shift and an arrow, C, M, H or V with several
//                     picked (place\place.c)
//    nudge pieces     the arrow keys with pieces picked move them a small
//                     step, Shift+arrow a larger one
//    size by wheel    Ctrl and the wheel over the title or a block: its
//                     words larger or smaller - those of all picked, over
//                     one of them (look\look.c)
//    a board          an @canvas slide: no edge, pieces anywhere, a block as
//                     tall as its words, not presented. The wheel moves the
//                     preview over it, sideways with Shift; Ctrl and the
//                     wheel zoom, about the mouse; the middle button held
//                     drags it. The right-click menu on open space puts it
//                     back over its page ("the board", below)
//    make a slide     Ctrl+Shift+N with pieces picked: they are taken off
//                     the slide - a board's ideas, most often - and written
//                     as a slide of their own at the deck's end
//    add a picture    drop an image or a video on the window: one more on the
//                     slide. With a picture selected - click it - that one
//                     is changed for it
//    add a slide      click the + at the end of the row of small slides
//    move a slide     Alt+Left earlier, Alt+Right later
//    bold, code       Ctrl+B and Ctrl+E: ** or ` around what is selected.
//                     Ctrl+B in insert mode only: else it is vim's
//    size, colour     Ctrl+] and Ctrl+[ make the words the cursor is in
//                     larger and smaller, and the Format panel's @size and
//                     @color lines have the rest to click (look\look.c)
//    open             Ctrl+O: the open screen (open\open.c). Or drop a folder
//                     or a text deck on the window
//
// The editor's own keys are in editor.c.

////////////////////////////////
//~ bgz: the format panel's lines

typedef struct _Edit_FormatRow _Edit_FormatRow;
struct _Edit_FormatRow {
   String8 syntax;       // as shown
   String8 what;         // what it does
   String8 insert;       // what a click writes
   U64     select_start; // the part of insert left selected, to type over
   U64     select_end;
   U32     action;       // an _Edit_FormatAction: what a click does, if not write insert
};

enum {
   _Edit_FormatAction_Insert,
   _Edit_FormatAction_Pictures, // opens the list of pictures
   _Edit_FormatAction_Theme,    // gives the deck the next theme
   _Edit_FormatAction_Layout,   // gives the slide the next layout
   _Edit_FormatAction_Wrap,     // puts insert before and after the selection (Editor_WrapSelection)
   _Edit_FormatAction_Size,     // nothing by itself: the row has smaller, larger and usual to click (look\look.c)
   _Edit_FormatAction_Color,    // nothing by itself: the row has the colours to click
   _Edit_FormatAction_Align,    // nothing by itself: the row has the sides to click
};

PRIVATE _Edit_FormatRow _edit_format_rows[] = {
    {Str8LitComp("# Title"), Str8LitComp("starts a new slide, with a title"), Str8LitComp("# Title"), 2, 7},
    {Str8LitComp("---"), Str8LitComp("starts a new slide with no title. @slide is the same"), Str8LitComp("---\n"), 4, 4},
    {Str8LitComp("@canvas"), Str8LitComp("starts a board: no edge, anything anywhere, not presented"), Str8LitComp("@canvas\n"), 8, 8},
    {Str8LitComp("## Heading"), Str8LitComp("a heading inside the slide"), Str8LitComp("## Heading"), 3, 10},
    {Str8LitComp("- point"), Str8LitComp("a bullet. Tab moves it in"), Str8LitComp("- point"), 2, 7},
    {Str8LitComp("  - point"), Str8LitComp("an inner bullet. Shift+Tab moves it out"), Str8LitComp("  - point"), 4, 9},
    {Str8LitComp("![](picture.png)"), Str8LitComp("a picture or video on the slide. Click to pick one"), Str8LitComp("![](picture.png)"), 4, 15, _Edit_FormatAction_Pictures},
    {Str8LitComp("Any other line"), Str8LitComp("a paragraph. An empty line adds room"), Str8LitComp("Text"), 0, 4},
    {Str8LitComp("**bold**"), Str8LitComp("bold words. Select some first, or Ctrl+B"), Str8LitComp("**"), 0, 0, _Edit_FormatAction_Wrap},
    {Str8LitComp("`code`"), Str8LitComp("words as code. Select some first, or Ctrl+E"), Str8LitComp("`"), 0, 0, _Edit_FormatAction_Wrap},
    {Str8LitComp("@note Say this"), Str8LitComp("a note to yourself. N shows notes while presenting"), Str8LitComp("@note Say this"), 6, 14},
    {Str8LitComp("@box"), Str8LitComp("another block of words on the slide, to drag on its own"), Str8LitComp("@box\nText"), 5, 9},
    {Str8LitComp("@video loop"), Str8LitComp("how the slide's video plays: loop, mute, manual"), Str8LitComp("@video loop"), 7, 11},
    // What these five do is said in Edit_Build, with what the deck, the slide and the words have now.
    {Str8LitComp("@size"), Str8LitComp(""), Str8LitComp(""), 0, 0, _Edit_FormatAction_Size},
    {Str8LitComp("@color"), Str8LitComp(""), Str8LitComp(""), 0, 0, _Edit_FormatAction_Color},
    {Str8LitComp("@align"), Str8LitComp(""), Str8LitComp(""), 0, 0, _Edit_FormatAction_Align},
    {Str8LitComp("@theme"), Str8LitComp(""), Str8LitComp(""), 0, 0, _Edit_FormatAction_Theme},
    {Str8LitComp("@layout"), Str8LitComp(""), Str8LitComp(""), 0, 0, _Edit_FormatAction_Layout},
};

////////////////////////////////
//~ bgz: layout

// The least width, in units, a divider leaves the side it is dragged
// toward, and how wide the strip is that takes hold of one.
#define _EDIT_SPLIT_LEAST 240.0f
#define _EDIT_SPLIT_GRIP  7.0f

// Where everything on the screen is, for a window of a size. Input and
// drawing both ask, and get the same answer.
typedef struct _Edit_Layout _Edit_Layout;
struct _Edit_Layout {
   F32 ui; // how many pixels a unit of the sizes below is

   Editor_Box  editor_box;
   Rng2F32 format_header; // always there: clicking it opens and closes the panel
   Rng2F32 format_rows;   // zero when the panel is closed
   F32     format_row_h;
   Rng2F32 status;
   Rng2F32 preview;       // the right side, all of it
   Rng2F32 page;          // the slide, in the preview
   B32     is_board;      // the slide is a board: its page is wherever it was moved to, any size
   Rng2F32 board;         // what the slide is drawn in: its page - or, for a board, the preview down to the caption
   Rng2F32 board_base;    // a board's page with nothing moved: where a slide's is
   Rng2F32 caption;       // under the slide
   Rng2F32 thumbs;        // the row of small slides, along the bottom
   Rng2F32 add_slide;     // the + at the row's right end
   Vec2F32 thumb_dim;     // the size of one
   F32     thumb_gap;     // between two

   F32 ui_px;   // the size of text that is not the editor's
   F32 mono_px; // the editor's
};

// A board's page, where the window was moved to over it: base is the page
// with nothing moved, where a slide's is.
PRIVATE Rng2F32
_Edit_BoardPage(Rng2F32 base) {
   B32     is_moved = (g_edit_view.board_zoom > 0);
   F32     zoom = is_moved ? g_edit_view.board_zoom : 1.0f;
   Vec2F32 at = is_moved ? g_edit_view.board_at : V2F32(0.5f, 0.5f);
   Vec2F32 dim = Dim2F32(base);
   Vec2F32 middle = V2F32(base.min.x + dim.x * 0.5f, base.min.y + dim.y * 0.5f);
   F32     x = (F32)(S32)(middle.x - at.x * dim.x * zoom);
   F32     y = (F32)(S32)(middle.y - at.y * dim.y * zoom);
   return R2F32P(x, y, x + dim.x * zoom, y + dim.y * zoom);
}

PRIVATE _Edit_Layout
_Edit_LayoutFromWindowDim(const App_State* app_state, Vec2F32 window_dim) {
   _Edit_Layout result = {0};

   F32 ui = FR_UIScale(window_dim);
   result.ui = ui;
   result.ui_px = (F32)(S32)(15.0f * ui);
   result.mono_px = (F32)(S32)(17.0f * ui);

   // The text has a little under half the window, within reason at the ends
   // - or what the divider was dragged to, with room left on both sides.
   F32 left_w = window_dim.x * 0.44f;
   left_w = Clamp(Min(380.0f * ui, window_dim.x), left_w, 900.0f * ui);
   if (g_edit_view.split_x > 0) {
      F32 least = Min(_EDIT_SPLIT_LEAST * ui, window_dim.x * 0.5f);
      left_w = Clamp(least, window_dim.x * g_edit_view.split_x, window_dim.x - least);
   }
   left_w = (F32)(S32)left_w;
   // Alt+[ puts the whole left side away, and the preview has the window.
   if (app_state->hide_text) {
      left_w = 0;
   }

   F32 status_h = (F32)(S32)(30.0f * ui);
   F32 header_h = (F32)(S32)(30.0f * ui);
   F32 row_h = (F32)(S32)(26.0f * ui);
   // All the format's lines if they fit, but the text keeps at least half
   // the window: in a short one the lines give way, two at the least, and
   // scroll (Edit_Build).
   F32 rows_h = 0;
   if (app_state->show_format) {
      F32 all_rows_h = row_h * (F32)ArrayCount(_edit_format_rows) + 8.0f * ui;
      F32 room_h = window_dim.y * 0.5f - header_h - status_h;
      rows_h = Min(all_rows_h, Max(room_h, row_h * 2.0f + 8.0f * ui));
      // Or what the divider over the panel was dragged to: a line at the
      // least, all of them at the most, and a few rows left for the text.
      if (g_edit_view.split_format > 0) {
         F32 most = Max(window_dim.y - header_h - status_h - result.mono_px * 4.0f, row_h + 8.0f * ui);
         rows_h = Clamp(row_h + 8.0f * ui, window_dim.y * g_edit_view.split_format, Min(all_rows_h, most));
         rows_h = (F32)(S32)rows_h;
      }
   }

   F32 status_y = window_dim.y - status_h;
   F32 rows_y = status_y - rows_h;
   F32 header_y = rows_y - header_h;

   result.editor_box.rect = R2F32P(0, 0, left_w, Max(header_y, 0));
   result.editor_box.font = app_state->fonts.mono;
   result.editor_box.px = result.mono_px;
   result.format_header = R2F32P(0, header_y, left_w, rows_y);
   if (app_state->show_format) {
      result.format_rows = R2F32P(0, rows_y, left_w, status_y);
   }
   result.format_row_h = row_h;
   result.status = R2F32P(0, status_y, left_w, window_dim.y);

   result.preview = R2F32P(left_w, 0, window_dim.x, window_dim.y);
   F32 pad = 28.0f * ui;
   F32 caption_h = 40.0f * ui;

   result.thumb_dim.y = (F32)(S32)(76.0f * ui);
   result.thumb_dim.x = (F32)(S32)(result.thumb_dim.y * VIEW_PAGE_W / VIEW_PAGE_H);
   result.thumb_gap = (F32)(S32)(10.0f * ui);
   F32 thumbs_y = window_dim.y - pad * 0.6f - result.thumb_dim.y;
   if (window_dim.x - pad > left_w + pad && thumbs_y > 0) {
      // The + has the right end, and the small slides what is left of it.
      F32 add_w = (F32)(S32)(result.thumb_dim.y * 0.6f);
      F32 add_x = Max(window_dim.x - pad - add_w, left_w + pad);
      result.add_slide = R2F32P(add_x, thumbs_y, window_dim.x - pad, thumbs_y + result.thumb_dim.y);
      result.thumbs = R2F32P(left_w + pad, thumbs_y, Max(add_x - result.thumb_gap, left_w + pad),
                             thumbs_y + result.thumb_dim.y);
   }

   Rng2F32 page_room = R2F32P(left_w + pad, pad, window_dim.x - pad, thumbs_y - caption_h);
   if (page_room.max.x > page_room.min.x && page_room.max.y > page_room.min.y) {
      result.page = View_PageRectFromRect(page_room);
      result.board = result.page;
      result.board_base = result.page;
      result.caption = R2F32P(result.page.min.x, result.page.max.y, result.page.max.x, result.page.max.y + caption_h);

      // A board has all of the preview over the caption, and its page is
      // where the window was moved to over it (_Edit_BoardInput).
      Deck_Deck* deck = app_state->deck;
      if (deck != 0 && app_state->slide_idx < deck->slide_count && deck->slides[app_state->slide_idx].is_canvas) {
         result.is_board = 1;
         result.page = _Edit_BoardPage(result.board_base);
         result.board = R2F32P(left_w + 1.0f, 0, window_dim.x, thumbs_y - caption_h);
         result.caption = R2F32P(result.board.min.x, result.board.max.y, result.board.max.x, result.board.max.y + caption_h);
      }
   }

   return result;
}

// Where a slide's small picture is in the row. With more slides than fit, the
// row shows the ones around the slide showing - as far as the ends of the
// deck allow - and always whole ones. A slide that is not in the row gets a
// zero rect.
PRIVATE Rng2F32
_Edit_ThumbRect(_Edit_Layout* layout, U64 slide_count, U64 slide_showing, U64 slide_idx) {
   Rng2F32 result = {0};

   F32 step = layout->thumb_dim.x + layout->thumb_gap;
   F32 row_w = Dim2F32(layout->thumbs).x;
   if (row_w >= layout->thumb_dim.x && step > 0) {
      U64 fit_count = (U64)((row_w + layout->thumb_gap) / step);
      U64 first = 0;
      if (slide_count > fit_count) {
         first = (slide_showing > fit_count / 2) ? slide_showing - fit_count / 2 : 0;
         first = Min(first, slide_count - fit_count);
      }

      if (slide_idx >= first && slide_idx < first + fit_count) {
         F32 x = (F32)(S32)(layout->thumbs.min.x + step * (F32)(slide_idx - first));
         result = R2F32P(x, layout->thumbs.min.y, x + layout->thumb_dim.x, layout->thumbs.max.y);
      }
   }

   return result;
}

////////////////////////////////
//~ bgz: slides, as pieces of the text
//
// A slide's piece of the text is from its text_pos to the next slide's: its
// # Title or --- line and everything under it. What is above the first
// slide - @theme and other lines that are about the deck, empty lines - is
// no slide's, and stays where it is when slides move. The deck has to be of
// the text as it is: FR_SyncDeck first.

PRIVATE Rng1U64
_Edit_TextRangeFromSlide(Editor_Editor* editor, Deck_Deck* deck, U64 slide_idx) {
   Rng1U64 result = R1U64(0, editor->size);
   if (slide_idx < deck->slide_count) {
      result.min = Min(deck->slides[slide_idx].text_pos, editor->size);
      if (slide_idx + 1 < deck->slide_count) {
         result.max = Clamp(result.min, deck->slides[slide_idx + 1].text_pos, editor->size);
      }
   }
   return result;
}

// Starts a slide after slide_idx - or the deck's first, if it has none -
// with its title selected, to be typed over.
PRIVATE void
_Edit_AddSlideAfter(Editor_Editor* editor, Deck_Deck* deck, U64 slide_idx) {
   U64     at = _Edit_TextRangeFromSlide(editor, deck, slide_idx).max;
   String8 text = Editor_Text(editor);
   Temp    scratch = ScratchBegin(0, 0);

   // On a line of its own: the slide before may end without a line ending,
   // and the slide after has to start on its own line too. And with an
   // empty line between it and each of its neighbours, as slides are
   // written: the one before, at the end of the text, often ends with no
   // empty line after it, and the one after starts right at the end of
   // what is added - "# Title" and "# Next" on lines one under the other
   // read as one block.
   B32         needs_line_before = (at > 0 && text.str[at - 1] != '\n');
   B32         needs_empty_before = (at > 0 && !(at > 1 && text.str[at - 1] == '\n' && text.str[at - 2] == '\n'));
   B32         needs_empty_after = (at < text.size);
   String8     before = needs_line_before ? Str8Lit("\n\n") : needs_empty_before ? Str8Lit("\n") : Str8Lit("");
   String8     added = PushStr8F(scratch.arena, "%S# Title\n%s", before, needs_empty_after ? "\n" : "");
   Editor_ReplaceRange(editor, at, at, added);

   U64 title_start = at + before.size + 2;
   Editor_SetCursor(editor, title_start + 5);
   editor->mark = title_start;

   ScratchEnd(scratch);
}

// Moves slide_idx a place later in the deck, or earlier: its piece of the
// text and its neighbour's change places, as one step to undo. The cursor
// stays where it was in the slide. 0 if there is no place to move it to.
PRIVATE B32
_Edit_MoveSlide(Editor_Editor* editor, Deck_Deck* deck, U64 slide_idx, B32 to_later) {
   B32 result = 0;

   // Either way two slides next to each other change places: first, second.
   U64 first_idx = to_later ? slide_idx : slide_idx - 1;
   if (slide_idx < deck->slide_count && (to_later ? slide_idx + 1 < deck->slide_count : slide_idx > 0)) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 text = Editor_Text(editor);
      Rng1U64 first_range = _Edit_TextRangeFromSlide(editor, deck, first_idx);
      Rng1U64 second_range = _Edit_TextRangeFromSlide(editor, deck, first_idx + 1);
      String8 first = Str8Substr(text, first_range);
      String8 second = Str8Substr(text, second_range);

      // The deck's first slide can start without a # Title or a --- : there
      // is nothing before it to be told from. Anywhere else it would be read
      // as more of the slide before, so it is given a --- to start with.
      B32 first_starts_itself = 0;
      for (U64 i = 0; i < first.size; i += 1) {
         if (first.str[i] != ' ' && first.str[i] != '\t' && first.str[i] != '\n') {
            first_starts_itself = (first.str[i] == '#' || first.str[i] == '-');
            if (first.str[i] == '@') {
               first_starts_itself = (Str8Match(Str8Prefix(Str8Skip(first, i), 6), Str8Lit("@slide"), StringMatchFlag_CaseInsensitive) ||
                                      Str8Match(Str8Prefix(Str8Skip(first, i), 7), Str8Lit("@canvas"), StringMatchFlag_CaseInsensitive) ||
                                      // Lines about the slide above its # Title: the deck reads them as its start.
                                      deck->slides[first_idx].title_range.max != 0);
            }
            if (first.str[i] == '-') {
               // A --- line, and not a bullet.
               U64 dash_count = 0;
               while (i + dash_count < first.size && first.str[i + dash_count] == '-') dash_count += 1;
               U64 after = i + dash_count;
               while (after < first.size && (first.str[after] == ' ' || first.str[after] == '\t')) after += 1;
               first_starts_itself = (dash_count >= 3 && (after == first.size || first.str[after] == '\n'));
            } else if (first.str[i] == '#') {
               // A # Title, and not a ## Heading.
               first_starts_itself = !(i + 1 < first.size && first.str[i + 1] == '#');
            }
            break;
         }
      }
      if (first.size == 0) {
         first_starts_itself = 1;
      }

      // What the two say change places, and the empty lines after each stay
      // where they were: the room between the slides is the text's, not
      // either slide's. Moved whole, the last slide - which usually ends
      // with no empty line, at the end of the text - would come up hard
      // under the one it moves above, and take that one's empty lines down
      // to the end of the text with it.
      U64 first_words = first.size;
      while (first_words > 0 && (first.str[first_words - 1] == '\n' || first.str[first_words - 1] == '\r' ||
                                 first.str[first_words - 1] == ' ' || first.str[first_words - 1] == '\t')) {
         first_words -= 1;
      }
      U64 second_words = second.size;
      while (second_words > 0 && (second.str[second_words - 1] == '\n' || second.str[second_words - 1] == '\r' ||
                                  second.str[second_words - 1] == ' ' || second.str[second_words - 1] == '\t')) {
         second_words -= 1;
      }
      String8 first_gap = Str8Skip(first, first_words);
      String8 second_gap = Str8Skip(second, second_words);
      // A slide that had nothing after it - the last, with no line ending -
      // is followed by one now.
      if (first_gap.size == 0 || first_gap.str[0] != '\n') {
         first_gap = PushStr8F(scratch.arena, "\n%S", first_gap);
      }
      String8 middle = PushStr8F(scratch.arena, "%S%s", first_gap, first_starts_itself ? "" : "---\n");
      String8 swapped = PushStr8F(scratch.arena, "%S%S%S%S", Str8Prefix(second, second_words), middle, Str8Prefix(first, first_words),
                                  second_gap);

      // Where the cursor was in the slide that was asked to be moved: in
      // what it says, at most at the end of that.
      Rng1U64 moved_range = to_later ? first_range : second_range;
      U64     moved_words = to_later ? first_words : second_words;
      U64     in_slide = Min(Clamp(moved_range.min, editor->cursor, moved_range.max) - moved_range.min, moved_words);
      U64     moved_start = to_later ? first_range.min + second_words + middle.size : first_range.min;

      Editor_ReplaceRange(editor, first_range.min, second_range.max, swapped);
      Editor_SetCursor(editor, moved_start + in_slide);

      ScratchEnd(scratch);
      result = 1;
   }

   return result;
}

////////////////////////////////
//~ bgz: themes and layouts
//
// The deck has to be of the text as it is: FR_SyncDeck first.

// Puts string where [start, end) of the text was, and leaves the cursor in
// the words it was in - not at the change, which may be on another slide.
PRIVATE void
_Edit_ReplaceKeepingCursor(Editor_Editor* editor, U64 start, U64 end, String8 string) {
   U64 cursor = editor->cursor;
   if (cursor >= end) {
      cursor = cursor - (end - start) + string.size;
   } else if (cursor > start) {
      cursor = start + string.size;
   }
   Editor_ReplaceRange(editor, start, end, string);
   Editor_SetCursor(editor, cursor);
}

// Writes a line that is about one piece of a slide - an @place, @size or
// @color line, for the idx-th picture or block of the body, or the title:
// over the one there is (old_line, with has_line), or as a new line above
// what it is about: the picture's line, the title's, or the block's first.
// One there is that is under its picture's or its title's line, as they
// were written before, is taken out and written above.
void
Edit_PutLineFor(Editor_Editor* editor, Deck_Slide* slide, Deck_PlaceKind kind, U64 idx, B32 has_line, Rng1U64 old_line,
                String8 line) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 text = Editor_Text(editor);

   // The line of the text the thing is on: the picture's, the title's,
   // or the first of the block's - which, for any block but the first,
   // is under its @box line, so the new line goes between the two.
   U64 on = slide->text_pos;
   B32 goes_above = 0;
   if (kind == Deck_PlaceKind_Picture && idx < slide->picture_count) {
      on = slide->pictures[idx].range.min;
      goes_above = 1;
   } else if (kind == Deck_PlaceKind_Title && slide->title_range.max != 0) {
      on = slide->title_range.min;
      goes_above = 1;
   } else if (kind == Deck_PlaceKind_Text && idx < slide->box_count && slide->boxes[idx].first_item != 0) {
      on = slide->boxes[idx].first_item->text_range.min;
   }
   on = Min(on, text.size);

   B32 is_under = (has_line && old_line.max <= text.size && goes_above && old_line.min > on);
   if (is_under) {
      // Out from under, which is after where it goes: that stays where it is.
      Edit_TakeOutLine(editor, old_line);
      text = Editor_Text(editor);
      has_line = 0;
   }

   if (has_line && old_line.max <= text.size) {
      _Edit_ReplaceKeepingCursor(editor, old_line.min, old_line.max, line);
   } else {
      U64 line_start = on;
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 line_end = on;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;

      // Above the line it is about: a picture's above the picture's line,
      // the title's above the title's - the deck reads both as that
      // picture's and that slide's (deck.h) - and a block's above its
      // first line: under that it would be the next line's own, and not
      // the block's.
      if (goes_above || kind == Deck_PlaceKind_Text) {
         _Edit_ReplaceKeepingCursor(editor, line_start, line_start, PushStr8F(scratch.arena, "%S\n", line));
      } else {
         _Edit_ReplaceKeepingCursor(editor, line_end, line_end, PushStr8F(scratch.arena, "\n%S", line));
      }
   }

   ScratchEnd(scratch);
}

// Takes such a line out of the text, with its line ending.
void
Edit_TakeOutLine(Editor_Editor* editor, Rng1U64 line) {
   String8 text = Editor_Text(editor);
   if (line.max <= text.size) {
      U64 start = line.min;
      U64 end = line.max;
      // All of the line: what is in front of the @ too.
      while (start > 0 && text.str[start - 1] != '\n') start -= 1;
      while (end < text.size && text.str[end] != '\n') end += 1;
      if (end < text.size) {
         end += 1;
      } else if (start > 0) {
         start -= 1;
      }
      _Edit_ReplaceKeepingCursor(editor, start, end, Str8Lit(""));
   }
}

////////////////////////////////
//~ bgz: text boxes made and pieces taken out, from the preview
//
// A press on the slide where nothing is - no words, no picture, no box -
// and a drag draws a text box there, as large as the mouse makes it, as
// presentation programs do: a block of its own,
//
//    @box
//    @place text 0.300 0.400 0.400 0.150
//    Text
//
// written at the end of the slide's lines, with "Text" selected to be typed
// over. A click there with no drag makes nothing: it lets go of what is
// selected, as Esc does. Left as it is - clicked away from, Esc, another slide - it is taken
// out again (_Edit_TakeOutNewBox). Anything done to it keeps it: a key, a
// drag.
//
// A piece picked whole (g_edit_view.picked_count) - a picture clicked, or the
// title or a block clicked in its box away from the words - is taken out by
// Delete or Backspace, all of its lines in one step to undo: a picture's
// line and its @place line; a block's lines, its @box line and its @place,
// @size and @color lines; the title's words, with its own lines, the slide
// kept by a --- in the title's place.

// The least a text box drawn is made, in parts of the page: a drag that
// barely moved still gives a box a word fits in.
#define _EDIT_NEW_BOX_MIN_W 0.08f
#define _EDIT_NEW_BOX_MIN_H 0.06f

// How large the text box a double click makes is, in parts of the page:
// from the click across and down, its first line where the click was.
#define _EDIT_CLICK_BOX_W 0.30f
#define _EDIT_CLICK_BOX_H 0.20f

// How far the mouse goes with the button down on open space before it is
// drawing a box and not a click. In UI units.
#define _EDIT_DRAW_START 4.0f

// What a click makes the new box's words: selected, to be typed over.
#define _EDIT_NEW_BOX_WORDS "Text"

// Takes out the text box a click made, if nothing has been done to it
// since - unless the cursor is in it: clicked again, it is being typed in.
// 1 if it was taken out.
PRIVATE B32
_Edit_TakeOutNewBox(App_State* app_state) {
   B32            result = 0;
   Editor_Editor* editor = app_state->editor;
   if (g_edit_view.has_new_box && editor != 0) {
      Rng1U64 range = g_edit_view.new_box_range;
      B32     is_untouched = (editor->edit_count == g_edit_view.new_box_edit_count && range.max <= editor->size);
      B32     is_in_it = (editor->cursor > range.min && editor->cursor <= range.max);
      if (!is_untouched) {
         g_edit_view.has_new_box = 0;
      } else if (!is_in_it) {
         _Edit_ReplaceKeepingCursor(editor, range.min, range.max, Str8Lit(""));
         editor->mark = editor->cursor;
         g_edit_view.has_new_box = 0;
         result = 1;
      }
   }
   return result;
}

// A text box on slide_idx where rect is, in parts of the page: see above.
// The deck has to be of the text as it is.
PRIVATE void
_Edit_AddTextBox(App_State* app_state, StateOp_List* ops, U64 slide_idx, Rng2F32 rect) {
   Editor_Editor* editor = app_state->editor;
   Deck_Deck*     deck = app_state->deck;
   if (slide_idx >= deck->slide_count) {
      return;
   }
   Deck_Slide* slide = &deck->slides[slide_idx];
   if (slide->box_count >= DECK_MAX_BOXES && slide->boxes[slide->box_count - 1].item_count != 0) {
      StateOp_App_Notice(ops)->notice = PushStr8F(ops->arena, "A slide can have %d text boxes at most", DECK_MAX_BOXES);
      return;
   }

   // Under the slide's last line that has anything on it: the empty lines
   // between it and the next slide stay where they are.
   String8 text = Editor_Text(editor);
   Rng1U64 range = _Edit_TextRangeFromSlide(editor, deck, slide_idx);
   U64     end = range.max;
   while (end > range.min && (text.str[end - 1] == '\n' || text.str[end - 1] == ' ' || text.str[end - 1] == '\t')) {
      end -= 1;
   }

   // All of it on the page, and no smaller than a word needs.
   F32 w = Clamp(_EDIT_NEW_BOX_MIN_W, rect.max.x - rect.min.x, 1.0f);
   F32 h = Clamp(_EDIT_NEW_BOX_MIN_H, rect.max.y - rect.min.y, 1.0f);
   F32 x = Clamp(0.0f, rect.min.x, 1.0f - w);
   F32 y = Clamp(0.0f, rect.min.y, 1.0f - h);
   if (slide->is_canvas) {
      // Anywhere, on a board.
      x = rect.min.x;
      y = rect.min.y;
   }

   Temp    scratch = ScratchBegin(0, 0);
   String8 added = PushStr8F(scratch.arena, "\n\n@box\n@place text %.3f %.3f %.3f %.3f\n%s", x, y, w, h,
                             _EDIT_NEW_BOX_WORDS);
   Editor_ReplaceRange(editor, end, end, added);
   U64 words_end = end + added.size;
   editor->mark = words_end - (sizeof(_EDIT_NEW_BOX_WORDS) - 1);
   editor->cursor = words_end;
   editor->last_input_us = NowTimeUS();
   g_caret_view.focus_hidden = 0;
   // To be typed over.
   Vim_StartTyping(app_state);
   ScratchEnd(scratch);

   g_edit_view.has_new_box = 1;
   g_edit_view.new_box_edit_count = editor->edit_count;
   g_edit_view.new_box_range = R1U64(end, words_end);
}

// The line of the text pos is on, from its start to past its ending - or,
// for the text's last line with none, from the ending before it.
PRIVATE Rng1U64
_Edit_WholeLineAt(String8 text, U64 pos) {
   U64 start = Min(pos, text.size);
   U64 end = start;
   while (start > 0 && text.str[start - 1] != '\n') start -= 1;
   while (end < text.size && text.str[end] != '\n') end += 1;
   if (end < text.size) {
      end += 1;
   } else if (start > 0) {
      start -= 1;
   }
   return R1U64(start, end);
}

// Lines of the text to take out of it, or to write over: what a piece taken
// out is written in.
#define _EDIT_MAX_PIECE_LINES 256
typedef struct _Edit_PieceLines _Edit_PieceLines;
struct _Edit_PieceLines {
   Rng1U64 lines[_EDIT_MAX_PIECE_LINES]; // whole lines, as _Edit_WholeLineAt
   String8 put[_EDIT_MAX_PIECE_LINES];   // what goes in the line's place: empty to take it out
   U64     count;
};

PRIVATE void
_Edit_PieceLinesAdd(_Edit_PieceLines* lines, String8 text, U64 pos, String8 put) {
   if (lines->count < _EDIT_MAX_PIECE_LINES && pos <= text.size) {
      Rng1U64 line = _Edit_WholeLineAt(text, pos);
      for (U64 i = 0; i < lines->count; i += 1) {
         if (lines->lines[i].min == line.min) {
            return;
         }
      }
      lines->lines[lines->count] = line;
      lines->put[lines->count] = put;
      lines->count += 1;
   }
}

// What the piece of kind and idx on slide_idx is a name for, in the notice
// that says it was taken out.
PRIVATE String8
_Edit_PieceName(Deck_PlaceKind kind) {
   String8 result = Str8Lit("The text box");
   if (kind == Deck_PlaceKind_Picture) {
      result = Str8Lit("The picture");
   } else if (kind == Deck_PlaceKind_Title) {
      result = Str8Lit("The title");
   }
   return result;
}

// Takes count pieces out of slide_idx, by kind and idx: see above. All in
// one change. The deck has to be of the text as it is. 1 if anything was.
PRIVATE B32
_Edit_DeletePieces(App_State* app_state, U64 slide_idx, Deck_PlaceKind* kinds, U64* idxs, U64 count) {
   Editor_Editor* editor = app_state->editor;
   Deck_Deck*     deck = app_state->deck;
   if (slide_idx >= deck->slide_count) {
      return 0;
   }
   Deck_Slide* slide = &deck->slides[slide_idx];
   String8     text = Editor_Text(editor);
   Temp        scratch = ScratchBegin(0, 0);
   _Edit_PieceLines* lines = PushArray(scratch.arena, _Edit_PieceLines, 1);
   String8           none = Str8Lit("");

   for (U64 piece = 0; piece < count; piece += 1) {
      Deck_PlaceKind kind = kinds[piece];
      U64            idx = idxs[piece];
      if (kind == Deck_PlaceKind_Picture && idx < slide->picture_count) {
         Deck_Picture* picture = &slide->pictures[idx];
         _Edit_PieceLinesAdd(lines, text, picture->range.min, none);
         if (picture->place.has_line) {
            _Edit_PieceLinesAdd(lines, text, picture->place.line.min, none);
         }
      } else if (kind == Deck_PlaceKind_Text && idx < slide->box_count) {
         Deck_Box* block = &slide->boxes[idx];
         Deck_Item* item = block->first_item;
         for (U64 i = 0; i < block->item_count && item != 0; i += 1, item = item->next) {
            _Edit_PieceLinesAdd(lines, text, item->text_range.min, none);
            // And the lines above it that are its own.
            if (item->look.has_size) {
               _Edit_PieceLinesAdd(lines, text, item->look.size_line.min, none);
            }
            if (item->look.has_color) {
               _Edit_PieceLinesAdd(lines, text, item->look.color_line.min, none);
            }
            if (item->look.has_align) {
               _Edit_PieceLinesAdd(lines, text, item->look.align_line.min, none);
            }
         }
         // Its @box line, which starts it: the slide's first block has none.
         if (block->text_pos != slide->text_pos) {
            _Edit_PieceLinesAdd(lines, text, block->text_pos, none);
         }
         if (block->place.has_line) {
            _Edit_PieceLinesAdd(lines, text, block->place.line.min, none);
         }
         if (block->look.has_size) {
            _Edit_PieceLinesAdd(lines, text, block->look.size_line.min, none);
         }
         if (block->look.has_color) {
            _Edit_PieceLinesAdd(lines, text, block->look.color_line.min, none);
         }
         if (block->look.has_align) {
            _Edit_PieceLinesAdd(lines, text, block->look.align_line.min, none);
         }
      } else if (kind == Deck_PlaceKind_Title && slide->title.size != 0) {
         // The title's line starts the slide: a --- in its place goes on doing
         // that, with no title.
         _Edit_PieceLinesAdd(lines, text, slide->title_range.min, Str8Lit("---\n"));
         Deck_Place* place = &slide->places[Deck_PlaceKind_Title];
         if (place->has_line) {
            _Edit_PieceLinesAdd(lines, text, place->line.min, none);
         }
         if (slide->title_look.has_size) {
            _Edit_PieceLinesAdd(lines, text, slide->title_look.size_line.min, none);
         }
         if (slide->title_look.has_color) {
            _Edit_PieceLinesAdd(lines, text, slide->title_look.color_line.min, none);
         }
         if (slide->title_look.has_align) {
            _Edit_PieceLinesAdd(lines, text, slide->title_look.align_line.min, none);
         }
      }
   }

   B32 result = (lines->count != 0);
   if (result) {
      // In the order they are in the text.
      for (U64 i = 1; i < lines->count; i += 1) {
         for (U64 k = i; k > 0 && lines->lines[k - 1].min > lines->lines[k].min; k -= 1) {
            Rng1U64 line = lines->lines[k - 1];
            String8 put = lines->put[k - 1];
            lines->lines[k - 1] = lines->lines[k];
            lines->put[k - 1] = lines->put[k];
            lines->lines[k] = line;
            lines->put[k] = put;
         }
      }

      // All of it as one change, from the first line to the last: one Ctrl+Z
      // brings it all back.
      U64 span_min = lines->lines[0].min;
      U64 span_max = lines->lines[lines->count - 1].max;
      String8List kept = {0};
      U64         at = span_min;
      for (U64 i = 0; i < lines->count; i += 1) {
         Rng1U64 line = lines->lines[i];
         if (line.min > at) {
            Str8ListPush(scratch.arena, &kept, Str8Substr(text, R1U64(at, line.min)));
         }
         if (lines->put[i].size != 0) {
            // The last line, with no ending, was taken from the one before.
            B32 has_ending = (line.max > line.min && text.str[line.max - 1] == '\n');
            String8 put = has_ending ? lines->put[i] : PushStr8F(scratch.arena, "\n%S", Str8Prefix(lines->put[i], lines->put[i].size - 1));
            Str8ListPush(scratch.arena, &kept, put);
         }
         at = Max(at, line.max);
      }
      String8 replaced = Str8ListJoin(scratch.arena, &kept, 0);

      // An empty line left on either side of where the lines were: one of
      // them goes too, as the slide's lines were written apart.
      B32 empty_above = (span_min >= 2 && text.str[span_min - 1] == '\n' && text.str[span_min - 2] == '\n');
      B32 empty_below = (span_max >= text.size || text.str[span_max] == '\n');
      if (replaced.size == 0 && empty_above && empty_below) {
         span_min -= 1;
      }

      // The cursor at the end of a line that is still the slide's: the last
      // kept between the lines taken out, or the one above them - the
      // title's or --- line at least is never taken out. Not on the slide
      // before, for lines above the slide's first that were about it.
      U64 slide_pos = slide->text_pos;
      Editor_ReplaceRange(editor, span_min, span_max, replaced);
      U64 cursor = (span_min > 0) ? span_min - 1 : 0;
      if (replaced.size != 0) {
         cursor = span_min + replaced.size - (replaced.str[replaced.size - 1] == '\n' ? 1 : 0);
      } else if (slide_pos >= span_min) {
         cursor = span_min;
      }
      Editor_SetCursor(editor, Min(cursor, editor->size));
      editor->mark = editor->cursor;
      editor->last_input_us = NowTimeUS();
      g_caret_view.focus_hidden = 1;
      g_caret_view.focus_hidden_cursor = editor->cursor;
   }

   ScratchEnd(scratch);
   return result;
}

B32
Edit_PickedIsGood(const App_State* app_state) {
   Editor_Editor* editor = app_state->editor;
   return (g_edit_view.picked_count != 0 && editor != 0 && editor->edit_count == g_edit_view.picked_edit_count &&
           editor->cursor == g_edit_view.picked_cursor && editor->mark == g_edit_view.picked_mark &&
           app_state->slide_idx == g_edit_view.picked_slide_idx && !g_caret_view.focus_hidden);
}

// Where the piece of kind and idx is in those picked, or -1.
PRIVATE S64
_Edit_PickedIndexOf(Deck_PlaceKind kind, U64 idx) {
   S64 result = -1;
   for (U64 i = 0; i < g_edit_view.picked_count && result < 0; i += 1) {
      if (g_edit_view.picked_kinds[i] == kind && g_edit_view.picked_idxs[i] == idx) {
         result = (S64)i;
      }
   }
   return result;
}

B32
Edit_PieceIsPicked(const App_State* app_state, Deck_PlaceKind kind, U64 idx) {
   return (Edit_PickedIsGood(app_state) && _Edit_PickedIndexOf(kind, idx) >= 0);
}

// What is picked is good for the text, the cursor and the slide as they are
// now.
PRIVATE void
_Edit_PickedSnapshot(App_State* app_state) {
   Editor_Editor* editor = app_state->editor;
   g_edit_view.picked_slide_idx = app_state->slide_idx;
   g_edit_view.picked_edit_count = editor->edit_count;
   g_edit_view.picked_cursor = editor->cursor;
   g_edit_view.picked_mark = editor->mark;
}

void
Edit_KeepPicked(App_State* app_state, B32 was_good) {
   if (was_good && app_state->editor != 0) {
      _Edit_PickedSnapshot(app_state);
   }
}

////////////////////////////////
//~ bgz: pieces copied, pasted and duplicated
//
// With pieces picked whole, Ctrl+C copies them, Ctrl+X copies them and takes
// them out, and Ctrl+D puts a copy of them a little down and right of where
// they are. Ctrl+V pastes what was copied on the slide showing - with or
// without anything picked - while the system's clipboard still has what
// copying put there (their lines, as the text has them): copied from
// anywhere else, it is text, the editor's. What is pasted or duplicated is
// picked after, to be moved or sized together.
//
// Each piece goes on the slide as one of its own, at the end of the slide's
// lines, as a text box drawn does (_Edit_AddTextBox), with an @place line
// where it was copied from: a block - or the title, as a block of a
// heading - is an @box block with its words' lines and its @size and
// @color lines, a picture is its line. All of a paste is one step to undo.

// How far an arrow key moves the pieces picked, and with Shift. In page
// units (VIEW_PAGE_W by VIEW_PAGE_H).
#define _EDIT_NUDGE_SMALL 4.0f
#define _EDIT_NUDGE_LARGE 20.0f

// How far down and right a paste on the slide copied from, or a duplicate,
// is from what it is of. In parts of the page.
#define _EDIT_PASTE_STEP 0.03f

// The whole line at pos, without its ending, then \n.
PRIVATE String8
_Edit_LineAt(Arena* arena, String8 text, U64 pos) {
   U64 start = Min(pos, text.size);
   U64 end = start;
   while (start > 0 && text.str[start - 1] != '\n') start -= 1;
   while (end < text.size && text.str[end] != '\n') end += 1;
   return PushStr8F(arena, "%S\n", Str8Substr(text, R1U64(start, end)));
}

// An @color line of the title's or a block's, as a block's: the colour,
// which is what follows "@color" - and the "title" or "text" after that, if
// the line says whose.
PRIVATE String8
_Edit_ColorLineAsText(Arena* arena, String8 text, Rng1U64 line, B32 for_line) {
   String8 whole = Str8Substr(text, R1U64(Min(line.min, text.size), Min(line.max, text.size)));
   U64     at = 0;
   for (U32 word = 0; word < 2; word += 1) {
      while (at < whole.size && (whole.str[at] == ' ' || whole.str[at] == '\t')) at += 1;
      U64 word_start = at;
      while (at < whole.size && whole.str[at] != ' ' && whole.str[at] != '\t') at += 1;
      String8 name = Str8Substr(whole, R1U64(word_start, at));
      if (word == 1 && !Str8Match(name, Str8Lit("title"), StringMatchFlag_CaseInsensitive) &&
          !Str8Match(name, Str8Lit("line"), StringMatchFlag_CaseInsensitive) &&
          !Str8Match(name, Str8Lit("text"), StringMatchFlag_CaseInsensitive)) {
         // The colour itself.
         at = word_start;
      }
   }
   while (at < whole.size && (whole.str[at] == ' ' || whole.str[at] == '\t')) at += 1;
   // for_line: one line's own, said so - it may be written at a block's top.
   return PushStr8F(arena, "@color %s%S\n", for_line ? "line " : "", Str8Skip(whole, at));
}

// The pieces picked, as they are written (Edit_ClipPiece), into pieces, on
// arena: how many. The deck, the text and the preview's boxes have to be of
// the text as it is.
PRIVATE U64
_Edit_ClipFromPicked(Arena* arena, const App_State* app_state, Edit_ClipPiece* pieces) {
   U64 count = 0;
   if (!Place_BoxesAreGood(app_state)) {
      return 0;
   }
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   String8     text = Editor_Text(app_state->editor);
   Rng2F32     page = g_edit_view.page;
   Vec2F32     page_dim = Dim2F32(page);
   for (U64 i = 0; i < g_edit_view.picked_count; i += 1) {
      Deck_PlaceKind kind = g_edit_view.picked_kinds[i];
      U64            idx = g_edit_view.picked_idxs[i];
      View_Box*      box = 0;
      for (U64 k = 0; k < g_edit_view.box_count; k += 1) {
         if (g_edit_view.boxes[k].kind == kind && g_edit_view.boxes[k].idx == idx) {
            box = &g_edit_view.boxes[k];
         }
      }
      if (box == 0) {
         continue;
      }
      Edit_ClipPiece piece = {0};
      piece.rect = R2F32P((box->rect.min.x - page.min.x) / page_dim.x, (box->rect.min.y - page.min.y) / page_dim.y,
                          (box->rect.max.x - page.min.x) / page_dim.x, (box->rect.max.y - page.min.y) / page_dim.y);
      String8List lines = {0};
      String8List looks = {0};
      Deck_Look*  look = 0;
      B32         ok = 1;
      if (kind == Deck_PlaceKind_Picture && idx < slide->picture_count) {
         piece.is_picture = 1;
         Str8ListPush(arena, &lines, _Edit_LineAt(arena, text, slide->pictures[idx].range.min));
      } else if (kind == Deck_PlaceKind_Text && idx < slide->box_count) {
         Deck_Box*  block = &slide->boxes[idx];
         Deck_Item* item = block->first_item;
         for (U64 n = 0; n < block->item_count && item != 0; n += 1, item = item->next) {
            // A line's own @size, @color and @align lines, above it.
            if (item->look.has_size) {
               Str8ListPush(arena, &lines, PushStr8F(arena, "@size line %.2f\n", item->look.size));
            }
            if (item->look.has_color) {
               Str8ListPush(arena, &lines, _Edit_ColorLineAsText(arena, text, item->look.color_line, 1));
            }
            if (item->look.has_align) {
               Str8ListPush(arena, &lines, PushStr8F(arena, "@align line %S\n", Deck_AlignName(item->look.align)));
            }
            Str8ListPush(arena, &lines, _Edit_LineAt(arena, text, item->text_range.min));
         }
         look = &block->look;
      } else if (kind == Deck_PlaceKind_Title && slide->title.size != 0) {
         Str8ListPush(arena, &lines, PushStr8F(arena, "## %S\n", slide->title));
         look = &slide->title_look;
      } else {
         ok = 0;
      }
      if (!ok || lines.node_count == 0) {
         continue;
      }
      if (look != 0 && look->has_size) {
         Str8ListPush(arena, &looks, PushStr8F(arena, "@size %.2f\n", look->size));
      }
      if (look != 0 && look->has_color) {
         Str8ListPush(arena, &looks, _Edit_ColorLineAsText(arena, text, look->color_line, 0));
      }
      if (look != 0 && look->has_align) {
         Str8ListPush(arena, &looks, PushStr8F(arena, "@align %S\n", Deck_AlignName(look->align)));
      }
      piece.lines = Str8ListJoin(arena, &lines, 0);
      piece.looks = Str8ListJoin(arena, &looks, 0);
      pieces[count] = piece;
      count += 1;
   }
   return count;
}

// Writes pieces on slide_idx, offset down and right by offset, and picks
// them. At the end of the slide's lines, where they are drawn last, over the
// slide's other pictures or blocks - or with at_back at its start, right
// under the line that starts it, where they are drawn first, under them:
// there an @box line after them starts the block that was the slide's first.
// The deck has to be of the text as it is. 1 if any were.
PRIVATE B32
_Edit_PastePieces(App_State* app_state, StateOp_List* ops, U64 slide_idx, Edit_ClipPiece* pieces, U64 count, F32 offset, B32 at_back) {
   Editor_Editor* editor = app_state->editor;
   Deck_Deck*     deck = app_state->deck;
   if (slide_idx >= deck->slide_count || count == 0) {
      return 0;
   }
   Temp        scratch = ScratchBegin(0, 0);
   Deck_Slide* slide = &deck->slides[slide_idx];
   String8     text = Editor_Text(editor);

   // Each piece's lines, with no line ending after the last.
   String8List added = {0};
   B32         has_block = 0;
   for (U64 i = 0; i < count; i += 1) {
      Edit_ClipPiece* piece = &pieces[i];
      F32 w = piece->rect.max.x - piece->rect.min.x;
      F32 h = piece->rect.max.y - piece->rect.min.y;
      F32 x = Clamp(0.0f, piece->rect.min.x + offset, Max(1.0f - w, 0.0f));
      F32 y = Clamp(0.0f, piece->rect.min.y + offset, Max(1.0f - h, 0.0f));
      if (slide->is_canvas) {
         x = piece->rect.min.x + offset;
         y = piece->rect.min.y + offset;
      }
      if (i != 0) {
         Str8ListPush(scratch.arena, &added, at_back ? Str8Lit("\n") : Str8Lit("\n\n"));
      }
      if (piece->is_picture) {
         Str8ListPush(scratch.arena, &added, PushStr8F(scratch.arena, "@place picture %.3f %.3f %.3f %.3f\n%S", x, y, w, h, Str8Prefix(piece->lines, piece->lines.size - 1)));
      } else {
         has_block = 1;
         Str8ListPush(scratch.arena, &added, PushStr8F(scratch.arena, "@box\n@place text %.3f %.3f %.3f %.3f\n%S%S", x, y, w, h,
                                                       piece->looks, Str8Prefix(piece->lines, piece->lines.size - 1)));
      }
   }
   String8 body = Str8ListJoin(scratch.arena, &added, 0);

   U64     at = 0;
   String8 joined = {0};
   if (!at_back) {
      // Under the slide's last line with anything on it, as a box drawn.
      Rng1U64 range = _Edit_TextRangeFromSlide(editor, deck, slide_idx);
      at = range.max;
      while (at > range.min && (text.str[at - 1] == '\n' || text.str[at - 1] == ' ' || text.str[at - 1] == '\t')) {
         at -= 1;
      }
      joined = PushStr8F(scratch.arena, "\n\n%S", body);
   } else {
      // Under the line that starts the slide, its title's or its ---. A
      // slide that starts with neither - the deck's first, straight into
      // its words - has them above its first line.
      U64 line_start = Min(slide->text_pos, text.size);
      U64 line_end = line_start;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
      String8 first_line = Str8Substr(text, R1U64(line_start, line_end));
      B32     starts_slide = (slide->title_range.max != 0 || (first_line.size >= 3 && MemoryMatch(first_line.str, "---", 3)) ||
                              Str8Match(Str8Prefix(first_line, 6), Str8Lit("@slide"), StringMatchFlag_CaseInsensitive) || slide->is_canvas);
      String8 after = has_block ? Str8Lit("\n@box") : Str8Lit("");
      // Under the title's line, when the title is not the first: a slide
      // begun by a --- or an @slide, with its title under that.
      if (slide->title_range.max != 0 && slide->title_range.max <= text.size) {
         line_end = slide->title_range.max;
         while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
      }
      if (starts_slide) {
         at = line_end;
         joined = PushStr8F(scratch.arena, "\n%S%S", body, after);
      } else {
         at = line_start;
         joined = PushStr8F(scratch.arena, "%S%S\n", body, after);
      }
   }
   Editor_ReplaceRange(editor, at, at, joined);
   U64 added_end = at + joined.size;
   Editor_SetCursor(editor, added_end);
   editor->mark = added_end;
   editor->last_input_us = NowTimeUS();
   g_caret_view.focus_hidden = 0;
   Vim_StartTyping(app_state);
   ScratchEnd(scratch);

   //- bgz: what was pasted, picked: the slide's pieces in what went in.
   // The deck made again is a new one: deck is gone.
   FR_SyncDeck(app_state);
   deck = app_state->deck;
   g_edit_view.picked_count = 0;
   if (slide_idx < deck->slide_count) {
      slide = &deck->slides[slide_idx];
      for (U64 i = 0; i < slide->picture_count && g_edit_view.picked_count < VIEW_MAX_BOXES; i += 1) {
         if (slide->pictures[i].range.min >= at && slide->pictures[i].range.min < added_end) {
            g_edit_view.picked_kinds[g_edit_view.picked_count] = Deck_PlaceKind_Picture;
            g_edit_view.picked_idxs[g_edit_view.picked_count] = i;
            g_edit_view.picked_count += 1;
         }
      }
      for (U64 i = 0; i < slide->box_count && g_edit_view.picked_count < VIEW_MAX_BOXES; i += 1) {
         Deck_Box* block = &slide->boxes[i];
         if (block->item_count != 0 && block->first_item != 0 && block->first_item->text_range.min >= at &&
             block->first_item->text_range.min <= added_end) {
            g_edit_view.picked_kinds[g_edit_view.picked_count] = Deck_PlaceKind_Text;
            g_edit_view.picked_idxs[g_edit_view.picked_count] = i;
            g_edit_view.picked_count += 1;
         }
      }
   }
   _Edit_PickedSnapshot(app_state);
   g_edit_view.picked_slide_idx = slide_idx;
   if (g_edit_view.picked_count < count) {
      StateOp_App_Notice(ops)->notice = PushStr8F(ops->arena, "Not all of it fits: a slide has at most %d pictures and %d text boxes", DECK_MAX_PICTURES, DECK_MAX_BOXES);
   }
   return 1;
}

// Gives the deck the theme after the one it has: its @theme line is
// written again, or a new one is put at the top of the text.
PRIVATE void
_Edit_NextTheme(Editor_Editor* editor, Deck_Deck* deck) {
   Temp       scratch = ScratchBegin(0, 0);
   Deck_Theme theme = (Deck_Theme)((deck->theme + 1) % Deck_Theme_COUNT);
   String8    line = PushStr8F(scratch.arena, "@theme %S", Deck_ThemeName(theme));

   if (deck->has_theme_line && deck->theme_line.max <= editor->size) {
      _Edit_ReplaceKeepingCursor(editor, deck->theme_line.min, deck->theme_line.max, line);
   } else {
      _Edit_ReplaceKeepingCursor(editor, 0, 0, PushStr8F(scratch.arena, "%S\n", line));
   }

   ScratchEnd(scratch);
}

// Gives slide_idx the layout after the one it has: its @layout line is
// written again, or a new one is put above the slide's # Title - or under
// the --- or @slide line it starts with.
PRIVATE void _Edit_WriteLayout(Editor_Editor* editor, Deck_Deck* deck, U64 slide_idx, Deck_Layout layout);

PRIVATE void
_Edit_NextLayout(Editor_Editor* editor, Deck_Deck* deck, U64 slide_idx) {
   if (slide_idx < deck->slide_count) {
      _Edit_WriteLayout(editor, deck, slide_idx, (Deck_Layout)((deck->slides[slide_idx].layout + 1) % Deck_Layout_COUNT));
   }
}

// The slide's @layout line, written: over the one it has, or as a new one.
PRIVATE void
_Edit_WriteLayout(Editor_Editor* editor, Deck_Deck* deck, U64 slide_idx, Deck_Layout layout) {
   if (slide_idx >= deck->slide_count) {
      return;
   }
   Temp        scratch = ScratchBegin(0, 0);
   Deck_Slide* slide = &deck->slides[slide_idx];
   String8     line = PushStr8F(scratch.arena, "@layout %S", Deck_LayoutName(layout));
   String8     text = Editor_Text(editor);

   if (slide->has_layout_line && slide->layout_line.max <= text.size) {
      _Edit_ReplaceKeepingCursor(editor, slide->layout_line.min, slide->layout_line.max, line);
   } else {
      // Under the slide's first line when that is its --- or @slide: the
      // line belongs to the slide from there. Else above it - the # Title,
      // or the lines about the slide that are above that already: what a
      // line is about is under it, and the deck reads those as the
      // slide's (deck.h).
      U64 line_start = Min(slide->text_pos, text.size);
      U64 line_end = line_start;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
      U64 first = line_start;
      while (first < line_end && (text.str[first] == ' ' || text.str[first] == '\t')) first += 1;

      B32 starts_slide = (first < line_end && (text.str[first] == '-' ||
                                               Str8Match(Str8Prefix(Str8Substr(text, R1U64(first, line_end)), 6), Str8Lit("@slide"),
                                                         StringMatchFlag_CaseInsensitive) ||
                                               slide->is_canvas));
      if (starts_slide) {
         _Edit_ReplaceKeepingCursor(editor, line_end, line_end, PushStr8F(scratch.arena, "\n%S", line));
      } else {
         _Edit_ReplaceKeepingCursor(editor, line_start, line_start, PushStr8F(scratch.arena, "%S\n", line));
      }
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: pictures

// How often the folder is looked in again while the list is open.
#define _EDIT_PICTURES_SCAN_SECONDS 1.0f

PRIVATE void
_Edit_ScanPictures(const App_State* app_state) {
   if (g_edit_view.pictures_arena == 0) {
      g_edit_view.pictures_arena = ArenaAlloc();
   }
   ArenaClear(g_edit_view.pictures_arena);
   g_edit_view.pictures = Deck_PictureNamesFromFolder(g_edit_view.pictures_arena, app_state->deck->folder);
   g_edit_view.pictures_scanned_us = NowTimeUS();
}

// Puts name on the slide the cursor is on, as a picture: one more, under
// the pictures the slide has - or on a new line at the cursor, if it has
// none. With a picture's line selected - a click on the picture selects
// it - that picture is changed for this one instead.
//
// A slide that has DECK_MAX_PICTURES already, and none selected, gets
// nothing but a notice, and 0 is returned. With only_ask, nothing else is
// done either way: whether there is room is all that is found out - for a
// file not to be copied into the deck's folder for a slide that cannot
// have it.
PRIVATE B32
_Edit_PutPicture(App_State* app_state, String8 name, B32 only_ask) {
   Editor_Editor* editor = app_state->editor;
   Deck_Deck* deck = app_state->deck;
   String8    text = Editor_Text(editor);
   Temp       scratch = ScratchBegin(0, 0);
   String8    line = PushStr8F(scratch.arena, "![](%S)", name);

   // The slide's part of the text. The deck is from the text as it was at
   // the last FR_SyncDeck, which is this frame's or the last's: positions
   // are kept inside the text as it is now.
   U64 slide_start = 0;
   U64 slide_end = text.size;
   if (deck->slide_count != 0) {
      U64 slide_idx = Deck_SlideFromTextPos(deck, editor->cursor);
      slide_start = Min(deck->slides[slide_idx].text_pos, text.size);
      if (slide_idx + 1 < deck->slide_count) {
         slide_end = Min(deck->slides[slide_idx + 1].text_pos, text.size);
      }
   }

   // The slide's lines that start with ![ - spaces before them aside: the
   // one the cursor is on, if it is on one, is written over - that picture
   // is changed for this one. Else the new line goes under the last of
   // them, and under that one's @place line: one more picture on the slide.
   B32 found = 0;
   B32 has_last = 0;
   U64 last_end = 0; // the end of the last picture's line, or of its @place line
   U64 picture_count = 0;
   U64 pos = slide_start;
   while (pos < slide_end && !found) {
      U64 line_start = pos;
      while (pos < slide_end && text.str[pos] != '\n') pos += 1;
      U64 line_end = pos;
      pos += 1;

      U64 first = line_start;
      while (first < line_end && text.str[first] == ' ') first += 1;
      if (first + 1 < line_end && text.str[first] == '!' && text.str[first + 1] == '[') {
         picture_count += 1;
         // Selected, as a click on the picture leaves it: not only the
         // cursor on the line, which is where a picture just put leaves
         // it - the next would take its place, not go under it.
         B32 is_selected = (editor->mark != editor->cursor && editor->cursor >= line_start && editor->cursor <= line_end &&
                            editor->mark >= line_start && editor->mark <= line_end);
         if (is_selected) {
            if (!only_ask) {
               Editor_ReplaceRange(editor, line_start, line_end, line);
            }
            found = 1;
         }
         has_last = 1;
         last_end = line_end;
      } else if (has_last && last_end + 1 == line_start &&
                 Str8Match(Str8Prefix(Str8Substr(text, R1U64(first, line_end)), 6), Str8Lit("@place"), StringMatchFlag_CaseInsensitive)) {
         last_end = line_end;
      }
   }
   B32 result = (found || picture_count < DECK_MAX_PICTURES);
   if (!result) {
      StateOp_App_Notice(app_state->op_list)->notice = Str8Lit("The slide has as many pictures as a slide can. Click one to change it for this one");
   } else if (only_ask) {
      // Nothing written.
   } else if (!found && has_last) {
      String8 added = PushStr8F(scratch.arena, "\n%S", line);
      Editor_ReplaceRange(editor, last_end, last_end, added);
      // On the new line, at its end: the next one put goes under it, and
      // the preview marks it.
      Editor_SetCursor(editor, last_end + added.size);
   } else if (!found) {
      Editor_InsertLine(editor, line, line.size, line.size);
   }
   if (!only_ask) {
      editor->last_input_us = NowTimeUS();
   }

   ScratchEnd(scratch);
   return result;
}

// An image or video file dropped on the window: it becomes the slide's
// picture. A file from somewhere else is copied into the deck's folder, so
// the deck and its pictures stay together - unless a file of that name is
// there already, and then that one is used. An image is copied first. A
// video, which can be very large, is copied while it already shows.
PRIVATE void
_Edit_DropPicture(App_State* app_state, String8 path) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 name = Str8SkipLastSlash(path);
   String8 in_folder = PushStr8F(scratch.arena, "%S\\%S", app_state->deck->folder, name);

   // Not copied at all for a slide with no room for it: the notice says so.
   if (!_Edit_PutPicture(app_state, name, 1)) {
      ScratchEnd(scratch);
      return;
   }

   B32 is_there = FilePathExists(in_folder);
   if (!is_there && Video_NameIsVideo(name)) {
      // A video is not waited for: it is copied in the background and
      // plays from where it is meanwhile (copy\copy.c).
      if (!Copy_IsCopyingTo(app_state, in_folder)) {
         Copy_Start(app_state, path, in_folder);
         app_state->quit_warned_copy = 0;
         StateOp_App_Notice(app_state->op_list)->notice = Str8Lit("Copying the video into the deck's folder. It plays from where it is meanwhile.");
      }
      _Edit_PutPicture(app_state, name, 0);
      ScratchEnd(scratch);
      return;
   }
   if (!is_there) {
      is_there = CopyFilePath(in_folder, path);
      if (is_there) {
         StateOp_App_Notice(app_state->op_list)->notice = Str8Lit("Picture copied into the deck's folder");
      } else {
         StateOp_App_Notice(app_state->op_list)->notice = Str8Lit("Could not copy the picture into the deck's folder");
      }
   }
   if (is_there) {
      _Edit_PutPicture(app_state, name, 0);
      g_edit_view.pictures_scanned_us = 0; // the list, if open, is out of date
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: input

// Which of the preview's pieces (g_edit_view.hits) a point is on, or -1.
// Asked by a click, which is already known to be on the slide's boxes: of
// several hits there, the one it goes to.
// Where pieces are over each other, the one drawn last: the slide's pictures
// are drawn first, in the order of their lines, then the title, then the
// body - so words on a picture are the words, and of pictures over each
// other the one on top is the one seen.
S64
Edit_PreviewHitAt(Vec2F32 pos) {
   S64 result = -1;
   for (U64 i = g_edit_view.hit_count; i > 0; i -= 1) {
      if (Contains2F32(g_edit_view.hits[i - 1].rect, pos)) {
         result = (S64)(i - 1);
         break;
      }
   }
   return result;
}

// Bold and code are for words: 0, with a notice, when the cursor is on a
// picture's line or an @ line, where the marks would be part of a file's
// name or of a setting - a picture that can no longer be found.
PRIVATE B32
_Edit_CanWrap(const App_State* app_state) {
   Editor_Editor* editor = app_state->editor;
   String8    text = Editor_Text(editor);
   U64        line_start = Min(editor->cursor, text.size);
   while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
   while (line_start < text.size && (text.str[line_start] == ' ' || text.str[line_start] == '\t')) line_start += 1;

   B32 is_picture = (line_start + 1 < text.size && text.str[line_start] == '!' && text.str[line_start + 1] == '[');
   B32 is_setting = (line_start < text.size && text.str[line_start] == '@');
   B32 result = !(is_picture || is_setting);
   if (!result) {
      StateOp_App_Notice(app_state->op_list)->notice = Str8Lit("Bold and code are for words, not for a picture's line or an @ line");
   }
   return result;
}

// Where a video of the slide showing is drawn in the preview: the place the
// preview last drew the picture_idx-th of the slide's pictures, if that is
// a video that is open. 0 if not, or if the text has changed since the
// preview was drawn. A picture behind the words has no place of its own to
// click (view.c): it is all of page, as the presentation has it.
PRIVATE Video_Video*
_Edit_PreviewVideo(const App_State* app_state, Rng2F32 page, U64 picture_idx, Rng2F32* rect_out) {
   Video_Video* result = 0;
   Deck_Deck* deck = app_state->deck;

   if (app_state->slide_idx < deck->slide_count && app_state->editor != 0 &&
       app_state->editor->edit_count == g_edit_view.hits_edit_count &&
       picture_idx < deck->slides[app_state->slide_idx].picture_count) {
      Deck_Slide*   slide = &deck->slides[app_state->slide_idx];
      Deck_Picture* picture = &slide->pictures[picture_idx];
      // A gif plays by itself, with no strip.
      if (picture->video == 0 || Video_NameIsMovingPicture(picture->path)) {
         return 0;
      }
      if (picture_idx == 0 && slide->layout == Deck_Layout_PictureBehind && Deck_SlideHasText(slide)) {
         *rect_out = page;
         return picture->video;
      }
      for (U64 i = 0; i < g_edit_view.hit_count; i += 1) {
         View_Hit* hit = &g_edit_view.hits[i];
         if (hit->text_range.min == picture->range.min && hit->text_range.max == picture->range.max) {
            *rect_out = hit->rect;
            result = picture->video;
            break;
         }
      }
   }

   return result;
}

// A slide added after the one the cursor is on (add), or that one moved a
// place later or earlier, and the deck made again from the text.
PRIVATE void
_Edit_ChangeSlides(App_State* app_state, B32 add, B32 to_later) {
   Editor_Editor* editor = app_state->editor;
   // The deck as the text is now: something this frame may have changed it.
   FR_SyncDeck(app_state);
   U64 slide_idx = Deck_SlideFromTextPos(app_state->deck, editor->cursor);
   if (add) {
      _Edit_AddSlideAfter(editor, app_state->deck, slide_idx);
   } else if (!_Edit_MoveSlide(editor, app_state->deck, slide_idx, to_later)) {
      StateOp_App_Notice(app_state->op_list)->notice = to_later ? Str8Lit("This is the last slide already") : Str8Lit("This is the first slide already");
   }
   editor->last_input_us = NowTimeUS();
   FR_SyncDeck(app_state);
   app_state->slide_idx = Deck_SlideFromTextPos(app_state->deck, editor->cursor);
}

////////////////////////////////
//~ bgz: state ops
//
// Everything the edit screen does to the text, the cursor and what is open.
// Edit_Input and Edit_Build push them; _FR_UpdateAppState runs them, and
// then makes the deck again from the text and finds the slide the cursor is
// on. An op that needs the deck of the text as an op before it left it makes
// it again itself (FR_SyncDeck).

// The mouse in the text: a press at pos, the button held and the mouse now
// at pos, and the wheel.
typedef struct Edit_TextPressOpParams Edit_TextPressOpParams;
struct Edit_TextPressOpParams {
   Editor_Box box;
   Vec2F32    pos;
   B32        keeps_mark;
};
void
Edit_TextPress(App_State* app_state, StateOp_List* ops, Edit_TextPressOpParams* params) {
   UnusedVariable(ops);
   if (app_state->editor != 0) {
      Editor_Press(app_state->editor, app_state->frame_arena, params->box, params->pos, params->keeps_mark);
      Vim_AfterTextMouse(app_state);
   }
}
STATE_OP(Edit_TextPress)

typedef struct Edit_TextDragOpParams Edit_TextDragOpParams;
struct Edit_TextDragOpParams {
   Editor_Box box;
   Vec2F32    pos;
};
void
Edit_TextDrag(App_State* app_state, StateOp_List* ops, Edit_TextDragOpParams* params) {
   UnusedVariable(ops);
   if (app_state->editor != 0) {
      Editor_Drag(app_state->editor, app_state->frame_arena, params->box, params->pos);
      Vim_AfterTextMouse(app_state);
   }
}
STATE_OP(Edit_TextDrag)

typedef struct Edit_TextWheelOpParams Edit_TextWheelOpParams;
struct Edit_TextWheelOpParams {
   Editor_Box box;
   F32        notches;
};
void
Edit_TextWheel(App_State* app_state, StateOp_List* ops, Edit_TextWheelOpParams* params) {
   UnusedVariable(ops);
   if (app_state->editor != 0) {
      Editor_Wheel(app_state->editor, params->box, params->notches);
   }
}
STATE_OP(Edit_TextWheel)

// F1, or a click on the Format panel's header: closes the list of pictures
// if that is what is open, else opens and closes the panel.
void
Edit_ToggleFormat(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   if (app_state->hide_text) {
      // The panel is on the left side, which is away: both come back.
      app_state->hide_text = 0;
      app_state->show_format = 1;
   } else if (app_state->show_pictures) {
      app_state->show_pictures = 0;
   } else {
      app_state->show_format = !app_state->show_format;
   }
}
STATE_OP_NO_PARAMS(Edit_ToggleFormat)

// Alt+[: the left side - the text, the Format panel and the status line -
// put away, so the preview has the whole window, and brought back.
void
Edit_ToggleText(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->hide_text = !app_state->hide_text;
   if (app_state->editor != 0) {
      app_state->editor->scroll_to_cursor = 1;
   }
}
STATE_OP_NO_PARAMS(Edit_ToggleText)

void
Edit_ClosePictures(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->show_pictures = 0;
}
STATE_OP_NO_PARAMS(Edit_ClosePictures)

// Esc: nothing selected, and no box in the preview around the piece the
// cursor is in.
void
Edit_Deselect(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor != 0) {
      // A text box a click made and left as it was goes.
      if (g_edit_view.has_new_box) {
         editor->cursor = g_edit_view.new_box_range.min;
         _Edit_TakeOutNewBox(app_state);
      }
      g_edit_view.picked_count = 0;
      editor->mark = editor->cursor;
      g_caret_view.focus_hidden = 1;
      g_caret_view.focus_hidden_cursor = editor->cursor;
   }
}
STATE_OP_NO_PARAMS(Edit_Deselect)

// Bold, or code: the marks around what is selected.
typedef struct Edit_WrapOpParams Edit_WrapOpParams;
struct Edit_WrapOpParams {
   B32 is_code;
};
void
Edit_Wrap(App_State* app_state, StateOp_List* ops, Edit_WrapOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor != 0 && _Edit_CanWrap(app_state)) {
      Editor_WrapSelection(editor, params->is_code ? Str8Lit("`") : Str8Lit("**"), params->is_code ? Str8Lit("code") : Str8Lit("bold"));
      Vim_StartTyping(app_state);
      editor->last_input_us = NowTimeUS();
   }
}
STATE_OP(Edit_Wrap)

void
Edit_Save(App_State* app_state, StateOp_List* ops) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   if (Editor_Save(editor)) {
      StateOp_App_Notice(ops)->notice = Str8Lit("Saved");
      app_state->quit_warned = 0;

      // A picture that was not found is looked for again: saving is when
      // the file is likely to have been put in place.
      FR_SyncDeck(app_state);
      for (U64 i = 0; i < app_state->deck->slide_count; i += 1) {
         for (U64 k = 0; k < app_state->deck->slides[i].picture_count; k += 1) {
            app_state->deck->slides[i].pictures[k].load_failed = 0;
         }
      }
   } else {
      StateOp_App_Notice(ops)->notice = Str8Lit("Could not save: the file could not be written");
   }
}
STATE_OP_NO_PARAMS(Edit_Save)

// F5: from the edit screen to the presentation, at the slide the cursor is
// on.
void
Edit_Present(App_State* app_state, StateOp_List* ops) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   app_state->slide_idx = Deck_SlideFromTextPos(app_state->deck, editor->cursor);
   // A board is not shown: from one, the slide after it - or, with none,
   // the one before.
   U64 shown_idx = Deck_SlideToShow(app_state->deck, app_state->slide_idx, 1);
   if (shown_idx >= app_state->deck->slide_count) {
      shown_idx = Deck_SlideToShow(app_state->deck, app_state->slide_idx, -1);
   }
   if (shown_idx < app_state->deck->slide_count) {
      app_state->slide_idx = shown_idx;
      app_state->is_editing = 0;
      // Presenting shows every block from its top (caret\caret.c).
      Caret_ScrollToTop();
      g_edit_view.hit_count = 0; // not drawn while presenting, and the window can change
      g_edit_view.box_count = 0;
      Place_GiveUp(); // a drag under way: the piece back where it was
      app_state->fade_active = 0;
      app_state->slide_changed_us = 0;
   } else {
      StateOp_App_Notice(ops)->notice = Str8Lit("Nothing to present yet: start a slide with  # Title  or  @slide");
   }
}
STATE_OP_NO_PARAMS(Edit_Present)

// The slide the cursor is on moved a place later or earlier, and a slide
// added after it.
typedef struct Edit_MoveSlideOpParams Edit_MoveSlideOpParams;
struct Edit_MoveSlideOpParams {
   B32 to_later;
};
void
Edit_MoveSlide(App_State* app_state, StateOp_List* ops, Edit_MoveSlideOpParams* params) {
   UnusedVariable(ops);
   if (app_state->editor != 0) {
      _Edit_ChangeSlides(app_state, 0, params->to_later);
   }
}
STATE_OP(Edit_MoveSlide)

void
Edit_AddSlide(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   if (app_state->editor != 0) {
      _Edit_ChangeSlides(app_state, 1, 0);
   }
}
STATE_OP_NO_PARAMS(Edit_AddSlide)

// The slide the cursor is on given a layout: its @layout line.
typedef struct Edit_SetLayoutOpParams Edit_SetLayoutOpParams;
struct Edit_SetLayoutOpParams {
   Deck_Layout layout;
};
void
Edit_SetLayout(App_State* app_state, StateOp_List* ops, Edit_SetLayoutOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor != 0 && params->layout < Deck_Layout_COUNT) {
      FR_SyncDeck(app_state);
      _Edit_WriteLayout(editor, app_state->deck, Deck_SlideFromTextPos(app_state->deck, editor->cursor), params->layout);
      editor->last_input_us = NowTimeUS();
   }
}
STATE_OP(Edit_SetLayout)

// 1 if a slide's piece of the text starts it wherever it is put: with a
// # Title, a ---, an @slide or an @canvas - or lines about it above its
// title. The deck's first slide can start with none of those.
PRIVATE B32
_Edit_SlideStartsItself(String8 piece, Deck_Slide* slide) {
   B32 result = 0;
   U64 i = 0;
   while (i < piece.size && (piece.str[i] == ' ' || piece.str[i] == '\t' || piece.str[i] == '\n')) i += 1;
   if (i < piece.size) {
      String8 rest = Str8Skip(piece, i);
      if (rest.str[0] == '#') {
         result = slide->is_canvas ? 0 : !(rest.size > 1 && rest.str[1] == '#');
      } else if (rest.str[0] == '-') {
         result = (rest.size >= 3 && rest.str[1] == '-' && rest.str[2] == '-');
      } else if (rest.str[0] == '@') {
         result = (Str8Match(Str8Prefix(rest, 6), Str8Lit("@slide"), StringMatchFlag_CaseInsensitive) ||
                   Str8Match(Str8Prefix(rest, 7), Str8Lit("@canvas"), StringMatchFlag_CaseInsensitive) || slide->title_range.max != 0);
      }
   }
   return result;
}

// What the slide menu does to the slide the cursor is on.
typedef enum Edit_SlideChange {
   Edit_SlideChange_Duplicate, // a copy of it after it, and the cursor in the copy
   Edit_SlideChange_Delete,    // its piece of the text taken out
   Edit_SlideChange_Board,     // a slide made a board (@canvas), or a board a slide
} Edit_SlideChange;
typedef struct Edit_ChangeSlideOpParams Edit_ChangeSlideOpParams;
struct Edit_ChangeSlideOpParams {
   Edit_SlideChange change;
};
void
Edit_ChangeSlide(App_State* app_state, StateOp_List* ops, Edit_ChangeSlideOpParams* params) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   Deck_Deck* deck = app_state->deck;
   if (deck->slide_count == 0) {
      return;
   }
   Temp        scratch = ScratchBegin(0, 0);
   U64         slide_idx = Deck_SlideFromTextPos(deck, editor->cursor);
   Deck_Slide* slide = &deck->slides[slide_idx];
   Rng1U64     range = _Edit_TextRangeFromSlide(editor, deck, slide_idx);
   String8     text = Editor_Text(editor);
   String8     piece = Str8Substr(text, range);
   U64         undo_before = editor->undo_count;

   if (params->change == Edit_SlideChange_Duplicate) {
      // On lines of its own, an empty line before it, and starting a slide
      // by itself - which the deck's first may not.
      B32     ends_line = (piece.size == 0 || piece.str[piece.size - 1] == '\n');
      B32     ends_empty = (piece.size >= 2 && piece.str[piece.size - 1] == '\n' && piece.str[piece.size - 2] == '\n');
      String8 before = !ends_line ? Str8Lit("\n\n") : !ends_empty ? Str8Lit("\n") : Str8Lit("");
      String8 start = _Edit_SlideStartsItself(piece, slide) ? Str8Lit("") : Str8Lit("---\n");
      String8 after = (range.max < text.size && !ends_line) ? Str8Lit("\n\n") : Str8Lit("");
      String8 added = PushStr8F(scratch.arena, "%S%S%S%S", before, start, piece, after);
      Editor_ReplaceRange(editor, range.max, range.max, added);
      Editor_SetCursor(editor, range.max + before.size);
   } else if (params->change == Edit_SlideChange_Delete) {
      // The slide after a first that did not start itself is the first now,
      // and still starts itself.
      Editor_ReplaceRange(editor, range.min, range.max, Str8Lit(""));
      Editor_SetCursor(editor, Min(range.min, editor->size));
      StateOp_App_Notice(ops)->notice = Str8Lit("Slide deleted      Ctrl+Z brings it back");
   } else {
      // The line the slide starts with.
      U64 first = 0;
      while (first < piece.size && (piece.str[first] == ' ' || piece.str[first] == '\t' || piece.str[first] == '\n')) first += 1;
      U64 line_end = first;
      while (line_end < piece.size && piece.str[line_end] != '\n') line_end += 1;
      String8 line = Str8Substr(piece, R1U64(first, line_end));
      B32     is_rule = (line.size >= 3 && line.str[0] == '-' && line.str[1] == '-' && line.str[2] == '-') ||
                    Str8Match(Str8Prefix(line, 6), Str8Lit("@slide"), StringMatchFlag_CaseInsensitive);
      B32     is_canvas_line = Str8Match(Str8Prefix(line, 7), Str8Lit("@canvas"), StringMatchFlag_CaseInsensitive);
      if (slide->is_canvas && is_canvas_line) {
         _Edit_ReplaceKeepingCursor(editor, range.min + first, range.min + line_end, Str8Lit("---"));
      } else if (!slide->is_canvas && is_rule) {
         _Edit_ReplaceKeepingCursor(editor, range.min + first, range.min + line_end, Str8Lit("@canvas"));
      } else if (!slide->is_canvas) {
         // A # Title, or words: the board starts above them, and the title
         // is a heading on it.
         _Edit_ReplaceKeepingCursor(editor, range.min + first, range.min + first, Str8Lit("@canvas\n"));
      }
   }
   Editor_JoinUndoSince(editor, undo_before);
   editor->last_input_us = NowTimeUS();
   FR_SyncDeck(app_state);
   app_state->slide_idx = Deck_SlideFromTextPos(app_state->deck, editor->cursor);
   ScratchEnd(scratch);
}
STATE_OP(Edit_ChangeSlide)

// What a line of a block is, set from the menu: how it starts.
typedef enum Edit_LineKind {
   Edit_LineKind_Paragraph,
   Edit_LineKind_Bullet,      // "- "
   Edit_LineKind_InnerBullet, // "  - "
   Edit_LineKind_Heading,     // "## "
} Edit_LineKind;
typedef struct Edit_SetLineKindOpParams Edit_SetLineKindOpParams;
struct Edit_SetLineKindOpParams {
   Edit_LineKind kind;
};
// Every line the selection is in - or the cursor's - made a line of that
// kind: what it starts with taken off, and the kind's put on. Not a
// picture's line, an @ line, an empty one, or a slide's # Title. One step
// to undo, and the lines left selected.
void
Edit_SetLineKind(App_State* app_state, StateOp_List* ops, Edit_SetLineKindOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   Deck_Deck* deck = app_state->deck;
   B32        on_board = (deck->slide_count != 0 && deck->slides[Deck_SlideFromTextPos(deck, editor->cursor)].is_canvas);
   String8    prefix = (params->kind == Edit_LineKind_Bullet)        ? Str8Lit("- ")
                       : (params->kind == Edit_LineKind_InnerBullet) ? Str8Lit("  - ")
                       : (params->kind == Edit_LineKind_Heading)     ? Str8Lit("## ")
                                                                     : Str8Lit("");
   U64 sel_min = Min(editor->cursor, editor->mark);
   U64 sel_max = Max(editor->cursor, editor->mark);
   U64 first_start = sel_min;
   {
      String8 text = Editor_Text(editor);
      while (first_start > 0 && text.str[first_start - 1] != '\n') first_start -= 1;
   }
   // A selection that ends at the start of a line is not in that line.
   U64 last_pos = (sel_max > sel_min && sel_max > first_start) ? sel_max - 1 : sel_max;
   U64 end = last_pos;
   {
      String8 text = Editor_Text(editor);
      while (end < text.size && text.str[end] != '\n') end += 1;
   }

   // The last line first: the ones before it stay where they are.
   U64 undo_before = editor->undo_count;
   U64 line_end = end;
   for (;;) {
      String8 text = Editor_Text(editor);
      U64     line_start = line_end;
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 first = line_start;
      while (first < line_end && (text.str[first] == ' ' || text.str[first] == '\t')) first += 1;
      U64 hashes = 0;
      while (first + hashes < line_end && text.str[first + hashes] == '#') hashes += 1;
      B32 is_title = (hashes == 1 && !on_board);
      B32 is_rule = (first + 2 < line_end && text.str[first] == '-' && text.str[first + 1] == '-' && text.str[first + 2] == '-');
      B32 is_other = (first == line_end || text.str[first] == '@' || text.str[first] == '!' || is_rule);
      if (!is_title && !is_other) {
         // What it starts with now: a bullet's "- ", or a heading's #s.
         U64 words = first;
         if (hashes != 0) {
            words = first + hashes;
         } else if (first + 1 < line_end && text.str[first] == '-' && text.str[first + 1] == ' ') {
            words = first + 1;
         }
         while (words < line_end && text.str[words] == ' ') words += 1;
         Editor_ReplaceRange(editor, line_start, words, prefix);
         end = end - (words - line_start) + prefix.size;
      }
      if (line_start <= first_start || line_start == 0) {
         break;
      }
      line_end = line_start - 1;
   }
   Editor_JoinUndoSince(editor, undo_before);
   editor->mark = Min(first_start, editor->size);
   editor->cursor = Min(end, editor->size);
   editor->last_input_us = NowTimeUS();
}
STATE_OP(Edit_SetLineKind)

// A click on a small slide: the cursor to that slide's first line.
typedef struct Edit_GoToSlideOpParams Edit_GoToSlideOpParams;
struct Edit_GoToSlideOpParams {
   U64 slide_idx;
};
void
Edit_GoToSlide(App_State* app_state, StateOp_List* ops, Edit_GoToSlideOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor != 0) {
      // Not with a text box a click made and left as it was, which would
      // move the slides after it.
      if (g_edit_view.has_new_box) {
         editor->cursor = editor->mark = g_edit_view.new_box_range.min;
         _Edit_TakeOutNewBox(app_state);
      }
      FR_SyncDeck(app_state);
      if (params->slide_idx < app_state->deck->slide_count) {
         Editor_SetCursor(editor, Min(app_state->deck->slides[params->slide_idx].text_pos, editor->size));
         editor->last_input_us = NowTimeUS();
      }
   }
}
STATE_OP(Edit_GoToSlide)

// An image or video file dropped on the window: on the slide the cursor is
// on, and into the deck's folder (_Edit_DropPicture). path: on ops->arena.
// (Its params are in edit.h: complete pushes it too.)
void
Edit_DropPicture(App_State* app_state, StateOp_List* ops, Edit_DropPictureOpParams* params) {
   UnusedVariable(ops);
   if (app_state->editor != 0) {
      FR_SyncDeck(app_state);
      Editor_Editor* editor = app_state->editor;
      U64            edit_count_before = editor->edit_count;
      U64            undo_before = editor->undo_count;
      _Edit_DropPicture(app_state, params->path);

      // Where it was asked for: an @place line above its own, which the
      // cursor is at the end of. Not when it took the place of a picture
      // that has one already.
      if (params->has_place && editor->edit_count != edit_count_before) {
         Temp    scratch = ScratchBegin(0, 0);
         String8 text = Editor_Text(editor);
         U64     line_start = Min(editor->cursor, text.size);
         while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
         U64 first = line_start;
         while (first < text.size && text.str[first] == ' ') first += 1;
         B32 is_picture = (first + 1 < text.size && text.str[first] == '!' && text.str[first + 1] == '[');
         U64 above = (line_start > 0) ? line_start - 1 : 0;
         while (above > 0 && text.str[above - 1] != '\n') above -= 1;
         B32 has_place_above = (line_start > 0 && Str8Match(Str8Prefix(Str8Substr(text, R1U64(above, line_start)), 6), Str8Lit("@place"),
                                                            StringMatchFlag_CaseInsensitive));
         if (is_picture && !has_place_above) {
            Rng2F32 place = params->place;
            // Under another picture's line an empty line first: an
            // @place right under a picture that has none is read as that
            // one's (deck.h).
            U64 above_first = above;
            while (above_first < line_start && text.str[above_first] == ' ') above_first += 1;
            B32 picture_above = (line_start > 0 && above_first + 1 < line_start && text.str[above_first] == '!' &&
                                 text.str[above_first + 1] == '[');
            String8 line = PushStr8F(scratch.arena, "%s@place picture %.3f %.3f %.3f %.3f\n", picture_above ? "\n" : "", place.min.x,
                                     place.min.y, place.max.x - place.min.x, place.max.y - place.min.y);
            _Edit_ReplaceKeepingCursor(editor, line_start, line_start, line);
            Editor_JoinUndoSince(editor, undo_before);
         }
         ScratchEnd(scratch);
      }
   }
}
STATE_OP(Edit_DropPicture)

// A click on a name in the list of pictures: on the slide the cursor is on,
// and the list closed. name: on ops->arena.
typedef struct Edit_PutPictureOpParams Edit_PutPictureOpParams;
struct Edit_PutPictureOpParams {
   String8 name;
};
void
Edit_PutPicture(App_State* app_state, StateOp_List* ops, Edit_PutPictureOpParams* params) {
   UnusedVariable(ops);
   if (app_state->editor != 0) {
      FR_SyncDeck(app_state);
      _Edit_PutPicture(app_state, params->name, 0);
      app_state->show_pictures = 0;
   }
}
STATE_OP(Edit_PutPicture)

// A click on a line of the Format panel: what the line does.
typedef struct Edit_FormatRowOpParams Edit_FormatRowOpParams;
struct Edit_FormatRowOpParams {
   U64 row_idx;
};
void
Edit_FormatRow(App_State* app_state, StateOp_List* ops, Edit_FormatRowOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || params->row_idx >= ArrayCount(_edit_format_rows)) {
      return;
   }
   _Edit_FormatRow* row = &_edit_format_rows[params->row_idx];
   if (row->action == _Edit_FormatAction_Pictures) {
      app_state->show_pictures = 1;
      MemoryZeroStruct(&g_edit_view.pictures_scroll); // the list starts at its top
      _Edit_ScanPictures(app_state);
   } else if (row->action == _Edit_FormatAction_Theme || row->action == _Edit_FormatAction_Layout) {
      // The deck as the text is now, to know what there is to change.
      FR_SyncDeck(app_state);
      if (row->action == _Edit_FormatAction_Theme) {
         _Edit_NextTheme(editor, app_state->deck);
      } else {
         _Edit_NextLayout(editor, app_state->deck, Deck_SlideFromTextPos(app_state->deck, editor->cursor));
      }
      editor->last_input_us = NowTimeUS();
   } else if (row->action == _Edit_FormatAction_Size || row->action == _Edit_FormatAction_Color ||
              row->action == _Edit_FormatAction_Align) {
      // The row's own click does nothing: what is in it is what is clicked.
   } else if (row->action == _Edit_FormatAction_Wrap && !_Edit_CanWrap(app_state)) {
      // Said by the notice.
   } else if (row->action == _Edit_FormatAction_Wrap) {
      B32 is_bold = Str8Match(row->insert, Str8Lit("**"), 0);
      Editor_WrapSelection(editor, row->insert, is_bold ? Str8Lit("bold") : Str8Lit("code"));
      Vim_StartTyping(app_state);
      editor->last_input_us = NowTimeUS();
   } else {
      Editor_InsertLine(editor, row->insert, row->select_start, row->select_end);
      // What is selected in it is to be typed over.
      Vim_StartTyping(app_state);
      editor->last_input_us = NowTimeUS();
   }
}
STATE_OP(Edit_FormatRow)

// The preview: a press on the slide at pos, as a click - on words, the
// cursor to the letter (caret\caret.c); on a picture, its file's name
// selected, to be typed over. From the slide as it was last laid out:
// nothing if the text has changed since.
void
Edit_PreviewClick(App_State* app_state, StateOp_List* ops, Edit_PreviewClickOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || editor->edit_count != g_edit_view.hits_edit_count) {
      return;
   }
   //- bgz: Ctrl: the piece added to those picked, or taken out of them. Shift
   // with pieces picked: every piece of the kind of the first. The cursor
   // stays where it is.
   B32 was_good = Edit_PickedIsGood(app_state);
   if (params->picks && (params->adds || params->picks_all)) {
      if (!was_good) {
         // Typing in a piece, and Ctrl+click on another: both are picked,
         // as in other editors.
         g_edit_view.picked_count = 0;
         U64            target_slide = 0;
         Deck_PlaceKind target_kind = Deck_PlaceKind_Title;
         U64            target_idx = 0;
         if (params->adds && !g_caret_view.focus_hidden && Look_Target(app_state, &target_slide, &target_kind, &target_idx) &&
             target_slide == app_state->slide_idx && !(target_kind == params->kind && target_idx == params->idx)) {
            g_edit_view.picked_kinds[0] = target_kind;
            g_edit_view.picked_idxs[0] = target_idx;
            g_edit_view.picked_count = 1;
         }
      }
      if (params->adds) {
         S64 at = _Edit_PickedIndexOf(params->kind, params->idx);
         if (at >= 0) {
            g_edit_view.picked_count -= 1;
            for (U64 i = (U64)at; i < g_edit_view.picked_count; i += 1) {
               g_edit_view.picked_kinds[i] = g_edit_view.picked_kinds[i + 1];
               g_edit_view.picked_idxs[i] = g_edit_view.picked_idxs[i + 1];
            }
         } else if (g_edit_view.picked_count < VIEW_MAX_BOXES) {
            g_edit_view.picked_kinds[g_edit_view.picked_count] = params->kind;
            g_edit_view.picked_idxs[g_edit_view.picked_count] = params->idx;
            g_edit_view.picked_count += 1;
         }
      } else {
         Deck_PlaceKind kind = (g_edit_view.picked_count != 0) ? g_edit_view.picked_kinds[0] : params->kind;
         Deck_Slide*    slide = &app_state->deck->slides[app_state->slide_idx];
         g_edit_view.picked_count = 0;
         for (U64 i = 0; i < g_edit_view.box_count && g_edit_view.picked_count < VIEW_MAX_BOXES; i += 1) {
            View_Box* box = &g_edit_view.boxes[i];
            if (box->kind == kind && !(kind == Deck_PlaceKind_Picture && box->idx >= slide->picture_count)) {
               g_edit_view.picked_kinds[g_edit_view.picked_count] = box->kind;
               g_edit_view.picked_idxs[g_edit_view.picked_count] = box->idx;
               g_edit_view.picked_count += 1;
            }
         }
      }
      g_caret_view.focus_hidden = 0;
      _Edit_PickedSnapshot(app_state);
      return;
   }

   //- bgz: a plain press on one of several picked: they stay picked, to be
   // dragged together (place\place.c). Let go of with no drag, it alone is
   // (picks_only).
   if (params->picks && !params->picks_only && was_good && g_edit_view.picked_count > 1 && _Edit_PickedIndexOf(params->kind, params->idx) >= 0) {
      return;
   }

   g_edit_view.picked_count = 0;
   if (!Caret_Click(app_state, params->pos, params->keeps_mark)) {
      S64 hit_idx = Edit_PreviewHitAt(params->pos);
      if (hit_idx >= 0 && g_edit_view.hits[hit_idx].text_range.max <= editor->size) {
         View_Hit* hit = &g_edit_view.hits[hit_idx];
         Editor_SetCursor(editor, hit->text_range.max);
         editor->mark = hit->text_range.min;
         g_caret_view.focus_hidden = 0;
         // The picture's name, to be typed over.
         Vim_StartTyping(app_state);
         editor->last_input_us = NowTimeUS();
      }
   }

   // A text box a click made and left as it was goes, now that the cursor
   // is somewhere else - which it stays in.
   _Edit_TakeOutNewBox(app_state);

   if (params->picks && !params->keeps_mark) {
      g_edit_view.picked_kinds[0] = params->kind;
      g_edit_view.picked_idxs[0] = params->idx;
      g_edit_view.picked_count = 1;
      _Edit_PickedSnapshot(app_state);
   }
}
STATE_OP(Edit_PreviewClick)

// A box drawn on the preview's slide where nothing is: a text box there
// (_Edit_AddTextBox), at rect, in parts of the page. From the slide as it
// was last laid out: nothing if the text has changed since.
typedef struct Edit_AddTextBoxOpParams Edit_AddTextBoxOpParams;
struct Edit_AddTextBoxOpParams {
   Rng2F32 rect;
};
void
Edit_AddTextBox(App_State* app_state, StateOp_List* ops, Edit_AddTextBoxOpParams* params) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || editor->edit_count != g_edit_view.hits_edit_count) {
      return;
   }
   g_edit_view.picked_count = 0;
   U64 slide_idx = app_state->slide_idx;
   // One made before and left as it was: this one is in its place.
   if (g_edit_view.has_new_box) {
      editor->cursor = editor->mark = g_edit_view.new_box_range.min;
      _Edit_TakeOutNewBox(app_state);
   }
   FR_SyncDeck(app_state);
   _Edit_AddTextBox(app_state, ops, slide_idx, params->rect);
}
STATE_OP(Edit_AddTextBox)

// Delete or Backspace with pieces picked whole: all of them taken out
// (_Edit_DeletePieces).
void
Edit_DeletePiece(App_State* app_state, StateOp_List* ops) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || !Edit_PickedIsGood(app_state)) {
      return;
   }
   U64 count = g_edit_view.picked_count;
   g_edit_view.picked_count = 0;
   FR_SyncDeck(app_state);
   if (_Edit_DeletePieces(app_state, g_edit_view.picked_slide_idx, g_edit_view.picked_kinds, g_edit_view.picked_idxs, count)) {
      String8 what = (count == 1) ? _Edit_PieceName(g_edit_view.picked_kinds[0])
                                  : PushStr8F(ops->arena, "%llu pieces", (unsigned long long)count);
      StateOp_App_Notice(ops)->notice = PushStr8F(ops->arena, "%S taken out. Ctrl+Z brings %s back", what, (count == 1) ? "it" : "them");
   }
}
STATE_OP_NO_PARAMS(Edit_DeletePiece)

// Ctrl+C, or Ctrl+X with is_cut, with pieces picked whole: copied, as
// pieces (g_edit_view.clip) and as their lines on the system's clipboard -
// and with is_cut taken out.
typedef struct Edit_CopyPiecesOpParams Edit_CopyPiecesOpParams;
struct Edit_CopyPiecesOpParams {
   B32 is_cut;
};
void
Edit_CopyPieces(App_State* app_state, StateOp_List* ops, Edit_CopyPiecesOpParams* params) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || !Edit_PickedIsGood(app_state)) {
      return;
   }
   FR_SyncDeck(app_state);
   if (g_edit_view.clip_arena == 0) {
      g_edit_view.clip_arena = ArenaAlloc();
   }
   ArenaClear(g_edit_view.clip_arena);
   g_edit_view.clip_count = _Edit_ClipFromPicked(g_edit_view.clip_arena, app_state, g_edit_view.clip);
   g_edit_view.clip_slide_idx = app_state->slide_idx;
   g_edit_view.clip_paste_count = 0;
   if (g_edit_view.clip_count == 0) {
      return;
   }

   // Their lines, as text: pasted anywhere else, they are what was copied.
   String8List lines = {0};
   for (U64 i = 0; i < g_edit_view.clip_count; i += 1) {
      Str8ListPush(g_edit_view.clip_arena, &lines, g_edit_view.clip[i].lines);
   }
   g_edit_view.clip_text = Str8ListJoin(g_edit_view.clip_arena, &lines, 0);
   Core_WM_SetClipboardText(g_edit_view.clip_text);

   U64 count = g_edit_view.clip_count;
   if (params->is_cut) {
      U64 picked = g_edit_view.picked_count;
      g_edit_view.picked_count = 0;
      _Edit_DeletePieces(app_state, app_state->slide_idx, g_edit_view.picked_kinds, g_edit_view.picked_idxs, picked);
      // Pasted back on this slide, it goes where it was.
      g_edit_view.clip_paste_count = (U64)-1;
   }
   StateOp_App_Notice(ops)->notice = PushStr8F(ops->arena, "%s %llu piece%s. Ctrl+V pastes %s", params->is_cut ? "Cut" : "Copied",
                                               (unsigned long long)count, (count == 1) ? "" : "s", (count == 1) ? "it" : "them");
}
STATE_OP(Edit_CopyPieces)

// Ctrl+V, with the clipboard as copying the pieces left it: they go on the
// slide showing, and are picked.
void
Edit_PastePieces(App_State* app_state, StateOp_List* ops) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || g_edit_view.clip_count == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   U64 slide_idx = app_state->slide_idx;
   F32 offset = 0;
   if (slide_idx == g_edit_view.clip_slide_idx) {
      g_edit_view.clip_paste_count += 1; // from -1 after a cut: 0, where it was
      offset = _EDIT_PASTE_STEP * (F32)g_edit_view.clip_paste_count;
   }
   _Edit_PastePieces(app_state, ops, slide_idx, g_edit_view.clip, g_edit_view.clip_count, offset, 0);
}
STATE_OP_NO_PARAMS(Edit_PastePieces)

// Ctrl+D with pieces picked whole: a copy of them a little down and right,
// picked in their place. The clipboard is left as it is.
void
Edit_DuplicatePieces(App_State* app_state, StateOp_List* ops) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || !Edit_PickedIsGood(app_state)) {
      return;
   }
   FR_SyncDeck(app_state);
   Temp            scratch = ScratchBegin(0, 0);
   Edit_ClipPiece* pieces = PushArray(scratch.arena, Edit_ClipPiece, VIEW_MAX_BOXES);
   U64             count = _Edit_ClipFromPicked(scratch.arena, app_state, pieces);
   _Edit_PastePieces(app_state, ops, app_state->slide_idx, pieces, count, _EDIT_PASTE_STEP, 0);
   ScratchEnd(scratch);
}
STATE_OP_NO_PARAMS(Edit_DuplicatePieces)

// Ctrl+Shift+] and Ctrl+Shift+[ with pieces picked whole: in front of the
// slide's other pieces of their kind, or with to_back behind them.
//
// What is over what is the order of the lines (view.c): the slide's
// pictures are drawn first, in the order of their lines, then the title,
// then the blocks in theirs. So a piece is brought to the front by moving
// its lines to the end of the slide's, and sent to the back by moving them
// to its start (_Edit_PastePieces) - among the pictures, or among the
// blocks: a picture is never over words, and the title's place in it is
// its own. A block with no @place line gets one, where it is.
typedef struct Edit_OrderPiecesOpParams Edit_OrderPiecesOpParams;
struct Edit_OrderPiecesOpParams {
   B32 to_back;
};
void
Edit_OrderPieces(App_State* app_state, StateOp_List* ops, Edit_OrderPiecesOpParams* params) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || !Edit_PickedIsGood(app_state) || !Place_BoxesAreGood(app_state)) {
      return;
   }
   FR_SyncDeck(app_state);

   // Not the title. And in the order they are in now, which they keep
   // among themselves.
   U64 count = 0;
   for (U64 i = 0; i < g_edit_view.picked_count; i += 1) {
      if (g_edit_view.picked_kinds[i] != Deck_PlaceKind_Title) {
         g_edit_view.picked_kinds[count] = g_edit_view.picked_kinds[i];
         g_edit_view.picked_idxs[count] = g_edit_view.picked_idxs[i];
         count += 1;
      }
   }
   g_edit_view.picked_count = count;
   if (count == 0) {
      StateOp_App_Notice(ops)->notice = Str8Lit("The title is always over the pictures and under the text boxes");
      return;
   }
   for (U64 i = 1; i < count; i += 1) {
      for (U64 k = i; k > 0; k -= 1) {
         B32 is_after = (g_edit_view.picked_kinds[k - 1] > g_edit_view.picked_kinds[k] ||
                         (g_edit_view.picked_kinds[k - 1] == g_edit_view.picked_kinds[k] &&
                          g_edit_view.picked_idxs[k - 1] > g_edit_view.picked_idxs[k]));
         if (!is_after) {
            break;
         }
         Deck_PlaceKind kind = g_edit_view.picked_kinds[k - 1];
         U64            idx = g_edit_view.picked_idxs[k - 1];
         g_edit_view.picked_kinds[k - 1] = g_edit_view.picked_kinds[k];
         g_edit_view.picked_idxs[k - 1] = g_edit_view.picked_idxs[k];
         g_edit_view.picked_kinds[k] = kind;
         g_edit_view.picked_idxs[k] = idx;
      }
   }

   Temp            scratch = ScratchBegin(0, 0);
   Edit_ClipPiece* pieces = PushArray(scratch.arena, Edit_ClipPiece, VIEW_MAX_BOXES);
   U64             clip_count = _Edit_ClipFromPicked(scratch.arena, app_state, pieces);
   if (clip_count == count) {
      Deck_PlaceKind kinds[VIEW_MAX_BOXES];
      U64            idxs[VIEW_MAX_BOXES];
      MemoryCopy(kinds, g_edit_view.picked_kinds, sizeof(kinds));
      MemoryCopy(idxs, g_edit_view.picked_idxs, sizeof(idxs));
      U64 slide_idx = app_state->slide_idx;
      U64 undo_before = editor->undo_count;
      g_edit_view.picked_count = 0;
      if (_Edit_DeletePieces(app_state, slide_idx, kinds, idxs, count)) {
         FR_SyncDeck(app_state);
         _Edit_PastePieces(app_state, ops, slide_idx, pieces, clip_count, 0, params->to_back);
         Editor_JoinUndoSince(editor, undo_before);
         StateOp_App_Notice(ops)->notice = params->to_back ? Str8Lit("Sent to the back") : Str8Lit("Brought to the front");
      }
   }
   ScratchEnd(scratch);
}
STATE_OP(Edit_OrderPieces)

// Ctrl+Shift+N with pieces picked whole: they are taken off the slide - a
// board, most often - and written as a slide of their own at the deck's
// end, so slides made this way are in the order they were made. The first
// block's first line is the new slide's title, if it is a heading or plain
// words; its other lines are the slide's body, and each other block is an
// @box of it. They keep their sizes and colours, and not their places:
// the new slide's layout puts them. One step to undo.
void
Edit_LiftPieces(App_State* app_state, StateOp_List* ops) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || !Edit_PickedIsGood(app_state) || !Place_BoxesAreGood(app_state)) {
      return;
   }
   FR_SyncDeck(app_state);
   Temp            scratch = ScratchBegin(0, 0);
   Edit_ClipPiece* pieces = PushArray(scratch.arena, Edit_ClipPiece, VIEW_MAX_BOXES);
   U64             count = _Edit_ClipFromPicked(scratch.arena, app_state, pieces);

   // The new slide's lines: the blocks, then the pictures.
   String8List added = {0};
   Str8ListPush(scratch.arena, &added, Str8Lit("@slide\n"));
   B32 has_block = 0;
   for (U64 i = 0; i < count; i += 1) {
      Edit_ClipPiece* piece = &pieces[i];
      if (piece->is_picture) {
         continue;
      }
      String8 lines = piece->lines;
      if (has_block) {
         Str8ListPush(scratch.arena, &added, Str8Lit("@box\n"));
      } else {
         // Its first line as the title: a heading without its #s, or plain
         // words. Not a bullet, nor a line that is about something.
         U64 first_end = 0;
         while (first_end < lines.size && lines.str[first_end] != '\n') first_end += 1;
         String8 first = Str8Prefix(lines, first_end);
         U64     at = 0;
         while (at < first.size && (first.str[at] == ' ' || first.str[at] == '\t')) at += 1;
         U64 hash_count = 0;
         while (at + hash_count < first.size && first.str[at + hash_count] == '#') hash_count += 1;
         U8  c0 = (at < first.size) ? first.str[at] : 0;
         B32 is_plain = (c0 != 0 && c0 != '-' && c0 != '*' && c0 != '!' && c0 != '@' && c0 != '#');
         if (hash_count != 0 || is_plain) {
            String8 words = Str8Skip(first, at + hash_count);
            while (words.size != 0 && (words.str[0] == ' ' || words.str[0] == '\t')) words = Str8Skip(words, 1);
            Str8ListPush(scratch.arena, &added, PushStr8F(scratch.arena, "# %S\n", words));
            lines = Str8Skip(lines, Min(first_end + 1, lines.size));
         }
      }
      has_block = 1;
      Str8ListPush(scratch.arena, &added, piece->looks);
      Str8ListPush(scratch.arena, &added, lines);
   }
   for (U64 i = 0; i < count; i += 1) {
      if (pieces[i].is_picture) {
         Str8ListPush(scratch.arena, &added, pieces[i].lines);
      }
   }
   String8 body = Str8ListJoin(scratch.arena, &added, 0);

   Deck_PlaceKind kinds[VIEW_MAX_BOXES];
   U64            idxs[VIEW_MAX_BOXES];
   MemoryCopy(kinds, g_edit_view.picked_kinds, sizeof(kinds));
   MemoryCopy(idxs, g_edit_view.picked_idxs, sizeof(idxs));
   U64 picked = g_edit_view.picked_count;
   U64 undo_before = editor->undo_count;
   g_edit_view.picked_count = 0;
   if (count != 0 && count == picked && _Edit_DeletePieces(app_state, app_state->slide_idx, kinds, idxs, picked)) {
      // Under the text's last line with anything on it, an empty line
      // between. The cursor stays where taking the pieces out left it: on
      // the board, for the next.
      String8 text = Editor_Text(editor);
      U64     end = text.size;
      while (end > 0 && (text.str[end - 1] == '\n' || text.str[end - 1] == ' ' || text.str[end - 1] == '\t')) {
         end -= 1;
      }
      String8 joined = PushStr8F(scratch.arena, "%s%S", (end != 0) ? "\n\n" : "", body);
      _Edit_ReplaceKeepingCursor(editor, end, text.size, joined);
      Editor_JoinUndoSince(editor, undo_before);
      FR_SyncDeck(app_state);
      StateOp_App_Notice(ops)->notice = PushStr8F(ops->arena, "Moved to a new slide, the deck's last: slide %llu",
                                                  (unsigned long long)app_state->deck->slide_count);
   }
   ScratchEnd(scratch);
}
STATE_OP_NO_PARAMS(Edit_LiftPieces)

// A drag on the slide's open space: every piece the box drawn touches is
// picked whole - in place of those picked already, or with adds (Shift)
// with them. rect: in pixels, of the slide as it was last laid out -
// nothing if the text has changed since.
typedef struct Edit_PickRectOpParams Edit_PickRectOpParams;
struct Edit_PickRectOpParams {
   Rng2F32 rect;
   B32     adds;
};
void
Edit_PickRect(App_State* app_state, StateOp_List* ops, Edit_PickRectOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || editor->edit_count != g_edit_view.hits_edit_count || app_state->slide_idx >= app_state->deck->slide_count) {
      return;
   }
   if (!Edit_PickedIsGood(app_state) || !params->adds) {
      g_edit_view.picked_count = 0;
   }
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   Rng2F32     rect = params->rect;
   for (U64 i = 0; i < g_edit_view.box_count && g_edit_view.picked_count < VIEW_MAX_BOXES; i += 1) {
      View_Box* box = &g_edit_view.boxes[i];
      B32 touches = (box->rect.min.x < rect.max.x && box->rect.max.x > rect.min.x && box->rect.min.y < rect.max.y &&
                     box->rect.max.y > rect.min.y);
      if (!touches || (box->kind == Deck_PlaceKind_Picture && box->idx >= slide->picture_count) ||
          _Edit_PickedIndexOf(box->kind, box->idx) >= 0) {
         continue;
      }
      g_edit_view.picked_kinds[g_edit_view.picked_count] = box->kind;
      g_edit_view.picked_idxs[g_edit_view.picked_count] = box->idx;
      g_edit_view.picked_count += 1;
   }
   g_caret_view.focus_hidden = 0;
   _Edit_PickedSnapshot(app_state);
}
STATE_OP(Edit_PickRect)

////////////////////////////////
//~ bgz: the board
//
// An @canvas slide has no edge: its page is only where things are measured
// from, and the preview is a window over it that is moved about. The wheel
// moves it up and down, and sideways with Shift; Ctrl and the wheel make
// it larger and smaller, about where the mouse is; the middle button held
// drags it. Where it is is the screen's own (g_edit_view.board_at,
// board_zoom), not in the text and not app state: written here, with no op.

#define _EDIT_BOARD_ZOOM_MIN  0.15f
#define _EDIT_BOARD_ZOOM_MAX  4.0f
#define _EDIT_BOARD_ZOOM_STEP 1.15f  // a notch of the wheel
#define _EDIT_BOARD_WHEEL     0.12f  // how far a notch moves it, in parts of the page as shown

// Where the board was is kept in the program's own folder, in board.txt,
// for the deck worked on last: three whole numbers - across, down and zoom,
// each times _EDIT_BOARD_FILE_UNIT - and under them the deck's file.
#define _EDIT_BOARD_FILE_UNIT 10000.0f

PRIVATE String8
_Edit_BoardFilePath(Arena* arena) {
   Temp    scratch = ScratchBegin(&arena, 1);
   String8 result = PushStr8F(arena, "%S\\board.txt", FR_LocalFolder(scratch.arena));
   ScratchEnd(scratch);
   return result;
}

// Not being able to write it loses only where the board was.
PRIVATE void
_Edit_BoardSave(void) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 data = PushStr8F(scratch.arena, "%d %d %d\n%S\n", (int)(g_edit_view.board_at.x * _EDIT_BOARD_FILE_UNIT),
                            (int)(g_edit_view.board_at.y * _EDIT_BOARD_FILE_UNIT), (int)(g_edit_view.board_zoom * _EDIT_BOARD_FILE_UNIT),
                            Str8(g_edit_view.board_file, g_edit_view.board_file_size));
   WriteDataToFilePath(_Edit_BoardFilePath(scratch.arena), data);
   g_edit_view.board_saved_at = g_edit_view.board_at;
   g_edit_view.board_saved_zoom = g_edit_view.board_zoom;
   ScratchEnd(scratch);
}

// The board as it was left, if what is written is of this deck's file: else
// not moved.
PRIVATE void
_Edit_BoardLoad(void) {
   g_edit_view.board_zoom = 0;
   g_edit_view.board_at = V2F32(0.5f, 0.5f);
   Temp    scratch = ScratchBegin(0, 0);
   String8 data = DataFromFilePath(scratch.arena, _Edit_BoardFilePath(scratch.arena));
   S64     numbers[3] = {0};
   U64     pos = 0;
   B32     ok = 1;
   for (U64 i = 0; i < ArrayCount(numbers) && ok; i += 1) {
      while (pos < data.size && data.str[pos] == ' ') pos += 1;
      B32 is_negative = (pos < data.size && data.str[pos] == '-');
      pos += is_negative ? 1 : 0;
      U64 digit_count = 0;
      for (; pos < data.size && data.str[pos] >= '0' && data.str[pos] <= '9' && digit_count < 9; pos += 1, digit_count += 1) {
         numbers[i] = numbers[i] * 10 + (S64)(data.str[pos] - '0');
      }
      numbers[i] = is_negative ? -numbers[i] : numbers[i];
      ok = (digit_count != 0);
   }
   while (pos < data.size && data.str[pos] != '\n') pos += 1;
   String8 file = Str8SkipChopWhitespace(Str8Skip(data, Min(pos, data.size)));
   if (ok && Str8Match(file, Str8(g_edit_view.board_file, g_edit_view.board_file_size), StringMatchFlag_CaseInsensitive)) {
      F32 zoom = (F32)numbers[2] / _EDIT_BOARD_FILE_UNIT;
      if (zoom >= _EDIT_BOARD_ZOOM_MIN && zoom <= _EDIT_BOARD_ZOOM_MAX) {
         g_edit_view.board_zoom = zoom;
         g_edit_view.board_at = V2F32(Clamp(-DECK_CANVAS_REACH, (F32)numbers[0] / _EDIT_BOARD_FILE_UNIT, DECK_CANVAS_REACH),
                                      Clamp(-DECK_CANVAS_REACH, (F32)numbers[1] / _EDIT_BOARD_FILE_UNIT, DECK_CANVAS_REACH));
      }
   }
   g_edit_view.board_saved_at = g_edit_view.board_at;
   g_edit_view.board_saved_zoom = g_edit_view.board_zoom;
   ScratchEnd(scratch);
}

// The cursor has gone somewhere, or the text has changed, and where the
// cursor is on the board does not show: the board is moved until it does.
// Only for what is done in the text - the mouse not over the board - or
// typed: a click or a drag on the board leaves it where it is. 1 if it
// moved, and the slide is to be laid out again. After the slide is laid out.
PRIVATE B32
_Edit_BoardFollow(const App_State* app_state, F32 ui) {
   Editor_Editor* editor = app_state->editor;
   Vec2F32        base_dim = Dim2F32(g_edit_view.board_base);
   if (editor == 0 || base_dim.x <= 0 || base_dim.y <= 0) {
      return 0;
   }
   B32 has_moved = (editor->cursor != g_edit_view.board_seen_cursor || editor->edit_count != g_edit_view.board_seen_edit_count ||
                    app_state->slide_idx != g_edit_view.board_seen_slide_idx);
   g_edit_view.board_seen_cursor = editor->cursor;
   g_edit_view.board_seen_edit_count = editor->edit_count;
   g_edit_view.board_seen_slide_idx = app_state->slide_idx;

   Rng2F32 board = g_edit_view.board;
   Vec2F32 mouse = Core_WM_MouseFromWindow(app_state->window);
   B32     is_typing = (app_state->vim.mode == Vim_Mode_Insert && g_place_view.drag.state == Place_DragState_None);
   Rng2F32 rect = {0};
   if (!has_moved || g_edit_view.board_panning || Core_WM_KeyIsDown(WM_Key_LeftMouseButton) ||
       (Contains2F32(board, mouse) && !is_typing) || !Caret_RectOfCursor(app_state, &rect)) {
      return 0;
   }

   // With some room around it, where the board is large enough for that.
   Vec2F32 board_dim = Dim2F32(board);
   F32     pad_x = Min(40.0f * ui, board_dim.x * 0.25f);
   F32     pad_y = Min(40.0f * ui, board_dim.y * 0.25f);
   F32     dx = 0;
   F32     dy = 0;
   if (rect.min.x < board.min.x + pad_x) {
      dx = rect.min.x - (board.min.x + pad_x);
   } else if (rect.max.x > board.max.x - pad_x) {
      dx = Min(rect.max.x - (board.max.x - pad_x), rect.min.x - (board.min.x + pad_x));
   }
   if (rect.min.y < board.min.y + pad_y) {
      dy = rect.min.y - (board.min.y + pad_y);
   } else if (rect.max.y > board.max.y - pad_y) {
      dy = Min(rect.max.y - (board.max.y - pad_y), rect.min.y - (board.min.y + pad_y));
   }
   if (dx == 0 && dy == 0) {
      return 0;
   }
   if (g_edit_view.board_zoom <= 0) {
      g_edit_view.board_zoom = 1.0f;
      g_edit_view.board_at = V2F32(0.5f, 0.5f);
   }
   g_edit_view.board_at.x += dx / (base_dim.x * g_edit_view.board_zoom);
   g_edit_view.board_at.y += dy / (base_dim.y * g_edit_view.board_zoom);
   return 1;
}

PRIVATE void
_Edit_BoardInput(const App_State* app_state) {
   // Another deck: its board where it was left, if it is the one written.
   String8 file = (app_state->editor != 0) ? app_state->editor->file : Str8Lit("");
   if (!Str8Match(file, Str8(g_edit_view.board_file, g_edit_view.board_file_size), 0)) {
      g_edit_view.board_file_size = Min(file.size, sizeof(g_edit_view.board_file));
      MemoryCopy(g_edit_view.board_file, file.str, g_edit_view.board_file_size);
      _Edit_BoardLoad();
   }

   Vec2F32 base_dim = Dim2F32(g_edit_view.board_base);
   if (!g_edit_view.is_board || base_dim.x <= 0 || base_dim.y <= 0) {
      g_edit_view.board_panning = 0;
      return;
   }
   if (g_edit_view.board_zoom <= 0) {
      g_edit_view.board_zoom = 1.0f;
      g_edit_view.board_at = V2F32(0.5f, 0.5f);
      g_edit_view.board_saved_at = g_edit_view.board_at;
      g_edit_view.board_saved_zoom = g_edit_view.board_zoom;
   }
   Vec2F32 at_before = g_edit_view.board_at;
   F32     zoom_before = g_edit_view.board_zoom;
   Vec2F32 mouse = Core_WM_MouseFromWindow(app_state->window);
   Vec2F32 middle = V2F32(g_edit_view.board_base.min.x + base_dim.x * 0.5f, g_edit_view.board_base.min.y + base_dim.y * 0.5f);
   B32     is_over = Contains2F32(g_edit_view.board, mouse) && !g_edit_view.menu_open;
   F32     zoom = g_edit_view.board_zoom;
   Vec2F32 at = g_edit_view.board_at;

   for (WM_Event* event = app_state->events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_Press && event->key == WM_Key_MiddleMouseButton && is_over) {
         g_edit_view.board_panning = 1;
         g_edit_view.board_pan_mouse = mouse;
      }
      if (event->kind != WM_EventKind_Scroll || event->delta.y == 0 || !is_over) {
         continue;
      }
      // A notch is 120, down is over 0.
      F32 notches = Clamp(-4.0f, event->delta.y / 120.0f, 4.0f);
      if (event->modifiers == WM_Modifier_Ctrl) {
         // The point of the page under the mouse stays under it.
         F32 zoom_new = Clamp(_EDIT_BOARD_ZOOM_MIN, zoom * PowF32(_EDIT_BOARD_ZOOM_STEP, -notches), _EDIT_BOARD_ZOOM_MAX);
         Vec2F32 under = V2F32(at.x + (mouse.x - middle.x) / (base_dim.x * zoom), at.y + (mouse.y - middle.y) / (base_dim.y * zoom));
         at = V2F32(under.x - (mouse.x - middle.x) / (base_dim.x * zoom_new), under.y - (mouse.y - middle.y) / (base_dim.y * zoom_new));
         zoom = zoom_new;
      } else if (event->modifiers == WM_Modifier_Shift) {
         at.x += notches * _EDIT_BOARD_WHEEL / zoom;
      } else if (event->modifiers == 0) {
         at.y += notches * _EDIT_BOARD_WHEEL / zoom;
      }
   }

   if (g_edit_view.board_panning) {
      if (!Core_WM_KeyIsDown(WM_Key_MiddleMouseButton)) {
         g_edit_view.board_panning = 0;
      } else {
         // The board goes with the mouse.
         at.x -= (mouse.x - g_edit_view.board_pan_mouse.x) / (base_dim.x * zoom);
         at.y -= (mouse.y - g_edit_view.board_pan_mouse.y) / (base_dim.y * zoom);
         g_edit_view.board_pan_mouse = mouse;
      }
   }

   g_edit_view.board_zoom = zoom;
   g_edit_view.board_at = V2F32(Clamp(-DECK_CANVAS_REACH, at.x, DECK_CANVAS_REACH), Clamp(-DECK_CANVAS_REACH, at.y, DECK_CANVAS_REACH));

   // Written once it has come to rest somewhere new, not at every notch.
   B32 is_at_rest = (!g_edit_view.board_panning && g_edit_view.board_zoom == zoom_before && g_edit_view.board_at.x == at_before.x &&
                     g_edit_view.board_at.y == at_before.y);
   B32 is_new = (g_edit_view.board_zoom != g_edit_view.board_saved_zoom || g_edit_view.board_at.x != g_edit_view.board_saved_at.x ||
                 g_edit_view.board_at.y != g_edit_view.board_saved_at.y);
   if (is_at_rest && is_new && g_edit_view.board_file_size != 0) {
      _Edit_BoardSave();
   }
}

////////////////////////////////
//~ bgz: input
//
// The keys. Nothing is changed here: each key pushes its op, in the order
// the keys are taken - the screen's own first, then the editor's, by an op
// that reads what is left of the frame's events.

void
Edit_Input(const App_State* app_state, StateOp_List* ops, Vec2F32 window_dim) {
   WM_EventList* events = app_state->events;
   WM_Window     window = app_state->window;

   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         StateOp_App_RequestQuit(ops);
      }
      // Of several things dropped at once, the first is used. An image or a
      // video is a picture for the slide. Anything else is a deck to open.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         String8 path = PushStr8Copy(ops->arena, event->strings.first->string);
         B32     is_folder = !!(PropertiesFromFilePath(path).flags & FilePropertyFlag_IsFolder);
         if (!is_folder && (Image_NameIsImage(path) || Video_NameIsVideo(path))) {
            StateOp_Edit_DropPicture(ops)->path = path;
         } else {
            // The rest of this frame's input was for the deck that is going.
            StateOp_App_OpenPath(ops)->path = path;
            return;
         }
      }
   }

   Editor_Editor* editor = app_state->editor;
   _Edit_Layout   layout = _Edit_LayoutFromWindowDim(app_state, window_dim);

   //- bgz: the right-click menu: Esc closes it, before Esc does anything else
   if (g_edit_view.menu_open && Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
      g_edit_view.menu_open = 0;
   }

   //- bgz: a piece of the slide dragged by the mouse (place\place.c): Esc
   // gives it up, before Esc does anything else
   Place_Keys(app_state);

   //- bgz: the list under an @ line being typed (complete\complete.c): its
   // keys - Up, Down, Tab, Return, Esc - while it shows, before they are
   // anything else's
   Complete_Input(app_state, ops);

   //- bgz: the screen's own keys, taken before the editor sees them
   B32 is_fullscreen = Core_WM_WindowIsFullscreen(window);
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F11) ||
       Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Return)) {
      StateOp_App_ToggleFullscreen(ops);
   }
   // Esc closes what is open, one thing a press: the list of pictures, then
   // vim's insert or visual mode, back to command mode (vim\vim.c), then
   // what is selected - the words, and the box in the preview around the
   // piece the cursor is in - then fullscreen.
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
      B32 has_selection = (editor->mark != editor->cursor);
      if (app_state->show_pictures) {
         StateOp_Edit_ClosePictures(ops);
      } else if (Vim_WantsEsc(app_state)) {
         StateOp_Vim_Escape(ops);
      } else if (has_selection || !g_caret_view.focus_hidden) {
         StateOp_Edit_Deselect(ops);
      } else if (is_fullscreen) {
         StateOp_App_ToggleFullscreen(ops);
      }
   }

   // As a click on the Format panel's header does (Edit_Build).
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F1)) {
      StateOp_Edit_ToggleFormat(ops);
   }

   // The left side put away, and brought back.
   if (Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_LeftBracket)) {
      StateOp_Edit_ToggleText(ops);
   }

   // Bold and code, around what is selected (the Format panel's two lines
   // for them do the same).
   // Ctrl+B while typing only: in vim's other modes it is words back.
   if (app_state->vim.mode == Vim_Mode_Insert && Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_B)) {
      StateOp_Edit_Wrap(ops)->is_code = 0;
   }
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_E)) {
      StateOp_Edit_Wrap(ops)->is_code = 1;
   }

   // The words the cursor is in, larger and smaller (look\look.c) - and
   // with Ctrl and the wheel, those the mouse is over, or those picked.
   Look_Input(app_state, ops);
   Place_CtrlWheel(app_state);
   _Edit_BoardInput(app_state);

   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_S)) {
      StateOp_Edit_Save(ops);
   }

   B32 present_pressed = Core_WM_KeyPress(events, window, 0, WM_Key_F5);

   //- bgz: the list of pictures, kept up with the folder while it is open
   //
   // Its clicks and wheel, and the Format lines', are taken where they are
   // built (Edit_Build).
   if (app_state->show_format && app_state->show_pictures) {
      U64 scan_us = (U64)(_EDIT_PICTURES_SCAN_SECONDS * 1000000.0f);
      if (NowTimeUS() - g_edit_view.pictures_scanned_us >= scan_us) {
         _Edit_ScanPictures(app_state);
      }
   }

   //- bgz: slides moved (the + that adds one is a UI box: Edit_Build)
   {
      B32 move_earlier = Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Left);
      B32 move_later = Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Right);
      if (move_earlier || move_later) {
         StateOp_Edit_MoveSlide(ops)->to_later = move_later;
      }
   }

   //- bgz: a piece of the slide picked whole - a picture clicked, or a box
   // clicked away from its words: Delete or Backspace takes it out, instead
   // of a letter
   if (Edit_PickedIsGood(app_state) && g_place_view.drag.state != Place_DragState_Dragging) {
      B32 is_delete = Core_WM_KeyPress(events, window, 0, WM_Key_Delete);
      B32 is_backspace = Core_WM_KeyPress(events, window, 0, WM_Key_Backspace);
      if (is_delete || is_backspace) {
         StateOp_Edit_DeletePiece(ops);
      }
      // Copied, cut and duplicated as pieces, not as the editor's text.
      if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_C)) {
         StateOp_Edit_CopyPieces(ops)->is_cut = 0;
      }
      if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_X)) {
         StateOp_Edit_CopyPieces(ops)->is_cut = 1;
      }
      if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_D)) {
         StateOp_Edit_DuplicatePieces(ops);
      }
      // In front of the others, and behind them.
      WM_Modifiers ctrl_shift = WM_Modifier_Ctrl | WM_Modifier_Shift;
      if (Core_WM_KeyPress(events, window, ctrl_shift, WM_Key_RightBracket)) {
         StateOp_Edit_OrderPieces(ops)->to_back = 0;
      }
      if (Core_WM_KeyPress(events, window, ctrl_shift, WM_Key_LeftBracket)) {
         StateOp_Edit_OrderPieces(ops)->to_back = 1;
      }
      // To a slide of their own, at the deck's end.
      if (Core_WM_KeyPress(events, window, ctrl_shift, WM_Key_N)) {
         StateOp_Edit_LiftPieces(ops);
      }
      // Lined up, and spaced evenly (place\place.c).
      struct {
         WM_Key            key;
         Place_ArrangeKind kind;
      } arranges[] = {
          {WM_Key_Left, Place_ArrangeKind_Left},  {WM_Key_Right, Place_ArrangeKind_Right}, {WM_Key_Up, Place_ArrangeKind_Top},
          {WM_Key_Down, Place_ArrangeKind_Bottom}, {WM_Key_C, Place_ArrangeKind_CenterX},   {WM_Key_M, Place_ArrangeKind_CenterY},
          {WM_Key_H, Place_ArrangeKind_SpaceX},    {WM_Key_V, Place_ArrangeKind_SpaceY},
      };
      for (U64 i = 0; i < ArrayCount(arranges); i += 1) {
         if (Core_WM_KeyPress(events, window, ctrl_shift, arranges[i].key)) {
            StateOp_Place_Arrange(ops)->kind = arranges[i].kind;
         }
      }
      // The arrows move them, not the cursor: a small step, a larger one
      // with Shift (place\place.c). In page units, the same across and down.
      WM_Key arrows[4] = {WM_Key_Left, WM_Key_Right, WM_Key_Up, WM_Key_Down};
      for (U64 i = 0; i < ArrayCount(arrows); i += 1) {
         B32 is_small = Core_WM_KeyPress(events, window, 0, arrows[i]);
         B32 is_large = !is_small && Core_WM_KeyPress(events, window, WM_Modifier_Shift, arrows[i]);
         if (is_small || is_large) {
            F32 units = is_small ? _EDIT_NUDGE_SMALL : _EDIT_NUDGE_LARGE;
            F32 sign = (i == 0 || i == 2) ? -1.0f : 1.0f;
            Place_NudgeOpParams* nudge = StateOp_Place_Nudge(ops);
            nudge->dx = (i < 2) ? sign * units / VIEW_PAGE_W : 0.0f;
            nudge->dy = (i < 2) ? 0.0f : sign * units / VIEW_PAGE_H;
         }
      }
   }

   //- bgz: pieces copied, pasted as pieces - while the clipboard has what
   // copying them put there (the \r Windows may give it aside). Looked at
   // before the key is taken: else it is the editor's, as text.
   B32 has_paste = 0;
   for (WM_Event* event = events->first; event != 0 && g_edit_view.clip_count != 0; event = event->next) {
      has_paste |= (event->kind == WM_EventKind_Press && event->key == WM_Key_V && event->modifiers == WM_Modifier_Ctrl);
   }
   if (has_paste && Place_BoxesAreGood(app_state)) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 got = Core_WM_GetClipboardText(scratch.arena);
      U8*     clean = PushArray(scratch.arena, U8, got.size + 1);
      U64     clean_size = 0;
      for (U64 i = 0; i < got.size; i += 1) {
         if (got.str[i] != '\r') {
            clean[clean_size] = got.str[i];
            clean_size += 1;
         }
      }
      if (Str8Match(Str8(clean, clean_size), g_edit_view.clip_text, 0) &&
          Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_V)) {
         StateOp_Edit_PastePieces(ops);
      }
      ScratchEnd(scratch);
   }

   //- bgz: everything else is the text's: vim's keys by the mode it is in,
   // then the editor's (vim\vim.c)
   if (events->first != 0) {
      StateOp_Vim_KeyInput(ops)->box = layout.editor_box;
   }

   if (present_pressed) {
      StateOp_Edit_Present(ops);
   }
}
////////////////////////////////
//~ bgz: the Format panel and the status line, in the UI layer
//
//    column      under the text, the left side's width, to the window's bottom
//       header   "Format: ...": a click opens and closes the panel (as F1)
//       rows     the format's lines - or, in their place, the list of
//                pictures (UI_ScrollRows), while it is open
//       status   a notice if one is up, else the file's name and the keys

// A line of text across a box, at the left, with optional text at the right.
PRIVATE void
_Edit_BuildBarText(F32 pad, String8 left_tag, String8 left, FNT_Tag left_font, String8 right) {
   UI_PrefHeight(UI_Pct(1, 1)) {
      UI_Spacer(UI_Px(pad, 1));
      UI_SetNextPrefWidth(UI_Pct(1, 0));
      UI_SetNextColorTag(left_tag);
      UI_SetNextFont(left_font);
      UI_Label(left);
      if (right.size != 0) {
         UI_SetNextPrefWidth(UI_TextDim(1, 1));
         UI_SetNextColorTag(Str8Lit("dim"));
         UI_Label(right);
      }
      UI_Spacer(UI_Px(pad, 1));
   }
}

// A word to click, in a row of the Format panel: as tall as the row leaves
// it, with a little room around it, and a white edge when it is what there
// is now, as a colour's square has. 1 when it is clicked.
PRIVATE B32
_Edit_BuildRowButton(String8 name, B32 is_now, F32 row_h, F32 ui, B32 interactive, char* fmt, ...) {
   Temp    scratch = ScratchBegin(0, 0);
   va_list args;
   va_start(args, fmt);
   String8 key = PushStr8FV(scratch.arena, fmt, args);
   va_end(args);

   B32 result = 0;
   F32 inset = Max((F32)(S32)(row_h * 0.14f), 1.0f);
   UI_SetNextPrefWidth(UI_ChildrenSum(1));
   UI_SetNextPrefHeight(UI_Px(row_h, 1));
   UI_SetNextChildLayoutAxis(Axis2_Y);
   UI_Box* cell = UI_BuildBoxFromStringf(0, "%S cell", key);
   UI_Parent(cell) {
      // No width: the cell is as wide as what is in it, and a spacer is
      // otherwise as wide as the default.
      UI_SetNextPrefWidth(UI_Px(0, 1));
      UI_Spacer(UI_Px(inset, 1));
      UI_SetNextPrefWidth(UI_TextDim(14.0f * ui, 1));
      UI_SetNextPrefHeight(UI_Px(row_h - inset * 2.0f, 1));
      UI_SetNextTextAlignment(UI_TextAlign_Center);
      UI_SetNextBackgroundColor(V4F32(1, 1, 1, 0.07f));
      UI_SetNextCornerRadius00(3.0f * ui);
      UI_SetNextCornerRadius01(3.0f * ui);
      UI_SetNextCornerRadius10(3.0f * ui);
      UI_SetNextCornerRadius11(3.0f * ui);
      // Keyed by key, but showing only the name: the screen is built with
      // UI_BoxFlag_DisableIDString (frame.c), so a "###" in what is shown
      // would be shown too.
      UI_Box* button = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText |
                                                  (is_now ? UI_BoxFlag_DrawBorder : 0) |
                                                  (interactive ? UI_BoxFlag_Clickable : 0),
                                              "%S", key);
      UI_BoxEquipDisplayString(button, name);
      button->border_color = V4F32(1, 1, 1, 1);
      if (interactive) {
         UI_Signal signal = UI_SignalFromBox(button);
         if (UI_Hovering(signal)) {
            button->background_color = V4F32(1, 1, 1, 0.18f);
         }
         result = !!UI_Clicked(signal);
      }
   }
   UI_Spacer(UI_Px(6.0f * ui, 1));

   ScratchEnd(scratch);
   return result;
}

// A square of a colour to click, side wide and gap before the next, in a
// row of the Format panel, with a white edge when it is the colour there is
// now. 1 when it is clicked.
PRIVATE B32
_Edit_BuildSwatch(Vec4F32 color, B32 is_now, F32 side, F32 gap, F32 row_h, F32 ui, B32 interactive, char* fmt, ...) {
   Temp    scratch = ScratchBegin(0, 0);
   va_list args;
   va_start(args, fmt);
   String8 key = PushStr8FV(scratch.arena, fmt, args);
   va_end(args);

   B32 result = 0;
   F32 inset = (F32)(S32)((row_h - side) * 0.5f);
   UI_SetNextPrefWidth(UI_Px(side, 1));
   UI_SetNextPrefHeight(UI_Px(row_h, 1));
   UI_SetNextChildLayoutAxis(Axis2_Y);
   UI_Box* cell = UI_BuildBoxFromStringf(0, "%S cell", key);
   UI_Parent(cell) {
      UI_Spacer(UI_Px(inset, 1));
      UI_SetNextPrefWidth(UI_Px(side, 1));
      UI_SetNextPrefHeight(UI_Px(side, 1));
      UI_SetNextBackgroundColor(color);
      UI_SetNextCornerRadius00(3.0f * ui);
      UI_SetNextCornerRadius01(3.0f * ui);
      UI_SetNextCornerRadius10(3.0f * ui);
      UI_SetNextCornerRadius11(3.0f * ui);
      UI_Box* swatch = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder |
                                                  (interactive ? UI_BoxFlag_Clickable : 0),
                                              "%S", key);
      // An edge always, faint: white and black are near a theme's panel.
      swatch->border_color = is_now ? V4F32(1, 1, 1, 1) : V4F32(1, 1, 1, 0.22f);
      if (interactive) {
         UI_Signal signal = UI_SignalFromBox(swatch);
         if (UI_Hovering(signal)) {
            swatch->border_color = V4F32(1, 1, 1, 0.75f);
         }
         result = !!UI_Clicked(signal);
      }
   }
   UI_Spacer(UI_Px(gap, 1));

   ScratchEnd(scratch);
   return result;
}

////////////////////////////////
//~ bgz: the preview, in the UI layer
//
// The slide is laid out first, for this frame - into a draw bucket that is
// never drawn - and its pieces are kept: where its words, pictures, title
// and blocks are (g_edit_view.hits, _runs, _boxes). The boxes over
// it are built from those, so they are where the pieces are this frame, and
// a signal is of where the box is now (FR_BuildRectBox):
//
//    1. what the mouse holds goes with it: a piece dragged (place\place.c),
//       a scroll bar (caret\caret.c)
//    2. the slide laid out
//    3. the boxes that are clicked, which are not drawn - first built, first
//       asked: the blocks' scroll bars, the corners' handles, the videos'
//       strips, the words and pictures, the pieces' boxes, the rooms of the
//       words, the small slides and the +
//    4. the slide laid out again, if a click or the wheel moved a block's
//       words. A click that changes the text or the cursor pushes a state
//       op, and shows the frame after
//    5. the boxes that are drawn - first built, drawn on top: the outline
//       of what the mouse is over, the videos' strips, the wash under the
//       mouse, the box of the piece the cursor is in, the selection, the
//       caret, the caption under the slide, the small slides and the +,
//       the slide itself, and the preview's background
//
// So nothing over the slide is a frame behind it: not the caret after
// typing at the bottom of a full block (the keys' ops have run before the
// build), not the outline of a box dragged, not a scroll bar dragged.

//- bgz: what the boxes draw that the UI cannot: slides

typedef struct _Edit_Slide _Edit_Slide;
struct _Edit_Slide {
   const App_State* app_state;
   Rng2F32          page;
   Rng2F32          board;    // what it is drawn in: the page, or more for a board
   B32              is_board;
   F32              edge;     // how thick the line around a board's page is
};

// The preview's slide, as it was laid out in the build: the same scroll,
// and its scroll bars are the UI's (caret\caret.c).
PRIVATE void
_Edit_DrawSlide(UI_Box* box, void* user_data) {
   UnusedVariable(box);
   _Edit_Slide*     slide = (_Edit_Slide*)user_data;
   const App_State* app_state = slide->app_state;
   Deck_Deck*       deck = app_state->deck;
   View_Colors colors = View_ColorsFromTheme(deck->theme);
   Core_D_Rect(slide->board, R2F32P(0, 0, 1, 1), colors.background);
   if (slide->is_board) {
      // Where a slide's page would be, as a faint line: how much of the
      // board one slide holds.
      Vec4F32 line = colors.body;
      line.w = 0.16f;
      Rng2F32 page = slide->page;
      F32     edge = slide->edge;
      Rng2F32 sides[4] = {
          R2F32P(page.min.x, page.min.y, page.max.x, page.min.y + edge),
          R2F32P(page.min.x, page.max.y - edge, page.max.x, page.max.y),
          R2F32P(page.min.x, page.min.y, page.min.x + edge, page.max.y),
          R2F32P(page.max.x - edge, page.min.y, page.max.x, page.max.y),
      };
      for (U64 i = 0; i < ArrayCount(sides); i += 1) {
         Rng2F32 side = Intersect2F32(sides[i], slide->board);
         if (side.max.x > side.min.x && side.max.y > side.min.y) {
            Core_D_Rect(side, R2F32P(0, 0, 1, 1), line);
         }
      }
   }
   if (app_state->slide_idx < deck->slide_count) {
      View_HitList hits = {0};
      hits.bars_by_caller = 1;
      Caret_BeforeDraw(app_state);
      Place_PutDrag(app_state, app_state->slide_idx);
      View_DrawSlideOn(app_state->frame_arena, &app_state->fonts, deck->theme, &deck->slides[app_state->slide_idx], slide->page, slide->board, 1.0f, &hits);
      Place_TakeDrag(app_state, app_state->slide_idx);
      Caret_AfterDrawOf(app_state, app_state->slide_idx);
   }
}

typedef struct _Edit_Thumb _Edit_Thumb;
struct _Edit_Thumb {
   const App_State* app_state;
   U64              slide_idx;
   Rng2F32          rect;      // exactly: the box's is on whole pixels, and the row is not
   B32              has_frame; // in the box's border colour, frame pixels wide
   F32              frame;
};

// A small slide. Only a picture that is loaded already is drawn: the slide
// showing and the ones next to it. Loading every picture of a long deck for
// its small slides would take the memory Deck_LoadSlide saves.
PRIVATE void
_Edit_DrawThumb(UI_Box* box, void* user_data) {
   _Edit_Thumb*     thumb = (_Edit_Thumb*)user_data;
   const App_State* app_state = thumb->app_state;
   Deck_Deck*       deck = app_state->deck;
   Rng2F32          rect = thumb->rect;
   if (thumb->slide_idx >= deck->slide_count) {
      return;
   }
   if (thumb->has_frame) {
      Core_D_Rect(Pad2F32(rect, thumb->frame), R2F32P(0, 0, 1, 1), box->border_color);
   }
   Core_D_Rect(rect, R2F32P(0, 0, 1, 1), View_ColorsFromTheme(deck->theme).background);
   B32 is_showing = (thumb->slide_idx == app_state->slide_idx);
   Place_PutDrag(app_state, thumb->slide_idx);
   View_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, &deck->slides[thumb->slide_idx], rect, is_showing ? 1.0f : 0.8f, 0);
   Place_TakeDrag(app_state, thumb->slide_idx);
}

typedef struct _Edit_Plus _Edit_Plus;
struct _Edit_Plus {
   Rng2F32 rect; // exactly, as the small slides'
   F32     half; // how thick a bar is, either side of its middle
};

// The + that adds a slide: its background, and a + of two bars in the
// middle, in the box's text colour - drawn, not written, to be in the
// middle whatever the font.
PRIVATE void
_Edit_DrawPlus(UI_Box* box, void* user_data) {
   _Edit_Plus* plus = (_Edit_Plus*)user_data;
   F32         half = plus->half;
   Rng2F32     rect = plus->rect;
   Vec2F32     dim = Dim2F32(rect);
   Core_D_Rect(rect, R2F32P(0, 0, 1, 1), box->background_color);
   F32     middle_x = (F32)(S32)(rect.min.x + dim.x * 0.5f);
   F32     middle_y = (F32)(S32)(rect.min.y + dim.y * 0.5f);
   F32     arm = (F32)(S32)(Min(dim.x, dim.y) * 0.22f);
   Core_D_Rect(R2F32P(middle_x - arm, middle_y - half, middle_x + arm, middle_y + half), R2F32P(0, 0, 1, 1), box->text_color);
   Core_D_Rect(R2F32P(middle_x - half, middle_y - arm, middle_x + half, middle_y + arm), R2F32P(0, 0, 1, 1), box->text_color);
}

// The preview's slide laid out at page, and its pieces kept. The caret's
// scroll follows the caret (Caret_Follow), and the slide is laid out again
// if it moved.
PRIVATE void
_Edit_LayoutPreview(const App_State* app_state, Rng2F32 page, F32 ui) {
   Deck_Deck* deck = app_state->deck;
   g_edit_view.hit_count = 0;
   g_edit_view.run_count = 0;
   g_edit_view.box_count = 0;
   if (Dim2F32(page).x <= 0 || app_state->slide_idx >= deck->slide_count) {
      return;
   }

   // Asking for the slide is what loads its pictures, and what releases the
   // pictures of slides far from this one.
   Deck_LoadSlide(deck, app_state->slide_idx);
   Deck_Slide*  slide = &deck->slides[app_state->slide_idx];
   View_HitList hits = {0};
   hits.bars_by_caller = 1;
   D_Bucket* bucket = Core_D_BucketMake();
   // With a piece being dragged where the mouse has it (place\place.c).
   Place_PutDrag(app_state, app_state->slide_idx);
   Core_D_BucketScope(bucket) {
      Caret_BeforeDraw(app_state);
      View_DrawSlideOn(app_state->frame_arena, &app_state->fonts, deck->theme, slide, page, g_edit_view.board, 1.0f, &hits);
      if (Caret_Follow(app_state, &hits)) {
         // A block's words moved to follow the caret, or back to where they
         // end: the slide again, as it now is - or for a frame the caret's
         // line would be cut off, or the wheel would show room past the
         // words.
         MemoryZeroStruct(&hits);
         hits.bars_by_caller = 1;
         Caret_BeforeDraw(app_state);
         View_DrawSlideOn(app_state->frame_arena, &app_state->fonts, deck->theme, slide, page, g_edit_view.board, 1.0f, &hits);
      }
   }
   Place_TakeDrag(app_state, app_state->slide_idx);
   Caret_AfterDrawOf(app_state, app_state->slide_idx);

   if (g_edit_view.hits_arena == 0) {
      g_edit_view.hits_arena = ArenaAlloc();
   }
   ArenaClear(g_edit_view.hits_arena);
   g_edit_view.hits = PushArray(g_edit_view.hits_arena, View_Hit, Max(hits.count, 1));
   // The deck laid out is of the text at parsed_edit_count.
   g_edit_view.hits_edit_count = app_state->parsed_edit_count;
   MemoryCopyArray(g_edit_view.boxes, hits.boxes);
   g_edit_view.box_count = hits.box_count;
   g_edit_view.page = hits.page;

   F32 grow = 4.0f * ui;
   for (View_Hit* hit = hits.first; hit != 0; hit = hit->next) {
      // A little larger than what was drawn, to be easy to click, and no
      // larger than the slide.
      Rng2F32 rect = Intersect2F32(R2F32P(hit->rect.min.x - grow, hit->rect.min.y - grow * 0.5f, hit->rect.max.x + grow,
                                          hit->rect.max.y + grow * 0.5f),
                                   g_edit_view.board);
      if (rect.max.x <= rect.min.x || rect.max.y <= rect.min.y) {
         continue;
      }
      View_Hit* kept = &g_edit_view.hits[g_edit_view.hit_count];
      g_edit_view.hit_count += 1;
      kept->next = 0;
      kept->rect = rect;
      kept->text_range = hit->text_range;
   }
   Caret_KeepRuns(&hits);
}

////////////////////////////////
//~ bgz: the right-click menu
//
// A right-click on the preview's slide opens a menu where the mouse is, of
// what can be done there, each line with its key beside it - so the keys do
// not have to be remembered, and are learned from it. On a piece: the piece
// is picked first, alone, unless it is one of those picked already, and the
// lines are about all that are picked. On open space: what is not about a
// piece. On words that are selected - a drag across them, right at the
// words - it is about those words and their lines, and nothing is picked:
// bold and code around the words, and a size, a colour and a side for the
// lines the selection is in, each its own (look\look.c). A click on a line
// does it and closes the menu; a click anywhere else, or Esc, closes it.
// Larger and smaller words leave it open, to be clicked again.
//
// A colour is a row of squares to click, and a side a row of words, with
// "usual" for none of its own; what there is now has a white edge.
//
// It is built first of the preview's boxes (_Edit_BuildPreview), so it is
// over everything there and asked before it, with a box the size of the
// window under its lines that takes the click anywhere else. What a line
// does is the op its key pushes.

typedef enum _Edit_MenuItemKind {
   _Edit_MenuItem_Separator,
   _Edit_MenuItem_Copy,
   _Edit_MenuItem_Cut,
   _Edit_MenuItem_Paste,
   _Edit_MenuItem_Duplicate,
   _Edit_MenuItem_Delete,
   _Edit_MenuItem_Front,
   _Edit_MenuItem_Back,
   _Edit_MenuItem_Larger,
   _Edit_MenuItem_Smaller,
   _Edit_MenuItem_UsualSize,
   _Edit_MenuItem_Colors, // a row: the colours with names, and usual
   _Edit_MenuItem_Aligns, // a row: left, centre, right, and usual
   _Edit_MenuItem_Bold,   // around the words selected
   _Edit_MenuItem_Code,
   _Edit_MenuItem_LineKind,    // with value: an Edit_LineKind for the lines selected
   _Edit_MenuItem_AddPicture,  // a file picked, and put where the menu was opened
   _Edit_MenuItem_NewSlide,    // after the one showing
   _Edit_MenuItem_SlideChange, // with value: an Edit_SlideChange
   _Edit_MenuItem_Layout,      // with value: a Deck_Layout for the slide
   _Edit_MenuItem_Arrange, // with arrange: lined up, or spaced
   _Edit_MenuItem_PutBack,
   _Edit_MenuItem_PickAll,
   _Edit_MenuItem_Lift,      // the pieces picked, to a slide of their own
   _Edit_MenuItem_BoardHome, // a board moved back to its page
} _Edit_MenuItemKind;

typedef struct _Edit_MenuItem _Edit_MenuItem;
struct _Edit_MenuItem {
   _Edit_MenuItemKind kind;
   String8            name;
   String8            keys;
   Place_ArrangeKind  arrange;
   U64                value;
};

#define _EDIT_MENU_MAX_ITEMS 32

// How large a picture added from the menu is, in parts of the page: the
// room it is fitted in, from where the menu was opened.
#define _EDIT_MENU_PICTURE_W 0.36f
#define _EDIT_MENU_PICTURE_H 0.36f

void
Edit_OpenMenu(const App_State* app_state, Vec2F32 pos, B32 has_piece, Deck_PlaceKind kind, U64 idx) {
   g_edit_view.menu_open = 1;
   g_edit_view.menu_pos = pos;
   g_edit_view.menu_has_piece = has_piece;
   g_edit_view.menu_has_text = 0;
   g_edit_view.menu_kind = kind;
   g_edit_view.menu_idx = idx;
   // On words selected in this piece: about them, and they stay selected.
   // Not a text box just made, whose line is selected only to be typed
   // over, and not a piece picked whole.
   Editor_Editor* editor = app_state->editor;
   // has_new_box stays up after the box is typed in, until a click looks
   // at it (_Edit_TakeOutNewBox): only one still as it was made counts.
   B32 is_new_box = (g_edit_view.has_new_box && editor != 0 && editor->edit_count == g_edit_view.new_box_edit_count);
   if (has_piece && kind != Deck_PlaceKind_Picture && editor != 0 && editor->cursor != editor->mark &&
       !is_new_box && !Edit_PickedIsGood(app_state) && app_state->parsed_edit_count == editor->edit_count) {
      U64            target_slide = 0;
      Deck_PlaceKind target_kind = Deck_PlaceKind_Title;
      U64            target_idx = 0;
      if (Look_Target(app_state, &target_slide, &target_kind, &target_idx) && target_slide == app_state->slide_idx &&
          target_kind == kind && target_idx == idx) {
         g_edit_view.menu_has_piece = 0;
         g_edit_view.menu_has_text = 1;
         return;
      }
   }
   // Not one of those picked: it is picked alone, as a click on it would.
   if (has_piece && !Edit_PieceIsPicked(app_state, kind, idx)) {
      Edit_PreviewClickOpParams* click = StateOp_Edit_PreviewClick(app_state->op_list);
      click->pos = pos;
      click->picks = 1;
      click->picks_only = 1;
      click->kind = kind;
      click->idx = idx;
   }
}

// One of the words to click in a row of the menu - a side, or usual: at x,
// with a white edge when it is what there is now. How wide it is to
// width_out, for the next; 1 when it is pressed.
PRIVATE B32
_Edit_BuildMenuWord(FNT_Tag font, F32 px, F32 x, F32 row_y, F32 row_h, F32 ui, String8 name, B32 is_now, U64 row_idx, U64 idx,
                    F32* width_out) {
   F32     line_h = Text_LineHeight(font, px);
   F32     inset = Max((F32)(S32)(row_h * 0.14f), 1.0f);
   F32     side_pad = (F32)(S32)(8.0f * ui);
   F32     width = (F32)(S32)(Text_Width(font, px, name) + side_pad * 2.0f);
   Rng2F32 rect = R2F32P(x, row_y + inset, x + width, row_y + row_h - inset);
   UI_Signal signal = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, rect, "###edit menu word %llu %llu",
                                                       (unsigned long long)row_idx, (unsigned long long)idx));
   FR_BuildWords(font, px, V2F32(x + side_pad, (F32)(S32)(row_y + (row_h - line_h) * 0.5f)), V4F32(0.93f, 0.93f, 0.93f, 1.0f), name,
                 "###edit menu word name %llu %llu", (unsigned long long)row_idx, (unsigned long long)idx);
   if (is_now) {
      FR_BuildEdge(rect, 1.0f, V4F32(1, 1, 1, 1), "###edit menu word now %llu %llu", (unsigned long long)row_idx,
                   (unsigned long long)idx);
   }
   UI_SetNextBackgroundColor(UI_Hovering(signal) ? V4F32(0.30f, 0.50f, 0.95f, 0.55f) : V4F32(1, 1, 1, 0.07f));
   FR_BuildRectBox(UI_BoxFlag_DrawBackground, rect, "###edit menu word ground %llu %llu", (unsigned long long)row_idx,
                   (unsigned long long)idx);
   *width_out = width;
   return !!(signal.f & UI_SignalFlag_LeftPressed);
}

// The words of the menu's rows of things to click, and what each is.
PRIVATE String8 _edit_menu_align_names[] = {Str8LitComp("Left"), Str8LitComp("Centre"), Str8LitComp("Right"), Str8LitComp("Usual")};
PRIVATE U64     _edit_menu_aligns[] = {Deck_Align_Left, Deck_Align_Center, Deck_Align_Right, Deck_Align_COUNT};

PRIVATE void
_Edit_BuildMenu(const App_State* app_state, _Edit_Layout* layout, B32 interactive) {
   if (!g_edit_view.menu_open) {
      return;
   }
   // Gone with what it was about: the slide, the pieces picked, or the
   // words selected.
   B32 text_gone = g_edit_view.menu_has_text && (app_state->editor == 0 || app_state->editor->cursor == app_state->editor->mark);
   if (!interactive || !Place_BoxesAreGood(app_state) || (g_edit_view.menu_has_piece && !Edit_PickedIsGood(app_state)) || text_gone) {
      g_edit_view.menu_open = 0;
      return;
   }
   B32 has_text = g_edit_view.menu_has_text;
   StateOp_List* ops = app_state->op_list;
   F32           ui = layout->ui;

   //- bgz: its lines
   _Edit_MenuItem items[_EDIT_MENU_MAX_ITEMS] = {0};
   U64            count = 0;
#define _Edit_MenuAdd(kind_, name_, keys_) \
   (items[count].kind = (kind_), items[count].name = Str8Lit(name_), items[count].keys = Str8Lit(keys_), count += 1)
#define _Edit_MenuAddArrange(arrange_, name_, keys_) \
   (items[count].arrange = (arrange_), _Edit_MenuAdd(_Edit_MenuItem_Arrange, name_, keys_))
#define _Edit_MenuAddValue(kind_, value_, name_, keys_) (items[count].value = (U64)(value_), _Edit_MenuAdd(kind_, name_, keys_))
   U64 picked = g_edit_view.picked_count;
   B32 has_words = 0;
   for (U64 i = 0; i < picked; i += 1) {
      has_words |= (g_edit_view.picked_kinds[i] != Deck_PlaceKind_Picture);
   }
   if (has_text) {
      _Edit_MenuAdd(_Edit_MenuItem_Bold, "Bold", "Ctrl+B");
      _Edit_MenuAdd(_Edit_MenuItem_Code, "Code", "Ctrl+E");
      _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_Larger, "Larger", "Ctrl+]");
      _Edit_MenuAdd(_Edit_MenuItem_Smaller, "Smaller", "Ctrl+[");
      _Edit_MenuAdd(_Edit_MenuItem_UsualSize, "Usual size", "");
      _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_Colors, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_Aligns, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
      _Edit_MenuAddValue(_Edit_MenuItem_LineKind, Edit_LineKind_Bullet, "Bullets", "- ");
      _Edit_MenuAddValue(_Edit_MenuItem_LineKind, Edit_LineKind_InnerBullet, "Inner bullets", "  - ");
      _Edit_MenuAddValue(_Edit_MenuItem_LineKind, Edit_LineKind_Heading, "Heading", "## ");
      _Edit_MenuAddValue(_Edit_MenuItem_LineKind, Edit_LineKind_Paragraph, "Plain lines", "");
   } else if (g_edit_view.menu_has_piece) {
      _Edit_MenuAdd(_Edit_MenuItem_Copy, "Copy", "Ctrl+C");
      _Edit_MenuAdd(_Edit_MenuItem_Cut, "Cut", "Ctrl+X");
      if (g_edit_view.clip_count != 0) {
         _Edit_MenuAdd(_Edit_MenuItem_Paste, "Paste", "Ctrl+V");
      }
      _Edit_MenuAdd(_Edit_MenuItem_Duplicate, "Duplicate", "Ctrl+D");
      _Edit_MenuAdd(_Edit_MenuItem_Delete, "Delete", "Del");
      _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_Lift, "Move to a new slide", "Ctrl+Shift+N");
      _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_Front, "Bring to front", "Ctrl+Shift+]");
      _Edit_MenuAdd(_Edit_MenuItem_Back, "Send to back", "Ctrl+Shift+[");
      if (has_words && g_edit_view.is_board) {
         // On a board Ctrl and the wheel zoom.
         _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
         _Edit_MenuAdd(_Edit_MenuItem_Larger, "Larger words", "Ctrl+]");
         _Edit_MenuAdd(_Edit_MenuItem_Smaller, "Smaller words", "Ctrl+[");
      } else if (has_words) {
         _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
         _Edit_MenuAdd(_Edit_MenuItem_Larger, "Larger words", "Ctrl+Wheel up");
         _Edit_MenuAdd(_Edit_MenuItem_Smaller, "Smaller words", "Ctrl+Wheel down");
      }
      if (has_words) {
         _Edit_MenuAdd(_Edit_MenuItem_UsualSize, "Usual size", "");
         _Edit_MenuAdd(_Edit_MenuItem_Colors, "", "");
         _Edit_MenuAdd(_Edit_MenuItem_Aligns, "", "");
      }
      if (picked >= 2) {
         _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
         _Edit_MenuAddArrange(Place_ArrangeKind_Left, "Line up left edges", "Ctrl+Shift+Left");
         _Edit_MenuAddArrange(Place_ArrangeKind_Right, "Line up right edges", "Ctrl+Shift+Right");
         _Edit_MenuAddArrange(Place_ArrangeKind_Top, "Line up tops", "Ctrl+Shift+Up");
         _Edit_MenuAddArrange(Place_ArrangeKind_Bottom, "Line up bottoms", "Ctrl+Shift+Down");
         _Edit_MenuAddArrange(Place_ArrangeKind_CenterX, "Centre on one line down", "Ctrl+Shift+C");
         _Edit_MenuAddArrange(Place_ArrangeKind_CenterY, "Centre on one line across", "Ctrl+Shift+M");
      }
      if (picked >= 3) {
         _Edit_MenuAddArrange(Place_ArrangeKind_SpaceX, "Space evenly across", "Ctrl+Shift+H");
         _Edit_MenuAddArrange(Place_ArrangeKind_SpaceY, "Space evenly down", "Ctrl+Shift+V");
      }
      _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_PutBack, "Put back where the layout has it", "");
   } else {
      if (g_edit_view.clip_count != 0) {
         _Edit_MenuAdd(_Edit_MenuItem_Paste, "Paste", "Ctrl+V");
      }
      _Edit_MenuAdd(_Edit_MenuItem_AddPicture, "Add a picture or video...", "");
      if (g_edit_view.is_board) {
         _Edit_MenuAdd(_Edit_MenuItem_PickAll, "Select everything on the board", "");
         _Edit_MenuAdd(_Edit_MenuItem_BoardHome, "Back to the page, at its size", "");
      } else {
         _Edit_MenuAdd(_Edit_MenuItem_PickAll, "Select everything on the slide", "");
      }
      // The slide itself.
      _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
      _Edit_MenuAdd(_Edit_MenuItem_NewSlide, "New slide after this one", "");
      _Edit_MenuAddValue(_Edit_MenuItem_SlideChange, Edit_SlideChange_Duplicate, "Duplicate slide", "");
      _Edit_MenuAddValue(_Edit_MenuItem_SlideChange, Edit_SlideChange_Delete, "Delete slide", "");
      if (g_edit_view.is_board) {
         _Edit_MenuAddValue(_Edit_MenuItem_SlideChange, Edit_SlideChange_Board, "Make this board a slide", "---");
      } else {
         _Edit_MenuAddValue(_Edit_MenuItem_SlideChange, Edit_SlideChange_Board, "Make this slide a board", "@canvas");
         // Its layout: each by its name, as written after @layout, and
         // "now" beside the one it has.
         _Edit_MenuAdd(_Edit_MenuItem_Separator, "", "");
         Deck_Layout now = (app_state->slide_idx < app_state->deck->slide_count) ? app_state->deck->slides[app_state->slide_idx].layout
                                                                                 : Deck_Layout_COUNT;
         for (U64 k = 0; k < Deck_Layout_COUNT; k += 1) {
            items[count].kind = _Edit_MenuItem_Layout;
            items[count].value = k;
            // Said by what it does: its name in the text (@layout) is by
            // where the picture goes, which reads as being about a picture.
            items[count].name = ((Deck_Layout)k == Deck_Layout_PictureRight)    ? Str8Lit("Slide layout: words left, picture right")
                                : ((Deck_Layout)k == Deck_Layout_PictureLeft)   ? Str8Lit("Slide layout: picture left, words right")
                                : ((Deck_Layout)k == Deck_Layout_PictureBehind) ? Str8Lit("Slide layout: picture behind the words")
                                : ((Deck_Layout)k == Deck_Layout_Center)        ? Str8Lit("Slide layout: everything centred")
                                                                                : Deck_LayoutName((Deck_Layout)k);
            items[count].keys = ((Deck_Layout)k == now) ? Str8Lit("now") : Str8Lit("");
            count += 1;
         }
      }
   }
#undef _Edit_MenuAdd
#undef _Edit_MenuAddArrange
#undef _Edit_MenuAddValue

   //- bgz: how large it is, and where: all of it in the preview
   FNT_Tag font = app_state->fonts.regular;
   F32     px = layout->ui_px;
   F32     line_h = Text_LineHeight(font, px);
   F32     row_h = (F32)(S32)(line_h * 1.5f);
   F32     sep_h = (F32)(S32)(row_h * 0.4f);
   F32     pad = (F32)(S32)(12.0f * ui);
   F32     name_w = 0;
   F32     keys_w = 0;
   F32     height = pad;
   for (U64 i = 0; i < count; i += 1) {
      name_w = Max(name_w, Text_Width(font, px, items[i].name));
      keys_w = Max(keys_w, Text_Width(font, px, items[i].keys));
      height += (items[i].kind == _Edit_MenuItem_Separator) ? sep_h : row_h;
   }
   F32     width = (F32)(S32)(pad + name_w + pad * 2.0f + keys_w + pad);

   // The rows of things to click: a square a colour, and a gap; a word a
   // side. The menu is as wide as the widest of them, too.
   F32     swatch = (F32)(S32)(row_h * 0.62f);
   F32     swatch_gap = (F32)(S32)(6.0f * ui);
   F32     word_gap = (F32)(S32)(4.0f * ui);
   F32     word_pad = (F32)(S32)(8.0f * ui) * 2.0f;
   String8 usual_name = Str8Lit("Usual");
   String8 color_label = Str8Lit("Colour");
   String8 align_label = Str8Lit("Side");
   F32     label_w = (F32)(S32)(Max(Text_Width(font, px, color_label), Text_Width(font, px, align_label)) + pad);
   F32     colors_w = label_w + (F32)DECK_NAMED_COLOR_COUNT * (swatch + swatch_gap) + Text_Width(font, px, usual_name) + word_pad;
   F32     aligns_w = label_w;
   for (U64 i = 0; i < ArrayCount(_edit_menu_align_names); i += 1) {
      aligns_w += (F32)(S32)(Text_Width(font, px, _edit_menu_align_names[i]) + word_pad) + word_gap;
   }
   for (U64 i = 0; i < count; i += 1) {
      if (items[i].kind == _Edit_MenuItem_Colors) {
         width = Max(width, (F32)(S32)(pad + colors_w + pad));
      } else if (items[i].kind == _Edit_MenuItem_Aligns) {
         width = Max(width, (F32)(S32)(pad + aligns_w + pad));
      }
   }

   // What the words the menu is about have now: the lines selected, or the
   // piece it was opened on.
   Deck_Look* piece_look = 0;
   if (!has_text && g_edit_view.menu_has_piece && g_edit_view.menu_kind != Deck_PlaceKind_Picture &&
       app_state->slide_idx < app_state->deck->slide_count) {
      piece_look = Look_Of(&app_state->deck->slides[app_state->slide_idx], g_edit_view.menu_kind, g_edit_view.menu_idx);
   }

   Rng2F32 within = layout->preview;
   F32     x = Clamp(within.min.x, g_edit_view.menu_pos.x, Max(within.max.x - width, within.min.x));
   F32     y = Clamp(within.min.y, g_edit_view.menu_pos.y, Max(within.max.y - height, within.min.y));
   x = (F32)(S32)x;
   y = (F32)(S32)y;
   Rng2F32 menu = R2F32P(x, y, x + width, y + height);

   Vec4F32 name_color = V4F32(0.93f, 0.93f, 0.93f, 1.0f);
   Vec4F32 keys_color = V4F32(0.60f, 0.62f, 0.66f, 1.0f);

   //- bgz: the lines, each over its wash when the mouse is on it
   B32 closes = 0;
   F32 row_y = y + (F32)(S32)(pad * 0.5f);
   for (U64 i = 0; i < count; i += 1) {
      _Edit_MenuItem* item = &items[i];
      if (item->kind == _Edit_MenuItem_Separator) {
         F32 line_y = (F32)(S32)(row_y + sep_h * 0.5f);
         UI_SetNextBackgroundColor(V4F32(1, 1, 1, 0.14f));
         FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(x + pad * 0.5f, line_y, x + width - pad * 0.5f, line_y + 1.0f),
                         "###edit menu sep %llu", (unsigned long long)i);
         row_y += sep_h;
         continue;
      }
      Rng2F32   row = R2F32P(x, row_y, x + width, row_y + row_h);
      F32       text_y = (F32)(S32)(row_y + (row_h - line_h) * 0.5f);

      //- bgz: a row of things to click: the colours, or the sides
      if (item->kind == _Edit_MenuItem_Colors || item->kind == _Edit_MenuItem_Aligns) {
         B32 is_colors = (item->kind == _Edit_MenuItem_Colors);
         S64 picked_value = -1;
         F32 at_x = x + pad + label_w;
         F32 word_w = 0;
         FR_BuildWords(font, px, V2F32(x + pad, text_y), keys_color, is_colors ? color_label : align_label, "###edit menu label %llu",
                       (unsigned long long)i);
         if (is_colors) {
            for (U64 k = 0; k < DECK_NAMED_COLOR_COUNT; k += 1) {
               Vec4F32 color = Deck_NamedColorFromIdx(k).color;
               B32     is_now = has_text ? Look_ColorIsNow(app_state, k)
                                         : (piece_look != 0 && piece_look->has_color && piece_look->color.x == color.x &&
                                            piece_look->color.y == color.y && piece_look->color.z == color.z);
               F32     top = (F32)(S32)(row_y + (row_h - swatch) * 0.5f);
               Rng2F32 rect = R2F32P(at_x, top, at_x + swatch, top + swatch);
               UI_SetNextBackgroundColor(color);
               UI_Signal square = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_DrawBackground | UI_BoxFlag_MouseClickable, rect,
                                                                   "###edit menu color %llu", (unsigned long long)k));
               // An edge always, faint: white and black are near the menu's
               // ground.
               FR_BuildEdge(Pad2F32(rect, 2.0f), 1.0f,
                            is_now ? V4F32(1, 1, 1, 1) : UI_Hovering(square) ? V4F32(1, 1, 1, 0.75f) : V4F32(1, 1, 1, 0.22f),
                            "###edit menu color edge %llu", (unsigned long long)k);
               if (square.f & UI_SignalFlag_LeftPressed) {
                  picked_value = (S64)k;
               }
               at_x += swatch + swatch_gap;
            }
            B32 is_usual = has_text ? Look_ColorIsNow(app_state, DECK_NAMED_COLOR_COUNT) : (piece_look != 0 && !piece_look->has_color);
            if (_Edit_BuildMenuWord(font, px, at_x, row_y, row_h, ui, usual_name, is_usual, i, 0, &word_w)) {
               picked_value = DECK_NAMED_COLOR_COUNT;
            }
         } else {
            for (U64 k = 0; k < ArrayCount(_edit_menu_aligns); k += 1) {
               U64 align = _edit_menu_aligns[k];
               B32 is_now = has_text ? Look_AlignIsNow(app_state, align)
                                     : (piece_look != 0 &&
                                        ((align >= Deck_Align_COUNT) ? !piece_look->has_align
                                                                     : (piece_look->has_align && (U64)piece_look->align == align)));
               if (_Edit_BuildMenuWord(font, px, at_x, row_y, row_h, ui, _edit_menu_align_names[k], is_now, i, k, &word_w)) {
                  picked_value = (S64)align;
               }
               at_x += word_w + word_gap;
            }
         }
         if (picked_value >= 0 && is_colors) {
            Look_SetColorOpParams* set = StateOp_Look_SetColor(ops);
            set->named_idx = (U64)picked_value;
            set->has_piece = !has_text;
            set->kind = g_edit_view.menu_kind;
            set->idx = g_edit_view.menu_idx;
            set->with_picked = !has_text;
            closes = 1;
         } else if (picked_value >= 0) {
            Look_SetAlignOpParams* set = StateOp_Look_SetAlign(ops);
            set->align = (U64)picked_value;
            set->has_piece = !has_text;
            set->kind = g_edit_view.menu_kind;
            set->idx = g_edit_view.menu_idx;
            set->with_picked = !has_text;
            closes = 1;
         }
         row_y += row_h;
         continue;
      }

      UI_Signal signal = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, row, "###edit menu row %llu", (unsigned long long)i));
      FR_BuildWords(font, px, V2F32(x + pad, text_y), name_color, item->name, "###edit menu name %llu", (unsigned long long)i);
      if (item->keys.size != 0) {
         FR_BuildWords(font, px, V2F32(x + width - pad - Text_Width(font, px, item->keys), text_y), keys_color, item->keys,
                       "###edit menu keys %llu", (unsigned long long)i);
      }
      if (UI_Hovering(signal)) {
         UI_SetNextBackgroundColor(V4F32(0.30f, 0.50f, 0.95f, 0.55f));
         FR_BuildRectBox(UI_BoxFlag_DrawBackground, row, "###edit menu hot");
      }
      if (signal.f & UI_SignalFlag_LeftPressed) {
         closes = 1;
         switch (item->kind) {
         case _Edit_MenuItem_Copy: StateOp_Edit_CopyPieces(ops)->is_cut = 0; break;
         case _Edit_MenuItem_Cut: StateOp_Edit_CopyPieces(ops)->is_cut = 1; break;
         case _Edit_MenuItem_Paste: StateOp_Edit_PastePieces(ops); break;
         case _Edit_MenuItem_Duplicate: StateOp_Edit_DuplicatePieces(ops); break;
         case _Edit_MenuItem_Delete: StateOp_Edit_DeletePiece(ops); break;
         case _Edit_MenuItem_Front: StateOp_Edit_OrderPieces(ops)->to_back = 0; break;
         case _Edit_MenuItem_Back: StateOp_Edit_OrderPieces(ops)->to_back = 1; break;
         case _Edit_MenuItem_Arrange: StateOp_Place_Arrange(ops)->kind = item->arrange; break;
         case _Edit_MenuItem_LineKind: StateOp_Edit_SetLineKind(ops)->kind = (Edit_LineKind)item->value; break;
         case _Edit_MenuItem_AddPicture: {
            // bgz's shell: the system's window, which does not return until
            // it is answered. It lists every file: a picture or a video is
            // what can be taken. Put as one dropped on the window is - into
            // the deck's folder too - with its top left where the menu was
            // opened.
            Temp    pick_scratch = ScratchBegin(0, 0);
            String8 folder = (app_state->deck != 0) ? FR_WholePath(pick_scratch.arena, app_state->deck->folder) : Str8Zero();
            String8 picked_file = ShPickFile(ops->arena, Str8Lit("Add a picture or video"), folder, Str8Zero());
            if (picked_file.size != 0 && (Image_NameIsImage(picked_file) || Video_NameIsVideo(picked_file))) {
               Edit_DropPictureOpParams* drop = StateOp_Edit_DropPicture(ops);
               Vec2F32                   page_dim = Dim2F32(g_edit_view.page);
               drop->path = picked_file;
               if (page_dim.x > 0 && page_dim.y > 0) {
                  F32 place_x = (g_edit_view.menu_pos.x - g_edit_view.page.min.x) / page_dim.x;
                  F32 place_y = (g_edit_view.menu_pos.y - g_edit_view.page.min.y) / page_dim.y;
                  if (!g_edit_view.is_board) {
                     // All of it on the page.
                     place_x = Clamp(0.0f, place_x, 1.0f - _EDIT_MENU_PICTURE_W);
                     place_y = Clamp(0.0f, place_y, 1.0f - _EDIT_MENU_PICTURE_H);
                  }
                  drop->has_place = 1;
                  drop->place = R2F32P(place_x, place_y, place_x + _EDIT_MENU_PICTURE_W, place_y + _EDIT_MENU_PICTURE_H);
               }
            } else if (picked_file.size != 0) {
               StateOp_App_Notice(ops)->notice = Str8Lit("That file is not a picture or a video the program can show");
            }
            ScratchEnd(pick_scratch);
         } break;
         case _Edit_MenuItem_NewSlide: StateOp_Edit_AddSlide(ops); break;
         case _Edit_MenuItem_SlideChange: StateOp_Edit_ChangeSlide(ops)->change = (Edit_SlideChange)item->value; break;
         case _Edit_MenuItem_Layout: StateOp_Edit_SetLayout(ops)->layout = (Deck_Layout)item->value; break;
         case _Edit_MenuItem_Bold: StateOp_Edit_Wrap(ops)->is_code = 0; break;
         case _Edit_MenuItem_Code: StateOp_Edit_Wrap(ops)->is_code = 1; break;
         case _Edit_MenuItem_Larger:
         case _Edit_MenuItem_Smaller:
         case _Edit_MenuItem_UsualSize: {
            S32 direction = (item->kind == _Edit_MenuItem_Larger) ? 1 : (item->kind == _Edit_MenuItem_Smaller) ? -1 : 0;
            if (has_text) {
               // The lines selected (look\look.c).
               StateOp_Look_StepSize(ops)->direction = direction;
            }
            // The words of all picked.
            for (U64 k = 0; k < picked && !has_text; k += 1) {
               if (g_edit_view.picked_kinds[k] != Deck_PlaceKind_Picture) {
                  Look_StepSizeOpParams* step = StateOp_Look_StepSize(ops);
                  step->direction = direction;
                  step->has_piece = 1;
                  step->kind = g_edit_view.picked_kinds[k];
                  step->idx = g_edit_view.picked_idxs[k];
                  step->with_picked = 1;
                  break;
               }
            }
            // Larger and smaller: left open, to be clicked again.
            closes = (direction == 0);
         } break;
         case _Edit_MenuItem_PutBack: {
            Place_RemoveLineOpParams* remove = StateOp_Place_RemoveLine(ops);
            remove->kind = g_edit_view.menu_kind;
            remove->idx = g_edit_view.menu_idx;
         } break;
         case _Edit_MenuItem_PickAll: {
            // A board's are past the page too, and past the window.
            StateOp_Edit_PickRect(ops)->rect = g_edit_view.is_board ? R2F32P(-1.0e9f, -1.0e9f, 1.0e9f, 1.0e9f) : g_edit_view.page;
         } break;
         case _Edit_MenuItem_Lift: StateOp_Edit_LiftPieces(ops); break;
         case _Edit_MenuItem_BoardHome: {
            // The screen's own, with no op: back over the page, at its size.
            g_edit_view.board_zoom = 0;
         } break;
         default: break;
         }
      }
      row_y += row_h;
   }

   //- bgz: its edge and its ground, under its lines
   FR_BuildEdge(menu, 1.0f, V4F32(1, 1, 1, 0.22f), "###edit menu edge");
   UI_SetNextBackgroundColor(V4F32(0.13f, 0.14f, 0.17f, 1.0f));
   UI_Signal ground = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_DrawBackground | UI_BoxFlag_MouseClickable, menu, "###edit menu ground"));
   UnusedVariable(ground); // a click between its lines does nothing

   //- bgz: anywhere else: a click closes it, and is not a click on what is
   // there
   UI_Signal away = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, R2F32P(0, 0, 100000.0f, 100000.0f), "###edit menu away"));
   if (away.f & (UI_SignalFlag_LeftPressed | UI_SignalFlag_RightPressed)) {
      closes = 1;
   }
   if (closes) {
      g_edit_view.menu_open = 0;
   }
}

PRIVATE void
_Edit_BuildPreview(const App_State* app_state, _Edit_Layout* layout, B32 interactive) {
   Editor_Editor* editor = app_state->editor;
   Deck_Deck*     deck = app_state->deck;
   StateOp_List*  ops = app_state->op_list;
   F32            ui = layout->ui;

   //- bgz: the right-click menu, over everything else here
   _Edit_BuildMenu(app_state, layout, interactive);

   //- bgz: 1, 2: what the mouse holds, then the slide laid out. The deck is
   // of the text as it is, and the slide is the one the cursor is on:
   // _FR_UpdateAppState saw to both.
   g_edit_view.is_board = layout->is_board;
   g_edit_view.board = layout->board;
   g_edit_view.board_base = layout->board_base;
   Place_BeforeLayout(app_state, ui);
   Caret_BeforeLayout(app_state);
   _Edit_LayoutPreview(app_state, layout->page, ui);
   // A board goes to where the cursor is, and is laid out again there.
   if (layout->is_board && _Edit_BoardFollow(app_state, ui)) {
      layout->page = _Edit_BoardPage(layout->board_base);
      _Edit_LayoutPreview(app_state, layout->page, ui);
   }

   //- bgz: 3: the boxes that are clicked. What a click does to the text or
   // the cursor is an op, run at the start of the next frame; changed is for
   // what is the preview's own - a block's scroll.
   // A board's pieces can be anywhere: their boxes are kept to what of
   // the board shows, off the text and the small slides.
   if (layout->is_board) {
      FR_BeginBuildClip(layout->board);
   }
   B32         changed = 0;
   Place_Hover hover = {0};
   hover.hit_idx = -1;
   changed |= Caret_BuildBars(app_state, ui, interactive);
   changed |= Place_BuildCorners(app_state, ui, interactive, &hover);
   for (U64 picture_idx = 0; picture_idx < DECK_MAX_PICTURES && interactive; picture_idx += 1) {
      Rng2F32    video_rect = {0};
      Video_Video* video = _Edit_PreviewVideo(app_state, layout->page, picture_idx, &video_rect);
      // Over its strip is over its picture, for its outline.
      if (video != 0 && SlideVideo_BuildStripHits(app_state, video, video_rect, ui, picture_idx) && !hover.is_over) {
         hover.is_over = 1;
         hover.has_piece = 1;
         hover.kind = Deck_PlaceKind_Picture;
         hover.idx = picture_idx;
      }
   }
   changed |= Place_BuildHits(app_state, ui, interactive, &hover);
   changed |= Caret_BuildRooms(app_state, ui, interactive);

   //- bgz: the slide's open space, under all of those: a press there and a
   // drag draw a box that picks every piece it touches (Edit_PickRect) -
   // or, with Ctrl, a text box as large as the mouse makes it
   // (Edit_AddTextBox) - and a click with no drag lets go of what is selected. Built after
   // them, so it is asked last: only where nothing else is. Built while a
   // box is drawn whatever else goes on, or the UI lets go of it.
   B32 open_space_hot = 0;
   if (interactive && ((Place_BoxesAreGood(app_state) && g_place_view.drag.state == Place_DragState_None) ||
                       g_edit_view.is_drawing)) {
      Rng2F32   page = g_edit_view.page;
      // A board's is all that shows of it: past the page too.
      Rng2F32   open_rect = g_edit_view.board;
      UI_Box*   space = FR_BuildRectBox(UI_BoxFlag_MouseClickable, open_rect, "###edit open space");
      space->hover_cursor = WM_Cursor_Pointer;
      UI_Signal signal = UI_SignalFromBox(space);
      open_space_hot = UI_Hovering(signal) && !hover.is_over;
      Vec2F32 mouse = Core_WM_MouseFromWindow(app_state->window);
      if ((signal.f & UI_SignalFlag_RightPressed) && !g_edit_view.is_drawing) {
         Edit_OpenMenu(app_state, mouse, 0, Deck_PlaceKind_Title, 0);
      }
      if (signal.f & UI_SignalFlag_LeftPressed) {
         g_edit_view.is_drawing = 1;
         g_edit_view.draw_from = UI_DragStartMouse();
         g_edit_view.draw_is_box = 0;
         // The box drawn picks what it touches, and makes nothing. With
         // Ctrl it is a text box.
         g_edit_view.draw_picks = !(signal.event_flags & WM_Modifier_Ctrl);
         g_edit_view.draw_is_double = !!(signal.f & UI_SignalFlag_LeftDoubleClicked);
      }
      if (g_edit_view.is_drawing) {
         F32 gone_x = mouse.x - g_edit_view.draw_from.x;
         F32 gone_y = mouse.y - g_edit_view.draw_from.y;
         F32 reach = _EDIT_DRAW_START * ui;
         if (gone_x > reach || gone_x < -reach || gone_y > reach || gone_y < -reach) {
            g_edit_view.draw_is_box = 1;
         }
         g_edit_view.draw_to = V2F32(Clamp(open_rect.min.x, mouse.x, open_rect.max.x), Clamp(open_rect.min.y, mouse.y, open_rect.max.y));
      }
      if (g_edit_view.is_drawing && (signal.f & UI_SignalFlag_LeftReleased)) {
         g_edit_view.is_drawing = 0;
         Vec2F32 page_dim = Dim2F32(page);
         if (!g_edit_view.draw_is_box && g_edit_view.draw_is_double && page_dim.x > 0 && page_dim.y > 0) {
            // A double click: a text box there, its first line at the click.
            F32 x = (g_edit_view.draw_from.x - page.min.x) / page_dim.x;
            F32 y = (g_edit_view.draw_from.y - page.min.y) / page_dim.y - 0.02f;
            StateOp_Edit_AddTextBox(ops)->rect = R2F32P(x, y, x + _EDIT_CLICK_BOX_W, y + _EDIT_CLICK_BOX_H);
         } else if (!g_edit_view.draw_is_box) {
            // A click: nothing selected. Not with Ctrl, which adds to what
            // is picked and so takes nothing away.
            if (g_edit_view.draw_picks) {
               StateOp_Edit_Deselect(ops);
            }
         } else if (g_edit_view.draw_picks) {
            Vec2F32 a = g_edit_view.draw_from;
            Vec2F32 b = g_edit_view.draw_to;
            Edit_PickRectOpParams* pick = StateOp_Edit_PickRect(ops);
            pick->rect = R2F32P(Min(a.x, b.x), Min(a.y, b.y), Max(a.x, b.x), Max(a.y, b.y));
            pick->adds = Core_WM_KeyIsDown(WM_Key_Shift);
         } else if (page_dim.x > 0 && page_dim.y > 0) {
            Vec2F32 a = g_edit_view.draw_from;
            Vec2F32 b = g_edit_view.draw_to;
            StateOp_Edit_AddTextBox(ops)->rect = R2F32P((Min(a.x, b.x) - page.min.x) / page_dim.x, (Min(a.y, b.y) - page.min.y) / page_dim.y,
                                                        (Max(a.x, b.x) - page.min.x) / page_dim.x, (Max(a.y, b.y) - page.min.y) / page_dim.y);
         }
      }
   } else {
      g_edit_view.is_drawing = 0;
   }
   FR_EndBuildClip();

   //- bgz: the row of small slides - a press goes to the slide - and the +
   // that adds one after the slide showing
   S64 thumb_hot = -1;
   B32 add_hot = 0;
   if (interactive) {
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         Rng2F32 rect = _Edit_ThumbRect(layout, deck->slide_count, app_state->slide_idx, i);
         if (rect.max.x > rect.min.x) {
            UI_Signal signal = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, rect, "###edit thumb %llu", (unsigned long long)i));
            if (UI_Hovering(signal)) {
               thumb_hot = (S64)i;
            }
            if (signal.f & UI_SignalFlag_LeftPressed) {
               StateOp_Edit_GoToSlide(ops)->slide_idx = i;
            }
         }
      }
      if (layout->add_slide.max.x > layout->add_slide.min.x) {
         UI_Signal signal = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, layout->add_slide, "###edit add slide"));
         add_hot = UI_Hovering(signal);
         if (signal.f & UI_SignalFlag_LeftPressed) {
            StateOp_Edit_AddSlide(ops);
         }
      }

      //- bgz: the preview around the slide, under all of the above: a click
      // there lets go of what is selected, as one on the slide's open space
      UI_Signal around = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, layout->preview, "###edit preview around"));
      if ((around.f & UI_SignalFlag_LeftPressed) && !g_edit_view.is_drawing) {
         StateOp_Edit_Deselect(ops);
      }
   }

   //- bgz: 4: laid out again, for what they did
   if (changed) {
      _Edit_LayoutPreview(app_state, layout->page, ui);
   }

   //- bgz: 5: the boxes that are drawn
   String8 caption = {0};
   if (layout->is_board) {
      FR_BeginBuildClip(layout->board);
   }
   if (app_state->slide_idx < deck->slide_count && Dim2F32(layout->page).x > 0) {
      // The box the mouse is over - a picture's, the title's, a block's -
      // with its corners, and what can be done with it said under the slide.
      B32 can_move = Place_BuildLook(app_state, ui, &hover);

      // The slide's videos: each its strip of controls, over the bottom of it.
      for (U64 picture_idx = 0; picture_idx < DECK_MAX_PICTURES; picture_idx += 1) {
         Rng2F32    video_rect = {0};
         Video_Video* video = _Edit_PreviewVideo(app_state, layout->page, picture_idx, &video_rect);
         if (video != 0) {
            SlideVideo_BuildStripLook(app_state, video, video_rect, ui, picture_idx, interactive);
         }
      }

      // Lit under the mouse - a wash of the theme's body colour, which shows
      // on a light theme as on a dark one: the one piece a click there would
      // go to.
      if (interactive && hover.hit_idx >= 0 && (U64)hover.hit_idx < g_edit_view.hit_count) {
         Vec4F32 lit = View_ColorsFromTheme(deck->theme).body;
         lit.w = 0.12f;
         UI_SetNextBackgroundColor(lit);
         FR_BuildRectBox(UI_BoxFlag_DrawBackground, g_edit_view.hits[hover.hit_idx].rect, "###edit preview lit");
      }

      // The words: the caret, and the box of the piece it is in
      // (caret\caret.c). The box built below is then a picture's.
      B32 words_have_box = Caret_BuildLook(app_state, ui);

      // A box around the picture the cursor is on the line of. Not after
      // Esc, until the cursor goes somewhere or a piece is clicked. In the
      // words' box's blue (frame.c's "focus").
      //
      // Anywhere on the line, not only in the file's name: a picture just
      // dropped or picked has the cursor at its line's end, past the ")".
      String8 text = Editor_Text(editor);
      U64     line_start = Min(editor->cursor, text.size);
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 line_end = Min(editor->cursor, text.size);
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
      for (U64 i = 0; i < g_edit_view.hit_count && !words_have_box && !g_caret_view.focus_hidden; i += 1) {
         View_Hit* hit = &g_edit_view.hits[i];
         B32       is_on_line = (hit->text_range.min >= line_start && hit->text_range.min <= line_end);
         if (is_on_line || (editor->cursor >= hit->text_range.min && editor->cursor <= hit->text_range.max)) {
            UI_ColorTag(Str8Lit("focus")) {
               FR_BuildEdge(hit->rect, Max(1.0f, (F32)(S32)(1.5f * ui)), UI_ColorFromSlot(UI_ColorSlot_Border), "###edit preview focus %llu",
                            (unsigned long long)i);
            }
         }
      }

      // A text box being drawn: its outline, where the mouse has it. One
      // that picks what it touches (Ctrl) is washed as well.
      if (g_edit_view.is_drawing && g_edit_view.draw_is_box) {
         Vec2F32 a = g_edit_view.draw_from;
         Vec2F32 b = g_edit_view.draw_to;
         UI_ColorTag(Str8Lit("focus")) {
            if (g_edit_view.draw_picks) {
               Vec4F32 wash = UI_ColorFromSlot(UI_ColorSlot_Border);
               wash.w = 0.15f;
               UI_SetNextBackgroundColor(wash);
               FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(Min(a.x, b.x), Min(a.y, b.y), Max(a.x, b.x), Max(a.y, b.y)),
                               "###edit drawn wash");
            }
            FR_BuildEdge(R2F32P(Min(a.x, b.x), Min(a.y, b.y), Max(a.x, b.x), Max(a.y, b.y)), Max(1.0f, (F32)(S32)(1.5f * ui)),
                         UI_ColorFromSlot(UI_ColorSlot_Border), "###edit drawn box");
         }
      }

      // A piece picked whole: its corners, in the same blue, as a
      // presentation program shows a box picked - and what Delete does is
      // said under the slide.
      B32 has_picked = 0;
      for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
         View_Box* box = &g_edit_view.boxes[i];
         if (Edit_PieceIsPicked(app_state, box->kind, box->idx) && Place_BoxesAreGood(app_state)) {
            has_picked = 1;
            F32 half = (F32)(S32)(4.0f * ui);
            F32 edge = Max(1.0f, (F32)(S32)(1.5f * ui));
            Vec2F32 corners[4] = {box->rect.min, V2F32(box->rect.max.x, box->rect.min.y), V2F32(box->rect.min.x, box->rect.max.y),
                                  box->rect.max};
            UI_ColorTag(Str8Lit("focus")) {
               FR_BuildEdge(box->rect, edge, UI_ColorFromSlot(UI_ColorSlot_Border), "###edit picked edge %llu", (unsigned long long)i);
               for (U64 k = 0; k < ArrayCount(corners); k += 1) {
                  UI_SetNextBackgroundColor(UI_ColorFromSlot(UI_ColorSlot_Border));
                  FR_BuildRectBox(UI_BoxFlag_DrawBackground,
                                  R2F32P(corners[k].x - half, corners[k].y - half, corners[k].x + half, corners[k].y + half),
                                  "###edit picked corner %llu %llu", (unsigned long long)i, (unsigned long long)k);
               }
            }
         }
      }

      // Right at the words a drag selects them (place\place.c): said there,
      // and while it does, instead of how the box is moved.
      B32 is_at_words = (g_place_view.drag.state == Place_DragState_Selecting ||
                         (can_move && hover.has_piece && hover.corner == 0 &&
                          Place_IsAtWords(hover.kind, hover.idx, Core_WM_MouseFromWindow(app_state->window))));
      if (g_edit_view.is_drawing && g_edit_view.draw_is_box) {
         caption = g_edit_view.draw_picks ? Str8Lit("Let go to select what the box touches") : Str8Lit("Let go to make the text box");
      } else if (is_at_words) {
         caption = Str8Lit("Drag across the words to select them      Right-click what is selected to colour or size it");
      } else if (has_picked && !can_move) {
         caption = layout->is_board ? Str8Lit("Ctrl+Shift+N moves it to a slide of its own      Delete takes it out      Esc lets go of it")
                                    : Str8Lit("Delete takes it out      Click its words to type in them      Esc lets go of it");
      } else if (can_move && has_picked && hover.has_piece && Edit_PieceIsPicked(app_state, hover.kind, hover.idx)) {
         caption = Str8Lit("Drag to move it      A handle resizes it      Delete takes it out      Right-click for more");
      } else if (open_space_hot && layout->is_board) {
         caption = Str8Lit("A board      Double-click for a text box      Drag selects      Wheel moves it, Ctrl+wheel zooms, the middle button drags it");
      } else if (open_space_hot) {
         caption = Str8Lit("Double-click for a text box      Drag selects what the box touches      Ctrl+drag draws a text box");
      } else if (can_move) {
         caption = Str8Lit("Drag to move it      Shift lines it up      A handle resizes it      Right-click for more");
      } else if (layout->is_board && !open_space_hot && !can_move) {
         caption = Str8Lit("A board: not shown when presenting      Select pieces, then Ctrl+Shift+N makes a slide of them");
      } else if (deck->slides[app_state->slide_idx].pictures_passed_over != 0) {
         // A picture line typed past the most a slide can have: not on the
         // slide, which should not go unsaid.
         caption = PushStr8F(UI_BuildArena(), "Slide %llu of %llu      Only the first %d pictures are shown",
                             (unsigned long long)(app_state->slide_idx + 1), (unsigned long long)deck->slide_count, DECK_MAX_PICTURES);
      } else {
         caption = PushStr8F(UI_BuildArena(), "Slide %llu of %llu      Alt+Left / Alt+Right moves it",
                             (unsigned long long)(app_state->slide_idx + 1), (unsigned long long)deck->slide_count);
      }
   }

   FR_EndBuildClip();

   //- bgz: the caption, under the slide
   if (caption.size != 0) {
      UI_SetNextColorTag(Str8Lit("dim"));
      UI_SetNextTextAlignment(UI_TextAlign_Center);
      UI_SetNextFontSize(FR_UIFontSize(layout->ui_px));
      UI_Box* box = FR_BuildRectBox(UI_BoxFlag_DrawText, layout->caption, "###edit caption");
      UI_BoxEquipDisplayString(box, caption);
   }

   //- bgz: the row of small slides, the one showing with a frame around it,
   // and one under the mouse with a fainter one
   UI_ColorTag(Str8Lit("thumb")) for (U64 i = 0; i < deck->slide_count; i += 1) {
      Rng2F32 rect = _Edit_ThumbRect(layout, deck->slide_count, app_state->slide_idx, i);
      if (rect.max.x > rect.min.x) {
         _Edit_Thumb* thumb = PushArray(UI_BuildArena(), _Edit_Thumb, 1);
         thumb->app_state = app_state;
         thumb->slide_idx = i;
         thumb->rect = rect;
         thumb->frame = (F32)(S32)(2.0f * ui);
         thumb->has_frame = (i == app_state->slide_idx || (S64)i == thumb_hot);
         UI_Box* box = FR_BuildRectBox(0, rect, "###edit thumb look %llu", (unsigned long long)i);
         box->border_color = (i == app_state->slide_idx) ? UI_ColorFromSlot(UI_ColorSlot_Border) : UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Border);
         UI_BoxEquipCustomDraw(box, _Edit_DrawThumb, thumb);
      }
   }

   //- bgz: the + that adds a slide
   if (layout->add_slide.max.x > layout->add_slide.min.x) {
      _Edit_Plus* plus = PushArray(UI_BuildArena(), _Edit_Plus, 1);
      plus->rect = layout->add_slide;
      plus->half = Max(1.0f, (F32)(S32)(1.5f * ui));
      UI_Box* box = FR_BuildRectBox(0, layout->add_slide, "###edit add slide look");
      box->background_color = UI_ThemeColor(add_hot ? Str8Lit("add hot") : Str8Lit("add"), UI_ColorSlot_Background);
      box->text_color = UI_ThemeColor(add_hot ? Str8Lit("add hot") : Str8Lit("add"), UI_ColorSlot_Text);
      UI_BoxEquipCustomDraw(box, _Edit_DrawPlus, plus);
   }

   //- bgz: the slide, under all that is over it
   if (Dim2F32(layout->page).x > 0) {
      if (deck->slide_count == 0) {
         // No slides yet: how to make the first.
         String8 lines[] = {
             Str8Lit("No slides yet"),
             Str8Lit("Type  # Title  to start one, or click a line under Format"),
         };
         F32     scale = View_ScaleFromDim(Dim2F32(layout->page));
         F32     sizes[] = {44.0f * scale, 24.0f * scale};
         Vec2F32 page_dim = Dim2F32(layout->page);
         F32     y = layout->page.min.y + page_dim.y * 0.5f - Text_LineHeight(app_state->fonts.bold, sizes[0]);
         for (U64 i = 0; i < ArrayCount(lines); i += 1) {
            FNT_Tag font = (i == 0) ? app_state->fonts.bold : app_state->fonts.regular;
            F32     line_h = Text_LineHeight(font, sizes[i]);
            UI_SetNextFont(font);
            UI_SetNextFontSize(FR_UIFontSize(sizes[i]));
            UI_SetNextTextAlignment(UI_TextAlign_Center);
            UI_SetNextColorTag((i == 0) ? Str8Lit("") : Str8Lit("dim"));
            UI_Box* box = FR_BuildRectBox(UI_BoxFlag_DrawText, R2F32P(layout->page.min.x, y, layout->page.max.x, y + line_h),
                                          "###edit no slides %llu", (unsigned long long)i);
            UI_BoxEquipDisplayString(box, lines[i]);
            y += line_h * 1.3f;
         }
      }

      _Edit_Slide* slide = PushArray(UI_BuildArena(), _Edit_Slide, 1);
      slide->app_state = app_state;
      slide->page = layout->page;
      slide->board = layout->board;
      slide->is_board = layout->is_board;
      slide->edge = Max(1.0f, (F32)(S32)(1.0f * ui));
      UI_Box* box = FR_BuildRectBox(0, layout->board, "###edit slide");
      UI_BoxEquipCustomDraw(box, _Edit_DrawSlide, slide);
   }

   //- bgz: the preview's background, with a line where the text ends and
   // the preview begins
   UI_SetNextColorTag(Str8Lit("divider"));
   FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(layout->preview.min.x, 0, layout->preview.min.x + 1.0f, layout->preview.max.y),
                   "###edit preview divider");
   UI_SetNextColorTag(Str8Lit("preview"));
   FR_BuildRectBox(UI_BoxFlag_DrawBackground, layout->preview, "###edit preview");
}

typedef struct _Edit_Pane _Edit_Pane;
struct _Edit_Pane {
   Editor_Editor* editor;
   Arena*     arena; // the frame's, for the rows it lays out
   Editor_Box     box;
};

// The part of the text that is the block of words the cursor is in - a
// slide's body, or what an @box line begins, to the next - for the editor
// to draw a line around: what is typed there is in that block, and an @size
// or @color line there is that block's. Its lines with something in them,
// and the cursor's. Zero when the cursor is in no block: on a title, or in
// a deck with no slide.
PRIVATE Rng1U64
_Edit_BlockRange(const App_State* app_state) {
   Rng1U64        result = {0};
   Editor_Editor* editor = app_state->editor;
   Deck_Deck*     deck = app_state->deck;
   if (editor == 0 || deck == 0 || deck->slide_count == 0 || app_state->parsed_edit_count != editor->edit_count) {
      return result;
   }
   String8     text = Editor_Text(editor);
   U64         cursor = editor->cursor;
   U64         slide_idx = Deck_SlideFromTextPos(deck, cursor);
   Deck_Slide* slide = &deck->slides[slide_idx];
   U64         slide_end = (slide_idx + 1 < deck->slide_count) ? Min(deck->slides[slide_idx + 1].text_pos, text.size) : text.size;

   for (U64 i = 0; i < slide->box_count; i += 1) {
      U64 start = Min(slide->boxes[i].text_pos, text.size);
      U64 end = (i + 1 < slide->box_count) ? Min(slide->boxes[i + 1].text_pos, text.size) : slide_end;
      if (i == 0) {
         // The slide's first line - its title, or --- or @slide - is not the
         // body's, nor is anything down to the title's line.
         if (slide->title_range.max != 0) {
            start = Clamp(start, slide->title_range.max, end);
         }
         while (start < end && text.str[start] != '\n') start += 1;
         if (start < end) start += 1;
      }
      if (cursor < start || cursor >= end + (end == text.size ? 1 : 0)) {
         continue;
      }

      // Without the empty lines at its ends - but with the cursor's line,
      // empty or not: a line begun under an @box is in the block already.
      U64 min = start;
      U64 max = end;
      while (min < max && (text.str[min] == '\n' || text.str[min] == ' ')) min += 1;
      while (max > min && (text.str[max - 1] == '\n' || text.str[max - 1] == ' ')) max -= 1;
      U64 line_start = Editor_LineStart(editor, cursor);
      U64 line_end = Editor_LineEnd(editor, cursor);
      if (min >= max) {
         min = line_start;
         max = line_end;
      }
      min = Editor_LineStart(editor, Min(min, line_start));
      max = Max(max, line_end);
      // Something to be a row, for a line with nothing in it yet.
      result = R1U64(min, Max(max, min + 1));
      break;
   }
   return result;
}

// The text, as the editor draws it.
PRIVATE void
_Edit_DrawPane(UI_Box* box, void* user_data) {
   UnusedVariable(box);
   _Edit_Pane* pane = (_Edit_Pane*)user_data;
   Editor_Draw(pane->editor, pane->arena, pane->box);
}

void
Edit_Build(const App_State* app_state, StateOp_List* ops, Vec2F32 window_dim, B32 interactive) {
   Editor_Editor*    editor = app_state->editor;
   const Text_Fonts* fonts = &app_state->fonts;
   _Edit_Layout layout = _Edit_LayoutFromWindowDim(app_state, window_dim);
   F32          ui = layout.ui;
   F32          pad = 12.0f * ui;
   F32          left_w = Dim2F32(layout.status).x;
   B32          shows_pictures = (app_state->show_format && app_state->show_pictures);
   Temp         scratch = ScratchBegin(0, 0);

   // What a click asked for, pushed as ops once everything is built.
   B32 toggle_format = 0;
   S64 format_picked = -1;
   S64 picture_picked = -1;
   B32 size_picked = 0;
   S32 size_direction = 0; // as Look_StepSize's
   S64 color_picked = -1;  // as Look_SetColor's named_idx
   S64 align_picked = -1;  // as Look_SetAlign's align

   UI_BoxFlags click_flags = interactive ? UI_BoxFlag_Clickable : 0;

   B32 shows_text = !app_state->hide_text;

   //- bgz: the list under an @ line being typed (complete\complete.c).
   // Built first: over everything.
   Complete_Build(app_state, layout.editor_box, window_dim, ui, layout.ui_px);

   //- bgz: the dividers: a strip over the edge between the left side and
   // the preview, and one over the Format panel's top edge, each dragged to
   // give one side more room and the other less. Built first: over what is
   // under them, and asked first. A double click puts one back where the
   // layout has it itself. They are the screen's own, written with no op.
   if (shows_text && interactive) {
      F32     grip = _EDIT_SPLIT_GRIP * ui;
      Vec2F32 mouse = Core_WM_MouseFromWindow(app_state->window);

      {
         F32     x = layout.preview.min.x;
         UI_Box* box = FR_BuildRectBox(UI_BoxFlag_MouseClickable, R2F32P(x - grip * 0.5f, 0, x + grip * 0.5f, window_dim.y), "###edit split x");
         box->hover_cursor = WM_Cursor_LeftRight;
         UI_Signal signal = UI_SignalFromBox(box);
         B32       is_held = UI_Dragging(signal);
         if (UI_DoubleClicked(signal)) {
            g_edit_view.split_x = 0;
         } else if (is_held && window_dim.x > 0) {
            g_edit_view.split_x = Clamp(0.01f, mouse.x / window_dim.x, 0.99f);
         }
         if (is_held || UI_Hovering(signal)) {
            // A line down the edge, to show what is held.
            UI_Box* line = FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(x - 1.0f, 0, x + 1.0f, window_dim.y), "###edit split x line");
            line->background_color = V4F32(0.35f, 0.60f, 1.0f, is_held ? 0.9f : 0.5f);
         }
      }

      if (app_state->show_format) {
         F32     y = layout.format_header.min.y;
         F32     right = layout.preview.min.x - grip * 0.5f;
         UI_Box* box = FR_BuildRectBox(UI_BoxFlag_MouseClickable, R2F32P(0, y - grip * 0.5f, right, y + grip * 0.5f), "###edit split format");
         box->hover_cursor = WM_Cursor_UpDown;
         UI_Signal signal = UI_SignalFromBox(box);
         B32       is_held = UI_Dragging(signal);
         if (UI_DoubleClicked(signal)) {
            g_edit_view.split_format = 0;
         } else if (is_held && window_dim.y > 0) {
            // The mouse is at the header's top: the lines have what is
            // from under the header to the status line.
            F32 rows_h = layout.status.min.y - Dim2F32(layout.format_header).y - mouse.y;
            g_edit_view.split_format = Clamp(0.01f, rows_h / window_dim.y, 0.99f);
         }
         if (is_held || UI_Hovering(signal)) {
            UI_Box* line = FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(0, y - 1.0f, right, y + 1.0f), "###edit split format line");
            line->background_color = V4F32(0.35f, 0.60f, 1.0f, is_held ? 0.9f : 0.5f);
         }
      }
   }

   //- bgz: the text: a box the editor draws in, whose mouse is the editor's
   // - a press, a drag, the wheel. Its keys are Edit_Input's.
   if (shows_text) {
      _Edit_Pane* pane = PushArray(UI_BuildArena(), _Edit_Pane, 1);
      pane->editor = editor;
      pane->arena = app_state->frame_arena;
      pane->box = layout.editor_box;
      pane->box.frame = _Edit_BlockRange(app_state);
      UI_Box* box = FR_BuildRectBox(interactive ? (UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll) : 0, layout.editor_box.rect, "###edit text");
      UI_BoxEquipCustomDraw(box, _Edit_DrawPane, pane);
      box->hover_cursor = WM_Cursor_IBar; // the bar that text is typed at
      if (interactive) {
         UI_Signal signal = UI_SignalFromBox(box);
         if (signal.f & UI_SignalFlag_LeftPressed) {
            Edit_TextPressOpParams* press = StateOp_Edit_TextPress(ops);
            press->box = layout.editor_box;
            press->pos = UI_DragStartMouse();
            press->keeps_mark = !!(signal.event_flags & WM_Modifier_Shift);
         } else if (UI_Dragging(signal)) {
            // Over the box or not, until the button comes up. The window's
            // mouse: the UI's is off the window when the window is not in
            // front and the mouse has not moved for a moment.
            Edit_TextDragOpParams* drag = StateOp_Edit_TextDrag(ops);
            drag->box = layout.editor_box;
            drag->pos = Core_WM_MouseFromWindow(app_state->window);
         }
         if (signal.scroll.y != 0) {
            Edit_TextWheelOpParams* wheel = StateOp_Edit_TextWheel(ops);
            wheel->box = layout.editor_box;
            wheel->notches = (F32)signal.scroll.y / FR_UI_SCROLL_PER_NOTCH;
         }
      }
   }

   UI_Box* column = 0;
   if (shows_text) {
      UI_SetNextRect(R2F32P(0, layout.format_header.min.y, left_w, layout.status.max.y));
      UI_SetNextChildLayoutAxis(Axis2_Y);
      UI_SetNextColorTag(Str8Lit("panel"));
      column = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clip, "###edit format column");
   }

   //- bgz: with the left side away, a notice is over the top of the
   // preview, where there is room above the slide
   if (!shows_text) {
      String8 notice = FR_CurrentNotice(app_state);
      if (notice.size != 0) {
         F32 notice_w = Text_Width(fonts->regular, layout.ui_px, notice);
         FR_BuildWords(fonts->regular, layout.ui_px, V2F32((window_dim.x - notice_w) * 0.5f, 6.0f * ui),
                       UI_ThemeColor(Str8Lit("notice"), UI_ColorSlot_Text), notice, "###edit notice");
      }
   }

   if (shows_text) UI_Parent(column) UI_FontSize(FR_UIFontSize(layout.ui_px)) {
      //- bgz: the header
      {
         String8 left = Str8Lit("Format: what can be written (click or F1 to open)");
         String8 right = Str8Lit("F1");
         if (shows_pictures) {
            left = Str8Lit("Pictures: click one to put it on this slide");
            right = Str8Lit("Esc closes");
         } else if (app_state->show_format) {
            left = Str8Lit("Format: click a line to write it into the text");
            right = Str8Lit("F1 closes");
         }

         UI_SetNextPrefWidth(UI_Px(left_w, 1));
         UI_SetNextPrefHeight(UI_Px(Dim2F32(layout.format_header).y, 1));
         UI_SetNextChildLayoutAxis(Axis2_X);
         UI_SetNextColorTag(Str8Lit("header"));
         UI_Box* header = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clip | click_flags,
                                                 "###edit format header");
         UI_Parent(header) {
            _Edit_BuildBarText(pad, Str8Lit(""), left, fonts->bold, right);
         }
         if (interactive) {
            UI_Signal signal = UI_SignalFromBox(header);
            if (UI_Hovering(signal)) {
               // Asked by its own tag: the tags pushed here are the column's.
               header->background_color = UI_ThemeColor(Str8Lit("header hot"), UI_ColorSlot_Background);
            }
            toggle_format = UI_Clicked(signal);
         }
      }

      //- bgz: the rows: the format's lines, or the list of pictures
      if (app_state->show_format) {
         F32 rows_h = Dim2F32(layout.format_rows).y;
         F32 row_h = layout.format_row_h;
         F32 edge = 4.0f * ui; // above the first row and below the last

         UI_SetNextPrefWidth(UI_Px(left_w, 1));
         UI_SetNextPrefHeight(UI_Px(rows_h, 1));
         UI_SetNextChildLayoutAxis(Axis2_Y);
         UI_Box* rows_box = UI_BuildBoxFromStringf(UI_BoxFlag_Clip, "###edit format rows");

         UI_Parent(rows_box) UI_ColorTag(Str8Lit("row")) {
            UI_Spacer(UI_Px(edge, 1));

            if (shows_pictures && g_edit_view.pictures.count == 0) {
               UI_SetNextPrefWidth(UI_Px(left_w, 1));
               UI_SetNextPrefHeight(UI_Px(row_h, 1));
               UI_Row {
                  _Edit_BuildBarText(pad, Str8Lit("dim"),
                                     Str8Lit("No pictures in the deck's folder yet. Drop one on the window to add it."),
                                     UI_TopFont(), Str8Zero());
               }
            } else if (shows_pictures) {
               //- bgz: the names of the folder's pictures, however many there are
               UI_ScrollRowsFlags scroll_flags = UI_ScrollRowsFlag_RowClicks;
               UI_ScrollRowsFrame frame = UI_ScrollRowsBegin(&g_edit_view.pictures_scroll,
                                                             V2F32(left_w, Max(rows_h - edge * 2.0f, row_h)),
                                                             (S64)g_edit_view.pictures.count, row_h, 0, scroll_flags,
                                                             "###edit pictures");
               UI_ScrollRowsEnd(&g_edit_view.pictures_scroll, &frame, 0, scroll_flags);
               UI_Parent(frame.rows_box) {
                  for (S64 i = frame.first_row; i < frame.last_row; i += 1) {
                     UI_Box* row = UI_ListRow(row_h, 0, UI_BoxFlag_DrawBackground | click_flags, "###edit picture %lld", i);
                     UI_Parent(row) {
                        _Edit_BuildBarText(pad, Str8Lit("picture"), g_edit_view.pictures.v[i], fonts->mono, Str8Zero());
                     }
                     if (interactive) {
                        UI_Signal signal = UI_SignalFromBox(row);
                        if (UI_Hovering(signal)) {
                           row->background_color = UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Background);
                        }
                        if (UI_Clicked(signal)) {
                           picture_picked = i;
                        }
                     }
                  }
               }
            } else {
               //- bgz: the format's lines. What each does starts where the
               // longest way of writing one ends.
               F32 syntax_w = 0;
               for (U64 i = 0; i < ArrayCount(_edit_format_rows); i += 1) {
                  syntax_w = Max(syntax_w, Text_Width(fonts->mono, layout.ui_px, _edit_format_rows[i].syntax));
               }

               // A list that scrolls, for a short window, where they do not
               // all fit (_Edit_LayoutFromWindowDim). With room for all, it
               // has no bar and does not move.
               UI_ScrollRowsFlags scroll_flags = UI_ScrollRowsFlag_RowClicks;
               UI_ScrollRowsFrame frame = UI_ScrollRowsBegin(&g_edit_view.format_scroll,
                                                             V2F32(left_w, Max(rows_h - edge * 2.0f, row_h)),
                                                             (S64)ArrayCount(_edit_format_rows), row_h, 0, scroll_flags,
                                                             "###edit format lines");
               UI_ScrollRowsEnd(&g_edit_view.format_scroll, &frame, 0, scroll_flags);

               UI_Parent(frame.rows_box) for (S64 i = frame.first_row; i < frame.last_row; i += 1) {
                  _Edit_FormatRow* format_row = &_edit_format_rows[i];

                  // The theme's and the layout's rows say what there is now.
                  // On the frame arena: a label's words are needed until the
                  // frame is drawn.
                  String8 what = format_row->what;
                  if (format_row->action == _Edit_FormatAction_Theme) {
                     what = PushStr8F(app_state->frame_arena, "the deck's colours: %S. Click for the next",
                                      Deck_ThemeName(app_state->deck->theme));
                  } else if (format_row->action == _Edit_FormatAction_Layout) {
                     Deck_Deck* deck = app_state->deck;
                     String8    now = (app_state->slide_idx < deck->slide_count)
                                          ? Deck_LayoutName(deck->slides[app_state->slide_idx].layout)
                                          : Deck_LayoutName(Deck_Layout_PictureRight);
                     what = PushStr8F(app_state->frame_arena, "this slide's layout: %S. Click for the next", now);
                  } else if (format_row->action == _Edit_FormatAction_Size) {
                     what = Look_Describe(app_state->frame_arena, app_state, 0);
                  } else if (format_row->action == _Edit_FormatAction_Color) {
                     what = Look_Describe(app_state->frame_arena, app_state, 1);
                  } else if (format_row->action == _Edit_FormatAction_Align) {
                     what = Look_DescribeAlign(app_state->frame_arena, app_state);
                  }
                  UI_Box* row = UI_ListRow(row_h, 0, UI_BoxFlag_DrawBackground | click_flags, "###edit format row %lld", i);
                  UI_Parent(row) UI_PrefHeight(UI_Pct(1, 1)) {
                     UI_Spacer(UI_Px(pad, 1));
                     UI_SetNextPrefWidth(UI_Px(syntax_w, 1));
                     UI_SetNextFont(fonts->mono);
                     UI_SetNextColorTag(Str8Lit("syntax"));
                     UI_Label(format_row->syntax);
                     UI_Spacer(UI_Px(18.0f * ui, 1));
                     UI_SetNextPrefWidth(UI_Pct(1, 0));
                     UI_SetNextColorTag(Str8Lit("dim"));
                     UI_Label(what);

                     //- bgz: the size's and the colour's rows: what there is
                     // to click, at the row's right end. Their clicks are
                     // taken here, before the row's own, which is under them.
                     if (format_row->action == _Edit_FormatAction_Size) {
                        String8 names[] = {Str8Lit("smaller"), Str8Lit("larger"), Str8Lit("usual")};
                        S32     directions[] = {-1, +1, 0};
                        B32     is_usual = Look_SizeIsUsual(app_state);
                        UI_Spacer(UI_Px(6.0f * ui, 1));
                        for (U64 k = 0; k < ArrayCount(names); k += 1) {
                           if (_Edit_BuildRowButton(names[k], is_usual && directions[k] == 0, row_h, ui, interactive,
                                                    "###edit size %llu", (unsigned long long)k)) {
                              size_picked = 1;
                              size_direction = directions[k];
                           }
                        }
                        UI_Spacer(UI_Px(pad - 6.0f * ui, 1));
                     } else if (format_row->action == _Edit_FormatAction_Color) {
                        // The squares as large as the row leaves room for,
                        // so that all of them and the theme's fit in a narrow
                        // window too: the row's width less what is before
                        // them, the theme's word and the room at the end.
                        F32 theme_w = Text_Width(fonts->regular, layout.ui_px, Str8Lit("theme's")) + 14.0f * ui + 6.0f * ui;
                        F32 room = left_w - pad - syntax_w - 18.0f * ui - 6.0f * ui - theme_w - pad;
                        F32 each = room / (F32)DECK_NAMED_COLOR_COUNT; // a square and the gap after it
                        F32 gap = Clamp(2.0f * ui, (F32)(S32)(each * 0.25f), 6.0f * ui);
                        F32 side = (F32)(S32)Clamp(6.0f * ui, each - gap, row_h * 0.62f);
                        UI_Spacer(UI_Px(6.0f * ui, 1));
                        for (U64 k = 0; k < DECK_NAMED_COLOR_COUNT; k += 1) {
                           if (_Edit_BuildSwatch(Deck_NamedColorFromIdx(k).color, Look_ColorIsNow(app_state, k), side, gap, row_h,
                                                 ui, interactive, "###edit color %llu", (unsigned long long)k)) {
                              color_picked = (S64)k;
                           }
                        }
                        if (_Edit_BuildRowButton(Str8Lit("theme's"), Look_ColorIsNow(app_state, DECK_NAMED_COLOR_COUNT), row_h,
                                                 ui, interactive, "###edit color theme")) {
                           color_picked = DECK_NAMED_COLOR_COUNT;
                        }
                        UI_Spacer(UI_Px(pad - 6.0f * ui, 1));
                     } else if (format_row->action == _Edit_FormatAction_Align) {
                        String8 names[] = {Str8Lit("left"), Str8Lit("center"), Str8Lit("right"), Str8Lit("usual")};
                        U64     aligns[] = {Deck_Align_Left, Deck_Align_Center, Deck_Align_Right, Deck_Align_COUNT};
                        UI_Spacer(UI_Px(6.0f * ui, 1));
                        for (U64 k = 0; k < ArrayCount(names); k += 1) {
                           if (_Edit_BuildRowButton(names[k], Look_AlignIsNow(app_state, aligns[k]), row_h, ui, interactive,
                                                    "###edit align %llu", (unsigned long long)k)) {
                              align_picked = (S64)aligns[k];
                           }
                        }
                        UI_Spacer(UI_Px(pad - 6.0f * ui, 1));
                     }
                  }
                  if (interactive) {
                     UI_Signal signal = UI_SignalFromBox(row);
                     if (UI_Hovering(signal)) {
                        row->background_color = UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Background);
                     }
                     if (UI_Clicked(signal)) {
                        format_picked = i;
                     }
                  }
               }
            }
         }
      }

      //- bgz: the status line: a notice if one is up, else the file and keys
      {
         UI_SetNextPrefWidth(UI_Px(left_w, 1));
         UI_SetNextPrefHeight(UI_Px(Dim2F32(layout.status).y, 1));
         UI_SetNextChildLayoutAxis(Axis2_X);
         UI_SetNextColorTag(Str8Lit("panel_bar"));
         UI_Box* status = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clip, "###edit status");
         UI_Parent(status) {
            String8 notice = FR_CurrentNotice(app_state);
            if (notice.size != 0) {
               _Edit_BuildBarText(pad, Str8Lit("notice"), notice, UI_TopFont(), Str8Zero());
            } else {
               String8 name = Str8SkipLastSlash(Str8(editor->file.str, editor->file.size));
               String8 left = PushStr8F(scratch.arena, "%S%s", name, Editor_HasUnsavedChanges(editor) ? "  (not saved)" : "");
               // The mode the text's keys are in (vim\vim.c), before the name.
               UI_PrefHeight(UI_Pct(1, 1)) {
                  UI_Spacer(UI_Px(pad, 1));
                  UI_SetNextPrefWidth(UI_TextDim(1, 1));
                  UI_SetNextColorTag(Vim_ModeTag(app_state));
                  UI_SetNextFont(fonts->bold);
                  UI_Label(Vim_ModeName(app_state));
               }
               _Edit_BuildBarText(pad, Str8Lit(""), left, UI_TopFont(), Str8Lit("Ctrl+O open    Ctrl+S save    F5 present"));
            }
         }
      }
   }

   //- bgz: what was clicked: its op, run at the start of the next frame
   if (toggle_format) {
      StateOp_Edit_ToggleFormat(ops);
   }
   if (picture_picked >= 0 && (U64)picture_picked < g_edit_view.pictures.count) {
      StateOp_Edit_PutPicture(ops)->name = PushStr8Copy(ops->arena, g_edit_view.pictures.v[picture_picked]);
   }
   if (format_picked >= 0) {
      StateOp_Edit_FormatRow(ops)->row_idx = (U64)format_picked;
   }
   // The words the cursor is in, sized and coloured (look\look.c).
   if (size_picked) {
      StateOp_Look_StepSize(ops)->direction = size_direction;
   }
   if (color_picked >= 0) {
      StateOp_Look_SetColor(ops)->named_idx = (U64)color_picked;
   }
   if (align_picked >= 0) {
      StateOp_Look_SetAlign(ops)->align = (U64)align_picked;
   }
   //- bgz: the preview: its slide, and what is over it
   _Edit_BuildPreview(app_state, &layout, interactive);

   ScratchEnd(scratch);
}

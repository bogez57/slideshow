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
// the + - are built with the UI layer (EDIT_Build, called from FrameWork's UI
// build), and take their own clicks and wheel there: see "the preview, in
// the UI layer" below. The slides are drawn by the boxes they are in (the
// view, as a box's custom draw), and so is the text (the editor, editor.h,
// which is also given the text box's press, drag and wheel). The keys are
// EDIT_Input's, before the build: the screen's own, then the editor's.
//
// The slide in the preview is typed into: a click on its title or on a
// block of its words puts the cursor at the letter clicked, with a caret
// there on the slide, and a thin blue box around the room the words have
// (frame_caret.c). A click on a picture puts the cursor on its line, with
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
//    fullscreen       F11 or Alt+Return. Esc leaves it
//    deselect         Esc: nothing selected, and no box in the preview
//    pictures         click the picture line under Format. Esc closes the list,
//                     the wheel or the scroll bar moves through it
//    edit a piece     click it in the preview, and type: the caret is on
//                     the slide, at the letter clicked (frame_caret.c)
//    move a piece     drag the picture or the body in the preview, or a
//                     corner of it to resize it. Right-click puts it back
//                     (frame_place.c)
//    add a picture    drop an image or a video on the window: one more on the
//                     slide. With a picture selected - click it - that one
//                     is changed for it
//    add a slide      click the + at the end of the row of small slides
//    move a slide     Alt+Left earlier, Alt+Right later
//    bold, code       Ctrl+B and Ctrl+E: ** or ` around what is selected
//    size, colour     Ctrl+] and Ctrl+[ make the words the cursor is in
//                     larger and smaller, and the Format panel's @size and
//                     @color lines have the rest to click (frame_look.c)
//    open             Ctrl+O: the open screen (frame_open.c). Or drop a folder
//                     or a text deck on the window
//
// The editor's own keys are in editor.c.

////////////////////////////////
//~ bgz: the format panel's lines

typedef struct _EDIT_FormatRow _EDIT_FormatRow;
struct _EDIT_FormatRow {
   String8 syntax;       // as shown
   String8 what;         // what it does
   String8 insert;       // what a click writes
   U64     select_start; // the part of insert left selected, to type over
   U64     select_end;
   U32     action;       // an _EDIT_FormatAction: what a click does, if not write insert
};

enum {
   _EDIT_FormatAction_Insert,
   _EDIT_FormatAction_Pictures, // opens the list of pictures
   _EDIT_FormatAction_Theme,    // gives the deck the next theme
   _EDIT_FormatAction_Layout,   // gives the slide the next layout
   _EDIT_FormatAction_Wrap,     // puts insert before and after the selection (ED_WrapSelection)
   _EDIT_FormatAction_Size,     // nothing by itself: the row has smaller, larger and usual to click (frame_look.c)
   _EDIT_FormatAction_Color,    // nothing by itself: the row has the colours to click
};

global _EDIT_FormatRow _edit_format_rows[] = {
    {Str8LitComp("# Title"), Str8LitComp("starts a new slide, with a title"), Str8LitComp("# Title"), 2, 7},
    {Str8LitComp("---"), Str8LitComp("starts a new slide with no title"), Str8LitComp("---\n"), 4, 4},
    {Str8LitComp("## Heading"), Str8LitComp("a heading inside the slide"), Str8LitComp("## Heading"), 3, 10},
    {Str8LitComp("- point"), Str8LitComp("a bullet. Tab moves it in"), Str8LitComp("- point"), 2, 7},
    {Str8LitComp("  - point"), Str8LitComp("an inner bullet. Shift+Tab moves it out"), Str8LitComp("  - point"), 4, 9},
    {Str8LitComp("![](picture.png)"), Str8LitComp("a picture or video on the slide. Click to pick one"), Str8LitComp("![](picture.png)"), 4, 15, _EDIT_FormatAction_Pictures},
    {Str8LitComp("Any other line"), Str8LitComp("a paragraph. An empty line adds room"), Str8LitComp("Text"), 0, 4},
    {Str8LitComp("**bold**"), Str8LitComp("bold words. Select some first, or Ctrl+B"), Str8LitComp("**"), 0, 0, _EDIT_FormatAction_Wrap},
    {Str8LitComp("`code`"), Str8LitComp("words as code. Select some first, or Ctrl+E"), Str8LitComp("`"), 0, 0, _EDIT_FormatAction_Wrap},
    {Str8LitComp("@note Say this"), Str8LitComp("a note to yourself. N shows notes while presenting"), Str8LitComp("@note Say this"), 6, 14},
    {Str8LitComp("@box"), Str8LitComp("another block of words on the slide, to drag on its own"), Str8LitComp("@box\nText"), 5, 9},
    {Str8LitComp("@video loop"), Str8LitComp("how the slide's video plays: loop, mute, manual"), Str8LitComp("@video loop"), 7, 11},
    // What these four do is said in EDIT_Build, with what the deck, the slide and the words have now.
    {Str8LitComp("@size"), Str8LitComp(""), Str8LitComp(""), 0, 0, _EDIT_FormatAction_Size},
    {Str8LitComp("@color"), Str8LitComp(""), Str8LitComp(""), 0, 0, _EDIT_FormatAction_Color},
    {Str8LitComp("@theme"), Str8LitComp(""), Str8LitComp(""), 0, 0, _EDIT_FormatAction_Theme},
    {Str8LitComp("@layout"), Str8LitComp(""), Str8LitComp(""), 0, 0, _EDIT_FormatAction_Layout},
};

////////////////////////////////
//~ bgz: layout

// Where everything on the screen is, for a window of a size. Input and
// drawing both ask, and get the same answer.
typedef struct _EDIT_Layout _EDIT_Layout;
struct _EDIT_Layout {
   F32 ui; // how many pixels a unit of the sizes below is

   ED_Box  editor_box;
   Rng2F32 format_header; // always there: clicking it opens and closes the panel
   Rng2F32 format_rows;   // zero when the panel is closed
   F32     format_row_h;
   Rng2F32 status;
   Rng2F32 preview;       // the right side, all of it
   Rng2F32 page;          // the slide, in the preview
   Rng2F32 caption;       // under the slide
   Rng2F32 thumbs;        // the row of small slides, along the bottom
   Rng2F32 add_slide;     // the + at the row's right end
   Vec2F32 thumb_dim;     // the size of one
   F32     thumb_gap;     // between two

   F32 ui_px;   // the size of text that is not the editor's
   F32 mono_px; // the editor's
};

internal _EDIT_Layout
_EDIT_LayoutFromWindowDim(App_State* app_state, Vec2F32 window_dim) {
   _EDIT_Layout result = {0};

   F32 ui = FR_UIScale(window_dim);
   result.ui = ui;
   result.ui_px = (F32)(S32)(15.0f * ui);
   result.mono_px = (F32)(S32)(17.0f * ui);

   // The text has a little under half the window, within reason at the ends.
   F32 left_w = window_dim.x * 0.44f;
   left_w = Clamp(Min(380.0f * ui, window_dim.x), left_w, 900.0f * ui);
   left_w = (F32)(S32)left_w;

   F32 status_h = (F32)(S32)(30.0f * ui);
   F32 header_h = (F32)(S32)(30.0f * ui);
   F32 row_h = (F32)(S32)(26.0f * ui);
   // All the format's lines if they fit, but the text keeps at least half
   // the window: in a short one the lines give way, two at the least, and
   // scroll (EDIT_Build).
   F32 rows_h = 0;
   if (app_state->show_format) {
      F32 all_rows_h = row_h * (F32)ArrayCount(_edit_format_rows) + 8.0f * ui;
      F32 room_h = window_dim.y * 0.5f - header_h - status_h;
      rows_h = Min(all_rows_h, Max(room_h, row_h * 2.0f + 8.0f * ui));
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
      result.page = VIEW_PageRectFromRect(page_room);
      result.caption = R2F32P(result.page.min.x, result.page.max.y, result.page.max.x, result.page.max.y + caption_h);
   }

   return result;
}

// Where a slide's small picture is in the row. With more slides than fit, the
// row shows the ones around the slide showing - as far as the ends of the
// deck allow - and always whole ones. A slide that is not in the row gets a
// zero rect.
internal Rng2F32
_EDIT_ThumbRect(_EDIT_Layout* layout, U64 slide_count, U64 slide_showing, U64 slide_idx) {
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

internal Rng1U64
_EDIT_TextRangeFromSlide(ED_Editor* editor, DECK_Deck* deck, U64 slide_idx) {
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
internal void
_EDIT_AddSlideAfter(ED_Editor* editor, DECK_Deck* deck, U64 slide_idx) {
   U64     at = _EDIT_TextRangeFromSlide(editor, deck, slide_idx).max;
   String8 text = ED_Text(editor);
   Temp    scratch = ScratchBegin(0, 0);

   // On a line of its own: the slide before may end without a line ending,
   // and the slide after has to start on its own line too.
   B32     needs_line_before = (at > 0 && text.str[at - 1] != '\n');
   String8 added = PushStr8F(scratch.arena, "%s# Title\n", needs_line_before ? "\n" : "");
   ED_ReplaceRange(editor, at, at, added);

   U64 title_start = at + (needs_line_before ? 1 : 0) + 2;
   ED_SetCursor(editor, title_start + 5);
   editor->mark = title_start;

   ScratchEnd(scratch);
}

// Moves slide_idx a place later in the deck, or earlier: its piece of the
// text and its neighbour's change places, as one step to undo. The cursor
// stays where it was in the slide. 0 if there is no place to move it to.
internal B32
_EDIT_MoveSlide(ED_Editor* editor, DECK_Deck* deck, U64 slide_idx, B32 to_later) {
   B32 result = 0;

   // Either way two slides next to each other change places: first, second.
   U64 first_idx = to_later ? slide_idx : slide_idx - 1;
   if (slide_idx < deck->slide_count && (to_later ? slide_idx + 1 < deck->slide_count : slide_idx > 0)) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 text = ED_Text(editor);
      Rng1U64 first_range = _EDIT_TextRangeFromSlide(editor, deck, first_idx);
      Rng1U64 second_range = _EDIT_TextRangeFromSlide(editor, deck, first_idx + 1);
      String8 first = Str8Substr(text, first_range);
      String8 second = Str8Substr(text, second_range);

      // The deck's first slide can start without a # Title or a --- : there
      // is nothing before it to be told from. Anywhere else it would be read
      // as more of the slide before, so it is given a --- to start with.
      B32 first_starts_itself = 0;
      for (U64 i = 0; i < first.size; i += 1) {
         if (first.str[i] != ' ' && first.str[i] != '\t' && first.str[i] != '\n') {
            first_starts_itself = (first.str[i] == '#' || first.str[i] == '-');
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

      // The second can be the text's last line, with no line ending.
      B32     second_needs_line_end = (second.size != 0 && second.str[second.size - 1] != '\n');
      String8 middle = PushStr8F(scratch.arena, "%s%s", second_needs_line_end ? "\n" : "",
                                 first_starts_itself ? "" : "---\n");
      String8 swapped = PushStr8F(scratch.arena, "%S%S%S", second, middle, first);

      // Where the cursor was in the slide that was asked to be moved.
      Rng1U64 moved_range = to_later ? first_range : second_range;
      U64     in_slide = Clamp(moved_range.min, editor->cursor, moved_range.max) - moved_range.min;
      U64     moved_start = to_later ? first_range.min + second.size + middle.size : first_range.min;

      ED_ReplaceRange(editor, first_range.min, second_range.max, swapped);
      ED_SetCursor(editor, moved_start + in_slide);

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
internal void
_EDIT_ReplaceKeepingCursor(ED_Editor* editor, U64 start, U64 end, String8 string) {
   U64 cursor = editor->cursor;
   if (cursor >= end) {
      cursor = cursor - (end - start) + string.size;
   } else if (cursor > start) {
      cursor = start + string.size;
   }
   ED_ReplaceRange(editor, start, end, string);
   ED_SetCursor(editor, cursor);
}

// Writes a line that is about one piece of a slide - an @place, @size or
// @color line, for the idx-th picture or block of the body, or the title:
// over the one there is (old_line, with has_line), or as a new line under
// the picture's line, under the title's, or above the block's first.
internal void
_EDIT_PutLineFor(ED_Editor* editor, DECK_Slide* slide, DECK_PlaceKind kind, U64 idx, B32 has_line, Rng1U64 old_line,
                 String8 line) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 text = ED_Text(editor);

   if (has_line && old_line.max <= text.size) {
      _EDIT_ReplaceKeepingCursor(editor, old_line.min, old_line.max, line);
   } else {
      // The line of the text the thing is on: the picture's, the title's,
      // or the first of the block's - which, for any block but the first,
      // is under its @box line, so the new line goes between the two.
      U64 on = slide->text_pos;
      if (kind == DECK_PlaceKind_Picture && idx < slide->picture_count) {
         on = slide->pictures[idx].range.min;
      } else if (kind == DECK_PlaceKind_Title) {
         on = slide->title_range.min;
      } else if (kind == DECK_PlaceKind_Text && idx < slide->box_count && slide->boxes[idx].first_item != 0) {
         on = slide->boxes[idx].first_item->text_range.min;
      }
      on = Min(on, text.size);
      U64 line_start = on;
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 line_end = on;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;

      // A block's goes above its first line, where it does not come between
      // two lines of the block - unless that line is what starts the slide:
      // above it would be the slide before. A picture's goes under the
      // picture's line: it is for the picture last written above it. The
      // title's goes under the title's, which is what starts the slide.
      if (kind == DECK_PlaceKind_Text && line_start > slide->text_pos) {
         _EDIT_ReplaceKeepingCursor(editor, line_start, line_start, PushStr8F(scratch.arena, "%S\n", line));
      } else {
         _EDIT_ReplaceKeepingCursor(editor, line_end, line_end, PushStr8F(scratch.arena, "\n%S", line));
      }
   }

   ScratchEnd(scratch);
}

// Takes such a line out of the text, with its line ending.
internal void
_EDIT_TakeOutLine(ED_Editor* editor, Rng1U64 line) {
   String8 text = ED_Text(editor);
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
      _EDIT_ReplaceKeepingCursor(editor, start, end, Str8Lit(""));
   }
}

// Gives the deck the theme after the one it has: its @theme line is
// written again, or a new one is put at the top of the text.
internal void
_EDIT_NextTheme(ED_Editor* editor, DECK_Deck* deck) {
   Temp       scratch = ScratchBegin(0, 0);
   DECK_Theme theme = (DECK_Theme)((deck->theme + 1) % DECK_Theme_COUNT);
   String8    line = PushStr8F(scratch.arena, "@theme %S", DECK_ThemeName(theme));

   if (deck->has_theme_line && deck->theme_line.max <= editor->size) {
      _EDIT_ReplaceKeepingCursor(editor, deck->theme_line.min, deck->theme_line.max, line);
   } else {
      _EDIT_ReplaceKeepingCursor(editor, 0, 0, PushStr8F(scratch.arena, "%S\n", line));
   }

   ScratchEnd(scratch);
}

// Gives slide_idx the layout after the one it has: its @layout line is
// written again, or a new one is put under the line the slide starts with.
internal void
_EDIT_NextLayout(ED_Editor* editor, DECK_Deck* deck, U64 slide_idx) {
   if (slide_idx >= deck->slide_count) {
      return;
   }
   Temp        scratch = ScratchBegin(0, 0);
   DECK_Slide* slide = &deck->slides[slide_idx];
   DECK_Layout layout = (DECK_Layout)((slide->layout + 1) % DECK_Layout_COUNT);
   String8     line = PushStr8F(scratch.arena, "@layout %S", DECK_LayoutName(layout));
   String8     text = ED_Text(editor);

   if (slide->has_layout_line && slide->layout_line.max <= text.size) {
      _EDIT_ReplaceKeepingCursor(editor, slide->layout_line.min, slide->layout_line.max, line);
   } else {
      // Under the slide's first line when that is its # Title or its --- :
      // the line belongs to the slide from there. A first slide that starts
      // with neither has it above, where it waits for the slide.
      U64 line_start = Min(slide->text_pos, text.size);
      U64 line_end = line_start;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
      U64 first = line_start;
      while (first < line_end && (text.str[first] == ' ' || text.str[first] == '\t')) first += 1;

      B32 starts_slide = (first < line_end && (text.str[first] == '#' || text.str[first] == '-'));
      if (starts_slide) {
         _EDIT_ReplaceKeepingCursor(editor, line_end, line_end, PushStr8F(scratch.arena, "\n%S", line));
      } else {
         _EDIT_ReplaceKeepingCursor(editor, line_start, line_start, PushStr8F(scratch.arena, "%S\n", line));
      }
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: pictures

// How often the folder is looked in again while the list is open.
#define _EDIT_PICTURES_SCAN_SECONDS 1.0f

internal void
_EDIT_ScanPictures(App_State* app_state) {
   if (app_state->pictures_arena == 0) {
      app_state->pictures_arena = ArenaAlloc();
   }
   ArenaClear(app_state->pictures_arena);
   app_state->pictures = DECK_PictureNamesFromFolder(app_state->pictures_arena, app_state->deck->folder);
   app_state->pictures_scanned_us = NowTimeUS();
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
internal B32
_EDIT_PutPicture(App_State* app_state, String8 name, B32 only_ask) {
   ED_Editor* editor = app_state->editor;
   DECK_Deck* deck = app_state->deck;
   String8    text = ED_Text(editor);
   Temp       scratch = ScratchBegin(0, 0);
   String8    line = PushStr8F(scratch.arena, "![](%S)", name);

   // The slide's part of the text. The deck is from the text as it was at
   // the last FR_SyncDeck, which is this frame's or the last's: positions
   // are kept inside the text as it is now.
   U64 slide_start = 0;
   U64 slide_end = text.size;
   if (deck->slide_count != 0) {
      U64 slide_idx = DECK_SlideFromTextPos(deck, editor->cursor);
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
               ED_ReplaceRange(editor, line_start, line_end, line);
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
      FR_Notice(app_state, Str8Lit("The slide has as many pictures as a slide can. Click one to change it for this one"));
   } else if (only_ask) {
      // Nothing written.
   } else if (!found && has_last) {
      String8 added = PushStr8F(scratch.arena, "\n%S", line);
      ED_ReplaceRange(editor, last_end, last_end, added);
      // On the new line, at its end: the next one put goes under it, and
      // the preview marks it.
      ED_SetCursor(editor, last_end + added.size);
   } else if (!found) {
      ED_InsertLine(editor, line, line.size, line.size);
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
internal void
_EDIT_DropPicture(App_State* app_state, String8 path) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 name = Str8SkipLastSlash(path);
   String8 in_folder = PushStr8F(scratch.arena, "%S\\%S", app_state->deck->folder, name);

   // Not copied at all for a slide with no room for it: the notice says so.
   if (!_EDIT_PutPicture(app_state, name, 1)) {
      ScratchEnd(scratch);
      return;
   }

   B32 is_there = FilePathExists(in_folder);
   if (!is_there && VID_NameIsVideo(name)) {
      // A video is not waited for: it is copied in the background and
      // plays from where it is meanwhile (frame_copy.c).
      if (!COPY_IsCopyingTo(app_state, in_folder)) {
         COPY_Start(app_state, path, in_folder);
         app_state->quit_warned_copy = 0;
         FR_Notice(app_state, Str8Lit("Copying the video into the deck's folder. It plays from where it is meanwhile."));
      }
      _EDIT_PutPicture(app_state, name, 0);
      ScratchEnd(scratch);
      return;
   }
   if (!is_there) {
      is_there = CopyFilePath(in_folder, path);
      if (is_there) {
         FR_Notice(app_state, Str8Lit("Picture copied into the deck's folder"));
      } else {
         FR_Notice(app_state, Str8Lit("Could not copy the picture into the deck's folder"));
      }
   }
   if (is_there) {
      _EDIT_PutPicture(app_state, name, 0);
      app_state->pictures_scanned_us = 0; // the list, if open, is out of date
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: input

// Which of the preview's pieces (app_state->preview_hits) a point is on, or -1.
// Asked by a click, which is already known to be on the slide's boxes: of
// several hits there, the one it goes to.
// Where pieces are over each other, the one drawn last: the slide's pictures
// are drawn first, in the order of their lines, then the title, then the
// body - so words on a picture are the words, and of pictures over each
// other the one on top is the one seen.
internal S64
_EDIT_PreviewHitAt(App_State* app_state, Vec2F32 pos) {
   S64 result = -1;
   for (U64 i = app_state->preview_hit_count; i > 0; i -= 1) {
      if (Contains2F32(app_state->preview_hits[i - 1].rect, pos)) {
         result = (S64)(i - 1);
         break;
      }
   }
   return result;
}

// Bold and code are for words: 0, with a notice, when the cursor is on a
// picture's line or an @ line, where the marks would be part of a file's
// name or of a setting - a picture that can no longer be found.
internal B32
_EDIT_CanWrap(App_State* app_state) {
   ED_Editor* editor = app_state->editor;
   String8    text = ED_Text(editor);
   U64        line_start = Min(editor->cursor, text.size);
   while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
   while (line_start < text.size && (text.str[line_start] == ' ' || text.str[line_start] == '\t')) line_start += 1;

   B32 is_picture = (line_start + 1 < text.size && text.str[line_start] == '!' && text.str[line_start + 1] == '[');
   B32 is_setting = (line_start < text.size && text.str[line_start] == '@');
   B32 result = !(is_picture || is_setting);
   if (!result) {
      FR_Notice(app_state, Str8Lit("Bold and code are for words, not for a picture's line or an @ line"));
   }
   return result;
}

// F1, or a click on the Format panel's header: closes the list of pictures
// if that is what is open, else opens and closes the panel.
internal void
_EDIT_ToggleFormat(App_State* app_state) {
   if (app_state->show_pictures) {
      app_state->show_pictures = 0;
   } else {
      app_state->show_format = !app_state->show_format;
   }
}

// Where a video of the slide showing is drawn in the preview: the place the
// preview last drew the picture_idx-th of the slide's pictures, if that is
// a video that is open. 0 if not, or if the text has changed since the
// preview was drawn. A picture behind the words has no place of its own to
// click (view.c): it is all of page, as the presentation has it.
internal VID_Video*
_EDIT_PreviewVideo(App_State* app_state, Rng2F32 page, U64 picture_idx, Rng2F32* rect_out) {
   VID_Video* result = 0;
   DECK_Deck* deck = app_state->deck;

   if (app_state->slide_idx < deck->slide_count && app_state->editor != 0 &&
       app_state->editor->edit_count == app_state->preview_hits_edit_count &&
       picture_idx < deck->slides[app_state->slide_idx].picture_count) {
      DECK_Slide*   slide = &deck->slides[app_state->slide_idx];
      DECK_Picture* picture = &slide->pictures[picture_idx];
      if (picture->video == 0) {
         return 0;
      }
      if (picture_idx == 0 && slide->layout == DECK_Layout_PictureBehind && DECK_SlideHasText(slide)) {
         *rect_out = page;
         return picture->video;
      }
      for (U64 i = 0; i < app_state->preview_hit_count; i += 1) {
         VIEW_Hit* hit = &app_state->preview_hits[i];
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
internal void
_EDIT_ChangeSlides(App_State* app_state, B32 add, B32 to_later) {
   ED_Editor* editor = app_state->editor;
   // The deck as the text is now: something this frame may have changed it.
   FR_SyncDeck(app_state);
   U64 slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);
   if (add) {
      _EDIT_AddSlideAfter(editor, app_state->deck, slide_idx);
   } else if (!_EDIT_MoveSlide(editor, app_state->deck, slide_idx, to_later)) {
      FR_Notice(app_state, to_later ? Str8Lit("This is the last slide already") : Str8Lit("This is the first slide already"));
   }
   editor->last_input_us = NowTimeUS();
   FR_SyncDeck(app_state);
   app_state->slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);
}

internal void
EDIT_PreviewClick(App_State* app_state, Vec2F32 pos, B32 keeps_mark) {
   ED_Editor* editor = app_state->editor;
   if (editor->edit_count != app_state->preview_hits_edit_count) {
      return;
   }
   // On words: the cursor to the letter (frame_caret.c). On a picture: its
   // file's name selected, to be typed over.
   if (!CARET_Click(app_state, pos, keeps_mark)) {
      S64 hit_idx = _EDIT_PreviewHitAt(app_state, pos);
      if (hit_idx >= 0 && app_state->preview_hits[hit_idx].text_range.max <= editor->size) {
         VIEW_Hit* hit = &app_state->preview_hits[hit_idx];
         ED_SetCursor(editor, hit->text_range.max);
         editor->mark = hit->text_range.min;
         app_state->preview_focus_hidden = 0;
         editor->last_input_us = NowTimeUS();
      }
   }
}

internal void
EDIT_Input(App_State* app_state, Vec2F32 window_dim) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;

   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         FR_RequestQuit(app_state);
      }
      // Of several things dropped at once, the first is used. An image or a
      // video is a picture for the slide. Anything else is a deck to open.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         String8 path = event->strings.first->string;
         B32     is_folder = !!(PropertiesFromFilePath(path).flags & FilePropertyFlag_IsFolder);
         if (!is_folder && (IMG_NameIsImage(path) || VID_NameIsVideo(path))) {
            _EDIT_DropPicture(app_state, path);
         } else {
            FR_OpenPath(app_state, path);
            if (!app_state->is_editing) {
               return;
            }
         }
      }
   }

   ED_Editor*   editor = app_state->editor;
   _EDIT_Layout layout = _EDIT_LayoutFromWindowDim(app_state, window_dim);

   //- bgz: a piece of the slide dragged by the mouse (frame_place.c): Esc
   // gives it up, before Esc does anything else
   PLACE_Keys(app_state);

   //- bgz: the screen's own keys, taken before the editor sees them
   B32 is_fullscreen = Core_WM_WindowIsFullscreen(window);
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F11) ||
       Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Return)) {
      app_state->toggle_fullscreen = 1;
   }
   // Esc closes what is open, one thing a press: the list of pictures, then
   // what is selected - the words, and the box in the preview around the
   // piece the cursor is in - then fullscreen.
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
      B32 has_selection = (editor->mark != editor->cursor);
      if (app_state->show_pictures) {
         app_state->show_pictures = 0;
      } else if (has_selection || !app_state->preview_focus_hidden) {
         editor->mark = editor->cursor;
         app_state->preview_focus_hidden = 1;
         app_state->preview_focus_hidden_cursor = editor->cursor;
      } else if (is_fullscreen) {
         app_state->toggle_fullscreen = 1;
      }
   }

   // As a click on the Format panel's header does (EDIT_Build): closes the
   // list of pictures if that is what is open, else opens and closes the
   // panel.
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F1)) {
      _EDIT_ToggleFormat(app_state);
      layout = _EDIT_LayoutFromWindowDim(app_state, window_dim);
   }

   // Bold and code, around what is selected (the Format panel's two lines
   // for them do the same).
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_B) && _EDIT_CanWrap(app_state)) {
      ED_WrapSelection(editor, Str8Lit("**"), Str8Lit("bold"));
      editor->last_input_us = NowTimeUS();
   }
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_E) && _EDIT_CanWrap(app_state)) {
      ED_WrapSelection(editor, Str8Lit("`"), Str8Lit("code"));
      editor->last_input_us = NowTimeUS();
   }

   // The words the cursor is in, larger and smaller (frame_look.c).
   LOOK_Input(app_state);

   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_S)) {
      if (ED_Save(editor)) {
         FR_Notice(app_state, Str8Lit("Saved"));
         app_state->quit_warned = 0;

         // A picture that was not found is looked for again: saving is when
         // the file is likely to have been put in place.
         for (U64 i = 0; i < app_state->deck->slide_count; i += 1) {
            for (U64 k = 0; k < app_state->deck->slides[i].picture_count; k += 1) {
               app_state->deck->slides[i].pictures[k].load_failed = 0;
            }
         }
      } else {
         FR_Notice(app_state, Str8Lit("Could not save: the file could not be written"));
      }
   }

   B32 present_pressed = Core_WM_KeyPress(events, window, 0, WM_Key_F5);

   //- bgz: the list of pictures, kept up with the folder while it is open
   //
   // Its clicks and wheel, and the Format lines', are taken where they are
   // built (EDIT_Build).
   if (app_state->show_format && app_state->show_pictures) {
      U64 scan_us = (U64)(_EDIT_PICTURES_SCAN_SECONDS * 1000000.0f);
      if (NowTimeUS() - app_state->pictures_scanned_us >= scan_us) {
         _EDIT_ScanPictures(app_state);
      }
   }

   //- bgz: slides moved (the + that adds one is a UI box: EDIT_Build)
   {
      B32 move_earlier = Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Left);
      B32 move_later = Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Right);
      if (move_earlier || move_later) {
         _EDIT_ChangeSlides(app_state, 0, move_later);
      }
   }

   //- bgz: everything else is the editor's
   ED_Input(editor, app_state->frame_arena, events, layout.editor_box);

   //- bgz: the deck, and the slide the cursor is on
   FR_SyncDeck(app_state);
   app_state->slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);

   if (present_pressed) {
      if (app_state->deck->slide_count != 0) {
         app_state->is_editing = 0;
         // Presenting shows every block from its top (frame_caret.c).
         MemoryZeroArray(app_state->caret_scroll);
         app_state->caret_body_scroll = 0;
         app_state->preview_hit_count = 0; // not drawn while presenting, and the window can change
         app_state->preview_box_count = 0;
         app_state->place_drag.state = EDIT_PlaceDragState_None;
         app_state->fade_active = 0;
         app_state->slide_changed_us = 0;
         app_state->scroll_left_over = 0;
      } else {
         FR_Notice(app_state, Str8Lit("Nothing to present yet: start a slide with  # Title"));
      }
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
internal void
_EDIT_BuildBarText(F32 pad, String8 left_tag, String8 left, FNT_Tag left_font, String8 right) {
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
internal B32
_EDIT_BuildRowButton(String8 name, B32 is_now, F32 row_h, F32 ui, B32 interactive, char* fmt, ...) {
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
internal B32
_EDIT_BuildSwatch(Vec4F32 color, B32 is_now, F32 side, F32 gap, F32 row_h, F32 ui, B32 interactive, char* fmt, ...) {
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
// and blocks are (app_state->preview_hits, _runs, _boxes). The boxes over
// it are built from those, so they are where the pieces are this frame, and
// a signal is of where the box is now (FR_BuildRectBox):
//
//    1. what the mouse holds goes with it: a piece dragged (frame_place.c),
//       a scroll bar (frame_caret.c)
//    2. the slide laid out
//    3. the boxes that are clicked, which are not drawn - first built, first
//       asked: the blocks' scroll bars, the corners' handles, the videos'
//       strips, the words and pictures, the pieces' boxes, the rooms of the
//       words, the small slides and the +
//    4. the slide laid out again, if a click changed the text, the cursor or
//       a block's scroll
//    5. the boxes that are drawn - first built, drawn on top: the outline
//       of what the mouse is over, the videos' strips, the wash under the
//       mouse, the box of the piece the cursor is in, the selection, the
//       caret, the caption under the slide, the small slides and the +,
//       the slide itself, and the preview's background
//
// So nothing over the slide is a frame behind it: not the caret after
// typing at the bottom of a full block, not the outline of a box dragged,
// not a scroll bar dragged.

//- bgz: what the boxes draw that the UI cannot: slides

typedef struct _EDIT_Slide _EDIT_Slide;
struct _EDIT_Slide {
   App_State* app_state;
   Rng2F32    page;
};

// The preview's slide, as it was laid out in the build: the same scroll,
// and its scroll bars are the UI's (frame_caret.c).
internal void
_EDIT_DrawSlide(UI_Box* box, void* user_data) {
   UnusedVariable(box);
   _EDIT_Slide* slide = (_EDIT_Slide*)user_data;
   App_State*   app_state = slide->app_state;
   DECK_Deck*   deck = app_state->deck;
   Core_D_Rect(slide->page, R2F32P(0, 0, 1, 1), VIEW_ColorsFromTheme(deck->theme).background);
   if (app_state->slide_idx < deck->slide_count) {
      VIEW_HitList hits = {0};
      hits.bars_by_caller = 1;
      CARET_BeforeDraw(app_state);
      VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, &deck->slides[app_state->slide_idx], slide->page, 1.0f, &hits);
      CARET_AfterDrawOf(app_state, app_state->slide_idx);
   }
}

typedef struct _EDIT_Thumb _EDIT_Thumb;
struct _EDIT_Thumb {
   App_State* app_state;
   U64        slide_idx;
   Rng2F32    rect;      // exactly: the box's is on whole pixels, and the row is not
   B32        has_frame; // in the box's border colour, frame pixels wide
   F32        frame;
};

// A small slide. Only a picture that is loaded already is drawn: the slide
// showing and the ones next to it. Loading every picture of a long deck for
// its small slides would take the memory DECK_LoadSlide saves.
internal void
_EDIT_DrawThumb(UI_Box* box, void* user_data) {
   _EDIT_Thumb* thumb = (_EDIT_Thumb*)user_data;
   App_State*   app_state = thumb->app_state;
   DECK_Deck*   deck = app_state->deck;
   Rng2F32      rect = thumb->rect;
   if (thumb->slide_idx >= deck->slide_count) {
      return;
   }
   if (thumb->has_frame) {
      Core_D_Rect(Pad2F32(rect, thumb->frame), R2F32P(0, 0, 1, 1), box->border_color);
   }
   Core_D_Rect(rect, R2F32P(0, 0, 1, 1), VIEW_ColorsFromTheme(deck->theme).background);
   B32 is_showing = (thumb->slide_idx == app_state->slide_idx);
   VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, &deck->slides[thumb->slide_idx], rect, is_showing ? 1.0f : 0.8f, 0);
}

typedef struct _EDIT_Plus _EDIT_Plus;
struct _EDIT_Plus {
   Rng2F32 rect; // exactly, as the small slides'
   F32     half; // how thick a bar is, either side of its middle
};

// The + that adds a slide: its background, and a + of two bars in the
// middle, in the box's text colour - drawn, not written, to be in the
// middle whatever the font.
internal void
_EDIT_DrawPlus(UI_Box* box, void* user_data) {
   _EDIT_Plus* plus = (_EDIT_Plus*)user_data;
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
// scroll follows the caret (CARET_Follow), and the slide is laid out again
// if it moved.
internal void
_EDIT_LayoutPreview(App_State* app_state, Rng2F32 page, F32 ui) {
   DECK_Deck* deck = app_state->deck;
   app_state->preview_hit_count = 0;
   app_state->preview_run_count = 0;
   app_state->preview_box_count = 0;
   if (Dim2F32(page).x <= 0 || app_state->slide_idx >= deck->slide_count) {
      return;
   }

   // Asking for the slide is what loads its pictures, and what releases the
   // pictures of slides far from this one.
   DECK_LoadSlide(deck, app_state->slide_idx);
   DECK_Slide*  slide = &deck->slides[app_state->slide_idx];
   VIEW_HitList hits = {0};
   hits.bars_by_caller = 1;
   D_Bucket* bucket = Core_D_BucketMake();
   Core_D_BucketScope(bucket) {
      CARET_BeforeDraw(app_state);
      VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, slide, page, 1.0f, &hits);
      if (CARET_Follow(app_state, &hits)) {
         // A block's words moved to follow the caret, or back to where they
         // end: the slide again, as it now is - or for a frame the caret's
         // line would be cut off, or the wheel would show room past the
         // words.
         MemoryZeroStruct(&hits);
         hits.bars_by_caller = 1;
         CARET_BeforeDraw(app_state);
         VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, slide, page, 1.0f, &hits);
      }
   }
   CARET_AfterDrawOf(app_state, app_state->slide_idx);

   if (app_state->preview_hits_arena == 0) {
      app_state->preview_hits_arena = ArenaAlloc();
   }
   ArenaClear(app_state->preview_hits_arena);
   app_state->preview_hits = PushArray(app_state->preview_hits_arena, VIEW_Hit, Max(hits.count, 1));
   // The deck laid out is of the text at parsed_edit_count.
   app_state->preview_hits_edit_count = app_state->parsed_edit_count;
   MemoryCopyArray(app_state->preview_boxes, hits.boxes);
   app_state->preview_box_count = hits.box_count;
   app_state->preview_page = hits.page;

   F32 grow = 4.0f * ui;
   for (VIEW_Hit* hit = hits.first; hit != 0; hit = hit->next) {
      // A little larger than what was drawn, to be easy to click, and no
      // larger than the slide.
      Rng2F32 rect = Intersect2F32(R2F32P(hit->rect.min.x - grow, hit->rect.min.y - grow * 0.5f, hit->rect.max.x + grow,
                                          hit->rect.max.y + grow * 0.5f),
                                   page);
      if (rect.max.x <= rect.min.x || rect.max.y <= rect.min.y) {
         continue;
      }
      VIEW_Hit* kept = &app_state->preview_hits[app_state->preview_hit_count];
      app_state->preview_hit_count += 1;
      kept->next = 0;
      kept->rect = rect;
      kept->text_range = hit->text_range;
   }
   CARET_KeepRuns(app_state, &hits);
}

internal void
_EDIT_BuildPreview(App_State* app_state, _EDIT_Layout* layout, B32 interactive) {
   ED_Editor* editor = app_state->editor;
   DECK_Deck* deck = app_state->deck;
   F32        ui = layout->ui;

   //- bgz: 1, 2: what the mouse holds, then the slide laid out
   PLACE_BeforeLayout(app_state, ui);
   CARET_BeforeLayout(app_state);
   FR_SyncDeck(app_state);
   deck = app_state->deck;
   app_state->slide_idx = DECK_SlideFromTextPos(deck, editor->cursor);
   _EDIT_LayoutPreview(app_state, layout->page, ui);

   //- bgz: 3: the boxes that are clicked
   U64         cursor_was = editor->cursor;
   U64         mark_was = editor->mark;
   U64         edit_count_was = editor->edit_count;
   B32         changed = 0;
   PLACE_Hover hover = {0};
   hover.hit_idx = -1;
   changed |= CARET_BuildBars(app_state, ui, interactive);
   changed |= PLACE_BuildCorners(app_state, ui, interactive, &hover);
   for (U64 picture_idx = 0; picture_idx < DECK_MAX_PICTURES && interactive; picture_idx += 1) {
      Rng2F32    video_rect = {0};
      VID_Video* video = _EDIT_PreviewVideo(app_state, layout->page, picture_idx, &video_rect);
      // Over its strip is over its picture, for its outline.
      if (video != 0 && FR_VideoBuildStripHits(app_state, video, video_rect, ui, picture_idx) && !hover.is_over) {
         hover.is_over = 1;
         hover.has_piece = 1;
         hover.kind = DECK_PlaceKind_Picture;
         hover.idx = picture_idx;
      }
   }
   changed |= PLACE_BuildHits(app_state, ui, interactive, &hover);
   changed |= CARET_BuildRooms(app_state, interactive);

   //- bgz: the row of small slides - a press goes to the slide - and the +
   // that adds one after the slide showing
   S64 thumb_hot = -1;
   B32 add_hot = 0;
   if (interactive) {
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         Rng2F32 rect = _EDIT_ThumbRect(layout, deck->slide_count, app_state->slide_idx, i);
         if (rect.max.x > rect.min.x) {
            UI_Signal signal = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, rect, "###edit thumb %llu", (unsigned long long)i));
            if (UI_Hovering(signal)) {
               thumb_hot = (S64)i;
            }
            if (signal.f & UI_SignalFlag_LeftPressed) {
               ED_SetCursor(editor, Min(deck->slides[i].text_pos, editor->size));
               editor->last_input_us = NowTimeUS();
            }
         }
      }
      if (layout->add_slide.max.x > layout->add_slide.min.x) {
         UI_Signal signal = UI_SignalFromBox(FR_BuildRectBox(UI_BoxFlag_MouseClickable, layout->add_slide, "###edit add slide"));
         add_hot = UI_Hovering(signal);
         if (signal.f & UI_SignalFlag_LeftPressed) {
            _EDIT_ChangeSlides(app_state, 1, 0);
         }
      }
   }

   //- bgz: 4: laid out again, for what they did
   changed |= (editor->cursor != cursor_was || editor->mark != mark_was || editor->edit_count != edit_count_was);
   if (changed) {
      FR_SyncDeck(app_state);
      deck = app_state->deck;
      app_state->slide_idx = DECK_SlideFromTextPos(deck, editor->cursor);
      _EDIT_LayoutPreview(app_state, layout->page, ui);
   }

   //- bgz: 5: the boxes that are drawn
   String8 caption = {0};
   if (app_state->slide_idx < deck->slide_count && Dim2F32(layout->page).x > 0) {
      // The box the mouse is over - a picture's, the title's, a block's -
      // with its corners, and what can be done with it said under the slide.
      B32 can_move = PLACE_BuildLook(app_state, ui, &hover);

      // The slide's videos: each its strip of controls, over the bottom of it.
      for (U64 picture_idx = 0; picture_idx < DECK_MAX_PICTURES; picture_idx += 1) {
         Rng2F32    video_rect = {0};
         VID_Video* video = _EDIT_PreviewVideo(app_state, layout->page, picture_idx, &video_rect);
         if (video != 0) {
            FR_VideoBuildStripLook(app_state, video, video_rect, ui, picture_idx, interactive);
         }
      }

      // Lit under the mouse - a wash of the theme's body colour, which shows
      // on a light theme as on a dark one: the one piece a click there would
      // go to.
      if (interactive && hover.hit_idx >= 0 && (U64)hover.hit_idx < app_state->preview_hit_count) {
         Vec4F32 lit = VIEW_ColorsFromTheme(deck->theme).body;
         lit.w = 0.12f;
         UI_SetNextBackgroundColor(lit);
         FR_BuildRectBox(UI_BoxFlag_DrawBackground, app_state->preview_hits[hover.hit_idx].rect, "###edit preview lit");
      }

      // The words: the caret, and the box of the piece it is in
      // (frame_caret.c). The box built below is then a picture's.
      B32 words_have_box = CARET_BuildLook(app_state, ui);

      // A box around the picture the cursor is on the line of. Not after
      // Esc, until the cursor goes somewhere or a piece is clicked. In the
      // words' box's blue (frame.c's "focus").
      for (U64 i = 0; i < app_state->preview_hit_count && !words_have_box && !app_state->preview_focus_hidden; i += 1) {
         VIEW_Hit* hit = &app_state->preview_hits[i];
         if (editor->cursor >= hit->text_range.min && editor->cursor <= hit->text_range.max) {
            UI_ColorTag(Str8Lit("focus")) {
               FR_BuildEdge(hit->rect, Max(1.0f, (F32)(S32)(1.5f * ui)), UI_ColorFromSlot(UI_ColorSlot_Border), "###edit preview focus %llu",
                            (unsigned long long)i);
            }
         }
      }

      if (can_move) {
         caption = Str8Lit("Drag to move it      Shift lines it up      A corner resizes it      Right-click puts it back");
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
      Rng2F32 rect = _EDIT_ThumbRect(layout, deck->slide_count, app_state->slide_idx, i);
      if (rect.max.x > rect.min.x) {
         _EDIT_Thumb* thumb = PushArray(UI_BuildArena(), _EDIT_Thumb, 1);
         thumb->app_state = app_state;
         thumb->slide_idx = i;
         thumb->rect = rect;
         thumb->frame = (F32)(S32)(2.0f * ui);
         thumb->has_frame = (i == app_state->slide_idx || (S64)i == thumb_hot);
         UI_Box* box = FR_BuildRectBox(0, rect, "###edit thumb look %llu", (unsigned long long)i);
         box->border_color = (i == app_state->slide_idx) ? UI_ColorFromSlot(UI_ColorSlot_Border) : UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Border);
         UI_BoxEquipCustomDraw(box, _EDIT_DrawThumb, thumb);
      }
   }

   //- bgz: the + that adds a slide
   if (layout->add_slide.max.x > layout->add_slide.min.x) {
      _EDIT_Plus* plus = PushArray(UI_BuildArena(), _EDIT_Plus, 1);
      plus->rect = layout->add_slide;
      plus->half = Max(1.0f, (F32)(S32)(1.5f * ui));
      UI_Box* box = FR_BuildRectBox(0, layout->add_slide, "###edit add slide look");
      box->background_color = UI_ThemeColor(add_hot ? Str8Lit("add hot") : Str8Lit("add"), UI_ColorSlot_Background);
      box->text_color = UI_ThemeColor(add_hot ? Str8Lit("add hot") : Str8Lit("add"), UI_ColorSlot_Text);
      UI_BoxEquipCustomDraw(box, _EDIT_DrawPlus, plus);
   }

   //- bgz: the slide, under all that is over it
   if (Dim2F32(layout->page).x > 0) {
      if (deck->slide_count == 0) {
         // No slides yet: how to make the first.
         String8 lines[] = {
             Str8Lit("No slides yet"),
             Str8Lit("Type  # Title  to start one, or click a line under Format"),
         };
         F32     scale = VIEW_ScaleFromDim(Dim2F32(layout->page));
         F32     sizes[] = {44.0f * scale, 24.0f * scale};
         Vec2F32 page_dim = Dim2F32(layout->page);
         F32     y = layout->page.min.y + page_dim.y * 0.5f - TXT_LineHeight(app_state->fonts.bold, sizes[0]);
         for (U64 i = 0; i < ArrayCount(lines); i += 1) {
            FNT_Tag font = (i == 0) ? app_state->fonts.bold : app_state->fonts.regular;
            F32     line_h = TXT_LineHeight(font, sizes[i]);
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

      _EDIT_Slide* slide = PushArray(UI_BuildArena(), _EDIT_Slide, 1);
      slide->app_state = app_state;
      slide->page = layout->page;
      UI_Box* box = FR_BuildRectBox(0, layout->page, "###edit slide");
      UI_BoxEquipCustomDraw(box, _EDIT_DrawSlide, slide);
   }

   //- bgz: the preview's background, with a line where the text ends and
   // the preview begins
   UI_SetNextColorTag(Str8Lit("divider"));
   FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(layout->preview.min.x, 0, layout->preview.min.x + 1.0f, layout->preview.max.y),
                   "###edit preview divider");
   UI_SetNextColorTag(Str8Lit("preview"));
   FR_BuildRectBox(UI_BoxFlag_DrawBackground, layout->preview, "###edit preview");
}

typedef struct _EDIT_Pane _EDIT_Pane;
struct _EDIT_Pane {
   ED_Editor* editor;
   Arena*     arena; // the frame's, for the rows it lays out
   ED_Box     box;
};

// The text, as the editor draws it.
internal void
_EDIT_DrawPane(UI_Box* box, void* user_data) {
   UnusedVariable(box);
   _EDIT_Pane* pane = (_EDIT_Pane*)user_data;
   ED_Draw(pane->editor, pane->arena, pane->box);
}

internal void
EDIT_Build(App_State* app_state, Vec2F32 window_dim, B32 interactive) {
   ED_Editor*   editor = app_state->editor;
   TXT_Fonts*   fonts = &app_state->fonts;
   _EDIT_Layout layout = _EDIT_LayoutFromWindowDim(app_state, window_dim);
   F32          ui = layout.ui;
   F32          pad = 12.0f * ui;
   F32          left_w = Dim2F32(layout.status).x;
   B32          shows_pictures = (app_state->show_format && app_state->show_pictures);
   Temp         scratch = ScratchBegin(0, 0);

   // What a click asked for, done once everything is built: it changes the
   // text, and the panel's own layout.
   B32 toggle_format = 0;
   S64 format_picked = -1;
   S64 picture_picked = -1;
   B32 size_picked = 0;
   S32 size_direction = 0; // as LOOK_StepSize's
   S64 color_picked = -1;  // as LOOK_SetColor's named_idx

   UI_BoxFlags click_flags = interactive ? UI_BoxFlag_Clickable : 0;

   //- bgz: the text: a box the editor draws in, whose mouse is the editor's
   // - a press, a drag, the wheel. Its keys are EDIT_Input's.
   {
      _EDIT_Pane* pane = PushArray(UI_BuildArena(), _EDIT_Pane, 1);
      pane->editor = editor;
      pane->arena = app_state->frame_arena;
      pane->box = layout.editor_box;
      UI_Box* box = FR_BuildRectBox(interactive ? (UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll) : 0, layout.editor_box.rect, "###edit text");
      UI_BoxEquipCustomDraw(box, _EDIT_DrawPane, pane);
      if (interactive) {
         UI_Signal signal = UI_SignalFromBox(box);
         if (signal.f & UI_SignalFlag_LeftPressed) {
            ED_Press(editor, app_state->frame_arena, layout.editor_box, UI_DragStartMouse(), !!(signal.event_flags & WM_Modifier_Shift));
         } else if (UI_Dragging(signal)) {
            // Over the box or not, until the button comes up. The window's
            // mouse: the UI's is off the window when the window is not in
            // front and the mouse has not moved for a moment.
            ED_Drag(editor, app_state->frame_arena, layout.editor_box, Core_WM_MouseFromWindow(app_state->window));
         }
         if (signal.scroll.y != 0) {
            ED_Wheel(editor, layout.editor_box, (F32)signal.scroll.y / FR_UI_SCROLL_PER_NOTCH);
         }
      }
   }

   UI_SetNextRect(R2F32P(0, layout.format_header.min.y, left_w, layout.status.max.y));
   UI_SetNextChildLayoutAxis(Axis2_Y);
   UI_SetNextColorTag(Str8Lit("panel"));
   UI_Box* column = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clip, "###edit format column");

   UI_Parent(column) UI_FontSize(FR_UIFontSize(layout.ui_px)) {
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
            _EDIT_BuildBarText(pad, Str8Lit(""), left, fonts->bold, right);
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

            if (shows_pictures && app_state->pictures.count == 0) {
               UI_SetNextPrefWidth(UI_Px(left_w, 1));
               UI_SetNextPrefHeight(UI_Px(row_h, 1));
               UI_Row {
                  _EDIT_BuildBarText(pad, Str8Lit("dim"),
                                     Str8Lit("No pictures in the deck's folder yet. Drop one on the window to add it."),
                                     UI_TopFont(), Str8Zero());
               }
            } else if (shows_pictures) {
               //- bgz: the names of the folder's pictures, however many there are
               UI_ScrollRowsFlags scroll_flags = UI_ScrollRowsFlag_RowClicks;
               UI_ScrollRowsFrame frame = UI_ScrollRowsBegin(&app_state->pictures_scroll,
                                                             V2F32(left_w, Max(rows_h - edge * 2.0f, row_h)),
                                                             (S64)app_state->pictures.count, row_h, 0, scroll_flags,
                                                             "###edit pictures");
               UI_ScrollRowsEnd(&app_state->pictures_scroll, &frame, 0, scroll_flags);
               UI_Parent(frame.rows_box) {
                  for (S64 i = frame.first_row; i < frame.last_row; i += 1) {
                     UI_Box* row = UI_ListRow(row_h, 0, UI_BoxFlag_DrawBackground | click_flags, "###edit picture %lld", i);
                     UI_Parent(row) {
                        _EDIT_BuildBarText(pad, Str8Lit("picture"), app_state->pictures.v[i], fonts->mono, Str8Zero());
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
                  syntax_w = Max(syntax_w, TXT_Width(fonts->mono, layout.ui_px, _edit_format_rows[i].syntax));
               }

               // A list that scrolls, for a short window, where they do not
               // all fit (_EDIT_LayoutFromWindowDim). With room for all, it
               // has no bar and does not move.
               UI_ScrollRowsFlags scroll_flags = UI_ScrollRowsFlag_RowClicks;
               UI_ScrollRowsFrame frame = UI_ScrollRowsBegin(&app_state->format_scroll,
                                                             V2F32(left_w, Max(rows_h - edge * 2.0f, row_h)),
                                                             (S64)ArrayCount(_edit_format_rows), row_h, 0, scroll_flags,
                                                             "###edit format lines");
               UI_ScrollRowsEnd(&app_state->format_scroll, &frame, 0, scroll_flags);

               UI_Parent(frame.rows_box) for (S64 i = frame.first_row; i < frame.last_row; i += 1) {
                  _EDIT_FormatRow* format_row = &_edit_format_rows[i];

                  // The theme's and the layout's rows say what there is now.
                  // On the frame arena: a label's words are needed until the
                  // frame is drawn.
                  String8 what = format_row->what;
                  if (format_row->action == _EDIT_FormatAction_Theme) {
                     what = PushStr8F(app_state->frame_arena, "the deck's colours: %S. Click for the next",
                                      DECK_ThemeName(app_state->deck->theme));
                  } else if (format_row->action == _EDIT_FormatAction_Layout) {
                     DECK_Deck* deck = app_state->deck;
                     String8    now = (app_state->slide_idx < deck->slide_count)
                                          ? DECK_LayoutName(deck->slides[app_state->slide_idx].layout)
                                          : DECK_LayoutName(DECK_Layout_PictureRight);
                     what = PushStr8F(app_state->frame_arena, "this slide's layout: %S. Click for the next", now);
                  } else if (format_row->action == _EDIT_FormatAction_Size) {
                     what = LOOK_Describe(app_state->frame_arena, app_state, 0);
                  } else if (format_row->action == _EDIT_FormatAction_Color) {
                     what = LOOK_Describe(app_state->frame_arena, app_state, 1);
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
                     if (format_row->action == _EDIT_FormatAction_Size) {
                        String8 names[] = {Str8Lit("smaller"), Str8Lit("larger"), Str8Lit("usual")};
                        S32     directions[] = {-1, +1, 0};
                        B32     is_usual = LOOK_SizeIsUsual(app_state);
                        UI_Spacer(UI_Px(6.0f * ui, 1));
                        for (U64 k = 0; k < ArrayCount(names); k += 1) {
                           if (_EDIT_BuildRowButton(names[k], is_usual && directions[k] == 0, row_h, ui, interactive,
                                                    "###edit size %llu", (unsigned long long)k)) {
                              size_picked = 1;
                              size_direction = directions[k];
                           }
                        }
                        UI_Spacer(UI_Px(pad - 6.0f * ui, 1));
                     } else if (format_row->action == _EDIT_FormatAction_Color) {
                        // The squares as large as the row leaves room for,
                        // so that all of them and the theme's fit in a narrow
                        // window too: the row's width less what is before
                        // them, the theme's word and the room at the end.
                        F32 theme_w = TXT_Width(fonts->regular, layout.ui_px, Str8Lit("theme's")) + 14.0f * ui + 6.0f * ui;
                        F32 room = left_w - pad - syntax_w - 18.0f * ui - 6.0f * ui - theme_w - pad;
                        F32 each = room / (F32)DECK_NAMED_COLOR_COUNT; // a square and the gap after it
                        F32 gap = Clamp(2.0f * ui, (F32)(S32)(each * 0.25f), 6.0f * ui);
                        F32 side = (F32)(S32)Clamp(6.0f * ui, each - gap, row_h * 0.62f);
                        UI_Spacer(UI_Px(6.0f * ui, 1));
                        for (U64 k = 0; k < DECK_NAMED_COLOR_COUNT; k += 1) {
                           if (_EDIT_BuildSwatch(DECK_NamedColorFromIdx(k).color, LOOK_ColorIsNow(app_state, k), side, gap, row_h,
                                                 ui, interactive, "###edit color %llu", (unsigned long long)k)) {
                              color_picked = (S64)k;
                           }
                        }
                        if (_EDIT_BuildRowButton(Str8Lit("theme's"), LOOK_ColorIsNow(app_state, DECK_NAMED_COLOR_COUNT), row_h,
                                                 ui, interactive, "###edit color theme")) {
                           color_picked = DECK_NAMED_COLOR_COUNT;
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
               _EDIT_BuildBarText(pad, Str8Lit("notice"), notice, UI_TopFont(), Str8Zero());
            } else {
               String8 name = Str8SkipLastSlash(Str8(editor->file.str, editor->file.size));
               String8 left = PushStr8F(scratch.arena, "%S%s", name, ED_HasUnsavedChanges(editor) ? "  (not saved)" : "");
               _EDIT_BuildBarText(pad, Str8Lit(""), left, UI_TopFont(), Str8Lit("Ctrl+O open    Ctrl+S save    F5 present"));
            }
         }
      }
   }

   //- bgz: what was clicked
   //
   // The deck is made again from the text at once, so the preview drawn this
   // frame is of the text as it now is.
   if (toggle_format) {
      _EDIT_ToggleFormat(app_state);
   }
   if (picture_picked >= 0 && (U64)picture_picked < app_state->pictures.count) {
      _EDIT_PutPicture(app_state, app_state->pictures.v[picture_picked], 0);
      app_state->show_pictures = 0;
   }
   if (format_picked >= 0) {
      _EDIT_FormatRow* row = &_edit_format_rows[format_picked];
      if (row->action == _EDIT_FormatAction_Pictures) {
         app_state->show_pictures = 1;
         MemoryZeroStruct(&app_state->pictures_scroll); // the list starts at its top
         _EDIT_ScanPictures(app_state);
      } else if (row->action == _EDIT_FormatAction_Theme || row->action == _EDIT_FormatAction_Layout) {
         // The deck as the text is now, to know what there is to change.
         FR_SyncDeck(app_state);
         if (row->action == _EDIT_FormatAction_Theme) {
            _EDIT_NextTheme(editor, app_state->deck);
         } else {
            _EDIT_NextLayout(editor, app_state->deck, DECK_SlideFromTextPos(app_state->deck, editor->cursor));
         }
         editor->last_input_us = NowTimeUS();
      } else if (row->action == _EDIT_FormatAction_Size || row->action == _EDIT_FormatAction_Color) {
         // The row's own click does nothing: what is in it is what is clicked.
      } else if (row->action == _EDIT_FormatAction_Wrap && !_EDIT_CanWrap(app_state)) {
         // Said by the notice.
      } else if (row->action == _EDIT_FormatAction_Wrap) {
         B32 is_bold = Str8Match(row->insert, Str8Lit("**"), 0);
         ED_WrapSelection(editor, row->insert, is_bold ? Str8Lit("bold") : Str8Lit("code"));
         editor->last_input_us = NowTimeUS();
      } else {
         ED_InsertLine(editor, row->insert, row->select_start, row->select_end);
         editor->last_input_us = NowTimeUS();
      }
   }
   // The words the cursor is in, sized and coloured (frame_look.c).
   if (size_picked) {
      LOOK_StepSize(app_state, size_direction);
   }
   if (color_picked >= 0) {
      LOOK_SetColor(app_state, (U64)color_picked);
   }
   if (picture_picked >= 0 || format_picked >= 0) {
      FR_SyncDeck(app_state);
      app_state->slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);
   }

   //- bgz: the preview: its slide, and what is over it
   _EDIT_BuildPreview(app_state, &layout, interactive);

   ScratchEnd(scratch);
}

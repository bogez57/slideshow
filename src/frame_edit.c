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
// The Format panel, the list of pictures and the status line are built with
// the UI layer (EDIT_Build, called from FrameWork's UI build), and take their
// own clicks and wheel there. The text, the preview, the small slides and
// the + are drawn by hand (EDIT_Draw), and their clicks are EDIT_Input's.
//
// The slide in the preview can be clicked: on its title, a line of its body
// or its picture, and the cursor goes to that line of the text, with the
// words selected, to be typed over. What the mouse is over is lit, and what
// the cursor is in has a mark beside it.
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
//    pictures         click the picture line under Format. Esc closes the list,
//                     the wheel or the scroll bar moves through it
//    edit a piece     click it in the preview
//    add a picture    drop an image on the window
//    add a slide      click the + at the end of the row of small slides
//    move a slide     Alt+Left earlier, Alt+Right later
//    bold, code       Ctrl+B and Ctrl+E: ** or ` around what is selected
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
};

global _EDIT_FormatRow _edit_format_rows[] = {
    {Str8LitComp("# Title"), Str8LitComp("starts a new slide, with a title"), Str8LitComp("# Title"), 2, 7},
    {Str8LitComp("---"), Str8LitComp("starts a new slide with no title"), Str8LitComp("---\n"), 4, 4},
    {Str8LitComp("## Heading"), Str8LitComp("a heading inside the slide"), Str8LitComp("## Heading"), 3, 10},
    {Str8LitComp("- point"), Str8LitComp("a bullet. Tab moves it in"), Str8LitComp("- point"), 2, 7},
    {Str8LitComp("  - point"), Str8LitComp("an inner bullet. Shift+Tab moves it out"), Str8LitComp("  - point"), 4, 9},
    {Str8LitComp("![](picture.png)"), Str8LitComp("the slide's picture. Click to pick one"), Str8LitComp("![](picture.png)"), 4, 15, _EDIT_FormatAction_Pictures},
    {Str8LitComp("Any other line"), Str8LitComp("a paragraph. An empty line adds room"), Str8LitComp("Text"), 0, 4},
    {Str8LitComp("**bold**"), Str8LitComp("bold words. Select some first, or Ctrl+B"), Str8LitComp("**"), 0, 0, _EDIT_FormatAction_Wrap},
    {Str8LitComp("`code`"), Str8LitComp("words as code. Select some first, or Ctrl+E"), Str8LitComp("`"), 0, 0, _EDIT_FormatAction_Wrap},
    {Str8LitComp("@note Say this"), Str8LitComp("a note to yourself. N shows notes while presenting"), Str8LitComp("@note Say this"), 6, 14},
    // What these two do is said in EDIT_Build, with what the deck and the slide have now.
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

// Makes name the picture of the slide the cursor is on: in place of the
// picture line the slide has, if it has one, else on a new line at the
// cursor. A slide shows only its first picture, so a second line would do
// nothing.
internal void
_EDIT_PutPicture(App_State* app_state, String8 name) {
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

   // Its first line that starts with ![ - spaces before it aside.
   B32 found = 0;
   U64 pos = slide_start;
   while (pos < slide_end && !found) {
      U64 line_start = pos;
      while (pos < slide_end && text.str[pos] != '\n') pos += 1;
      U64 line_end = pos;
      pos += 1;

      U64 first = line_start;
      while (first < line_end && text.str[first] == ' ') first += 1;
      if (first + 1 < line_end && text.str[first] == '!' && text.str[first + 1] == '[') {
         ED_ReplaceRange(editor, line_start, line_end, line);
         found = 1;
      }
   }
   if (!found) {
      ED_InsertLine(editor, line, line.size, line.size);
   }
   editor->last_input_us = NowTimeUS();

   ScratchEnd(scratch);
}

// An image file dropped on the window: it becomes the slide's picture. A
// file from somewhere else is copied into the deck's folder first, so the
// deck and its pictures stay together - unless a file of that name is there
// already, and then that one is used.
internal void
_EDIT_DropPicture(App_State* app_state, String8 path) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 name = Str8SkipLastSlash(path);
   String8 in_folder = PushStr8F(scratch.arena, "%S\\%S", app_state->deck->folder, name);

   B32 is_there = FilePathExists(in_folder);
   if (!is_there) {
      is_there = CopyFilePath(in_folder, path);
      if (is_there) {
         FR_Notice(app_state, Str8Lit("Picture copied into the deck's folder"));
      } else {
         FR_Notice(app_state, Str8Lit("Could not copy the picture into the deck's folder"));
      }
   }
   if (is_there) {
      _EDIT_PutPicture(app_state, name);
      app_state->pictures_scanned_us = 0; // the list, if open, is out of date
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: input

// A left press inside rect, eaten. 0 if there was none.
internal B32
_EDIT_ClickIn(App_State* app_state, Rng2F32 rect) {
   B32 result = 0;
   for (WM_Event* event = app_state->events.first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_Press && event->key == WM_Key_LeftMouseButton &&
          Contains2F32(rect, event->pos)) {
         Core_WM_EatEvent(&app_state->events, event);
         result = 1;
         break;
      }
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

// Where the slide's video is drawn in the preview, if the slide showing has
// one that is open: the place the preview last drew its picture. 0 if not,
// or if the text has changed since the preview was drawn.
internal VID_Video*
_EDIT_PreviewVideo(App_State* app_state, Rng2F32* rect_out) {
   VID_Video* result = 0;
   DECK_Deck* deck = app_state->deck;
   VID_Video* video = DECK_VideoFromSlide(deck, app_state->slide_idx);

   if (video != 0 && app_state->editor != 0 && app_state->editor->edit_count == app_state->preview_hits_edit_count) {
      Rng1U64 range = deck->slides[app_state->slide_idx].image_range;
      for (U64 i = 0; i < app_state->preview_hit_count; i += 1) {
         VIEW_Hit* hit = &app_state->preview_hits[i];
         if (hit->text_range.min == range.min && hit->text_range.max == range.max) {
            *rect_out = hit->rect;
            result = video;
            break;
         }
      }
   }

   return result;
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

   //- bgz: the screen's own keys, taken before the editor sees them
   B32 is_fullscreen = Core_WM_WindowIsFullscreen(window);
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F11) ||
       Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Return)) {
      app_state->toggle_fullscreen = 1;
   }
   // Esc closes what is open: the list of pictures, then fullscreen.
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
      if (app_state->show_pictures) {
         app_state->show_pictures = 0;
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
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_B)) {
      ED_WrapSelection(editor, Str8Lit("**"), Str8Lit("bold"));
      editor->last_input_us = NowTimeUS();
   }
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_E)) {
      ED_WrapSelection(editor, Str8Lit("`"), Str8Lit("code"));
      editor->last_input_us = NowTimeUS();
   }

   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_S)) {
      if (ED_Save(editor)) {
         FR_Notice(app_state, Str8Lit("Saved"));
         app_state->quit_warned = 0;

         // A picture that was not found is looked for again: saving is when
         // the file is likely to have been put in place.
         for (U64 i = 0; i < app_state->deck->slide_count; i += 1) {
            app_state->deck->slides[i].load_failed = 0;
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

   //- bgz: a click on a piece of the slide in the preview: to its words
   //
   // Before anything this frame can have changed the text: where the pieces
   // are from was worked out for the text as it was drawn.
   // The video's strip of controls first: it is over the picture, and a
   // click on it is not a click on the picture.
   {
      Rng2F32    video_rect = {0};
      VID_Video* video = _EDIT_PreviewVideo(app_state, &video_rect);
      if (video != 0) {
         for (WM_Event* event = events->first; event != 0; event = event->next) {
            if (event->kind == WM_EventKind_Press && event->key == WM_Key_LeftMouseButton &&
                FR_VideoClickStrip(app_state, video, video_rect, layout.ui, event->pos)) {
               Core_WM_EatEvent(events, event);
               break;
            }
         }
      }
   }

   if (editor->edit_count == app_state->preview_hits_edit_count) {
      for (U64 i = 0; i < app_state->preview_hit_count; i += 1) {
         VIEW_Hit* hit = &app_state->preview_hits[i];
         if (hit->text_range.max <= editor->size && _EDIT_ClickIn(app_state, hit->rect)) {
            ED_SetCursor(editor, hit->text_range.max);
            editor->mark = hit->text_range.min;
            editor->last_input_us = NowTimeUS();
            break;
         }
      }
   }

   //- bgz: slides added and moved
   {
      B32 move_earlier = Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Left);
      B32 move_later = Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Right);
      B32 add = _EDIT_ClickIn(app_state, layout.add_slide);
      if (move_earlier || move_later || add) {
         // The deck as the text is now: something above may have changed it.
         FR_SyncDeck(app_state);
         U64 slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);
         if (add) {
            _EDIT_AddSlideAfter(editor, app_state->deck, slide_idx);
         } else if (!_EDIT_MoveSlide(editor, app_state->deck, slide_idx, move_later)) {
            FR_Notice(app_state, move_later ? Str8Lit("This is the last slide already")
                                            : Str8Lit("This is the first slide already"));
         }
         editor->last_input_us = NowTimeUS();
         FR_SyncDeck(app_state);
         app_state->slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);
      }
   }

   //- bgz: the row of small slides: a click goes to the slide
   {
      DECK_Deck* deck = app_state->deck;
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         Rng2F32 rect = _EDIT_ThumbRect(&layout, deck->slide_count, app_state->slide_idx, i);
         if (rect.max.x > rect.min.x && _EDIT_ClickIn(app_state, rect)) {
            ED_SetCursor(editor, Min(deck->slides[i].text_pos, editor->size));
            editor->last_input_us = NowTimeUS();
            break;
         }
      }
   }

   //- bgz: everything else is the editor's
   ED_Input(editor, app_state->frame_arena, events, window, layout.editor_box);

   //- bgz: the deck, and the slide the cursor is on
   FR_SyncDeck(app_state);
   app_state->slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);

   if (present_pressed) {
      if (app_state->deck->slide_count != 0) {
         app_state->is_editing = 0;
         app_state->preview_hit_count = 0; // not drawn while presenting, and the window can change
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

   UI_BoxFlags click_flags = interactive ? UI_BoxFlag_Clickable : 0;

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
      _EDIT_PutPicture(app_state, app_state->pictures.v[picture_picked]);
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
      } else if (row->action == _EDIT_FormatAction_Wrap) {
         B32 is_bold = Str8Match(row->insert, Str8Lit("**"), 0);
         ED_WrapSelection(editor, row->insert, is_bold ? Str8Lit("bold") : Str8Lit("code"));
         editor->last_input_us = NowTimeUS();
      } else {
         ED_InsertLine(editor, row->insert, row->select_start, row->select_end);
         editor->last_input_us = NowTimeUS();
      }
   }
   if (picture_picked >= 0 || format_picked >= 0) {
      FR_SyncDeck(app_state);
      app_state->slide_idx = DECK_SlideFromTextPos(app_state->deck, editor->cursor);
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: drawing

internal void
EDIT_Draw(App_State* app_state, Vec2F32 window_dim) {
   ED_Editor*   editor = app_state->editor;
   DECK_Deck*   deck = app_state->deck;
   TXT_Fonts*   fonts = &app_state->fonts;
   _EDIT_Layout layout = _EDIT_LayoutFromWindowDim(app_state, window_dim);
   F32          ui = layout.ui;
   Vec2F32      mouse = Core_WM_MouseFromWindow(app_state->window);
   Temp         scratch = ScratchBegin(0, 0);

   Vec4F32 color_text = V4F32(0.84f, 0.86f, 0.90f, 1.0f);
   Vec4F32 color_dim = V4F32(0.84f, 0.86f, 0.90f, 0.55f);
   Vec4F32 color_syntax = V4F32(1.00f, 0.80f, 0.40f, 1.0f);

   //- bgz: the text. The Format panel and the status line under it are the
   // UI's (EDIT_Build), drawn after this.
   ED_Draw(editor, app_state->frame_arena, layout.editor_box);

   //- bgz: the preview
   app_state->preview_hit_count = 0; // made again below, if a slide is drawn
   Core_D_Rect(layout.preview, R2F32P(0, 0, 1, 1), V4F32(0.125f, 0.13f, 0.15f, 1.0f));
   // A line where the text ends and the preview begins.
   Core_D_Rect(R2F32P(layout.preview.min.x, 0, layout.preview.min.x + 1.0f, window_dim.y), R2F32P(0, 0, 1, 1),
               V4F32(0, 0, 0, 0.5f));

   if (Dim2F32(layout.page).x > 0) {
      Core_D_Rect(layout.page, R2F32P(0, 0, 1, 1), VIEW_ColorsFromTheme(deck->theme).background);

      String8 caption = {0};
      if (app_state->slide_idx < deck->slide_count) {
         // Asking for the picture is what loads it, and what releases the
         // pictures of slides far from this one.
         IMG_Image* image = DECK_ImageFromSlide(deck, app_state->slide_idx);
         VIEW_HitList hits = {0};
         VIEW_DrawSlide(app_state->frame_arena, fonts, deck->theme, &deck->slides[app_state->slide_idx], image, layout.page, 1.0f,
                        &hits);

         //- bgz: the slide's pieces, kept for the next frame's clicks
         if (app_state->preview_hits_arena == 0) {
            app_state->preview_hits_arena = ArenaAlloc();
         }
         ArenaClear(app_state->preview_hits_arena);
         app_state->preview_hits = PushArray(app_state->preview_hits_arena, VIEW_Hit, Max(hits.count, 1));
         app_state->preview_hit_count = 0;
         // The deck drawn is of the text at parsed_edit_count.
         app_state->preview_hits_edit_count = app_state->parsed_edit_count;

         F32 grow = 4.0f * ui;
         for (VIEW_Hit* hit = hits.first; hit != 0; hit = hit->next) {
            // A little larger than what was drawn, to be easy to click, and
            // no larger than the slide.
            Rng2F32 rect = Intersect2F32(R2F32P(hit->rect.min.x - grow, hit->rect.min.y - grow * 0.5f, hit->rect.max.x + grow,
                                                hit->rect.max.y + grow * 0.5f),
                                         layout.page);
            if (rect.max.x <= rect.min.x || rect.max.y <= rect.min.y) {
               continue;
            }
            VIEW_Hit* kept = &app_state->preview_hits[app_state->preview_hit_count];
            app_state->preview_hit_count += 1;
            kept->next = 0;
            kept->rect = rect;
            kept->text_range = hit->text_range;

            // Lit under the mouse - a wash of the theme's body colour, which
            // shows on a light theme as on a dark one. And a mark beside the
            // piece the cursor is in: where typing goes.
            if (Contains2F32(rect, mouse) && !app_state->show_open) {
               Vec4F32 lit = VIEW_ColorsFromTheme(deck->theme).body;
               lit.w = 0.12f;
               Core_D_Rect(rect, R2F32P(0, 0, 1, 1), lit);
            }
            if (editor->cursor >= hit->text_range.min && editor->cursor <= hit->text_range.max) {
               F32 mark_w = Max(2.0f, (F32)(S32)(3.0f * ui));
               Core_D_Rect(R2F32P(rect.min.x - mark_w, rect.min.y, rect.min.x, rect.max.y), R2F32P(0, 0, 1, 1),
                           VIEW_ColorsFromTheme(deck->theme).accent);
            }
         }
         // The slide's video: its strip of controls, over the bottom of it.
         {
            Rng2F32    video_rect = {0};
            VID_Video* video = _EDIT_PreviewVideo(app_state, &video_rect);
            if (video != 0) {
               FR_VideoDrawStrip(app_state, video, video_rect, ui, mouse);
            }
         }
         caption = PushStr8F(scratch.arena, "Slide %llu of %llu      Alt+Left / Alt+Right moves it",
                             (unsigned long long)(app_state->slide_idx + 1), (unsigned long long)deck->slide_count);
      } else {
         // No slides yet: how to make the first.
         String8 lines[] = {
             Str8Lit("No slides yet"),
             Str8Lit("Type  # Title  to start one, or click a line under Format"),
         };
         F32     scale = VIEW_ScaleFromDim(Dim2F32(layout.page));
         F32     sizes[] = {44.0f * scale, 24.0f * scale};
         Vec2F32 page_dim = Dim2F32(layout.page);
         F32     y = layout.page.min.y + page_dim.y * 0.5f - TXT_LineHeight(fonts->bold, sizes[0]);
         for (U64 i = 0; i < ArrayCount(lines); i += 1) {
            FNT_Tag font = (i == 0) ? fonts->bold : fonts->regular;
            F32     w = TXT_Width(font, sizes[i], lines[i]);
            TXT_Draw(font, sizes[i], V2F32(layout.page.min.x + (page_dim.x - w) * 0.5f, y),
                     (i == 0) ? color_text : color_dim, lines[i]);
            y += TXT_LineHeight(font, sizes[i]) * 1.3f;
         }
      }

      if (caption.size != 0) {
         F32 caption_w = TXT_Width(fonts->regular, layout.ui_px, caption);
         F32 caption_text_h = TXT_LineHeight(fonts->regular, layout.ui_px);
         TXT_Draw(fonts->regular, layout.ui_px,
                  V2F32(layout.caption.min.x + (Dim2F32(layout.caption).x - caption_w) * 0.5f,
                        layout.caption.min.y + (Dim2F32(layout.caption).y - caption_text_h) * 0.5f),
                  color_dim, caption);
      }
   }

   //- bgz: the row of small slides, the one showing with a frame around it
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      Rng2F32 rect = _EDIT_ThumbRect(&layout, deck->slide_count, app_state->slide_idx, i);
      if (rect.max.x > rect.min.x) {
         B32 is_showing = (i == app_state->slide_idx);
         if (is_showing || Contains2F32(rect, mouse)) {
            F32 frame = (F32)(S32)(2.0f * ui);
            Core_D_Rect(R2F32P(rect.min.x - frame, rect.min.y - frame, rect.max.x + frame, rect.max.y + frame),
                        R2F32P(0, 0, 1, 1), is_showing ? color_syntax : V4F32(1, 1, 1, 0.35f));
         }
         Core_D_Rect(rect, R2F32P(0, 0, 1, 1), VIEW_ColorsFromTheme(deck->theme).background);

         // Only a picture that is loaded already is drawn: the slide showing
         // and the ones next to it. Loading every picture of a long deck for
         // its small slides would take the memory DECK_ImageFromSlide saves.
         VIEW_DrawSlide(app_state->frame_arena, fonts, deck->theme, &deck->slides[i], DECK_LoadedImageFromSlide(deck, i), rect,
                        is_showing ? 1.0f : 0.8f, 0);
      }
   }

   //- bgz: the + that adds a slide
   if (layout.add_slide.max.x > layout.add_slide.min.x) {
      B32 is_hot = Contains2F32(layout.add_slide, mouse);
      Core_D_Rect(layout.add_slide, R2F32P(0, 0, 1, 1), is_hot ? V4F32(1, 1, 1, 0.16f) : V4F32(1, 1, 1, 0.07f));

      // Drawn, not written: a + of two bars, in the middle.
      Vec2F32 dim = Dim2F32(layout.add_slide);
      F32     middle_x = (F32)(S32)(layout.add_slide.min.x + dim.x * 0.5f);
      F32     middle_y = (F32)(S32)(layout.add_slide.min.y + dim.y * 0.5f);
      F32     arm = (F32)(S32)(Min(dim.x, dim.y) * 0.22f);
      F32     half = Max(1.0f, (F32)(S32)(1.5f * ui));
      Core_D_Rect(R2F32P(middle_x - arm, middle_y - half, middle_x + arm, middle_y + half), R2F32P(0, 0, 1, 1),
                  is_hot ? color_text : color_dim);
      Core_D_Rect(R2F32P(middle_x - half, middle_y - arm, middle_x + half, middle_y + arm), R2F32P(0, 0, 1, 1),
                  is_hot ? color_text : color_dim);
   }

   ScratchEnd(scratch);
}

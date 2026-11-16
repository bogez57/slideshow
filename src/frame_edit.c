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
// one writes it into the text at the cursor, ready to be typed over.
//
// Clicking the picture line there opens, in the panel's place, the list of
// the pictures in the deck's folder: clicking one puts it on the slide.
// Dropping an image file on the window does the same, copying the file into
// the deck's folder first if it is not there.
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
//    pictures         click the picture line under Format. Esc closes the list
//    edit a piece     click it in the preview
//    add a picture    drop an image on the window
//    add a slide      click the + at the end of the row of small slides
//    move a slide     Alt+Left earlier, Alt+Right later
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
   B32     opens_pictures; // a click opens the list of pictures instead
};

global _EDIT_FormatRow _edit_format_rows[] = {
    {Str8LitComp("# Title"), Str8LitComp("starts a new slide, with a title"), Str8LitComp("# Title"), 2, 7},
    {Str8LitComp("---"), Str8LitComp("starts a new slide with no title"), Str8LitComp("---\n"), 4, 4},
    {Str8LitComp("## Heading"), Str8LitComp("a heading inside the slide"), Str8LitComp("## Heading"), 3, 10},
    {Str8LitComp("- point"), Str8LitComp("a bullet. Tab moves it in, Shift+Tab out"), Str8LitComp("- point"), 2, 7},
    {Str8LitComp("  - point"), Str8LitComp("an inner bullet: two more spaces in front"), Str8LitComp("  - point"), 4, 9},
    {Str8LitComp("![](picture.png)"), Str8LitComp("the slide's picture. Click to pick one"), Str8LitComp("![](picture.png)"), 4, 15, 1},
    {Str8LitComp("Any other line"), Str8LitComp("a paragraph. An empty line adds room"), Str8LitComp("Text"), 0, 4},
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

   // Sizes are for a window 900 pixels tall, and grow with a taller one - a
   // large or dense screen - but do not shrink with a shorter.
   F32 ui = Clamp(1.0f, window_dim.y / 900.0f, 2.0f);
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
   F32 rows_h = app_state->show_format ? row_h * (F32)ArrayCount(_edit_format_rows) + 8.0f * ui : 0;

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

internal Rng2F32
_EDIT_FormatRowRect(_EDIT_Layout* layout, U64 row_idx) {
   F32 y = layout->format_rows.min.y + 4.0f * layout->ui + layout->format_row_h * (F32)row_idx;
   return R2F32P(layout->format_rows.min.x, y, layout->format_rows.max.x, y + layout->format_row_h);
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
// # Title or --- line and everything under it. The first slide's begins at
// the text's beginning, whatever is there. The deck has to be of the text as
// it is: FR_SyncDeck first.

internal Rng1U64
_EDIT_TextRangeFromSlide(ED_Editor* editor, DECK_Deck* deck, U64 slide_idx) {
   Rng1U64 result = R1U64(0, editor->size);
   if (slide_idx < deck->slide_count) {
      if (slide_idx != 0) {
         result.min = Min(deck->slides[slide_idx].text_pos, editor->size);
      }
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

   U64 row_count = ArrayCount(_edit_format_rows);
   U64 max_first = (app_state->pictures.count > row_count) ? app_state->pictures.count - row_count : 0;
   app_state->pictures_first_row = Min(app_state->pictures_first_row, max_first);
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

internal void
EDIT_Input(App_State* app_state, Vec2F32 window_dim) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;

   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         FR_RequestQuit(app_state);
      }
      // Of several things dropped at once, the first is used. An image is a
      // picture for the slide. Anything else is a deck to open.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         String8 path = event->strings.first->string;
         B32     is_folder = !!(PropertiesFromFilePath(path).flags & FilePropertyFlag_IsFolder);
         if (!is_folder && IMG_NameIsImage(path)) {
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

   // The header closes the list of pictures if that is what is open, else
   // opens and closes the panel.
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F1) || _EDIT_ClickIn(app_state, layout.format_header)) {
      if (app_state->show_pictures) {
         app_state->show_pictures = 0;
      } else {
         app_state->show_format = !app_state->show_format;
      }
      layout = _EDIT_LayoutFromWindowDim(app_state, window_dim);
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

   //- bgz: the panel: the list of pictures, or the format's lines
   U64 panel_row_count = ArrayCount(_edit_format_rows);
   if (app_state->show_format && app_state->show_pictures) {
      U64 scan_us = (U64)(_EDIT_PICTURES_SCAN_SECONDS * 1000000.0f);
      if (NowTimeUS() - app_state->pictures_scanned_us >= scan_us) {
         _EDIT_ScanPictures(app_state);
      }

      // The wheel, over the list, moves through it a row a notch.
      if (Contains2F32(layout.format_rows, Core_WM_MouseFromWindow(window))) {
         WM_Event* next_event = 0;
         for (WM_Event* event = events->first; event != 0; event = next_event) {
            next_event = event->next;
            if (event->kind == WM_EventKind_Scroll) {
               U64 max_first = (app_state->pictures.count > panel_row_count) ? app_state->pictures.count - panel_row_count : 0;
               if (event->delta.y > 0 && app_state->pictures_first_row < max_first) {
                  app_state->pictures_first_row += 1;
               } else if (event->delta.y < 0 && app_state->pictures_first_row > 0) {
                  app_state->pictures_first_row -= 1;
               }
               Core_WM_EatEvent(events, event);
            }
         }
      }

      // A click on a name puts that picture on the slide, and closes the list.
      for (U64 i = 0; i < panel_row_count; i += 1) {
         U64 picture_idx = app_state->pictures_first_row + i;
         if (_EDIT_ClickIn(app_state, _EDIT_FormatRowRect(&layout, i)) && picture_idx < app_state->pictures.count) {
            _EDIT_PutPicture(app_state, app_state->pictures.v[picture_idx]);
            app_state->show_pictures = 0;
            break;
         }
      }
   } else if (app_state->show_format) {
      // A click writes the line - or, on the picture line, opens the list.
      for (U64 i = 0; i < panel_row_count; i += 1) {
         if (_EDIT_ClickIn(app_state, _EDIT_FormatRowRect(&layout, i))) {
            _EDIT_FormatRow* row = &_edit_format_rows[i];
            if (row->opens_pictures) {
               app_state->show_pictures = 1;
               app_state->pictures_first_row = 0;
               _EDIT_ScanPictures(app_state);
            } else {
               ED_InsertLine(editor, row->insert, row->select_start, row->select_end);
               editor->last_input_us = NowTimeUS();
            }
            break;
         }
      }
   }

   //- bgz: a click on a piece of the slide in the preview: to its words
   //
   // Before anything this frame can have changed the text: where the pieces
   // are from was worked out for the text as it was drawn.
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

   //- bgz: the text
   ED_Draw(editor, app_state->frame_arena, layout.editor_box);

   //- bgz: the format panel
   {
      F32 text_h = TXT_LineHeight(fonts->regular, layout.ui_px);
      F32 pad = 12.0f * ui;

      Core_D_PushClip(R2F32P(layout.format_header.min.x, layout.format_header.min.y, layout.status.max.x, layout.status.max.y));

      B32 header_is_hot = Contains2F32(layout.format_header, mouse);
      Core_D_Rect(layout.format_header, R2F32P(0, 0, 1, 1),
                  header_is_hot ? V4F32(0.17f, 0.18f, 0.21f, 1.0f) : V4F32(0.14f, 0.15f, 0.17f, 1.0f));

      F32     header_y = layout.format_header.min.y + (Dim2F32(layout.format_header).y - text_h) * 0.5f;
      B32     shows_pictures = (app_state->show_format && app_state->show_pictures);
      String8 header_left = Str8Lit("Format: what can be written (click or F1 to open)");
      String8 header_right = Str8Lit("F1");
      if (shows_pictures) {
         header_left = Str8Lit("Pictures: click one to put it on this slide");
         header_right = Str8Lit("Esc closes");
      } else if (app_state->show_format) {
         header_left = Str8Lit("Format: click a line to write it into the text");
         header_right = Str8Lit("F1 closes");
      }
      TXT_Draw(fonts->bold, layout.ui_px, V2F32(layout.format_header.min.x + pad, header_y), color_text, header_left);
      F32 header_right_w = TXT_Width(fonts->regular, layout.ui_px, header_right);
      TXT_Draw(fonts->regular, layout.ui_px, V2F32(layout.format_header.max.x - pad - header_right_w, header_y),
               color_dim, header_right);

      if (app_state->show_format) {
         Core_D_Rect(layout.format_rows, R2F32P(0, 0, 1, 1), V4F32(0.11f, 0.115f, 0.135f, 1.0f));

         F32 mono_h = TXT_LineHeight(fonts->mono, layout.ui_px);
         U64 row_count = ArrayCount(_edit_format_rows);

         if (shows_pictures && app_state->pictures.count == 0) {
            Rng2F32 rect = _EDIT_FormatRowRect(&layout, 0);
            TXT_Draw(fonts->regular, layout.ui_px, V2F32(rect.min.x + pad, rect.min.y + (Dim2F32(rect).y - text_h) * 0.5f),
                     color_dim, Str8Lit("No pictures in the deck's folder yet. Drop one on the window to add it."));
         } else if (shows_pictures) {
            //- bgz: the names of the folder's pictures, as many as there are rows
            for (U64 i = 0; i < row_count; i += 1) {
               U64 picture_idx = app_state->pictures_first_row + i;
               if (picture_idx >= app_state->pictures.count) {
                  break;
               }
               Rng2F32 rect = _EDIT_FormatRowRect(&layout, i);
               if (Contains2F32(rect, mouse)) {
                  Core_D_Rect(rect, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.07f));
               }
               TXT_Draw(fonts->mono, layout.ui_px, V2F32(rect.min.x + pad, rect.min.y + (Dim2F32(rect).y - mono_h) * 0.5f),
                        V4F32(0.60f, 0.85f, 0.60f, 1.0f), app_state->pictures.v[picture_idx]);
            }

            // More than fit: which of them these are, at the right of the
            // first row, where a name would have to be very long to reach.
            if (app_state->pictures.count > row_count) {
               Rng2F32 rect = _EDIT_FormatRowRect(&layout, 0);
               U64     last = Min(app_state->pictures_first_row + row_count, app_state->pictures.count);
               String8 more = PushStr8F(scratch.arena, "%llu-%llu of %llu, wheel for more",
                                        (unsigned long long)(app_state->pictures_first_row + 1), (unsigned long long)last,
                                        (unsigned long long)app_state->pictures.count);
               F32     more_w = TXT_Width(fonts->regular, layout.ui_px, more);
               TXT_Draw(fonts->regular, layout.ui_px,
                        V2F32(rect.max.x - pad - more_w, rect.min.y + (Dim2F32(rect).y - text_h) * 0.5f), color_dim, more);
            }
         } else {
            // What each does starts where the longest way of writing one ends.
            F32 syntax_w = 0;
            for (U64 i = 0; i < ArrayCount(_edit_format_rows); i += 1) {
               syntax_w = Max(syntax_w, TXT_Width(fonts->mono, layout.ui_px, _edit_format_rows[i].syntax));
            }

            for (U64 i = 0; i < ArrayCount(_edit_format_rows); i += 1) {
               _EDIT_FormatRow* row = &_edit_format_rows[i];
               Rng2F32          rect = _EDIT_FormatRowRect(&layout, i);
               if (Contains2F32(rect, mouse)) {
                  Core_D_Rect(rect, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.07f));
               }
               F32 row_h = Dim2F32(rect).y;
               TXT_Draw(fonts->mono, layout.ui_px, V2F32(rect.min.x + pad, rect.min.y + (row_h - mono_h) * 0.5f),
                        color_syntax, row->syntax);
               TXT_Draw(fonts->regular, layout.ui_px,
                        V2F32(rect.min.x + pad + syntax_w + 18.0f * ui, rect.min.y + (row_h - text_h) * 0.5f), color_dim,
                        row->what);
            }
         }
      }

      //- bgz: the status line: a notice if one is up, else the file and keys
      Core_D_Rect(layout.status, R2F32P(0, 0, 1, 1), V4F32(0.16f, 0.17f, 0.20f, 1.0f));
      F32     status_y = layout.status.min.y + (Dim2F32(layout.status).y - text_h) * 0.5f;
      String8 notice = FR_CurrentNotice(app_state);
      if (notice.size != 0) {
         TXT_Draw(fonts->regular, layout.ui_px, V2F32(layout.status.min.x + pad, status_y),
                  V4F32(1.0f, 0.85f, 0.5f, 1.0f), notice);
      } else {
         String8 name = Str8SkipLastSlash(Str8(editor->file.str, editor->file.size));
         String8 left = PushStr8F(scratch.arena, "%S%s", name, ED_HasUnsavedChanges(editor) ? "  (not saved)" : "");
         String8 right = Str8Lit("Ctrl+O open    Ctrl+S save    F5 present");
         F32     right_w = TXT_Width(fonts->regular, layout.ui_px, right);
         TXT_Draw(fonts->regular, layout.ui_px, V2F32(layout.status.min.x + pad, status_y), color_text, left);
         TXT_Draw(fonts->regular, layout.ui_px, V2F32(layout.status.max.x - pad - right_w, status_y), color_dim, right);
      }

      Core_D_PopClip();
   }

   //- bgz: the preview
   app_state->preview_hit_count = 0; // made again below, if a slide is drawn
   Core_D_Rect(layout.preview, R2F32P(0, 0, 1, 1), V4F32(0.125f, 0.13f, 0.15f, 1.0f));
   // A line where the text ends and the preview begins.
   Core_D_Rect(R2F32P(layout.preview.min.x, 0, layout.preview.min.x + 1.0f, window_dim.y), R2F32P(0, 0, 1, 1),
               V4F32(0, 0, 0, 0.5f));

   if (Dim2F32(layout.page).x > 0) {
      Core_D_Rect(layout.page, R2F32P(0, 0, 1, 1), FR_COLOR_BACKGROUND);

      String8 caption = {0};
      if (app_state->slide_idx < deck->slide_count) {
         // Asking for the picture is what loads it, and what releases the
         // pictures of slides far from this one.
         IMG_Image* image = DECK_ImageFromSlide(deck, app_state->slide_idx);
         VIEW_HitList hits = {0};
         VIEW_DrawSlide(app_state->frame_arena, fonts, &deck->slides[app_state->slide_idx], image, layout.page, 1.0f,
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

            // Lit under the mouse. And a mark beside the piece the cursor is
            // in: where typing goes.
            if (Contains2F32(rect, mouse) && !app_state->show_open) {
               Core_D_Rect(rect, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.10f));
            }
            if (editor->cursor >= hit->text_range.min && editor->cursor <= hit->text_range.max) {
               F32 mark_w = Max(2.0f, (F32)(S32)(3.0f * ui));
               Core_D_Rect(R2F32P(rect.min.x - mark_w, rect.min.y, rect.min.x, rect.max.y), R2F32P(0, 0, 1, 1),
                           V4F32(0.40f, 0.75f, 1.00f, 0.9f));
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
         Core_D_Rect(rect, R2F32P(0, 0, 1, 1), FR_COLOR_BACKGROUND);

         // Only a picture that is loaded already is drawn: the slide showing
         // and the ones next to it. Loading every picture of a long deck for
         // its small slides would take the memory DECK_ImageFromSlide saves.
         VIEW_DrawSlide(app_state->frame_arena, fonts, &deck->slides[i], DECK_LoadedImageFromSlide(deck, i), rect,
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

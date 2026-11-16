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
//    +---------------------------+                              |
//    |  status                   |                              |
//    +---------------------------+------------------------------+
//
// The Format panel is how the format is learned without leaving the program:
// every kind of line there is, as it is written, with what it does. Clicking
// one writes it into the text at the cursor, ready to be typed over.
//
//    present          F5 (and F5 or Esc comes back)
//    save             Ctrl+S
//    format panel     F1 opens and closes it
//    fullscreen       F11 or Alt+Return. Esc leaves it
//    open             drop a folder, an image or a text deck on the window
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
};

global _EDIT_FormatRow _edit_format_rows[] = {
    {Str8LitComp("# Title"), Str8LitComp("starts a new slide, with a title"), Str8LitComp("# Title"), 2, 7},
    {Str8LitComp("---"), Str8LitComp("starts a new slide with no title"), Str8LitComp("---\n"), 4, 4},
    {Str8LitComp("## Heading"), Str8LitComp("a heading inside the slide"), Str8LitComp("## Heading"), 3, 10},
    {Str8LitComp("- point"), Str8LitComp("a bullet. Tab moves it in, Shift+Tab out"), Str8LitComp("- point"), 2, 7},
    {Str8LitComp("  - point"), Str8LitComp("an inner bullet: two more spaces in front"), Str8LitComp("  - point"), 4, 9},
    {Str8LitComp("![](picture.png)"), Str8LitComp("the slide's picture, from this file's folder"), Str8LitComp("![](picture.png)"), 4, 15},
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
   F32     pad = 28.0f * ui;
   F32     caption_h = 40.0f * ui;
   Rng2F32 page_room = R2F32P(left_w + pad, pad, window_dim.x - pad, window_dim.y - pad - caption_h);
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
      // Of several things dropped at once, the first is opened.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         FR_OpenPath(app_state, event->strings.first->string);
         if (!app_state->is_editing) {
            return;
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
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc) && is_fullscreen) {
      app_state->toggle_fullscreen = 1;
   }

   if (Core_WM_KeyPress(events, window, 0, WM_Key_F1) || _EDIT_ClickIn(app_state, layout.format_header)) {
      app_state->show_format = !app_state->show_format;
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

   //- bgz: the format panel: a click writes the line
   if (app_state->show_format) {
      for (U64 i = 0; i < ArrayCount(_edit_format_rows); i += 1) {
         if (_EDIT_ClickIn(app_state, _EDIT_FormatRowRect(&layout, i))) {
            _EDIT_FormatRow* row = &_edit_format_rows[i];
            ED_InsertLine(editor, row->insert, row->select_start, row->select_end);
            editor->last_input_us = NowTimeUS();
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
      String8 header_left = app_state->show_format ? Str8Lit("Format: click a line to write it into the text")
                                                   : Str8Lit("Format: what can be written (click or F1 to open)");
      String8 header_right = app_state->show_format ? Str8Lit("F1 closes") : Str8Lit("F1");
      TXT_Draw(fonts->bold, layout.ui_px, V2F32(layout.format_header.min.x + pad, header_y), color_text, header_left);
      F32 header_right_w = TXT_Width(fonts->regular, layout.ui_px, header_right);
      TXT_Draw(fonts->regular, layout.ui_px, V2F32(layout.format_header.max.x - pad - header_right_w, header_y),
               color_dim, header_right);

      if (app_state->show_format) {
         Core_D_Rect(layout.format_rows, R2F32P(0, 0, 1, 1), V4F32(0.11f, 0.115f, 0.135f, 1.0f));

         // What each does starts where the longest way of writing one ends.
         F32 mono_h = TXT_LineHeight(fonts->mono, layout.ui_px);
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
         String8 right = Str8Lit("Ctrl+S save    F5 present");
         F32     right_w = TXT_Width(fonts->regular, layout.ui_px, right);
         TXT_Draw(fonts->regular, layout.ui_px, V2F32(layout.status.min.x + pad, status_y), color_text, left);
         TXT_Draw(fonts->regular, layout.ui_px, V2F32(layout.status.max.x - pad - right_w, status_y), color_dim, right);
      }

      Core_D_PopClip();
   }

   //- bgz: the preview
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
         VIEW_DrawSlide(app_state->frame_arena, fonts, &deck->slides[app_state->slide_idx], image, layout.page, 1.0f);
         caption = PushStr8F(scratch.arena, "Slide %llu of %llu", (unsigned long long)(app_state->slide_idx + 1),
                             (unsigned long long)deck->slide_count);
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

   ScratchEnd(scratch);
}

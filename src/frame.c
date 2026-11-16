////////////////////////////////
//~ bgz: one frame's work
//
// FrameWork is everything between the renderer's begin and end of a frame
// (main.c): read this frame's input and the clock, then draw the screen that
// is up - the presentation, which is in this file, or the edit screen
// (frame_edit.c). It also runs with no events at all, when Windows asks for a
// repaint during a resize - see _Main_RepaintFrame in main.c.

////////////////////////////////
//~ bgz: what the two screens share

// One notch of a mouse wheel, in the units Windows reports it in.
#define _FR_SCROLL_PER_SLIDE 120.0f

// How often a text deck's file is looked at for having been written.
#define _FR_RELOAD_CHECK_SECONDS 0.25f

internal void
FR_Notice(App_State* app_state, String8 notice) {
   app_state->notice = notice;
   app_state->notice_us = NowTimeUS();
}

internal String8
FR_CurrentNotice(App_State* app_state) {
   String8 result = {0};
   U64     notice_us = (U64)(FR_NOTICE_SECONDS * 1000000.0f);
   if (app_state->notice.size != 0 && NowTimeUS() - app_state->notice_us < notice_us) {
      result = app_state->notice;
   }
   return result;
}

internal void
FR_RequestQuit(App_State* app_state) {
   B32 has_unsaved = (app_state->editor != 0 && ED_HasUnsavedChanges(app_state->editor));
   if (has_unsaved && !app_state->quit_warned) {
      app_state->quit_warned = 1;
      FR_Notice(app_state, Str8Lit("Unsaved changes. Ctrl+S saves. Close again to quit without saving."));
   } else {
      app_state->quit = 1;
   }
}

internal void
FR_OpenPath(App_State* app_state, String8 path) {
   if (app_state->editor != 0 && ED_HasUnsavedChanges(app_state->editor)) {
      FR_Notice(app_state, Str8Lit("Unsaved changes. Save (Ctrl+S) before opening something else."));
      return;
   }

   U64        first_slide = 0;
   DECK_Deck* deck = DECK_OpenPath(path, &first_slide);
   B32        is_text = (deck->file.size != 0);

   // A text file with no slides yet is a deck to be written. Anything else
   // with no slides is nothing to show, and the deck that is open stays.
   if (!is_text && deck->slide_count == 0 && app_state->deck != 0) {
      printf("No images in %.*s\n", Str8VArg(path));
      FR_Notice(app_state, Str8Lit("Nothing to show there: no images, and not a text deck (.md, .txt)."));
      DECK_Close(deck);
      return;
   }

   printf("%llu slides in %.*s\n", (unsigned long long)deck->slide_count,
          Str8VArg(is_text ? deck->file : deck->folder));

   if (app_state->deck != 0) {
      DECK_Close(app_state->deck);
   }
   if (app_state->editor != 0) {
      ED_Close(app_state->editor);
      app_state->editor = 0;
   }

   U64 now_us = NowTimeUS();
   app_state->deck = deck;
   app_state->slide_idx = first_slide;
   app_state->slide_changed_us = now_us;
   app_state->play_from_us = now_us;
   app_state->playing = 0;
   app_state->quit_warned = 0;

   // A text deck opens on the edit screen. Its deck is made again from the
   // editor's text at the next FR_SyncDeck: 0 is no edit_count an editor has.
   if (is_text) {
      app_state->editor = ED_Open(deck->file);
      app_state->parsed_edit_count = 0;
   }
   app_state->is_editing = is_text;
   app_state->show_format = 1;

   // The old deck's slides are gone, so the first of the new one fades in
   // over the background.
   app_state->fade_active = 1;
   app_state->fade_from_idx = deck->slide_count;
   app_state->fade_start_us = 0;
}

internal void
FR_SyncDeck(App_State* app_state) {
   ED_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }

   //- bgz: the file, written by something else
   //
   // Read again only if nothing here would be lost by it. With unsaved
   // changes the text here wins, and saving it writes over the file.
   U64 now_us = NowTimeUS();
   U64 check_us = (U64)(_FR_RELOAD_CHECK_SECONDS * 1000000.0f);
   if (now_us - app_state->reload_checked_us >= check_us) {
      app_state->reload_checked_us = now_us;
      if (ED_FileHasChanged(editor) && !ED_HasUnsavedChanges(editor)) {
         ED_Reload(editor);
      }
   }

   //- bgz: the deck, from the text as it is now
   if (editor->edit_count != app_state->parsed_edit_count) {
      app_state->parsed_edit_count = editor->edit_count;

      // Pictures already loaded go over to the new deck, or every key typed
      // would decode the slide's picture again.
      DECK_Deck* deck = DECK_OpenText(ED_Text(editor), editor->file);
      DECK_TakeImages(deck, app_state->deck);
      DECK_Close(app_state->deck);
      app_state->deck = deck;

      if (app_state->slide_idx >= deck->slide_count) {
         app_state->slide_idx = (deck->slide_count != 0) ? deck->slide_count - 1 : 0;
      }
      // The slide a fade was from may be no more.
      app_state->fade_active = 0;
   }
}

////////////////////////////////
//~ bgz: the presentation: input
//
//    next slide       Right, Down, Space, Return, Page Down, left click, wheel down
//    previous slide   Left, Up, Backspace, Page Up, right click, wheel up
//    first / last     Home / End
//    play / pause     P
//    fullscreen       F, F11 or Alt+Return. Esc leaves it
//    slide number     I
//    this list        H or F1
//    edit             F5 or Esc, for a text deck: back to the edit screen
//    open             drop a folder, an image or a text deck (.md, .txt) on the
//                     window
//    quit             Esc (a folder of images), or closing the window

internal void
_FR_PresentInput(App_State* app_state) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;
   U64           now_us = NowTimeUS();

   F32 scroll = 0;
   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         FR_RequestQuit(app_state);
      }
      if (event->kind == WM_EventKind_Scroll) {
         scroll += event->delta.y;
      }
      // Of several things dropped at once, the first is opened.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         FR_OpenPath(app_state, event->strings.first->string);
         if (app_state->is_editing) {
            return;
         }
      }
   }

   FR_SyncDeck(app_state);

   //- bgz: overlays
   if (Core_WM_KeyPress(events, window, 0, WM_Key_I)) {
      app_state->show_info = !app_state->show_info;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_H) || Core_WM_KeyPress(events, window, 0, WM_Key_F1)) {
      app_state->show_help = !app_state->show_help;
   }

   //- bgz: fullscreen, the edit screen and quit
   B32 is_fullscreen = Core_WM_WindowIsFullscreen(window);
   B32 fullscreen_pressed = 0;
   fullscreen_pressed |= Core_WM_KeyPress(events, window, 0, WM_Key_F);
   fullscreen_pressed |= Core_WM_KeyPress(events, window, 0, WM_Key_F11);
   fullscreen_pressed |= Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Return);
   if (fullscreen_pressed) {
      app_state->toggle_fullscreen = 1;
   }

   B32 edit_pressed = Core_WM_KeyPress(events, window, 0, WM_Key_F5);
   if (edit_pressed && app_state->editor == 0) {
      // Said, rather than nothing happening: F5 is in the list of keys.
      FR_Notice(app_state, Str8Lit("A folder of images has no text to edit. F5 is for text decks (.md)."));
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
      if (is_fullscreen) {
         app_state->toggle_fullscreen = 1;
      } else if (app_state->editor != 0) {
         edit_pressed = 1;
      } else {
         FR_RequestQuit(app_state);
      }
   }
   if (edit_pressed && app_state->editor != 0) {
      // Back to the text, at the slide that was showing - unless the cursor
      // is on that slide already, and then it stays where it was.
      DECK_Deck* deck = app_state->deck;
      if (deck->slide_count != 0 &&
          DECK_SlideFromTextPos(deck, app_state->editor->cursor) != app_state->slide_idx) {
         ED_SetCursor(app_state->editor, deck->slides[app_state->slide_idx].text_pos);
      }
      app_state->is_editing = 1;
      app_state->playing = 0;
      app_state->fade_active = 0;
      if (is_fullscreen) {
         app_state->toggle_fullscreen = 1;
      }
      return;
   }

   //- bgz: which slide
   WM_Key next_keys[] = {WM_Key_Right, WM_Key_Down, WM_Key_Space, WM_Key_Return, WM_Key_PageDown, WM_Key_LeftMouseButton};
   WM_Key prev_keys[] = {WM_Key_Left, WM_Key_Up, WM_Key_Backspace, WM_Key_PageUp, WM_Key_RightMouseButton};

   U64 slide_count = app_state->deck->slide_count;
   U64 old_slide_idx = app_state->slide_idx;
   if (slide_count != 0) {
      U64 last_idx = slide_count - 1;

      // A key held down repeats, and each repeat is a press of its own. The
      // deck does not wrap by hand: next on the last slide stays there.
      for (U64 i = 0; i < ArrayCount(next_keys); i += 1) {
         while (Core_WM_KeyPress(events, window, 0, next_keys[i])) {
            if (app_state->slide_idx < last_idx) {
               app_state->slide_idx += 1;
            }
         }
      }
      for (U64 i = 0; i < ArrayCount(prev_keys); i += 1) {
         while (Core_WM_KeyPress(events, window, 0, prev_keys[i])) {
            if (app_state->slide_idx > 0) {
               app_state->slide_idx -= 1;
            }
         }
      }

      // The wheel: a slide a notch. A touchpad sends many small movements,
      // which add up here until they come to a notch.
      app_state->scroll_left_over += scroll;
      while (app_state->scroll_left_over >= _FR_SCROLL_PER_SLIDE) {
         app_state->scroll_left_over -= _FR_SCROLL_PER_SLIDE;
         if (app_state->slide_idx < last_idx) {
            app_state->slide_idx += 1;
         }
      }
      while (app_state->scroll_left_over <= -_FR_SCROLL_PER_SLIDE) {
         app_state->scroll_left_over += _FR_SCROLL_PER_SLIDE;
         if (app_state->slide_idx > 0) {
            app_state->slide_idx -= 1;
         }
      }

      if (Core_WM_KeyPress(events, window, 0, WM_Key_Home)) {
         app_state->slide_idx = 0;
      }
      if (Core_WM_KeyPress(events, window, 0, WM_Key_End)) {
         app_state->slide_idx = last_idx;
      }

      //- bgz: playing
      if (Core_WM_KeyPress(events, window, 0, WM_Key_P)) {
         app_state->playing = !app_state->playing;
         app_state->play_from_us = now_us;
      }
      // Only when nothing above moved the deck this frame: a slide changed by
      // hand starts its turn over, below.
      if (app_state->playing && slide_count > 1 && app_state->slide_idx == old_slide_idx) {
         U64 play_us = (U64)(FR_PLAY_SECONDS * 1000000.0f);
         if (now_us - app_state->play_from_us >= play_us) {
            app_state->slide_idx = (app_state->slide_idx + 1) % slide_count;
         }
      }
   }

   // However many presses there were this frame, the fade is from the slide
   // that was on screen to the one that ends up showing.
   //
   // A change that comes before the last one's fade is over is a cut, with no
   // fade. Otherwise a held key, which changes slide many times in the length
   // of one fade, would show every slide it passes only faintly.
   if (app_state->slide_idx != old_slide_idx) {
      U64 fade_us = (U64)(FR_FADE_SECONDS * 1000000.0f);
      B32 is_soon_after_last = (app_state->fade_active ||
                                (app_state->slide_changed_us != 0 && now_us - app_state->slide_changed_us < fade_us));

      app_state->fade_active = !is_soon_after_last;
      app_state->fade_from_idx = old_slide_idx;
      app_state->fade_start_us = 0;

      app_state->slide_changed_us = now_us;
      app_state->play_from_us = now_us;
   }
}

////////////////////////////////
//~ bgz: the presentation: drawing

// Overlays are sized with the page (view.h), but not below this part of full
// size: in a small window the slides can be read small, a list of keys not.
#define _FR_MIN_OVERLAY_SCALE 0.7f

internal void
_FR_DrawSlide(App_State* app_state, Vec2F32 window_dim) {
   DECK_Deck* deck = app_state->deck;
   Rng2F32    window_rect = R2F32P(0, 0, window_dim.x, window_dim.y);

   // The slide showing is asked for first: that is what loads its picture,
   // and what releases the pictures of slides far from it.
   IMG_Image* image = DECK_ImageFromSlide(deck, app_state->slide_idx);

   // How far through the fade this frame is, 0 to 1. The clock starts here,
   // after the load above, and not at the key press: decoding a big photo can
   // take longer than the whole fade, which would then never be seen.
   F32 fade_t = 1.0f;
   if (app_state->fade_active) {
      U64 now_us = NowTimeUS();
      if (app_state->fade_start_us == 0) {
         app_state->fade_start_us = now_us;
      }
      F32 seconds = (F32)(now_us - app_state->fade_start_us) / 1000000.0f;
      F32 t = seconds / FR_FADE_SECONDS;
      if (t >= 1.0f) {
         app_state->fade_active = 0;
      } else {
         fade_t = t * t * (3.0f - 2.0f * t); // eased at both ends
      }
   }

   // The slide being left is drawn with its picture only if that is still
   // loaded. It is after a step to a neighbour. After a jump across the deck
   // it has been released, and the slide goes without it. fade_from_idx past
   // the last slide is no slide: the new one fades in over the background.
   if (app_state->fade_active && app_state->fade_from_idx < deck->slide_count) {
      IMG_Image* from_image = DECK_LoadedImageFromSlide(deck, app_state->fade_from_idx);
      VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, &deck->slides[app_state->fade_from_idx], from_image,
                     window_rect, 1.0f - fade_t);
   }
   if (app_state->slide_idx < deck->slide_count) {
      VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, &deck->slides[app_state->slide_idx], image,
                     window_rect, fade_t);
   }
}

// What is drawn over the slide: the play bar, the slide's number, the list of
// keys, a notice - and, with no slides at all, what to do about that.
internal void
_FR_DrawOverlays(App_State* app_state, Vec2F32 window_dim) {
   DECK_Deck* deck = app_state->deck;
   TXT_Fonts* fonts = &app_state->fonts;
   F32        scale = Max(VIEW_ScaleFromDim(window_dim), _FR_MIN_OVERLAY_SCALE);
   Temp       scratch = ScratchBegin(0, 0);

   //- bgz: no slides
   if (deck->slide_count == 0) {
      String8 lines[] = {
          Str8Lit("No slides"),
          Str8Lit("Drop a folder of images, or a text deck (.md), on this window"),
      };
      F32     sizes[] = {40.0f * scale, 22.0f * scale};
      Vec4F32 colors[] = {V4F32(1, 1, 1, 0.9f), V4F32(1, 1, 1, 0.55f)};

      F32 y = window_dim.y * 0.5f - TXT_LineHeight(fonts->bold, sizes[0]);
      for (U64 i = 0; i < ArrayCount(lines); i += 1) {
         FNT_Tag font = (i == 0) ? fonts->bold : fonts->regular;
         F32     w = TXT_Width(font, sizes[i], lines[i]);
         TXT_Draw(font, sizes[i], V2F32((window_dim.x - w) * 0.5f, y), colors[i], lines[i]);
         y += TXT_LineHeight(font, sizes[i]) * 1.2f;
      }
   }

   //- bgz: playing
   //
   // A thin bar along the bottom that fills as the slide's turn runs out -
   // which is also what shows that it is playing at all.
   if (app_state->playing && deck->slide_count > 1) {
      F32 play_seconds = (F32)(NowTimeUS() - app_state->play_from_us) / 1000000.0f;
      F32 play_t = Clamp(0, play_seconds / FR_PLAY_SECONDS, 1);
      F32 bar_h = 3.0f;
      Core_D_Rect(R2F32P(0, window_dim.y - bar_h, window_dim.x * play_t, window_dim.y), R2F32P(0, 0, 1, 1),
                  V4F32(1, 1, 1, 0.5f));
   }

   //- bgz: the slide's number, and the name of the file it is from
   if (app_state->show_info && deck->slide_count != 0) {
      DECK_Slide* slide = &deck->slides[app_state->slide_idx];
      String8     name = Str8SkipLastSlash(deck->file.size != 0 ? deck->file : slide->image_path);
      String8     text = PushStr8F(scratch.arena, "%llu / %llu   %S", (unsigned long long)(app_state->slide_idx + 1),
                                   (unsigned long long)deck->slide_count, name);

      F32 px = 18.0f * scale;
      F32 pad = 8.0f * scale;
      F32 w = TXT_Width(fonts->regular, px, text);
      F32 h = TXT_LineHeight(fonts->regular, px);
      F32 x = window_dim.x - w - pad * 3.0f;
      F32 y = window_dim.y - h - pad * 3.0f;

      // On a dark panel, since it may be over a bright picture.
      Core_D_Rect(R2F32P(x - pad, y - pad * 0.5f, x + w + pad, y + h + pad * 0.5f), R2F32P(0, 0, 1, 1),
                  V4F32(0, 0, 0, 0.55f));
      TXT_Draw(fonts->regular, px, V2F32(x, y), V4F32(1, 1, 1, 0.85f), text);
   }

   //- bgz: the list of keys
   if (app_state->show_help) {
      // Pairs: the keys, then what they do.
      String8 rows[][2] = {
          {Str8Lit("Right, Down, Space, Enter, click"), Str8Lit("Next slide")},
          {Str8Lit("Left, Up, Backspace, right click"), Str8Lit("Previous slide")},
          {Str8Lit("Mouse wheel"), Str8Lit("Next and previous slide")},
          {Str8Lit("Home, End"), Str8Lit("First and last slide")},
          {Str8Lit("P"), Str8Lit("Play and pause")},
          {Str8Lit("F, F11, Alt+Enter"), Str8Lit("Fullscreen")},
          {Str8Lit("I"), Str8Lit("Slide number")},
          {Str8Lit("H, F1"), Str8Lit("This list")},
          {Str8Lit("F5, Esc"), Str8Lit("Back to editing (a text deck)")},
          {Str8Lit("Esc"), Str8Lit("Leave fullscreen, or quit")},
          {Str8Lit("Drop a folder or file"), Str8Lit("Open it")},
      };

      F32 px = 20.0f * scale;
      F32 row_h = TXT_LineHeight(fonts->regular, px) * 1.25f;
      F32 pad = 28.0f * scale;
      F32 column_gap = 36.0f * scale;

      // As wide as the widest of each column.
      F32 key_w = 0;
      F32 what_w = 0;
      for (U64 i = 0; i < ArrayCount(rows); i += 1) {
         key_w = Max(key_w, TXT_Width(fonts->bold, px, rows[i][0]));
         what_w = Max(what_w, TXT_Width(fonts->regular, px, rows[i][1]));
      }

      F32 panel_w = pad + key_w + column_gap + what_w + pad;
      F32 panel_h = pad + row_h * (F32)ArrayCount(rows) + pad;
      F32 panel_x = (window_dim.x - panel_w) * 0.5f;
      F32 panel_y = (window_dim.y - panel_h) * 0.5f;

      R_Rect2DInst* panel = Core_D_Rect(R2F32P(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h),
                                        R2F32P(0, 0, 1, 1), V4F32(0.02f, 0.02f, 0.03f, 0.88f));
      for (U64 i = 0; i < ArrayCount(panel->corner_radii); i += 1) {
         panel->corner_radii[i] = 10.0f * scale;
      }
      panel->edge_softness = 1.0f;

      F32 y = panel_y + pad;
      for (U64 i = 0; i < ArrayCount(rows); i += 1) {
         TXT_Draw(fonts->bold, px, V2F32(panel_x + pad, y), V4F32(1, 1, 1, 0.95f), rows[i][0]);
         TXT_Draw(fonts->regular, px, V2F32(panel_x + pad + key_w + column_gap, y), V4F32(1, 1, 1, 0.7f), rows[i][1]);
         y += row_h;
      }
   }

   //- bgz: a notice, along the top
   String8 notice = FR_CurrentNotice(app_state);
   if (notice.size != 0) {
      F32 px = 20.0f * scale;
      F32 pad = 10.0f * scale;
      F32 w = TXT_Width(fonts->regular, px, notice);
      F32 h = TXT_LineHeight(fonts->regular, px);
      F32 x = (window_dim.x - w) * 0.5f;
      F32 y = pad * 2.0f;
      Core_D_Rect(R2F32P(x - pad * 1.5f, y - pad * 0.5f, x + w + pad * 1.5f, y + h + pad * 0.5f), R2F32P(0, 0, 1, 1),
                  V4F32(0, 0, 0, 0.75f));
      TXT_Draw(fonts->regular, px, V2F32(x, y), V4F32(1.0f, 0.85f, 0.5f, 1.0f), notice);
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: the frame

// The font cache's turn of the frame, before any text is measured or drawn.
internal void
_FR_BeginText(App_State* app_state, Vec2F32 window_dim) {
   // Text is drawn at sizes that follow the window's (view.h), and the font
   // cache keeps every glyph of every size it has drawn, in textures. Left
   // alone, dragging the window's edge would fill it with sizes never used
   // again. So a new window size empties it: what this frame draws is put
   // back in as it is drawn.
   if (window_dim.x != app_state->text_window_dim.x || window_dim.y != app_state->text_window_dim.y) {
      app_state->text_window_dim = window_dim;
      Core_FNT_Reset();
   }

   // Lets go of the last frame's measured strings.
   Core_FNT_Frame();
}

internal void
FrameWork(App_State* app_state) {
   Vec2F32 window_dim = Dim2F32(Core_WM_ClientRectFromWindow(app_state->window));

   // Before input: the edit screen measures text to know what a click is on.
   _FR_BeginText(app_state, window_dim);

   // Input can change which screen is up, so that is asked again for drawing.
   if (app_state->is_editing) {
      EDIT_Input(app_state, window_dim);
   } else {
      _FR_PresentInput(app_state);
   }

   Core_D_BeginFrame();
   D_Bucket* bucket = Core_D_BucketMake();
   Core_D_BucketScope(bucket) {
      // The background, which is all there is behind a slide that does not
      // fill the window.
      Core_D_Rect(R2F32P(0, 0, window_dim.x, window_dim.y), R2F32P(0, 0, 1, 1), FR_COLOR_BACKGROUND);

      if (app_state->is_editing) {
         EDIT_Draw(app_state, window_dim);
      } else {
         _FR_DrawSlide(app_state, window_dim);
         _FR_DrawOverlays(app_state, window_dim);
      }
   }
   Core_D_SubmitBucket(app_state->window, app_state->window_eqp, bucket);
}

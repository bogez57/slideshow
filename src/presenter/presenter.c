////////////////////////////////
//~ bgz: the presenter's window
//
// A second window for whoever presents, on the screen only they see, while
// the main window shows the audience the slide and nothing else:
//
//    +--------------------------------+------------------+
//    |                                |  Next            |
//    |   the slide showing            |  +------------+  |
//    |                                |  | the slide  |  |
//    |                                |  | after it   |  |
//    +--------------------------------+  +------------+  |
//    |  the slide's notes             |  [Previous][Next]|
//    |  (@note lines)                 |  Slide 3 of 12   |
//    |                                |  04:31           |
//    |                                |  [Pause] [Reset] |
//    +--------------------------------+------------------+
//    |  [1] [2] [3] [4] ...  every slide, small          |
//    +---------------------------------------------------+
//
// V, while presenting, opens it and closes it. Closing the window itself
// closes it too, and it goes when the presentation ends. The clock counts
// from when it was opened.
//
// The show can be run from it. Its keys are the presentation's: every key
// pressed in it is handed to the main window, and so is its wheel. Its mouse
// is its own: a click on the next slide or on Next goes on, on Previous back,
// on a small slide along the bottom to that slide, and on the clock or the
// two buttons under it stops the clock, starts it again, or sets it to 00:00.
//
// It is built with the UI layer, as the main window's screens are, with a
// UI_State of its own (app_state->presenter_ui_state) and its own events:
// PRES_Draw builds it, lays it out and draws it, in its own frame after the
// main window's. A slide the presenter asks for is gone to by the
// presentation's input, the next frame, so that it fades as the others do.

internal void
PRES_Toggle(App_State* app_state) {
   if (app_state->presenter_open) {
      Core_R_WindowUnequip(app_state->presenter_window, app_state->presenter_window_eqp);
      Core_WM_WindowClose(app_state->presenter_window);
      MemoryZeroStruct(&app_state->presenter_window);
      MemoryZeroStruct(&app_state->presenter_window_eqp);
      app_state->presenter_open = 0;
   } else {
      if (app_state->presenter_ui_state == 0) {
         app_state->presenter_ui_state = UI_StateAlloc();
      }
      app_state->presenter_window =
          Core_WM_WindowOpen(R2F32P(0, 0, 1100, 620), WM_WindowFlag_UseDefaultPosition, Str8Lit("Slideshow - presenter"));
      app_state->presenter_window_eqp = Core_R_WindowEquip(app_state->presenter_window);
      // Open before it is shown, so that the repaint showing it asks for
      // draws it too.
      app_state->presenter_open = 1;
      app_state->presenter_started_us = NowTimeUS();
      app_state->presenter_paused_us = 0;
      app_state->presenter_ui_last_frame_us = 0;
      app_state->presenter_goto = 0;
      Core_WM_WindowFirstPaint(app_state->presenter_window);
   }
}

internal void
PRES_Input(App_State* app_state) {
   if (!app_state->presenter_open) {
      return;
   }
   WM_EventList* events = &app_state->events;

   WM_Event* next_event = 0;
   for (WM_Event* event = events->first; event != 0; event = next_event) {
      next_event = event->next;
      if (event->window.u64[0] != app_state->presenter_window.u64[0]) {
         continue;
      }

      B32 is_mouse_button = (event->key == WM_Key_LeftMouseButton || event->key == WM_Key_MiddleMouseButton ||
                             event->key == WM_Key_RightMouseButton);
      B32 is_mouse = (event->kind == WM_EventKind_MouseMove ||
                      ((event->kind == WM_EventKind_Press || event->kind == WM_EventKind_Release) && is_mouse_button));
      if (event->kind == WM_EventKind_WindowClose) {
         // Only this window: the program goes on.
         app_state->toggle_presenter = 1;
         Core_WM_EatEvent(events, event);
      } else if (event->kind == WM_EventKind_Press && !is_mouse_button) {
         // A key: as if pressed in the main window.
         event->window = app_state->window;
      } else if (event->kind == WM_EventKind_Scroll) {
         // The wheel: left for the presentation, which turns the slides by
         // it whichever window it was over.
      } else if (is_mouse) {
         // Left for this window's UI (PRES_Draw), which asks for its own
         // window's events only. Nothing of the main window's reads another
         // window's mouse.
      } else {
         Core_WM_EatEvent(events, event);
      }
   }
}

//- bgz: what the boxes draw that the UI cannot: slides

typedef struct _PRES_Slide _PRES_Slide;
struct _PRES_Slide {
   App_State* app_state;
   U64        slide_idx;
   Rng2F32    rect;         // exactly: the box's is on whole pixels
   F32        alpha;
   B32        keeps_scroll; // as far down its blocks' words as the screen has them
   F32        frame;        // a frame this wide around it, in the box's border colour. 0: none
   F32        bar;          // a bar this tall along its bottom, in the box's border colour. 0: none
};

internal void
_PRES_DrawSlide(UI_Box* box, void* user_data) {
   _PRES_Slide* s = (_PRES_Slide*)user_data;
   App_State*   app_state = s->app_state;
   DECK_Deck*   deck = app_state->deck;
   if (s->slide_idx >= deck->slide_count) {
      return;
   }
   if (s->frame > 0) {
      Core_D_Rect(Pad2F32(s->rect, s->frame), R2F32P(0, 0, 1, 1), box->border_color);
   }
   Core_D_Rect(s->rect, R2F32P(0, 0, 1, 1), VIEW_ColorsFromTheme(deck->theme).background);
   if (s->keeps_scroll) {
      CARET_BeforeDrawOf(app_state, s->slide_idx);
   }
   VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, &deck->slides[s->slide_idx], s->rect, s->alpha, 0);
   if (s->keeps_scroll) {
      CARET_AfterDrawOf(app_state, s->slide_idx);
   }
   if (s->bar > 0) {
      Core_D_Rect(R2F32P(s->rect.min.x, s->rect.max.y - s->bar, s->rect.max.x, s->rect.max.y), R2F32P(0, 0, 1, 1), box->border_color);
   }
}

// A slide in a box of its own at rect; box_out, if not 0, is given the box.
internal _PRES_Slide*
_PRES_BuildSlide(App_State* app_state, U64 slide_idx, Rng2F32 rect, B32 is_clicked, UI_Box** box_out, char* key) {
   _PRES_Slide* s = PushArray(UI_BuildArena(), _PRES_Slide, 1);
   s->app_state = app_state;
   s->slide_idx = slide_idx;
   s->rect = rect;
   s->alpha = 1.0f;
   UI_Box* box = FR_BuildRectBox(is_clicked ? UI_BoxFlag_MouseClickable : 0, rect, "%s %llu", key, (unsigned long long)slide_idx);
   UI_BoxEquipCustomDraw(box, _PRES_DrawSlide, s);
   if (box_out != 0) {
      *box_out = box;
   }
   return s;
}

// A button: a box with its word in the middle, lighter under the mouse. One
// that is not enabled is faint, and takes no clicks. 1 when pressed.
internal B32
_PRES_Button(App_State* app_state, Rng2F32 rect, String8 label, F32 px, B32 is_enabled, char* key) {
   TXT_Fonts* fonts = &app_state->fonts;

   // Its word shrinks to fit across it.
   F32 label_w = TXT_Width(fonts->regular, px, label);
   F32 room = Dim2F32(rect).x * 0.9f;
   if (label_w > room && label_w > 0) {
      px = (F32)(S32)Max(px * room / label_w, 8.0f);
   }
   UI_SetNextFontSize(FR_UIFontSize(px));
   UI_SetNextTextAlignment(UI_TextAlign_Center);
   UI_SetNextTextColor(UI_ThemeColor(is_enabled ? Str8Lit("") : Str8Lit("button off"), UI_ColorSlot_Text));
   UI_Box* box = FR_BuildRectBox(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText | UI_BoxFlag_Clip | (is_enabled ? UI_BoxFlag_MouseClickable : 0),
                                 rect, "%s", key);
   UI_BoxEquipDisplayString(box, label);

   B32 result = 0;
   B32 is_hot = 0;
   if (is_enabled) {
      UI_Signal signal = UI_SignalFromBox(box);
      is_hot = UI_Hovering(signal);
      result = !!(signal.f & UI_SignalFlag_LeftPressed);
   }
   box->background_color = UI_ThemeColor(!is_enabled ? Str8Lit("button off") : is_hot ? Str8Lit("button hot") : Str8Lit("button"),
                                         UI_ColorSlot_Background);
   return result;
}

// The clock stopped, or let run on from where it stopped: the time it stood
// still is not counted.
internal void
_PRES_ToggleClock(App_State* app_state) {
   U64 now_us = NowTimeUS();
   if (app_state->presenter_paused_us != 0) {
      app_state->presenter_started_us += now_us - app_state->presenter_paused_us;
      app_state->presenter_paused_us = 0;
   } else {
      app_state->presenter_paused_us = now_us;
   }
}

internal void
_PRES_Build(App_State* app_state, Vec2F32 window_dim) {
   DECK_Deck* deck = app_state->deck;
   TXT_Fonts* fonts = &app_state->fonts;

   Vec4F32 color_text = UI_ThemeColor(Str8Lit(""), UI_ColorSlot_Text);
   Vec4F32 color_dim = UI_ThemeColor(Str8Lit("dim"), UI_ColorSlot_Text);
   Vec4F32 color_clock = UI_ThemeColor(Str8Lit("clock"), UI_ColorSlot_Text);

   // Sizes follow the window: its height, or its width when it is narrow
   // for its height, so the right column's text still fits across it. Not
   // so small that the words can't be read: a window too small for them
   // cuts them off instead.
   F32 ui = Clamp(0.5f, Min(window_dim.y / 620.0f, window_dim.x / 1100.0f), 3.0f);
   F32 pad = (F32)(S32)(20.0f * ui);
   F32 label_px = (F32)(S32)Max(15.0f * ui, 11.0f);
   F32 notes_px = (F32)(S32)Max(22.0f * ui, 13.0f);
   F32 count_px = (F32)(S32)Max(20.0f * ui, 12.0f);
   F32 clock_px = (F32)(S32)Max(52.0f * ui, 20.0f);
   F32 label_h = TXT_LineHeight(fonts->regular, label_px);
   F32 button_px = (F32)(S32)Max(16.0f * ui, 11.0f);
   F32 button_h = (F32)(S32)(TXT_LineHeight(fonts->regular, button_px) * 1.7f);
   F32 gap = (F32)(S32)Max(pad * 0.4f, 2.0f);

   // Along the bottom, every slide, small. A window too short to spare
   // the room has none: what is above it needs about 300 pixels.
   F32 strip_h = (F32)(S32)Clamp(36.0f, window_dim.y * 0.12f, 160.0f);
   if (window_dim.y - strip_h - pad * 3.0f < 300.0f) {
      strip_h = 0;
   }
   F32     columns_max_y = window_dim.y - pad - ((strip_h != 0) ? strip_h + pad : 0);
   Rng2F32 strip = R2F32P(pad, window_dim.y - pad - strip_h, window_dim.x - pad, window_dim.y - pad);

   // The left has about two thirds, for the slide showing and its notes.
   F32     split_x = (F32)(S32)(window_dim.x * 0.64f);
   Rng2F32 left = R2F32P(pad, pad, split_x - pad * 0.5f, columns_max_y);
   Rng2F32 right = R2F32P(split_x + pad * 0.5f, pad, window_dim.x - pad, columns_max_y);

   if (deck->slide_count != 0 && Dim2F32(left).x > 0 && Dim2F32(right).x > 0 && Dim2F32(left).y > 0) {
      U64         slide_idx = Min(app_state->slide_idx, deck->slide_count - 1);
      DECK_Slide* slide = &deck->slides[slide_idx];

      //- bgz: the slide showing: as wide as the left, and no taller than
      // leaves the notes a third of the height. As far down its blocks'
      // words as the wheel has them on the screen.
      F32     slide_w = Dim2F32(left).x;
      F32     slide_h = Min(slide_w * VIEW_PAGE_H / VIEW_PAGE_W, Dim2F32(left).y * 0.66f);
      Rng2F32 slide_rect = VIEW_PageRectFromRect(R2F32P(left.min.x, left.min.y, left.max.x, left.min.y + slide_h));
      slide_rect = R2F32P(left.min.x, left.min.y, left.min.x + Dim2F32(slide_rect).x, left.min.y + Dim2F32(slide_rect).y);
      _PRES_BuildSlide(app_state, slide_idx, slide_rect, 0, 0, "###presenter slide")->keeps_scroll = 1;

      //- bgz: its notes, under it
      {
         F32 y = slide_rect.max.y + pad;
         FR_BuildWords(fonts->bold, label_px, V2F32(left.min.x, y), color_dim, Str8Lit("Notes"), "###presenter notes label");
         y += label_h * 1.4f;

         F32     line_h = TXT_LineHeight(fonts->regular, notes_px);
         UI_Box* notes_box = FR_BuildRectBox(UI_BoxFlag_Clip, R2F32P(left.min.x, y, left.max.x, left.max.y), "###presenter notes");
         UI_Parent(notes_box) {
            String8List notes = slide->notes;
            Vec4F32     notes_color = color_text;
            if (notes.node_count == 0) {
               MemoryZeroStruct(&notes);
               Str8ListPush(UI_BuildArena(), &notes, Str8Lit("None on this slide. A line of  @note Say this  adds one."));
               notes_color = color_dim;
            }
            // A line that does not fit whole is left out, and the rest after it.
            U64 line_idx = 0;
            for (String8Node* note = notes.first; note != 0; note = note->next) {
               String8List lines = TXT_Wrap(UI_BuildArena(), fonts->regular, notes_px, note->string, Dim2F32(left).x);
               for (String8Node* line = lines.first; line != 0; line = line->next) {
                  if (y + line_h > left.max.y) {
                     break;
                  }
                  FR_BuildWords(fonts->regular, notes_px, V2F32(left.min.x, y), notes_color, line->string, "###presenter note %llu",
                                (unsigned long long)line_idx);
                  line_idx += 1;
                  y += line_h;
               }
               y += line_h * 0.35f; // a little room between one note and the next
            }
         }
      }

      //- bgz: the slide after it. The right column is clipped to itself: in
      // a small window what does not fit is cut off, not drawn over the left.
      UI_Box* right_box = FR_BuildRectBox(UI_BoxFlag_Clip, right, "###presenter right");
      UI_Parent(right_box) {
         F32 right_w = Dim2F32(right).x;
         F32 right_h = Dim2F32(right).y;

         // Where in the deck, and how long it has been: worked out first, for
         // the room they take under the next slide. Both shrink to fit across
         // the column.
         String8 count = PushStr8F(UI_BuildArena(), "Slide %llu of %llu", (unsigned long long)(slide_idx + 1),
                                   (unsigned long long)deck->slide_count);
         F32 count_w = TXT_Width(fonts->regular, count_px, count);
         if (count_w > right_w && count_w > 0) {
            count_px = (F32)(S32)Max(count_px * right_w / count_w, 8.0f);
         }
         F32 count_h = TXT_LineHeight(fonts->regular, count_px);

         // A stopped clock stands at the time it was stopped.
         B32     is_paused = (app_state->presenter_paused_us != 0);
         U64     clock_now_us = is_paused ? app_state->presenter_paused_us : NowTimeUS();
         U64     seconds = (clock_now_us - app_state->presenter_started_us) / 1000000;
         String8 clock = (seconds >= 3600)
                             ? PushStr8F(UI_BuildArena(), "%llu:%02llu:%02llu", (unsigned long long)(seconds / 3600),
                                         (unsigned long long)((seconds / 60) % 60), (unsigned long long)(seconds % 60))
                             : PushStr8F(UI_BuildArena(), "%02llu:%02llu", (unsigned long long)(seconds / 60),
                                         (unsigned long long)(seconds % 60));
         F32 clock_w = TXT_Width(fonts->bold, clock_px, clock);
         if (clock_w > right_w && clock_w > 0) {
            clock_px = (F32)(S32)Max(clock_px * right_w / clock_w, 8.0f);
            clock_w = TXT_Width(fonts->bold, clock_px, clock);
         }
         F32 clock_h = TXT_LineHeight(fonts->bold, clock_px);

         // All that goes under the next slide, top to bottom: the buttons,
         // the count, the clock, its buttons, the hint.
         F32 under_h = gap * 1.5f + button_h + pad + count_h * 1.5f + clock_h * 1.1f + button_h + gap * 1.5f + label_h;

         F32 y = right.min.y;
         FR_BuildWords(fonts->bold, label_px, V2F32(right.min.x, y), color_dim, Str8Lit("Next"), "###presenter next label");
         y += label_h * 1.4f;

         // No taller than leaves what goes under it room, nor than a bit over
         // a third of the column. In a window too short for both, it keeps a
         // little and what is under it goes, from the bottom up.
         F32 next_w = right_w;
         F32 next_h = next_w * VIEW_PAGE_H / VIEW_PAGE_W;
         F32 next_max_h = Max(Min(right_h * 0.38f, right_h - label_h * 1.4f - under_h), Max(right_h * 0.15f, 1.0f));
         if (next_h > next_max_h) {
            next_h = next_max_h;
            next_w = next_h * VIEW_PAGE_W / VIEW_PAGE_H;
         }
         Rng2F32 next_rect = R2F32P(right.min.x, y, right.min.x + next_w, y + next_h);
         B32     has_next = (slide_idx + 1 < deck->slide_count);
         B32     has_prev = (slide_idx > 0);
         if (has_next) {
            // Its picture is loaded here: the main window only loads the one
            // showing. Being next to that one, it is not released again
            // until the deck moves on from both.
            DECK_LoadSlideOnly(deck, slide_idx + 1);
            // A click on it goes on to it. Under the mouse it has a frame,
            // to say so.
            UI_Box*      next_box = 0;
            _PRES_Slide* next = _PRES_BuildSlide(app_state, slide_idx + 1, next_rect, 1, &next_box, "###presenter next");
            UI_Signal    signal = UI_SignalFromBox(next_box);
            if (UI_Hovering(signal)) {
               next->frame = (F32)(S32)Max(2.0f * ui, 1.0f);
               next_box->border_color = UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Border);
            }
            if (signal.f & UI_SignalFlag_LeftPressed) {
               app_state->presenter_goto = slide_idx + 2;
            }
         } else {
            UI_SetNextBackgroundColor(UI_ThemeColor(Str8Lit("end"), UI_ColorSlot_Background));
            UI_SetNextFontSize(FR_UIFontSize(count_px));
            UI_SetNextTextAlignment(UI_TextAlign_Center);
            UI_SetNextTextColor(color_dim);
            UI_Box* end = FR_BuildRectBox(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText, next_rect, "###presenter the end");
            UI_BoxEquipDisplayString(end, Str8Lit("The end"));
         }
         y = next_rect.max.y + gap * 1.5f;

         // Each row under it is built only if it fits whole: half a button
         // cut off at the bottom is neither readable nor safe to click.

         //- bgz: back and on, side by side, as wide as the column
         F32 half_w = (F32)(S32)((right_w - gap) * 0.5f);
         if (y + button_h <= right.max.y) {
            if (_PRES_Button(app_state, R2F32P(right.min.x, y, right.min.x + half_w, y + button_h), Str8Lit("Previous"), button_px, has_prev,
                             "###presenter previous")) {
               app_state->presenter_goto = slide_idx;
            }
            if (_PRES_Button(app_state, R2F32P(right.min.x + half_w + gap, y, right.max.x, y + button_h), Str8Lit("Next"), button_px, has_next,
                             "###presenter next button")) {
               app_state->presenter_goto = slide_idx + 2;
            }
         }
         y += button_h + pad;

         //- bgz: where in the deck, and how long it has been
         if (y + count_h <= right.max.y) {
            FR_BuildWords(fonts->regular, count_px, V2F32(right.min.x, y), color_text, count, "###presenter count");
         }
         y += count_h * 1.5f;

         // Stopped, it is dim. A click on it stops it and starts it, as the
         // button under it does.
         if (y + clock_h <= right.max.y) {
            FR_BuildWords(fonts->bold, clock_px, V2F32(right.min.x, y), is_paused ? color_dim : color_clock, clock, "###presenter clock");
            UI_Box* clock_box = FR_BuildRectBox(UI_BoxFlag_MouseClickable, R2F32P(right.min.x, y, right.min.x + Min(clock_w, right_w), y + clock_h),
                                                "###presenter clock click");
            if (UI_SignalFromBox(clock_box).f & UI_SignalFlag_LeftPressed) {
               _PRES_ToggleClock(app_state);
            }
         }
         y += clock_h * 1.1f;

         if (y + button_h <= right.max.y) {
            if (_PRES_Button(app_state, R2F32P(right.min.x, y, right.min.x + half_w, y + button_h), is_paused ? Str8Lit("Resume") : Str8Lit("Pause"),
                             button_px, 1, "###presenter pause")) {
               _PRES_ToggleClock(app_state);
            }
            if (_PRES_Button(app_state, R2F32P(right.min.x + half_w + gap, y, right.max.x, y + button_h), Str8Lit("Reset"), button_px, 1,
                             "###presenter reset")) {
               // To 00:00, running or stopped as it was.
               U64 now_us = NowTimeUS();
               app_state->presenter_started_us = now_us;
               if (app_state->presenter_paused_us != 0) {
                  app_state->presenter_paused_us = now_us;
               }
            }
         }
         y += button_h + gap * 1.5f;

         // The slide's videos, if it has any: played and paused from here
         // too, all together, as K does.
         if (DECK_VideoFromSlide(deck, slide_idx) != 0) {
            if (y + button_h <= right.max.y) {
               if (_PRES_Button(app_state, R2F32P(right.min.x, y, right.max.x, y + button_h),
                                SVID_SlideIsPlaying(deck, slide_idx) ? Str8Lit("Pause video") : Str8Lit("Play video"), button_px, 1,
                                "###presenter video")) {
                  SVID_ToggleSlide(deck, slide_idx);
               }
            }
            y += button_h + gap * 1.5f;
         }

         String8List hint = TXT_Wrap(UI_BuildArena(), fonts->regular, label_px, Str8Lit("V closes this window"), right_w);
         U64         hint_idx = 0;
         for (String8Node* line = hint.first; line != 0 && y + label_h <= right.max.y; line = line->next, hint_idx += 1) {
            FR_BuildWords(fonts->regular, label_px, V2F32(right.min.x, y), color_dim, line->string, "###presenter hint %llu",
                          (unsigned long long)hint_idx);
            y += label_h;
         }
      }

      //- bgz: every slide, small, along the bottom: a click goes to it
      //
      // More than fit across are scrolled so that the one showing is in the
      // middle, as far as the ends allow.
      if (strip_h != 0 && Dim2F32(strip).x > 0) {
         F32 thumb_h = strip_h;
         F32 thumb_w = (F32)(S32)(thumb_h * VIEW_PAGE_W / VIEW_PAGE_H);
         F32 step = thumb_w + gap;
         F32 all_w = step * (F32)deck->slide_count - gap;
         F32 strip_w = Dim2F32(strip).x;
         F32 scroll = 0;
         if (all_w > strip_w) {
            scroll = Clamp(0.0f, step * (F32)slide_idx + thumb_w * 0.5f - strip_w * 0.5f, all_w - strip_w);
         }

         // Only the part of each inside the strip shows, and can be clicked.
         UI_Box* strip_box = FR_BuildRectBox(UI_BoxFlag_Clip, strip, "###presenter strip");
         UI_Parent(strip_box) for (U64 i = 0; i < deck->slide_count; i += 1) {
            F32     x = strip.min.x + step * (F32)i - scroll;
            Rng2F32 rect = R2F32P(x, strip.min.y, x + thumb_w, strip.max.y);
            if (rect.max.x <= strip.min.x || rect.min.x >= strip.max.x) {
               continue;
            }
            B32 is_showing = (i == slide_idx);
            // Only a picture that is loaded already is drawn, as in the edit
            // screen's row: loading every picture of a long deck for its
            // small slides is what the deck avoids.
            UI_Box*      thumb_box = 0;
            _PRES_Slide* thumb = _PRES_BuildSlide(app_state, i, rect, 1, &thumb_box, "###presenter thumb");
            UI_Signal    signal = UI_SignalFromBox(thumb_box);
            B32          is_hot = UI_Hovering(signal);
            if (signal.f & UI_SignalFlag_LeftPressed) {
               app_state->presenter_goto = i + 1;
            }
            // The one showing has a bar under it, in the clock's colour.
            thumb->alpha = (is_showing || is_hot) ? 1.0f : 0.7f;
            if (is_showing || is_hot) {
               thumb->bar = (F32)(S32)Max(3.0f * ui, 2.0f);
               thumb_box->border_color = UI_ThemeColor(is_showing ? Str8Lit("now") : Str8Lit("thumb hot"), UI_ColorSlot_Border);
            }
         }
      }
   }

   //- bgz: the window's background, under everything
   UI_SetNextBackgroundColor(UI_ThemeColor(Str8Lit(""), UI_ColorSlot_Background));
   FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(0, 0, window_dim.x, window_dim.y), "###presenter background");
}

internal void
PRES_Draw(App_State* app_state) {
   Vec2F32 window_dim = Dim2F32(Core_WM_ClientRectFromWindow(app_state->presenter_window));

   FR_BeginUIBuild(app_state, app_state->presenter_ui_state, app_state->presenter_window, &app_state->presenter_ui_last_frame_us);
   // What is shown is text as it is - a note, a title - never a box's key.
   UI_FlagsAdd(UI_BoxFlag_DisableIDString) UI_ColorTag(Str8Lit("presenter")) {
      _PRES_Build(app_state, window_dim);
   }
   UI_EndBuild();

   D_Bucket* bucket = Core_D_BucketMake();
   Core_D_BucketScope(bucket) {
      FR_DrawUITree(app_state->presenter_ui_state);
   }
   Core_D_SubmitBucket(app_state->presenter_window, app_state->presenter_window_eqp, bucket);

   // The main window's UI is the one the next frame starts from.
   UI_SelectState(app_state->ui_state);
}

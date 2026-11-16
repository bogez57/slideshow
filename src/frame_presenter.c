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
// What a click lands on is what PRES_Draw laid out the frame before
// (app_state->presenter_hits).

internal void
PRES_Toggle(App_State* app_state) {
   if (app_state->presenter_open) {
      Core_R_WindowUnequip(app_state->presenter_window, app_state->presenter_window_eqp);
      Core_WM_WindowClose(app_state->presenter_window);
      MemoryZeroStruct(&app_state->presenter_window);
      MemoryZeroStruct(&app_state->presenter_window_eqp);
      app_state->presenter_open = 0;
   } else {
      app_state->presenter_window =
          Core_WM_WindowOpen(R2F32P(0, 0, 1100, 620), WM_WindowFlag_UseDefaultPosition, Str8Lit("Slideshow - presenter"));
      app_state->presenter_window_eqp = Core_R_WindowEquip(app_state->presenter_window);
      // Open before it is shown, so that the repaint showing it asks for
      // draws it too.
      app_state->presenter_open = 1;
      app_state->presenter_started_us = NowTimeUS();
      app_state->presenter_paused_us = 0;
      app_state->presenter_hit_count = 0;
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
      } else {
         // A left press on something that can be clicked does what that
         // does. The last such thing under it: what is drawn later is on
         // top.
         if (event->kind == WM_EventKind_Press && event->key == WM_Key_LeftMouseButton) {
            PRES_Hit* hit = 0;
            for (U64 i = 0; i < app_state->presenter_hit_count; i += 1) {
               if (Contains2F32(app_state->presenter_hits[i].rect, event->pos)) {
                  hit = &app_state->presenter_hits[i];
               }
            }
            if (hit != 0) {
               U64 now_us = NowTimeUS();
               switch (hit->kind) {
                  case PRES_HitKind_Slide: {
                     app_state->presenter_goto = hit->slide_idx + 1;
                  } break;
                  case PRES_HitKind_Clock: {
                     if (app_state->presenter_paused_us != 0) {
                        // On from where it stopped: the time it stood still
                        // is not counted.
                        app_state->presenter_started_us += now_us - app_state->presenter_paused_us;
                        app_state->presenter_paused_us = 0;
                     } else {
                        app_state->presenter_paused_us = now_us;
                     }
                  } break;
                  case PRES_HitKind_ClockReset: {
                     // To 00:00, running or stopped as it was.
                     app_state->presenter_started_us = now_us;
                     if (app_state->presenter_paused_us != 0) {
                        app_state->presenter_paused_us = now_us;
                     }
                  } break;
                  default: break;
               }
            }
         }
         // The rest of its mouse: where it points is in this window, and
         // would mean something else in the other.
         Core_WM_EatEvent(events, event);
      }
   }
}

// Something a click does something to, for PRES_Input to find next frame.
internal void
_PRES_PushHit(App_State* app_state, PRES_HitKind kind, Rng2F32 rect, U64 slide_idx) {
   if (app_state->presenter_hit_count < PRES_MAX_HITS && rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
      PRES_Hit* hit = &app_state->presenter_hits[app_state->presenter_hit_count];
      app_state->presenter_hit_count += 1;
      hit->kind = kind;
      hit->rect = rect;
      hit->slide_idx = slide_idx;
   }
}

// A button: a box with its word in the middle, lighter under the mouse. One
// that is not enabled is drawn faint, and a click on it does nothing.
internal void
_PRES_Button(App_State* app_state, Rng2F32 rect, String8 label, F32 px, Vec2F32 mouse, B32 is_enabled, PRES_HitKind kind,
             U64 slide_idx) {
   TXT_Fonts* fonts = &app_state->fonts;
   B32        is_hot = (is_enabled && Contains2F32(rect, mouse));
   Core_D_Rect(rect, R2F32P(0, 0, 1, 1), is_hot ? V4F32(1, 1, 1, 0.22f) : V4F32(1, 1, 1, is_enabled ? 0.10f : 0.04f));

   // Its word shrinks to fit across it.
   F32 label_w = TXT_Width(fonts->regular, px, label);
   F32 room = Dim2F32(rect).x * 0.9f;
   if (label_w > room && label_w > 0) {
      px = (F32)(S32)Max(px * room / label_w, 8.0f);
      label_w = TXT_Width(fonts->regular, px, label);
   }
   Core_D_PushClip(rect);
   TXT_Draw(fonts->regular, px,
            V2F32(rect.min.x + (Dim2F32(rect).x - label_w) * 0.5f,
                  rect.min.y + (Dim2F32(rect).y - TXT_LineHeight(fonts->regular, px)) * 0.5f),
            V4F32(0.92f, 0.93f, 0.95f, is_enabled ? 1.0f : 0.35f), label);
   Core_D_PopClip();

   if (is_enabled) {
      _PRES_PushHit(app_state, kind, rect, slide_idx);
   }
}

internal void
PRES_Draw(App_State* app_state) {
   DECK_Deck* deck = app_state->deck;
   TXT_Fonts* fonts = &app_state->fonts;
   Vec2F32    window_dim = Dim2F32(Core_WM_ClientRectFromWindow(app_state->presenter_window));
   Temp       scratch = ScratchBegin(0, 0);
   Vec2F32    mouse = Core_WM_MouseFromWindow(app_state->presenter_window);

   // Laid out again below, as it is drawn.
   app_state->presenter_hit_count = 0;

   Vec4F32 color_text = V4F32(0.92f, 0.93f, 0.95f, 1.0f);
   Vec4F32 color_dim = V4F32(0.92f, 0.93f, 0.95f, 0.55f);
   Vec4F32 color_clock = V4F32(1.00f, 0.85f, 0.50f, 1.0f);

   D_Bucket* bucket = Core_D_BucketMake();
   Core_D_BucketScope(bucket) {
      Core_D_Rect(R2F32P(0, 0, window_dim.x, window_dim.y), R2F32P(0, 0, 1, 1), V4F32(0.09f, 0.095f, 0.11f, 1.0f));

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
         VIEW_Colors colors = VIEW_ColorsFromTheme(deck->theme);

         //- bgz: the slide showing: as wide as the left, and no taller than
         // leaves the notes a third of the height
         F32     slide_w = Dim2F32(left).x;
         F32     slide_h = Min(slide_w * VIEW_PAGE_H / VIEW_PAGE_W, Dim2F32(left).y * 0.66f);
         Rng2F32 slide_rect = VIEW_PageRectFromRect(R2F32P(left.min.x, left.min.y, left.max.x, left.min.y + slide_h));
         slide_rect = R2F32P(left.min.x, left.min.y, left.min.x + Dim2F32(slide_rect).x, left.min.y + Dim2F32(slide_rect).y);
         Core_D_Rect(slide_rect, R2F32P(0, 0, 1, 1), colors.background);
         VIEW_DrawSlide(app_state->frame_arena, fonts, deck->theme, slide, DECK_LoadedImageFromSlide(deck, slide_idx),
                        slide_rect, 1.0f, 0);

         //- bgz: its notes, under it
         {
            F32 y = slide_rect.max.y + pad;
            TXT_Draw(fonts->bold, label_px, V2F32(left.min.x, y), color_dim, Str8Lit("Notes"));
            y += label_h * 1.4f;

            F32 line_h = TXT_LineHeight(fonts->regular, notes_px);
            Core_D_PushClip(R2F32P(left.min.x, y, left.max.x, left.max.y));
            String8List notes = slide->notes;
            Vec4F32     notes_color = color_text;
            if (notes.node_count == 0) {
               MemoryZeroStruct(&notes);
               Str8ListPush(scratch.arena, &notes, Str8Lit("None on this slide. A line of  @note Say this  adds one."));
               notes_color = color_dim;
            }
            // A line that does not fit whole is left out, and the rest after it.
            for (String8Node* note = notes.first; note != 0; note = note->next) {
               String8List lines = TXT_Wrap(scratch.arena, fonts->regular, notes_px, note->string, Dim2F32(left).x);
               for (String8Node* line = lines.first; line != 0; line = line->next) {
                  if (y + line_h > left.max.y) {
                     break;
                  }
                  TXT_Draw(fonts->regular, notes_px, V2F32(left.min.x, y), notes_color, line->string);
                  y += line_h;
               }
               y += line_h * 0.35f; // a little room between one note and the next
            }
            Core_D_PopClip();
         }

         //- bgz: the slide after it. The right column is clipped to itself:
         // in a small window what does not fit is cut off, not drawn over
         // the left.
         Core_D_PushClip(right);
         F32 right_w = Dim2F32(right).x;
         F32 right_h = Dim2F32(right).y;

         // Where in the deck, and how long it has been: worked out first, for
         // the room they take under the next slide. Both shrink to fit across
         // the column.
         String8 count = PushStr8F(scratch.arena, "Slide %llu of %llu", (unsigned long long)(slide_idx + 1),
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
                             ? PushStr8F(scratch.arena, "%llu:%02llu:%02llu", (unsigned long long)(seconds / 3600),
                                         (unsigned long long)((seconds / 60) % 60), (unsigned long long)(seconds % 60))
                             : PushStr8F(scratch.arena, "%02llu:%02llu", (unsigned long long)(seconds / 60),
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
         TXT_Draw(fonts->bold, label_px, V2F32(right.min.x, y), color_dim, Str8Lit("Next"));
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
            // A click on it goes on to it. Under the mouse it has a frame,
            // to say so.
            if (Contains2F32(next_rect, mouse)) {
               F32 frame = (F32)(S32)Max(2.0f * ui, 1.0f);
               Core_D_Rect(R2F32P(next_rect.min.x - frame, next_rect.min.y - frame, next_rect.max.x + frame,
                                  next_rect.max.y + frame),
                           R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.45f));
            }
            _PRES_PushHit(app_state, PRES_HitKind_Slide, next_rect, slide_idx + 1);
            Core_D_Rect(next_rect, R2F32P(0, 0, 1, 1), colors.background);
            // Its picture is loaded here: the main window only loads the one
            // showing. Being next to that one, it is not released again
            // until the deck moves on from both.
            VIEW_DrawSlide(app_state->frame_arena, fonts, deck->theme, &deck->slides[slide_idx + 1],
                           DECK_LoadImageOfSlide(deck, slide_idx + 1), next_rect, 1.0f, 0);
         } else {
            Core_D_Rect(next_rect, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.04f));
            String8 end = Str8Lit("The end");
            F32     end_w = TXT_Width(fonts->regular, count_px, end);
            TXT_Draw(fonts->regular, count_px,
                     V2F32(next_rect.min.x + (next_w - end_w) * 0.5f,
                           next_rect.min.y + (next_h - TXT_LineHeight(fonts->regular, count_px)) * 0.5f),
                     color_dim, end);
         }
         y = next_rect.max.y + gap * 1.5f;

         // Each row under it is drawn only if it fits whole: half a button
         // cut off at the bottom is neither readable nor safe to click.

         //- bgz: back and on, side by side, as wide as the column
         F32 half_w = (F32)(S32)((right_w - gap) * 0.5f);
         if (y + button_h <= right.max.y) {
            _PRES_Button(app_state, R2F32P(right.min.x, y, right.min.x + half_w, y + button_h), Str8Lit("Previous"),
                         button_px, mouse, has_prev, PRES_HitKind_Slide, has_prev ? slide_idx - 1 : 0);
            _PRES_Button(app_state, R2F32P(right.min.x + half_w + gap, y, right.max.x, y + button_h), Str8Lit("Next"),
                         button_px, mouse, has_next, PRES_HitKind_Slide, slide_idx + 1);
         }
         y += button_h + pad;

         //- bgz: where in the deck, and how long it has been
         if (y + count_h <= right.max.y) {
            TXT_Draw(fonts->regular, count_px, V2F32(right.min.x, y), color_text, count);
         }
         y += count_h * 1.5f;

         // Stopped, it is dim. A click on it stops it and starts it, as the
         // button under it does.
         if (y + clock_h <= right.max.y) {
            TXT_Draw(fonts->bold, clock_px, V2F32(right.min.x, y), is_paused ? color_dim : color_clock, clock);
            _PRES_PushHit(app_state, PRES_HitKind_Clock,
                          R2F32P(right.min.x, y, right.min.x + Min(clock_w, right_w), y + clock_h), 0);
         }
         y += clock_h * 1.1f;

         if (y + button_h <= right.max.y) {
            _PRES_Button(app_state, R2F32P(right.min.x, y, right.min.x + half_w, y + button_h),
                         is_paused ? Str8Lit("Resume") : Str8Lit("Pause"), button_px, mouse, 1, PRES_HitKind_Clock, 0);
            _PRES_Button(app_state, R2F32P(right.min.x + half_w + gap, y, right.max.x, y + button_h), Str8Lit("Reset"),
                         button_px, mouse, 1, PRES_HitKind_ClockReset, 0);
         }
         y += button_h + gap * 1.5f;

         String8List hint = TXT_Wrap(scratch.arena, fonts->regular, label_px, Str8Lit("V closes this window"), right_w);
         for (String8Node* line = hint.first; line != 0 && y + label_h <= right.max.y; line = line->next) {
            TXT_Draw(fonts->regular, label_px, V2F32(right.min.x, y), color_dim, line->string);
            y += label_h;
         }
         Core_D_PopClip();

         //- bgz: every slide, small, along the bottom: a click goes to it
         //
         // More than fit across are scrolled so that the one showing is in
         // the middle, as far as the ends allow.
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

            Core_D_PushClip(strip);
            for (U64 i = 0; i < deck->slide_count; i += 1) {
               F32     x = strip.min.x + step * (F32)i - scroll;
               Rng2F32 rect = R2F32P(x, strip.min.y, x + thumb_w, strip.max.y);
               if (rect.max.x <= strip.min.x || rect.min.x >= strip.max.x) {
                  continue;
               }
               // Only the part of it inside the strip can be clicked.
               Rng2F32 seen = R2F32P(Max(rect.min.x, strip.min.x), rect.min.y, Min(rect.max.x, strip.max.x), rect.max.y);
               B32     is_showing = (i == slide_idx);
               B32     is_hot = Contains2F32(seen, mouse);

               Core_D_Rect(rect, R2F32P(0, 0, 1, 1), colors.background);
               // Only a picture that is loaded already is drawn, as in the
               // edit screen's row: loading every picture of a long deck for
               // its small slides is what the deck avoids.
               VIEW_DrawSlide(app_state->frame_arena, fonts, deck->theme, &deck->slides[i], DECK_LoadedImageFromSlide(deck, i),
                              rect, (is_showing || is_hot) ? 1.0f : 0.7f, 0);
               // The one showing has a bar under it, in the clock's colour.
               if (is_showing || is_hot) {
                  F32 bar = (F32)(S32)Max(3.0f * ui, 2.0f);
                  Core_D_Rect(R2F32P(rect.min.x, rect.max.y - bar, rect.max.x, rect.max.y), R2F32P(0, 0, 1, 1),
                              is_showing ? color_clock : V4F32(1, 1, 1, 0.6f));
               }
               _PRES_PushHit(app_state, PRES_HitKind_Slide, seen, i);
            }
            Core_D_PopClip();
         }
      }
   }
   Core_D_SubmitBucket(app_state->presenter_window, app_state->presenter_window_eqp, bucket);

   ScratchEnd(scratch);
}

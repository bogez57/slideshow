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
//    |  the slide's notes             |                  |
//    |  (@note lines)                 |  Slide 3 of 12   |
//    |                                |  04:31           |
//    +--------------------------------+------------------+
//
// V, while presenting, opens it and closes it. Closing the window itself
// closes it too, and it goes when the presentation ends. The clock counts
// from when it was opened.
//
// Its keys are the presentation's: every key pressed in it is handed to the
// main window, so the slides can be driven from either. Its mouse is not -
// a click in it changes nothing.

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
      } else {
         // Its mouse, and the rest: where they point is in this window, and
         // would mean something else in the other.
         Core_WM_EatEvent(events, event);
      }
   }
}

internal void
PRES_Draw(App_State* app_state) {
   DECK_Deck* deck = app_state->deck;
   TXT_Fonts* fonts = &app_state->fonts;
   Vec2F32    window_dim = Dim2F32(Core_WM_ClientRectFromWindow(app_state->presenter_window));
   Temp       scratch = ScratchBegin(0, 0);

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

      // The left has about two thirds, for the slide showing and its notes.
      F32     split_x = (F32)(S32)(window_dim.x * 0.64f);
      Rng2F32 left = R2F32P(pad, pad, split_x - pad * 0.5f, window_dim.y - pad);
      Rng2F32 right = R2F32P(split_x + pad * 0.5f, pad, window_dim.x - pad, window_dim.y - pad);

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
            for (String8Node* note = notes.first; note != 0; note = note->next) {
               String8List lines = TXT_Wrap(scratch.arena, fonts->regular, notes_px, note->string, Dim2F32(left).x);
               for (String8Node* line = lines.first; line != 0; line = line->next) {
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
         F32 y = right.min.y;
         TXT_Draw(fonts->bold, label_px, V2F32(right.min.x, y), color_dim, Str8Lit("Next"));
         y += label_h * 1.4f;

         // No taller than leaves the count and the clock room under it.
         F32 next_w = Dim2F32(right).x;
         F32 next_h = next_w * VIEW_PAGE_H / VIEW_PAGE_W;
         F32 next_max_h = Max(Dim2F32(right).y * 0.45f, 1.0f);
         if (next_h > next_max_h) {
            next_h = next_max_h;
            next_w = next_h * VIEW_PAGE_W / VIEW_PAGE_H;
         }
         Rng2F32 next_rect = R2F32P(right.min.x, y, right.min.x + next_w, y + next_h);
         if (slide_idx + 1 < deck->slide_count) {
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
         y = next_rect.max.y + pad * 1.5f;

         //- bgz: where in the deck, and how long it has been
         // The count and the clock shrink to fit across the column.
         F32     right_w = Dim2F32(right).x;
         String8 count = PushStr8F(scratch.arena, "Slide %llu of %llu", (unsigned long long)(slide_idx + 1),
                                   (unsigned long long)deck->slide_count);
         F32 count_w = TXT_Width(fonts->regular, count_px, count);
         if (count_w > right_w && count_w > 0) {
            count_px = (F32)(S32)Max(count_px * right_w / count_w, 8.0f);
         }
         TXT_Draw(fonts->regular, count_px, V2F32(right.min.x, y), color_text, count);
         y += TXT_LineHeight(fonts->regular, count_px) * 1.5f;

         U64     seconds = (NowTimeUS() - app_state->presenter_started_us) / 1000000;
         String8 clock = (seconds >= 3600)
                             ? PushStr8F(scratch.arena, "%llu:%02llu:%02llu", (unsigned long long)(seconds / 3600),
                                         (unsigned long long)((seconds / 60) % 60), (unsigned long long)(seconds % 60))
                             : PushStr8F(scratch.arena, "%02llu:%02llu", (unsigned long long)(seconds / 60),
                                         (unsigned long long)(seconds % 60));
         F32 clock_w = TXT_Width(fonts->bold, clock_px, clock);
         if (clock_w > right_w && clock_w > 0) {
            clock_px = (F32)(S32)Max(clock_px * right_w / clock_w, 8.0f);
         }
         TXT_Draw(fonts->bold, clock_px, V2F32(right.min.x, y), color_clock, clock);
         y += TXT_LineHeight(fonts->bold, clock_px) * 1.1f;

         String8List hint = TXT_Wrap(scratch.arena, fonts->regular, label_px, Str8Lit("V closes this window"), right_w);
         for (String8Node* line = hint.first; line != 0; line = line->next) {
            TXT_Draw(fonts->regular, label_px, V2F32(right.min.x, y), color_dim, line->string);
            y += label_h;
         }
         Core_D_PopClip();
      }
   }
   Core_D_SubmitBucket(app_state->presenter_window, app_state->presenter_window_eqp, bucket);

   ScratchEnd(scratch);
}

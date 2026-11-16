////////////////////////////////
//~ bgz: one frame's work
//
// FrameWork is everything between the renderer's begin and end of a frame
// (main.c): read this frame's input and the clock, then draw the slide that
// is showing, fading from the one before it if the slide just changed. It
// also runs with no events at all, when Windows asks for a repaint during a
// resize - see _Main_RepaintFrame in main.c.

////////////////////////////////
//~ bgz: input
//
//    next slide       Right, Down, Space, Return, Page Down, left click, wheel down
//    previous slide   Left, Up, Backspace, Page Up, right click, wheel up
//    first / last     Home / End
//    play / pause     P
//    fullscreen       F, F11 or Alt+Return. Esc leaves it
//    open             drop a folder, or one image of a folder, on the window
//    quit             Esc, or closing the window

#define _FR_FADE_SECONDS 0.25f
#define _FR_PLAY_SECONDS 4.0f

// One notch of a mouse wheel, in the units Windows reports it in.
#define _FR_SCROLL_PER_SLIDE 120.0f

// Swaps the deck for the one at path, if that has any slides in it. If not,
// the deck showing stays.
internal void
_FR_OpenDeck(App_State* app_state, String8 path, U64 now_us) {
   U64        first_slide = 0;
   DECK_Deck* deck = DECK_OpenPath(path, &first_slide);

   if (deck->slide_count == 0) {
      printf("No images in %.*s\n", Str8VArg(path));
      DECK_Close(deck);
   } else {
      printf("%llu slides in %.*s\n", (unsigned long long)deck->slide_count, Str8VArg(deck->folder));
      DECK_Close(app_state->deck);
      app_state->deck = deck;
      app_state->slide_idx = first_slide;
      app_state->slide_changed_us = now_us;
      app_state->play_from_us = now_us;

      // The old deck's slides are gone, so the first of the new one fades in
      // over the background.
      app_state->fade_active = 1;
      app_state->fade_from_idx = deck->slide_count;
      app_state->fade_start_us = 0;
   }
}

internal void
_FR_HandleInput(App_State* app_state) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;
   U64           now_us = NowTimeUS();

   F32 scroll = 0;
   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         app_state->quit = 1;
      }
      if (event->kind == WM_EventKind_Scroll) {
         scroll += event->delta.y;
      }
      // Of several things dropped at once, the first is opened.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         _FR_OpenDeck(app_state, event->strings.first->string, now_us);
      }
   }

   //- bgz: fullscreen and quit
   B32 is_fullscreen = Core_WM_WindowIsFullscreen(window);
   B32 fullscreen_pressed = 0;
   fullscreen_pressed |= Core_WM_KeyPress(events, window, 0, WM_Key_F);
   fullscreen_pressed |= Core_WM_KeyPress(events, window, 0, WM_Key_F11);
   fullscreen_pressed |= Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Return);
   if (fullscreen_pressed) {
      app_state->toggle_fullscreen = 1;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
      if (is_fullscreen) {
         app_state->toggle_fullscreen = 1;
      } else {
         app_state->quit = 1;
      }
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
         U64 play_us = (U64)(_FR_PLAY_SECONDS * 1000000.0f);
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
      U64 fade_us = (U64)(_FR_FADE_SECONDS * 1000000.0f);
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
//~ bgz: drawing

// As large as fits in the window with its shape kept, in the middle of it.
// alpha is how much of the image shows over what is already drawn.
internal void
_FR_DrawImageFit(IMG_Image* image, Vec2F32 window_dim, F32 alpha) {
   F32 image_w = (F32)image->size.x;
   F32 image_h = (F32)image->size.y;
   F32 scale = Min(window_dim.x / image_w, window_dim.y / image_h);
   F32 w = image_w * scale;
   F32 h = image_h * scale;
   F32 x = (window_dim.x - w) * 0.5f;
   F32 y = (window_dim.y - h) * 0.5f;

   // The source rect is in the image's own pixels.
   Core_D_Img(R2F32P(x, y, x + w, y + h), R2F32P(0, 0, image_w, image_h), image->texture, V4F32(1, 1, 1, alpha));
}

internal void
_FR_DrawSlide(App_State* app_state) {
   Vec2F32 window_dim = Dim2F32(Core_WM_ClientRectFromWindow(app_state->window));

   // The background, which is all there is when the deck is empty or the
   // slide's file could not be loaded.
   Core_D_Rect(R2F32P(0, 0, window_dim.x, window_dim.y), R2F32P(0, 0, 1, 1), V4F32(0.05f, 0.05f, 0.06f, 1.0f));

   // The slide showing is asked for first: that is what loads it, and what
   // releases the slides far from it.
   IMG_Image* image = DECK_ImageFromSlide(app_state->deck, app_state->slide_idx);

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
      F32 t = seconds / _FR_FADE_SECONDS;
      if (t >= 1.0f) {
         app_state->fade_active = 0;
      } else {
         fade_t = t * t * (3.0f - 2.0f * t); // eased at both ends
      }
   }

   if (window_dim.x > 0 && window_dim.y > 0) {
      // The slide being left is only drawn if its image is still loaded. It
      // is after a step to a neighbour. After a jump across the deck it has
      // been released, and the new slide fades in over the background alone.
      if (app_state->fade_active) {
         IMG_Image* from_image = DECK_LoadedImageFromSlide(app_state->deck, app_state->fade_from_idx);
         if (from_image != 0) {
            _FR_DrawImageFit(from_image, window_dim, 1.0f - fade_t);
         }
      }
      if (image != 0) {
         _FR_DrawImageFit(image, window_dim, fade_t);
      }

      // While playing, a thin bar along the bottom that fills as the slide's
      // turn runs out - which is also what shows that it is playing at all.
      if (app_state->playing && app_state->deck->slide_count > 1) {
         F32 play_seconds = (F32)(NowTimeUS() - app_state->play_from_us) / 1000000.0f;
         F32 play_t = Clamp(0, play_seconds / _FR_PLAY_SECONDS, 1);
         F32 bar_h = 3.0f;
         Core_D_Rect(R2F32P(0, window_dim.y - bar_h, window_dim.x * play_t, window_dim.y), R2F32P(0, 0, 1, 1),
                     V4F32(1, 1, 1, 0.5f));
      }
   }
}

////////////////////////////////
//~ bgz: the frame

internal void
FrameWork(App_State* app_state) {
   _FR_HandleInput(app_state);

   Core_D_BeginFrame();
   D_Bucket* bucket = Core_D_BucketMake();
   Core_D_BucketScope(bucket) {
      _FR_DrawSlide(app_state);
   }
   Core_D_SubmitBucket(app_state->window, app_state->window_eqp, bucket);
}

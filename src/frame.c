////////////////////////////////
//~ bgz: one frame's work
//
// FrameWork is everything between the renderer's begin and end of a frame
// (main.c): read this frame's input, then draw the slide that is showing,
// fading from the one before it if the slide just changed. It
// also runs with no events at all, when Windows asks for a repaint during a
// resize - see _Main_RepaintFrame in main.c.

////////////////////////////////
//~ bgz: input
//
//    next slide       Right, Down, Space, Return, Page Down
//    previous slide   Left, Up, Backspace, Page Up
//    first / last     Home / End
//    quit             Esc, or closing the window

internal void
_FR_HandleInput(App_State* app_state) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;

   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         app_state->quit = 1;
      }
   }

   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
      app_state->quit = 1;
   }

   WM_Key next_keys[] = {WM_Key_Right, WM_Key_Down, WM_Key_Space, WM_Key_Return, WM_Key_PageDown};
   WM_Key prev_keys[] = {WM_Key_Left, WM_Key_Up, WM_Key_Backspace, WM_Key_PageUp};

   U64 slide_count = app_state->deck->slide_count;
   U64 old_slide_idx = app_state->slide_idx;
   if (slide_count != 0) {
      U64 last_idx = slide_count - 1;

      // A key held down repeats, and each repeat is a press of its own. The
      // deck does not wrap: next on the last slide stays there.
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
      if (Core_WM_KeyPress(events, window, 0, WM_Key_Home)) {
         app_state->slide_idx = 0;
      }
      if (Core_WM_KeyPress(events, window, 0, WM_Key_End)) {
         app_state->slide_idx = last_idx;
      }
   }

   // However many presses there were this frame, the fade is from the slide
   // that was on screen to the one that ends up showing. A change in the
   // middle of a fade starts a new one from the slide being faded to.
   if (app_state->slide_idx != old_slide_idx) {
      app_state->fade_active = 1;
      app_state->fade_from_idx = old_slide_idx;
      app_state->fade_start_us = 0;
   }
}

////////////////////////////////
//~ bgz: drawing

#define _FR_FADE_SECONDS 0.25f

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

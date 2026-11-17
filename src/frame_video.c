////////////////////////////////
//~ bgz: a slide's video: when it plays, and its controls
//
// A slide whose picture is a video file (src\video) shows it playing:
//
//  - Presenting, a slide's video starts from its beginning when the slide
//    comes up, and stops when the slide goes.
//  - Editing, it waits at wherever it is, until it is played by hand.
//
// Its controls are a strip along the bottom of where it is drawn:
//
//    [Pause]  ====|--------------------  0:12 / 1:30  [Sound]
//
// A click on the first word plays and pauses, a click on the bar goes to
// that point, a click on the last word turns all sound off and on. The edit screen draws the strip in the preview and takes its clicks
// (frame_edit.c). The presentation draws it for a moment after one of the
// video's keys, and while the video is paused (frame.c):
//
//    K            play, pause
//    J / L        back / on, 5 seconds
//    M            sound off, on (all of the program's: src\audio)
//
// Nothing here knows how a video is decoded: it uses video.h only.

#define FR_VIDEO_STEP_SECONDS 5.0
// How long the presentation shows the strip after a key of the video's.
#define FR_VIDEO_STRIP_SECONDS 2.5f

typedef struct _FR_VideoStrip _FR_VideoStrip;
struct _FR_VideoStrip {
   Rng2F32 strip;  // all of it
   Rng2F32 button; // the word that plays and pauses
   Rng2F32 bar;    // the length of the video, left to right
   Rng2F32 sound;  // the word that turns sound off and on
   F32     px;
   F32     time_x; // where the time is written
};

internal String8
_FR_VideoClock(Arena* arena, F64 time_s) {
   U64     seconds = (U64)Max(time_s, 0.0);
   String8 result = PushStr8F(arena, "%llu:%02llu", (unsigned long long)(seconds / 60), (unsigned long long)(seconds % 60));
   return result;
}

// Where the strip's parts are, for a video drawn at video_rect. ui: how many
// pixels a unit of the sizes is.
internal _FR_VideoStrip
_FR_VideoStripFromRect(App_State* app_state, Rng2F32 video_rect, F32 ui) {
   _FR_VideoStrip result = {0};
   TXT_Fonts*     fonts = &app_state->fonts;

   F32 px = (F32)(S32)Max(14.0f * ui, 11.0f);
   F32 strip_h = (F32)(S32)(TXT_LineHeight(fonts->regular, px) * 1.6f);
   F32 pad = (F32)(S32)(px * 0.6f);
   strip_h = Min(strip_h, Dim2F32(video_rect).y);

   result.px = px;
   result.strip = R2F32P(video_rect.min.x, video_rect.max.y - strip_h, video_rect.max.x, video_rect.max.y);

   // As wide as the wider of its two words, so that it does not change
   // size between them.
   F32 button_w = Max(TXT_Width(fonts->regular, px, Str8Lit("Pause")), TXT_Width(fonts->regular, px, Str8Lit("Play"))) + pad * 2.0f;
   result.button = R2F32P(result.strip.min.x, result.strip.min.y, Min(result.strip.min.x + button_w, result.strip.max.x),
                          result.strip.max.y);

   // At the right end, as wide as the wider of its two words.
   F32 sound_w = Max(TXT_Width(fonts->regular, px, Str8Lit("Sound")), TXT_Width(fonts->regular, px, Str8Lit("Muted"))) + pad * 2.0f;
   result.sound = R2F32P(Max(result.strip.max.x - sound_w, result.button.max.x), result.strip.min.y, result.strip.max.x,
                         result.strip.max.y);

   // The time takes as much room as its widest: "00:00 / 00:00".
   F32 time_w = TXT_Width(fonts->regular, px, Str8Lit("00:00 / 00:00"));
   result.time_x = result.sound.min.x - pad - time_w;
   result.bar = R2F32P(result.button.max.x + pad, result.strip.min.y, result.time_x - pad, result.strip.max.y);
   if (result.bar.max.x < result.bar.min.x) {
      // Too narrow for the bar and the time: the word alone.
      result.bar.max.x = result.bar.min.x;
      result.time_x = result.strip.max.x;
      result.sound.min.x = result.sound.max.x; // nor for the last word
   }

   return result;
}

internal void
FR_VideoDrawStrip(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, Vec2F32 mouse) {
   TXT_Fonts*     fonts = &app_state->fonts;
   _FR_VideoStrip strip = _FR_VideoStripFromRect(app_state, video_rect, ui);
   Temp           scratch = ScratchBegin(0, 0);

   Vec4F32 color_text = V4F32(1, 1, 1, 0.95f);
   F32     text_y = strip.strip.min.y + (Dim2F32(strip.strip).y - TXT_LineHeight(fonts->regular, strip.px)) * 0.5f;

   Core_D_PushClip(strip.strip);
   // Dark, whatever the video and the theme: the words on it are white.
   Core_D_Rect(strip.strip, R2F32P(0, 0, 1, 1), V4F32(0, 0, 0, 0.62f));

   //- bgz: the word
   if (Contains2F32(strip.button, mouse)) {
      Core_D_Rect(strip.button, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.18f));
   }
   String8 word = VID_IsPlaying(video) ? Str8Lit("Pause") : Str8Lit("Play");
   F32     word_w = TXT_Width(fonts->regular, strip.px, word);
   TXT_Draw(fonts->regular, strip.px, V2F32(strip.button.min.x + (Dim2F32(strip.button).x - word_w) * 0.5f, text_y),
            color_text, word);

   //- bgz: the last word: dim when sound is off
   if (Dim2F32(strip.sound).x > 0) {
      if (Contains2F32(strip.sound, mouse)) {
         Core_D_Rect(strip.sound, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.18f));
      }
      B32     is_muted = AUD_IsMuted();
      String8 sound_word = is_muted ? Str8Lit("Muted") : Str8Lit("Sound");
      F32     sound_word_w = TXT_Width(fonts->regular, strip.px, sound_word);
      TXT_Draw(fonts->regular, strip.px, V2F32(strip.sound.min.x + (Dim2F32(strip.sound).x - sound_word_w) * 0.5f, text_y),
               is_muted ? V4F32(1, 1, 1, 0.5f) : color_text, sound_word);
   }

   //- bgz: the bar: a thin line, the part played brighter, thicker under the mouse
   F64 duration_s = VID_Duration(video);
   F64 position_s = VID_Position(video);
   if (Dim2F32(strip.bar).x > 0) {
      F32 bar_h = (F32)(S32)Max(Contains2F32(strip.bar, mouse) ? 6.0f * ui : 3.0f * ui, 2.0f);
      F32 bar_y = (F32)(S32)((strip.bar.min.y + strip.bar.max.y - bar_h) * 0.5f);
      F32 played = (duration_s > 0) ? (F32)Clamp(0.0, position_s / duration_s, 1.0) : 0.0f;
      F32 played_x = strip.bar.min.x + Dim2F32(strip.bar).x * played;
      Core_D_Rect(R2F32P(strip.bar.min.x, bar_y, strip.bar.max.x, bar_y + bar_h), R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.25f));
      Core_D_Rect(R2F32P(strip.bar.min.x, bar_y, played_x, bar_y + bar_h), R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.95f));

      //- bgz: the time: where it is, of how long it is
      String8 time = (duration_s > 0) ? PushStr8F(scratch.arena, "%S / %S", _FR_VideoClock(scratch.arena, position_s),
                                                  _FR_VideoClock(scratch.arena, duration_s))
                                      : _FR_VideoClock(scratch.arena, position_s);
      TXT_Draw(fonts->regular, strip.px, V2F32(strip.time_x, text_y), color_text, time);
   }
   Core_D_PopClip();

   ScratchEnd(scratch);
}

// A press at pos, for a video drawn at video_rect: plays, pauses or seeks if
// it is on the strip. Returns whether it was - whether the press is used up.
internal B32
FR_VideoClickStrip(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, Vec2F32 pos) {
   B32            result = 0;
   _FR_VideoStrip strip = _FR_VideoStripFromRect(app_state, video_rect, ui);

   if (Contains2F32(strip.strip, pos)) {
      result = 1;
      if (Contains2F32(strip.button, pos)) {
         VID_SetPlaying(video, !VID_IsPlaying(video));
      } else if (Contains2F32(strip.sound, pos)) {
         AUD_SetMuted(!AUD_IsMuted());
      } else if (Contains2F32(strip.bar, pos) && Dim2F32(strip.bar).x > 0 && VID_Duration(video) > 0) {
         F64 part = (F64)(pos.x - strip.bar.min.x) / (F64)Dim2F32(strip.bar).x;
         VID_Seek(video, VID_Duration(video) * Clamp(0.0, part, 1.0));
      }
   }

   return result;
}

// The video's keys, while presenting. Returns whether one was pressed.
internal B32
FR_VideoKeys(App_State* app_state, VID_Video* video) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;
   B32           result = 0;

   if (Core_WM_KeyPress(events, window, 0, WM_Key_K)) {
      VID_SetPlaying(video, !VID_IsPlaying(video));
      result = 1;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_M)) {
      AUD_SetMuted(!AUD_IsMuted());
      result = 1;
   }
   // A key held down repeats: each is a step.
   while (Core_WM_KeyPress(events, window, 0, WM_Key_J)) {
      VID_Seek(video, VID_Position(video) - FR_VIDEO_STEP_SECONDS);
      result = 1;
   }
   while (Core_WM_KeyPress(events, window, 0, WM_Key_L)) {
      VID_Seek(video, VID_Position(video) + FR_VIDEO_STEP_SECONDS);
      result = 1;
   }

   return result;
}

// Once a frame, after the frame's input: starts the video of the slide that
// has come up in the presentation, stops the one of the slide that has
// gone, and moves every open video on.
internal void
FR_VideoUpdate(App_State* app_state) {
   DECK_Deck* deck = app_state->deck;

   // The slide whose video plays of itself: the one presented. None while
   // editing. Plus one, so that 0 is none.
   U64 want = (!app_state->is_editing && deck->slide_count != 0) ? app_state->slide_idx + 1 : 0;
   if (want != app_state->video_auto_slide) {
      VID_Video* old_video = (app_state->video_auto_slide != 0) ? DECK_VideoFromSlide(deck, app_state->video_auto_slide - 1) : 0;
      if (old_video != 0) {
         VID_SetPlaying(old_video, 0);
      }

      if (want == 0 || !DECK_SlideHasVideo(deck, want - 1)) {
         app_state->video_auto_slide = want;
      } else {
         // Opened when the slide is first drawn, which may be later this
         // frame: until then this is asked again.
         VID_Video* video = DECK_VideoFromSlide(deck, want - 1);
         if (video != 0) {
            VID_Seek(video, 0);
            VID_SetPlaying(video, 1);
            app_state->video_auto_slide = want;
         }
      }
   }

   DECK_UpdateVideos(deck);
}

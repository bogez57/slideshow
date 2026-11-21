////////////////////////////////
//~ bgz: a slide's video: when it plays, and its controls
//
// A slide whose picture is a video file (src\video) shows it playing:
//
//  - Presenting, a slide's video starts from its beginning when the slide
//    comes up, and stops when the slide goes. A slide with several has
//    them start, pause and step together: the keys below are for all of
//    them, and so is the presenter's button.
//  - Editing, it waits at wherever it is, until it is played by hand -
//    each by its own strip.
//
// A slide's @video line changes that (deck.h): loop has it start over at
// its end, mute keeps it silent, manual has it wait for K when the slide
// comes up.
//
// Its controls are a strip along the bottom of where it is drawn:
//
//    [Pause]  ====|--------------------  0:12 / 1:30  [Sound]
//
// A click on the first word plays and pauses, a click on the bar goes to
// that point, a click on the last word turns all sound off and on. The edit
// screen builds the strip in the preview as UI boxes, which take its clicks
// (edit\edit.c). The presentation draws it for a moment after one of the
// video's keys, and while the video is paused (frame.c):
//
//    K            play, pause
//    J / L        back / on, 5 seconds
//    M            sound off, on (all of the program's: src\audio)
//
// Nothing here knows how a video is decoded: it uses video.h only.

#define _SVID_STEP_SECONDS 5.0

typedef struct _SVID_Strip _SVID_Strip;
struct _SVID_Strip {
   Rng2F32 strip;  // all of it
   Rng2F32 button; // the word that plays and pauses
   Rng2F32 bar;    // the length of the video, left to right
   Rng2F32 sound;  // the word that turns sound off and on
   F32     px;
   F32     time_x; // where the time is written
};

PRIVATE String8
_SVID_Clock(Arena* arena, F64 time_s) {
   U64     seconds = (U64)Max(time_s, 0.0);
   String8 result = PushStr8F(arena, "%llu:%02llu", (unsigned long long)(seconds / 60), (unsigned long long)(seconds % 60));
   return result;
}

// Where the strip's parts are, for a video drawn at video_rect. ui: how many
// pixels a unit of the sizes is.
PRIVATE _SVID_Strip
_SVID_StripFromRect(App_State* app_state, Rng2F32 video_rect, F32 ui) {
   _SVID_Strip result = {0};
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

//- bgz: the strip in the UI layer
//
// Two sets of boxes, as the edit screen's preview has for everything over
// its slide: ones that are clicked and not drawn, built before the slide's
// own (place\place.c) so a press on the strip is not a press on the
// picture, and ones that are drawn, built after the slide is laid out again
// for what the clicks did - over it, and under a piece's outline. The parts
// of the strip are in its own boxes, with whole pixels: as the hand-drawn
// strip was.

PRIVATE String8
_SVID_PartKey(Arena* arena, U64 id, char* part) {
   return PushStr8F(arena, "###video %llu %s", (unsigned long long)id, part);
}

B32
SVID_BuildStripHits(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, U64 id) {
   _SVID_Strip strip = _SVID_StripFromRect(app_state, video_rect, ui);
   UI_BoxFlags    flags = UI_BoxFlag_MouseClickable;
   Temp           scratch = ScratchBegin(0, 0);

   // The parts first, then the strip under them: a press on the strip that
   // misses them is the strip's, and not the picture's.
   UI_Box* button = FR_BuildRectBox(flags, strip.button, "%S", _SVID_PartKey(scratch.arena, id, "button"));
   UI_Box* sound = (Dim2F32(strip.sound).x > 0) ? FR_BuildRectBox(flags, strip.sound, "%S", _SVID_PartKey(scratch.arena, id, "sound")) : 0;
   UI_Box* bar = (Dim2F32(strip.bar).x > 0) ? FR_BuildRectBox(flags, strip.bar, "%S", _SVID_PartKey(scratch.arena, id, "bar")) : 0;
   UI_Box* all = FR_BuildRectBox(flags, strip.strip, "%S", _SVID_PartKey(scratch.arena, id, "strip"));

   B32       result = 0;
   UI_Signal signal = UI_SignalFromBox(button);
   result |= UI_Hovering(signal);
   if (signal.f & UI_SignalFlag_LeftPressed) {
      VID_SetPlaying(video, !VID_IsPlaying(video));
   }
   if (sound != 0) {
      signal = UI_SignalFromBox(sound);
      result |= UI_Hovering(signal);
      if (signal.f & UI_SignalFlag_LeftPressed) {
         AUD_SetMuted(!AUD_IsMuted());
      }
   }
   if (bar != 0) {
      signal = UI_SignalFromBox(bar);
      result |= UI_Hovering(signal);
      if ((signal.f & UI_SignalFlag_LeftPressed) && VID_Duration(video) > 0) {
         F64 part = (F64)(UI_DragStartMouse().x - strip.bar.min.x) / (F64)Dim2F32(strip.bar).x;
         VID_Seek(video, VID_Duration(video) * Clamp(0.0, part, 1.0));
      }
   }
   result |= UI_Hovering(UI_SignalFromBox(all));

   ScratchEnd(scratch);
   return result;
}

// Whether the mouse is on a part of the strip: its click box hovered, or -
// with no click boxes - under the mouse.
PRIVATE B32
_SVID_PartIsHot(U64 id, char* part, Rng2F32 rect, B32 interactive) {
   B32 result = 0;
   if (interactive) {
      Temp scratch = ScratchBegin(0, 0);
      result = UI_KeyMatch(UI_HotKey(), UI_KeyFromString(UI_ActiveSeedKey(), _SVID_PartKey(scratch.arena, id, part)));
      ScratchEnd(scratch);
   } else {
      // The window's mouse: the UI's is off the window when the window is not
      // in front and the mouse has not moved for a moment.
      result = Contains2F32(rect, Core_WM_MouseFromWindow(UI_Window()));
   }
   return result;
}

// A box of the strip's, at rect in the window, inside the strip's box,
// which starts at origin.
PRIVATE UI_Box*
_SVID_StripBox(UI_BoxFlags flags, Rng2F32 rect, Vec2F32 origin, U64 id, char* part) {
   UI_SetNextRect(R2F32P(rect.min.x - origin.x, rect.min.y - origin.y, rect.max.x - origin.x, rect.max.y - origin.y));
   return UI_BuildBoxFromStringf(flags, "###video look %llu %s", (unsigned long long)id, part);
}

void
SVID_BuildStripLook(App_State* app_state, VID_Video* video, Rng2F32 video_rect, F32 ui, U64 id, B32 interactive) {
   _SVID_Strip strip = _SVID_StripFromRect(app_state, video_rect, ui);
   Temp           scratch = ScratchBegin(0, 0);

   // Asked before the strip's box is the parent: the boxes clicked are keyed
   // under the window's root.
   B32 bar_hot = _SVID_PartIsHot(id, "bar", strip.bar, interactive);
   B32 sound_hot = _SVID_PartIsHot(id, "sound", strip.sound, interactive);
   B32 button_hot = _SVID_PartIsHot(id, "button", strip.button, interactive);

   // Dark, whatever the video and the theme: the words on it are white.
   // Whole pixels, as it was drawn by hand: the parts are inside it.
   Rng2F32 all = R2F32P((F32)(S32)strip.strip.min.x, (F32)(S32)strip.strip.min.y, strip.strip.max.x, strip.strip.max.y);
   UI_Box* box = 0;
   UI_ColorTag(Str8Lit("strip")) {
      box = FR_BuildRectBox(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clip, all, "###video look %llu", (unsigned long long)id);
   }
   UI_Parent(box) UI_ColorTag(Str8Lit("strip")) UI_FontSize(FR_UIFontSize(strip.px)) {
      Vec2F32 origin = all.min;

      //- bgz: the time: where it is, of how long it is. First: on top
      F64 duration_s = VID_Duration(video);
      F64 position_s = VID_Position(video);
      if (Dim2F32(strip.bar).x > 0) {
         String8 time = (duration_s > 0) ? PushStr8F(UI_BuildArena(), "%S / %S", _SVID_Clock(scratch.arena, position_s),
                                                     _SVID_Clock(scratch.arena, duration_s))
                                         : _SVID_Clock(UI_BuildArena(), position_s);
         UI_SetNextTextAlignment(UI_TextAlign_Left);
         UI_SetNextTextPadding(0);
         UI_Box* time_box = _SVID_StripBox(UI_BoxFlag_DrawText, R2F32P(strip.time_x, strip.strip.min.y, strip.sound.min.x, strip.strip.max.y),
                                              origin, id, "time");
         UI_BoxEquipDisplayString(time_box, time);

         //- bgz: the bar: a thin line, the part played brighter, thicker
         // under the mouse
         F32 bar_h = (F32)(S32)Max(bar_hot ? 6.0f * ui : 3.0f * ui, 2.0f);
         F32 bar_y = (F32)(S32)((strip.bar.min.y + strip.bar.max.y - bar_h) * 0.5f);
         F32 played = (duration_s > 0) ? (F32)Clamp(0.0, position_s / duration_s, 1.0) : 0.0f;
         F32 played_x = strip.bar.min.x + Dim2F32(strip.bar).x * played;
         UI_SetNextBackgroundColor(UI_ThemeColor(Str8Lit("played"), UI_ColorSlot_Background));
         _SVID_StripBox(UI_BoxFlag_DrawBackground, R2F32P(strip.bar.min.x, bar_y, played_x, bar_y + bar_h), origin, id, "played");
         UI_SetNextBackgroundColor(UI_ThemeColor(Str8Lit("track"), UI_ColorSlot_Background));
         _SVID_StripBox(UI_BoxFlag_DrawBackground, R2F32P(strip.bar.min.x, bar_y, strip.bar.max.x, bar_y + bar_h), origin, id, "track");
      }

      //- bgz: the last word: dim when sound is off
      if (Dim2F32(strip.sound).x > 0) {
         B32 is_muted = AUD_IsMuted();
         UI_SetNextTextAlignment(UI_TextAlign_Center);
         if (is_muted) {
            UI_SetNextTextColor(UI_ThemeColor(Str8Lit("muted"), UI_ColorSlot_Text));
         }
         UI_Box* sound = _SVID_StripBox(UI_BoxFlag_DrawText, strip.sound, origin, id, "sound");
         UI_BoxEquipDisplayString(sound, is_muted ? Str8Lit("Muted") : Str8Lit("Sound"));
         if (sound_hot) {
            UI_SetNextBackgroundColor(UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Background));
            _SVID_StripBox(UI_BoxFlag_DrawBackground, strip.sound, origin, id, "sound hot");
         }
      }

      //- bgz: the word that plays and pauses
      UI_SetNextTextAlignment(UI_TextAlign_Center);
      UI_Box* button = _SVID_StripBox(UI_BoxFlag_DrawText, strip.button, origin, id, "button");
      UI_BoxEquipDisplayString(button, VID_IsPlaying(video) ? Str8Lit("Pause") : Str8Lit("Play"));
      if (button_hot) {
         UI_SetNextBackgroundColor(UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Background));
         _SVID_StripBox(UI_BoxFlag_DrawBackground, strip.button, origin, id, "button hot");
      }
   }

   ScratchEnd(scratch);
}

// Whether any of videos is playing.
PRIVATE B32
_FR_AnyPlaying(VID_Video** videos, U64 count) {
   B32 result = 0;
   for (U64 i = 0; i < count; i += 1) {
      result |= VID_IsPlaying(videos[i]);
   }
   return result;
}

B32
SVID_SlideIsPlaying(DECK_Deck* deck, U64 slide_idx) {
   VID_Video* videos[DECK_MAX_PICTURES] = {0};
   U64        count = DECK_VideosFromSlide(deck, slide_idx, videos);
   return _FR_AnyPlaying(videos, count);
}

// A slide with several videos plays and pauses them together: paused if any
// is playing, else all played. Not by the first alone: a shorter one that
// has reached its end while the others go on would have K start it over,
// and pause nothing.
void
SVID_ToggleSlide(DECK_Deck* deck, U64 slide_idx) {
   VID_Video* videos[DECK_MAX_PICTURES] = {0};
   U64        count = DECK_VideosFromSlide(deck, slide_idx, videos);
   B32        play = !_FR_AnyPlaying(videos, count);
   for (U64 i = 0; i < count; i += 1) {
      VID_SetPlaying(videos[i], play);
   }
}

// The keys of the videos of the slide showing, while presenting: one key
// is for all of them. Returns whether one was pressed.
B32
SVID_Keys(App_State* app_state) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;
   B32           result = 0;
   VID_Video*    videos[DECK_MAX_PICTURES] = {0};
   U64           count = DECK_VideosFromSlide(app_state->deck, app_state->slide_idx, videos);
   if (count == 0) {
      return 0;
   }

   if (Core_WM_KeyPress(events, window, 0, WM_Key_K)) {
      SVID_ToggleSlide(app_state->deck, app_state->slide_idx);
      result = 1;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_M)) {
      AUD_SetMuted(!AUD_IsMuted());
      result = 1;
   }
   // A key held down repeats: each is a step. Playing, every one of them
   // plays on from where the step puts it: one that L took to its end and
   // J brings back goes on with the others, not paused there while they
   // play.
   B32 was_playing = _FR_AnyPlaying(videos, count);
   while (Core_WM_KeyPress(events, window, 0, WM_Key_J)) {
      for (U64 i = 0; i < count; i += 1) {
         VID_Seek(videos[i], VID_Position(videos[i]) - _SVID_STEP_SECONDS);
         VID_SetPlaying(videos[i], was_playing);
      }
      result = 1;
   }
   while (Core_WM_KeyPress(events, window, 0, WM_Key_L)) {
      for (U64 i = 0; i < count; i += 1) {
         VID_Seek(videos[i], VID_Position(videos[i]) + _SVID_STEP_SECONDS);
         VID_SetPlaying(videos[i], was_playing);
      }
      result = 1;
   }

   return result;
}

// Once a frame, after the frame's input: starts the video of the slide that
// has come up in the presentation, stops the one of the slide that has
// gone, and moves every open video on.
void
SVID_Update(App_State* app_state) {
   DECK_Deck* deck = app_state->deck;

   // The slide whose video plays of itself: the one presented. None while
   // editing. Plus one, so that 0 is none.
   U64 want = (!app_state->is_editing && deck->slide_count != 0) ? app_state->slide_idx + 1 : 0;
   if (want != app_state->video_auto_slide) {
      VID_Video* videos[DECK_MAX_PICTURES] = {0};
      U64        old_count = (app_state->video_auto_slide != 0) ? DECK_VideosFromSlide(deck, app_state->video_auto_slide - 1, videos) : 0;
      for (U64 i = 0; i < old_count; i += 1) {
         VID_SetPlaying(videos[i], 0);
      }

      if (want == 0 || !DECK_SlideHasVideo(deck, want - 1)) {
         app_state->video_auto_slide = want;
      } else {
         // Opened when the slide is first drawn, which may be later this
         // frame: until then this is asked again. All of the slide's
         // videos start together.
         U64 count = DECK_VideosFromSlide(deck, want - 1, videos);
         if (count != 0 && DECK_SlideVideosAreOpen(deck, want - 1)) {
            for (U64 i = 0; i < count; i += 1) {
               VID_Seek(videos[i], 0);
               // manual: at its start, and waiting. Paused, its strip
               // shows, which says there is something to play.
               VID_SetPlaying(videos[i], !deck->slides[want - 1].video_manual);
            }
            app_state->video_auto_slide = want;
         }
      }
   }

   // What each slide's @video line says, of the videos that are open. Asked
   // every frame: the line can be written or taken away at any time.
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      DECK_Slide* slide = &deck->slides[i];
      for (U64 k = 0; k < slide->picture_count; k += 1) {
         VID_Video* video = slide->pictures[k].video;
         if (video != 0) {
            VID_SetMuted(video, slide->video_mute);
            if (slide->video_loop && VID_HasEnded(video)) {
               VID_SetPlaying(video, 1); // from its start
            }
         }
      }
   }

   DECK_UpdateVideos(deck);
}

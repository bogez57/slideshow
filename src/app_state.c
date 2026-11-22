////////////////////////////////
//~ bgz: app state's ops
//
// The state ops for what belongs to no layer: the notice, quitting, what is
// shown over a slide, which slide is showing. Declared in app_state.h, pushed
// with StateOp_<Name>(ops), and run by _FR_UpdateAppState (frame.c).

void
App_Notice(App_State* app_state, StateOp_List* ops, App_NoticeOpParams* params) {
   UnusedVariable(ops);
   app_state->notice = params->notice;
   app_state->notice_us = NowTimeUS();
}
STATE_OP(App_Notice)

void
App_RequestQuit(App_State* app_state, StateOp_List* ops) {
   B32 has_unsaved = (app_state->editor != 0 && Editor_HasUnsavedChanges(app_state->editor));
   if (has_unsaved && !app_state->quit_warned) {
      app_state->quit_warned = 1;
      StateOp_App_Notice(ops)->notice = Str8Lit("Unsaved changes. Ctrl+S saves. Close again to quit without saving.");
   } else if (Copy_IsBusy(app_state) && !app_state->quit_warned_copy) {
      // Its slide names the file as it will be in the deck's folder.
      app_state->quit_warned_copy = 1;
      StateOp_App_Notice(ops)->notice = Str8Lit("Still copying a video into the deck's folder. Close again to quit anyway.");
   } else {
      app_state->quit = 1;
   }
}
STATE_OP_NO_PARAMS(App_RequestQuit)

void
App_ToggleInfo(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->show_info = !app_state->show_info;
}
STATE_OP_NO_PARAMS(App_ToggleInfo)

void
App_ToggleNotes(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->show_notes = !app_state->show_notes;
}
STATE_OP_NO_PARAMS(App_ToggleNotes)

void
App_ToggleHelp(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->show_help = !app_state->show_help;
}
STATE_OP_NO_PARAMS(App_ToggleHelp)

void
App_ToggleFullscreen(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->toggle_fullscreen = 1;
}
STATE_OP_NO_PARAMS(App_ToggleFullscreen)

void
App_TogglePresenter(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->toggle_presenter = 1;
}
STATE_OP_NO_PARAMS(App_TogglePresenter)

void
App_TogglePlaying(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->playing = !app_state->playing;
   app_state->play_from_us = NowTimeUS();
}
STATE_OP_NO_PARAMS(App_TogglePlaying)

void
App_StartEditing(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   if (app_state->editor == 0) {
      return;
   }

   // Back to the text, at the slide that was showing - unless the cursor
   // is on that slide already, and then it stays where it was.
   Deck_Deck* deck = app_state->deck;
   if (deck->slide_count != 0 && Deck_SlideFromTextPos(deck, app_state->editor->cursor) != app_state->slide_idx) {
      Editor_SetCursor(app_state->editor, deck->slides[app_state->slide_idx].text_pos);
   }
   app_state->is_editing = 1;
   app_state->playing = 0;
   app_state->fade_active = 0;
   if (Core_WM_WindowIsFullscreen(app_state->window)) {
      app_state->toggle_fullscreen = 1;
   }
}
STATE_OP_NO_PARAMS(App_StartEditing)

void
App_OpenPath(App_State* app_state, StateOp_List* ops, App_OpenPathOpParams* params) {
   UnusedVariable(ops);
   if (FR_OpenPath(app_state, params->path)) {
      app_state->show_open = 0;
   }
}
STATE_OP(App_OpenPath)

void
App_GoToSlide(App_State* app_state, StateOp_List* ops, App_GoToSlideOpParams* params) {
   UnusedVariable(ops);
   U64 slide_count = app_state->deck->slide_count;
   U64 slide_idx = (slide_count != 0) ? Min(params->slide_idx, slide_count - 1) : app_state->slide_idx;

   // The fade is from the slide that was on screen to this one.
   //
   // A change that comes before the last one's fade is over is a cut, with no
   // fade. Otherwise a held key, which changes slide many times in the length
   // of one fade, would show every slide it passes only faintly.
   if (slide_idx != app_state->slide_idx) {
      U64 now_us = NowTimeUS();
      U64 fade_us = (U64)(FR_FADE_SECONDS * 1000000.0f);
      B32 is_soon_after_last = (FR_IsFading(app_state) ||
                                (app_state->slide_changed_us != 0 && now_us - app_state->slide_changed_us < fade_us));

      app_state->fade_active = !is_soon_after_last;
      app_state->fade_from_idx = app_state->slide_idx;

      app_state->slide_idx = slide_idx;
      app_state->slide_changed_us = now_us;
      app_state->play_from_us = now_us;
   }
}
STATE_OP(App_GoToSlide)

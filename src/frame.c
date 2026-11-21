////////////////////////////////
//~ bgz: one frame's work
//
// FrameWork is everything between the renderer's begin and end of a frame
// (main.c): read this frame's input and the clock, build the UI layer's tree
// (the open screen, and the whole of the screen that is up: the edit screen,
// edit\edit.c, or the presentation, which is in this file), then draw it. It also runs with no events at
// all, when Windows asks for a repaint during a resize - see
// _Main_RepaintFrame in main.c.

////////////////////////////////
//~ bgz: what the two screens share

// One notch of a mouse wheel, in the units Windows reports it in.
#define _FR_SCROLL_PER_SLIDE 120.0f

// How often a text deck's file is looked at for having been written when
// nothing says it has been: the watch on its folder (FileWatchOpen) says so
// at once, and this is for a folder that could not be watched, or a change
// the watch missed.
#define _FR_RELOAD_CHECK_SECONDS 2.0f

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
   } else if (COPY_IsBusy(app_state) && !app_state->quit_warned_copy) {
      // Its slide names the file as it will be in the deck's folder.
      app_state->quit_warned_copy = 1;
      FR_Notice(app_state, Str8Lit("Still copying a video into the deck's folder. Close again to quit anyway."));
   } else {
      app_state->quit = 1;
   }
}

internal String8
FR_WholePath(Arena* arena, String8 path) {
   B32 is_whole = ((path.size >= 2 && path.str[1] == ':') ||
                   (path.size >= 1 && (path.str[0] == '\\' || path.str[0] == '/')));

   String8 result = {0};
   if (is_whole) {
      result = PushStr8Copy(arena, path);
   } else {
      Temp    scratch = ScratchBegin(&arena, 1);
      String8 start = GetCurrentPath(scratch.arena);
      while (start.size > 0 && (start.str[start.size - 1] == '\\' || start.str[start.size - 1] == '/')) {
         start.size -= 1;
      }
      // "." and ".\\name" are the folder itself and name in it.
      while (path.size >= 1 && path.str[0] == '.' &&
             (path.size == 1 || path.str[1] == '\\' || path.str[1] == '/')) {
         path = Str8Skip(path, Min(path.size, 2));
      }
      result = (path.size != 0) ? PushStr8F(arena, "%S\\%S", start, path) : PushStr8Copy(arena, start);
      ScratchEnd(scratch);
   }

   for (U64 i = 0; i < result.size; i += 1) {
      if (result.str[i] == '/') {
         result.str[i] = '\\';
      }
   }
   // No \ at the end - but a drive's own folder keeps its one: C:\ .
   while (result.size > 3 && result.str[result.size - 1] == '\\') {
      result.size -= 1;
   }

   return result;
}

////////////////////////////////
//~ bgz: the log

internal String8
FR_LocalFolder(Arena* arena) {
   String8 result = PushStr8F(arena, "%S\\local", GetProcessInfo()->binary_path);
   if (!FilePathExists(result)) {
      MakeDirectory(result);
   }
   return result;
}

internal void
FR_OpenLogFile(App_State* app_state) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 folder = PushStr8F(scratch.arena, "%S\\logs", FR_LocalFolder(scratch.arena));
   if (!FilePathExists(folder)) {
      MakeDirectory(folder);
   }

   // The last run's log is kept, one back: it is the one wanted after a run
   // that went wrong.
   String8 path = PushStr8F(scratch.arena, "%S\\slideshow_log.txt", folder);
   if (FilePathExists(path)) {
      CopyFilePath(PushStr8F(scratch.arena, "%S\\slideshow_log_previous.txt", folder), path);
   }

   // ShareRead: it can be read while the program runs.
   app_state->log_file = FileOpen(AccessFlag_Write | AccessFlag_ShareRead, path);
   app_state->log_file_off = 0;

   ScratchEnd(scratch);
}

internal void
FR_WriteLogs(App_State* app_state, LogScopeResult logs) {
   Temp scratch = ScratchBegin(0, 0);

   char* prefixes[LogMsgKind_COUNT] = {0};
   prefixes[LogMsgKind_Info] = ">>> ";
   prefixes[LogMsgKind_UserError] = "!!! ";
   prefixes[LogMsgKind_UserWarning] = "??? ";

   for (U64 kind = 0; kind < LogMsgKind_COUNT; kind += 1) {
      // A line at a time, each with what kind it is in front.
      String8 text = logs.strings[kind];
      U64     pos = 0;
      while (pos < text.size) {
         U64 line_start = pos;
         while (pos < text.size && text.str[pos] != '\n') pos += 1;
         String8 line = Str8SkipChopWhitespace(Str8Substr(text, R1U64(line_start, pos)));
         pos += 1;
         if (line.size == 0) {
            continue;
         }

         String8 out = PushStr8F(scratch.arena, "%s%S\n", prefixes[kind], line);
         if (!FileMatch(app_state->log_file, FileZero())) {
            app_state->log_file_off +=
                FileWrite(app_state->log_file, R1U64(app_state->log_file_off, app_state->log_file_off + out.size), out.str);
         }
         // And to the console, for a run started from one.
         printf("%.*s", Str8VArg(out));
      }
   }

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: the decks opened lately

// How many are kept.
#define _FR_RECENTS_MAX 8

// In the program's local folder, beside it, so that it goes where the
// program goes and is found wherever the program is started from.
internal String8
_FR_RecentsFilePath(Arena* arena) {
   Temp    scratch = ScratchBegin(&arena, 1);
   String8 result = PushStr8F(arena, "%S\\recent.txt", FR_LocalFolder(scratch.arena));
   ScratchEnd(scratch);
   return result;
}

internal void
FR_LoadRecents(App_State* app_state) {
   if (app_state->recents_arena == 0) {
      app_state->recents_arena = ArenaAlloc();
   }
   Arena* arena = app_state->recents_arena;
   ArenaClear(arena);

   Temp    scratch = ScratchBegin(0, 0);
   String8 data = DataFromFilePath(scratch.arena, _FR_RecentsFilePath(scratch.arena));

   // A path a line. Ones that are no longer there are left out.
   String8List paths = {0};
   U64         pos = 0;
   while (pos < data.size && paths.node_count < _FR_RECENTS_MAX) {
      U64 line_start = pos;
      while (pos < data.size && data.str[pos] != '\n') pos += 1;
      String8 line = Str8SkipChopWhitespace(Str8Substr(data, R1U64(line_start, pos)));
      pos += 1;
      if (line.size != 0 && FilePathExists(line)) {
         Str8ListPush(arena, &paths, PushStr8Copy(arena, line));
      }
   }
   app_state->recents = Str8ArrayFromList(arena, &paths);

   ScratchEnd(scratch);
}

// Puts path first among the decks opened lately, and writes them out.
internal void
_FR_RememberRecent(App_State* app_state, String8 path) {
   if (app_state->recents_arena == 0) {
      app_state->recents_arena = ArenaAlloc();
   }
   Temp scratch = ScratchBegin(0, 0);

   // The new list is put together apart from the old, which is in the arena
   // the new one is about to have.
   String8List paths = {0};
   Str8ListPush(scratch.arena, &paths, FR_WholePath(scratch.arena, path));
   for (U64 i = 0; i < app_state->recents.count && paths.node_count < _FR_RECENTS_MAX; i += 1) {
      if (!Str8Match(app_state->recents.v[i], paths.first->string, StringMatchFlag_CaseInsensitive)) {
         Str8ListPush(scratch.arena, &paths, PushStr8Copy(scratch.arena, app_state->recents.v[i]));
      }
   }

   String8List lines = {0};
   for (String8Node* node = paths.first; node != 0; node = node->next) {
      Str8ListPush(scratch.arena, &lines, node->string);
      Str8ListPush(scratch.arena, &lines, Str8Lit("\n"));
   }
   // Not being able to write it loses only the list.
   WriteDataToFilePath(_FR_RecentsFilePath(scratch.arena), Str8ListJoin(scratch.arena, &lines, 0));

   Arena* arena = app_state->recents_arena;
   ArenaClear(arena);
   String8List kept = {0};
   for (String8Node* node = paths.first; node != 0; node = node->next) {
      Str8ListPush(arena, &kept, PushStr8Copy(arena, node->string));
   }
   app_state->recents = Str8ArrayFromList(arena, &kept);

   ScratchEnd(scratch);
}

internal B32
FR_OpenPath(App_State* app_state, String8 path) {
   if (app_state->editor != 0 && ED_HasUnsavedChanges(app_state->editor)) {
      FR_Notice(app_state, Str8Lit("Unsaved changes. Save (Ctrl+S) before opening something else."));
      return 0;
   }

   U64        first_slide = 0;
   DECK_Deck* deck = DECK_OpenPath(path, &first_slide);
   B32        is_text = (deck->file.size != 0);

   // A text file with no slides yet is a deck to be written. Anything else
   // with no slides is nothing to show, and the deck that is open stays.
   if (!is_text && deck->slide_count == 0 && app_state->deck != 0) {
      LogUserErrorF("Nothing to show in %S: no images, and not a text deck (.md, .txt)\n", path);
      FR_Notice(app_state, Str8Lit("Nothing to show there: no images, and not a text deck (.md, .txt)."));
      DECK_Close(deck);
      return 0;
   }

   LogInfoF("Opened %S: %llu slides\n", is_text ? deck->file : deck->folder, (unsigned long long)deck->slide_count);

   if (app_state->deck != 0) {
      DECK_Close(app_state->deck);
   }
   if (app_state->editor != 0) {
      ED_Close(app_state->editor);
      app_state->editor = 0;
   }
   if (app_state->deck_watch != 0) {
      FileWatchClose(app_state->deck_watch);
      app_state->deck_watch = 0;
   }

   U64 now_us = NowTimeUS();
   app_state->deck = deck;
   app_state->slide_idx = first_slide;
   app_state->slide_changed_us = now_us;
   app_state->play_from_us = now_us;
   app_state->playing = 0;
   app_state->quit_warned = 0;
   // The slide numbers are the new deck's: none of its videos was started.
   app_state->video_auto_slide = 0;
   app_state->video_key_us = 0;

   // A text deck opens on the edit screen. Its deck is made again from the
   // editor's text at the next FR_SyncDeck: 0 is no edit_count an editor has.
   if (is_text) {
      app_state->editor = ED_Open(deck->file);
      app_state->parsed_edit_count = 0;

      // Its folder is watched, to know at once when something else writes
      // the file.
      if (app_state->watch_arena == 0) {
         app_state->watch_arena = ArenaAlloc();
      }
      ArenaClear(app_state->watch_arena);
      Temp scratch = ScratchBegin(0, 0);
      app_state->deck_watch = FileWatchOpen(app_state->watch_arena, FR_WholePath(scratch.arena, deck->folder), 0);
      if (app_state->deck_watch == 0) {
         LogInfoF("Could not watch %S for changes: the deck's file is looked at every %.0f seconds instead\n", deck->folder,
                  _FR_RELOAD_CHECK_SECONDS);
      }
      ScratchEnd(scratch);
   }
   app_state->is_editing = is_text;
   app_state->show_format = 1;

   // The old deck's slides are gone, so the first of the new one fades in
   // over the background.
   app_state->fade_active = 1;
   app_state->fade_from_idx = deck->slide_count;
   app_state->fade_start_us = 0;

   // Remembered, to be opened again at the next start - if it is there: a
   // text deck that has not been saved yet is not.
   String8 opened = is_text ? deck->file : deck->folder;
   if (opened.size != 0 && FilePathExists(opened)) {
      _FR_RememberRecent(app_state, opened);
   }

   return 1;
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
   //
   // The watch only says that something in the folder changed - when to
   // look. What is looked at is the file's own time (ED_FileHasChanged).
   B32 look_now = 0;
   if (app_state->deck_watch != 0) {
      Temp        scratch = ScratchBegin(0, 0);
      FileChanges changes = FileWatchPoll(scratch.arena, app_state->deck_watch);
      look_now = (changes.paths.node_count != 0 || changes.overflowed);
      ScratchEnd(scratch);
   }
   U64 now_us = NowTimeUS();
   U64 check_us = (U64)(_FR_RELOAD_CHECK_SECONDS * 1000000.0f);
   if (look_now || now_us - app_state->reload_checked_us >= check_us) {
      app_state->reload_checked_us = now_us;
      if (ED_FileHasChanged(editor) && !ED_HasUnsavedChanges(editor)) {
         ED_Reload(editor);
         LogInfoF("%S was written by something else: read again\n", editor->file);
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
//    notes            N: the slide's @note lines and what comes next
//    video            K plays and pauses the slide's video, J and L go back
//                     and on 5 seconds, M turns sound off and on (slide_video\slide_video.c)
//    presenter view   V: a second window with the slide, the next one, the
//                     notes and a clock (presenter\presenter.c)
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
         // Over a block with more words than room, the wheel moves through
         // its words (caret\caret.c). Anywhere else, through the slides.
         //
         // Over the block in this window only: the presenter's window hands
         // its wheel on too (PRES_Input), and where its mouse is says
         // nothing of what is under it here - asked of this window's mouse,
         // a turn over the presenter's notes would scroll whatever block of
         // the slide happens to be at the same place in this one. The
         // event's own place, which is in the window it came to.
         B32 is_here = (event->window.u64[0] == window.u64[0]);
         if (!(is_here && CARET_PresentWheel(app_state, event->pos, event->delta.y / 120.0f))) {
            scroll += event->delta.y;
         }
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
   if (Core_WM_KeyPress(events, window, 0, WM_Key_N)) {
      app_state->show_notes = !app_state->show_notes;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_V)) {
      app_state->toggle_presenter = 1;
   }

   //- bgz: the slide's video, if it has one (slide_video\slide_video.c)
   {
      if (SVID_Keys(app_state)) {
         app_state->video_key_us = now_us;
      }
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

      // A slide clicked in the presenter's window (presenter\presenter.c).
      if (app_state->presenter_goto != 0) {
         app_state->slide_idx = Min(app_state->presenter_goto - 1, last_idx);
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

   app_state->presenter_goto = 0;

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
//
// UI boxes, built in FrameWork's UI build, that take no clicks: the slide is
// a box the size of the window whose custom draw is _FR_DrawSlide, and what
// is over it - the play bar, the slide's number, the notes, the list of
// keys, a notice, the video's strip of controls - are boxes of their own.
// The presentation's clicks and wheel are the whole window's, read with its
// keys (_FR_PresentInput): a click anywhere is the next slide, and the wheel
// adds up a touchpad's small movements until they come to a notch.

// Overlays are sized with the page (view.h), but not below this part of full
// size: in a small window the slides can be read small, a list of keys not.
#define _FR_MIN_OVERLAY_SCALE 0.7f

// The slide, and the one fading out under it: the custom draw of the
// window's box.
internal void
_FR_DrawSlide(UI_Box* box, void* user_data) {
   App_State* app_state = (App_State*)user_data;
   DECK_Deck* deck = app_state->deck;
   Rng2F32    window_rect = box->rect;

   // The slide showing is asked for first: that is what loads its picture,
   // and what releases the pictures of slides far from it.
   DECK_LoadSlide(deck, app_state->slide_idx);

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
      // As far down its blocks' words as it was: not back at their top.
      CARET_BeforeDrawOf(app_state, app_state->fade_from_idx);
      VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, &deck->slides[app_state->fade_from_idx],
                     window_rect, 1.0f - fade_t, 0);
      CARET_AfterDrawOf(app_state, app_state->fade_from_idx);
   }
   if (app_state->slide_idx < deck->slide_count) {
      // With its blocks as far down their words as the wheel has them
      // (caret\caret.c): a block with more words than room has a scroll
      // bar - the view's own, part of the slide, which fades with it - and
      // the wheel over it moves it.
      VIEW_HitList hits = {0};
      CARET_BeforeDraw(app_state);
      VIEW_DrawSlide(app_state->frame_arena, &app_state->fonts, deck->theme, &deck->slides[app_state->slide_idx], window_rect,
                     fade_t, &hits);
      CARET_PresentAfterDraw(app_state, &hits);
   }
}

internal void
FR_BuildWords(FNT_Tag font, F32 px, Vec2F32 top_left, Vec4F32 color, String8 words, char* fmt, ...) {
   Temp    scratch = ScratchBegin(0, 0);
   va_list args;
   va_start(args, fmt);
   String8 key = PushStr8FV(scratch.arena, fmt, args);
   va_end(args);
   // A little wider than the words: the UI cuts words that do not fit their
   // box short, and its measure can be a pixel over the font cache's.
   F32 w = TXT_Width(font, px, words) + px;
   // A line's height: the UI puts the words in the middle of it, down, which
   // is where TXT_Draw puts them from its top - to the pixel at most sizes,
   // a pixel higher at some.
   F32 top = top_left.y;
   F32 h = TXT_LineHeight(font, px);
   UI_SetNextFont(font);
   UI_SetNextFontSize(FR_UIFontSize(px));
   UI_SetNextTextColor(color);
   UI_SetNextTextPadding(0);
   UI_SetNextTextAlignment(UI_TextAlign_Left);
   UI_Box* box = FR_BuildRectBox(UI_BoxFlag_DrawText, R2F32P(top_left.x, top, top_left.x + w, top + h), "%S", key);
   UI_BoxEquipDisplayString(box, words);
   ScratchEnd(scratch);
}

// A panel's background at rect, in the colour of tag.
internal UI_Box*
_FR_BuildPanel(String8 tag, Rng2F32 rect, F32 radius, char* key) {
   UI_SetNextColorTag(tag);
   UI_SetNextCornerRadius00(radius);
   UI_SetNextCornerRadius01(radius);
   UI_SetNextCornerRadius10(radius);
   UI_SetNextCornerRadius11(radius);
   return FR_BuildRectBox(UI_BoxFlag_DrawBackground, rect, "%s", key);
}

// What is drawn over the slide, top first: a notice, the list of keys, the
// notes, the slide's number, the play bar, what to do with no slides, the
// video's strip - then the slide itself.
internal void
_FR_BuildPresentation(App_State* app_state, Vec2F32 window_dim) {
   DECK_Deck* deck = app_state->deck;
   TXT_Fonts* fonts = &app_state->fonts;
   F32        scale = Max(VIEW_ScaleFromDim(window_dim), _FR_MIN_OVERLAY_SCALE);
   Temp       scratch = ScratchBegin(0, 0);

   //- bgz: a notice, along the top
   String8 notice = FR_CurrentNotice(app_state);
   if (notice.size != 0) {
      F32 px = 20.0f * scale;
      F32 pad = 10.0f * scale;
      F32 w = TXT_Width(fonts->regular, px, notice);
      F32 h = TXT_LineHeight(fonts->regular, px);
      F32 x = (window_dim.x - w) * 0.5f;
      F32 y = pad * 2.0f;
      FR_BuildWords(fonts->regular, px, V2F32(x, y), UI_ThemeColor(Str8Lit("notice"), UI_ColorSlot_Text), notice, "###present notice");
      _FR_BuildPanel(Str8Lit("notice"), R2F32P(x - pad * 1.5f, y - pad * 0.5f, x + w + pad * 1.5f, y + h + pad * 0.5f), 0,
                     "###present notice panel");
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
          {Str8Lit("N"), Str8Lit("Notes, and what comes next")},
          {Str8Lit("V"), Str8Lit("Presenter view, in a second window")},
          {Str8Lit("K"), Str8Lit("Play or pause the slide's video")},
          {Str8Lit("J / L"), Str8Lit("Video back or on 5 seconds")},
          {Str8Lit("M"), Str8Lit("Sound off or on")},
          {Str8Lit("H, F1"), Str8Lit("This list")},
          {Str8Lit("F5, Esc"), Str8Lit("Back to editing (a text deck)")},
          {Str8Lit("Esc"), Str8Lit("Leave fullscreen, or quit")},
          {Str8Lit("Ctrl+O"), Str8Lit("Open another deck, or start a new one")},
          {Str8Lit("Drop a folder or file"), Str8Lit("Open it")},
      };

      // Measured at the overlays' size, and then, in a window too small for
      // that - narrower than the list is wide, or shorter than it is tall -
      // measured again smaller, to fit inside it. The overlays' least size
      // is for reading, but a list cut off at both sides, the keys gone off
      // the left and what they do off the right, can't be read at all.
      F32 list_scale = scale;
      F32 px = 0;
      F32 row_h = 0;
      F32 pad = 0;
      F32 column_gap = 0;
      F32 key_w = 0;
      F32 what_w = 0;
      F32 panel_w = 0;
      F32 panel_h = 0;
      for (U64 pass = 0; pass < 2; pass += 1) {
         px = 20.0f * list_scale;
         row_h = TXT_LineHeight(fonts->regular, px) * 1.25f;
         pad = 28.0f * list_scale;
         column_gap = 36.0f * list_scale;

         // As wide as the widest of each column.
         key_w = 0;
         what_w = 0;
         for (U64 i = 0; i < ArrayCount(rows); i += 1) {
            key_w = Max(key_w, TXT_Width(fonts->bold, px, rows[i][0]));
            what_w = Max(what_w, TXT_Width(fonts->regular, px, rows[i][1]));
         }

         panel_w = pad + key_w + column_gap + what_w + pad;
         panel_h = pad + row_h * (F32)ArrayCount(rows) + pad;

         F32 fit = Min(window_dim.x / Max(panel_w, 1.0f), window_dim.y / Max(panel_h, 1.0f));
         if (fit >= 1.0f) {
            break;
         }
         // A little under what fits: the sizes are whole points, and the
         // line heights do not shrink exactly with them.
         list_scale *= fit * 0.95f;
      }
      F32 panel_x = (window_dim.x - panel_w) * 0.5f;
      F32 panel_y = (window_dim.y - panel_h) * 0.5f;

      Vec4F32 key_color = UI_ThemeColor(Str8Lit("keys"), UI_ColorSlot_Text);
      Vec4F32 what_color = UI_ThemeColor(Str8Lit("keys what"), UI_ColorSlot_Text);
      F32     y = panel_y + pad;
      for (U64 i = 0; i < ArrayCount(rows); i += 1) {
         FR_BuildWords(fonts->bold, px, V2F32(panel_x + pad, y), key_color, rows[i][0], "###present key %llu", (unsigned long long)i);
         FR_BuildWords(fonts->regular, px, V2F32(panel_x + pad + key_w + column_gap, y), what_color, rows[i][1], "###present what %llu",
                        (unsigned long long)i);
         y += row_h;
      }
      _FR_BuildPanel(Str8Lit("keys"), R2F32P(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h), 10.0f * scale, "###present keys");
   }

   //- bgz: the notes: the slide's @note lines and the next slide's title, on a
   // strip along the bottom - for whoever presents, on a screen only they see
   if (app_state->show_notes && deck->slide_count != 0) {
      DECK_Slide* slide = &deck->slides[app_state->slide_idx];
      F32         px = 22.0f * scale;
      F32         small_px = 17.0f * scale;
      F32         pad = 18.0f * scale;
      F32         line_h = TXT_LineHeight(fonts->regular, px);
      F32         small_h = TXT_LineHeight(fonts->regular, small_px);
      F32         text_w = Max(window_dim.x - pad * 2.0f, 1.0f);

      // What comes next: the next slide's title, or that this is the last.
      // On the build arena: its box shows it once the build is done.
      String8 next = Str8Lit("Next: the end");
      if (app_state->slide_idx + 1 < deck->slide_count) {
         DECK_Slide* next_slide = &deck->slides[app_state->slide_idx + 1];
         String8     next_name = next_slide->title;
         if (next_name.size == 0) {
            next_name = (next_slide->picture_count != 0) ? Str8SkipLastSlash(next_slide->pictures[0].path)
                                                         : Str8Lit("a slide with no title");
         }
         next = PushStr8F(UI_BuildArena(), "Next: %S", next_name);
      }

      // The notes' lines, wrapped, to know how tall the strip is.
      String8List lines = {0};
      for (String8Node* note = slide->notes.first; note != 0; note = note->next) {
         String8List wrapped = TXT_Wrap(UI_BuildArena(), fonts->regular, px, note->string, text_w);
         for (String8Node* line = wrapped.first; line != 0; line = line->next) {
            Str8ListPush(scratch.arena, &lines, line->string);
         }
      }
      if (lines.node_count == 0) {
         Str8ListPush(scratch.arena, &lines, Str8Lit("No notes on this slide. A line of  @note Say this  adds one."));
      }

      F32 strip_h = pad + line_h * (F32)lines.node_count + pad * 0.5f + small_h + pad;
      F32 strip_y = window_dim.y - strip_h;

      F32     y = strip_y + pad;
      B32     has_notes = (slide->notes.node_count != 0);
      Vec4F32 color = UI_ThemeColor(has_notes ? Str8Lit("notes") : Str8Lit("notes none"), UI_ColorSlot_Text);
      U64     line_idx = 0;
      for (String8Node* line = lines.first; line != 0; line = line->next, line_idx += 1) {
         FR_BuildWords(fonts->regular, px, V2F32(pad, y), color, line->string, "###present note %llu", (unsigned long long)line_idx);
         y += line_h;
      }
      y += pad * 0.5f;
      FR_BuildWords(fonts->regular, small_px, V2F32(pad, y), UI_ThemeColor(Str8Lit("notes next"), UI_ColorSlot_Text), next,
                     "###present next");
      _FR_BuildPanel(Str8Lit("notes"), R2F32P(0, strip_y, window_dim.x, window_dim.y), 0, "###present notes");
   }

   //- bgz: the slide's number, and the name of the file it is from
   if (app_state->show_info && deck->slide_count != 0) {
      DECK_Slide* slide = &deck->slides[app_state->slide_idx];
      String8     name = Str8SkipLastSlash(deck->file.size != 0 ? deck->file
                                                                    : (slide->picture_count != 0 ? slide->pictures[0].path : Str8Lit("")));
      String8     text = PushStr8F(UI_BuildArena(), "%llu / %llu   %S", (unsigned long long)(app_state->slide_idx + 1),
                                   (unsigned long long)deck->slide_count, name);

      F32 px = 18.0f * scale;
      F32 pad = 8.0f * scale;
      F32 w = TXT_Width(fonts->regular, px, text);
      F32 h = TXT_LineHeight(fonts->regular, px);
      F32 x = window_dim.x - w - pad * 3.0f;
      F32 y = window_dim.y - h - pad * 3.0f;

      FR_BuildWords(fonts->regular, px, V2F32(x, y), UI_ThemeColor(Str8Lit("info"), UI_ColorSlot_Text), text, "###present info");
      _FR_BuildPanel(Str8Lit("info"), R2F32P(x - pad, y - pad * 0.5f, x + w + pad, y + h + pad * 0.5f), 0, "###present info panel");
   }

   //- bgz: playing: a thin bar along the bottom that fills as the slide's
   // turn runs out - which is also what shows that it is playing at all
   if (app_state->playing && deck->slide_count > 1) {
      F32 play_seconds = (F32)(NowTimeUS() - app_state->play_from_us) / 1000000.0f;
      F32 play_t = Clamp(0, play_seconds / FR_PLAY_SECONDS, 1);
      F32 bar_h = 3.0f;
      UI_SetNextBackgroundColor(UI_ThemeColor(Str8Lit("present play"), UI_ColorSlot_Background));
      FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(0, window_dim.y - bar_h, window_dim.x * play_t, window_dim.y), "###present play");
   }

   //- bgz: no slides
   if (deck->slide_count == 0) {
      String8 lines[] = {
          Str8Lit("No slides"),
          Str8Lit("Ctrl+O opens a deck or starts a new one"),
      };
      F32     sizes[] = {40.0f * scale, 22.0f * scale};
      Vec4F32 colors[] = {UI_ThemeColor(Str8Lit("present"), UI_ColorSlot_Text), UI_ThemeColor(Str8Lit("present hint"), UI_ColorSlot_Text)};

      F32 y = window_dim.y * 0.5f - TXT_LineHeight(fonts->bold, sizes[0]);
      for (U64 i = 0; i < ArrayCount(lines); i += 1) {
         FNT_Tag font = (i == 0) ? fonts->bold : fonts->regular;
         F32     w = TXT_Width(font, sizes[i], lines[i]);
         FR_BuildWords(font, sizes[i], V2F32((window_dim.x - w) * 0.5f, y), colors[i], lines[i], "###present empty %llu", (unsigned long long)i);
         y += TXT_LineHeight(font, sizes[i]) * 1.2f;
      }
   }

   //- bgz: the slide's video has its strip of controls along the bottom of
   // the slide for a moment after one of its keys, and while it is paused -
   // once it has been started: not in the frame or two before it is. Not
   // clicked: a click is the next slide. Lit where the mouse is.
   VID_Video* video = DECK_VideoFromSlide(deck, app_state->slide_idx);
   if (video != 0 && app_state->video_auto_slide == app_state->slide_idx + 1) {
      U64 strip_us = (U64)(FR_VIDEO_STRIP_SECONDS * 1000000.0f);
      B32 key_was_recent = (app_state->video_key_us != 0 && NowTimeUS() - app_state->video_key_us < strip_us);
      // Paused: all of the slide's videos, not only the first, which may be
      // a short one that has ended while the others play on.
      if (!SVID_SlideIsPlaying(deck, app_state->slide_idx) || key_was_recent) {
         SVID_BuildStripLook(app_state, video, VIEW_PageRectFromRect(R2F32P(0, 0, window_dim.x, window_dim.y)), FR_UIScale(window_dim), 0, 0);
      }
   }

   //- bgz: the slide, under all of it
   UI_Box* slide_box = FR_BuildRectBox(0, R2F32P(0, 0, window_dim.x, window_dim.y), "###present slide");
   UI_BoxEquipCustomDraw(slide_box, _FR_DrawSlide, app_state);

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: the UI layer
//
// The editor project's UI library (src\ui), built once a frame in FrameWork:
// FR_BeginUIBuild, then the boxes, then UI_EndBuild, which lays them out.
// The tree is drawn by FR_DrawUITree, over the window's background. Ported
// from the editor's frame.c (_FR_ProcessEvents, _FR_BeginUIBuild,
// _FR_DrawUITree). The presenter's window is built the same way, with a
// UI_State of its own (PRES_Draw, presenter\presenter.c).
//
// Built with it: the open screen (OPEN_Build, open\open.c) and the whole
// edit screen (EDIT_Build, edit\edit.c): the text (drawn by its box, with the
// editor), the Format panel, list of pictures, status line, and the preview -
// the slide (drawn by its box, with the view), what is over it (the caret,
// the boxes of its pieces and their corners, scroll bars, videos' controls),
// the caption, the small slides and the + - and the presentation
// (_FR_BuildPresentation): its slide, drawn by its box, and what is over it.
//
// The UI's events are what the screens' own input (EDIT_Input, OPEN_Input)
// left: a click that took is eaten there. The hand-drawn parts that take
// clicks and the UI parts do not overlap, so neither gets the other's
// clicks; while the open screen is up, the edit screen's input does not run
// at all, and its UI boxes are built only to be seen.

// This frame's events of window as the UI's. Selects the UI's state first:
// the list is on UI_BuildArena, which is the selected state's, and lasts
// until the next UI_EndBuild. Another window's events are not this one's: a
// press in the presenter's window is at a place in that window.
internal UI_EventList
_FR_UIEventsFromEvents(App_State* app_state, UI_State* ui_state, WM_Window window) {
   UI_SelectState(ui_state);

   UI_EventList ui_events = {0};
   for (WM_Event* event = app_state->events.first; event != 0; event = event->next) {
      if (event->window.u64[0] != window.u64[0] && event->window.u64[0] != 0) {
         continue;
      }
      UI_EventKind kind = UI_EventKind_Null;
      switch (event->kind) {
         default: {
         } break;
         case WM_EventKind_Press: {
            kind = UI_EventKind_Press;
         } break;
         case WM_EventKind_Release: {
            kind = UI_EventKind_Release;
         } break;
         case WM_EventKind_MouseMove: {
            kind = UI_EventKind_MouseMove;
         } break;
         case WM_EventKind_Text: {
            kind = UI_EventKind_Text;
         } break;
         case WM_EventKind_Scroll: {
            // Not a sideways wheel: with Shift held the UI turns one into an
            // up and down one, and a touchpad sends a steady trickle of them
            // (the editor's frame.c).
            if (event->delta.y != 0) {
               kind = UI_EventKind_Scroll;
            }         } break;
      }

      UI_Event ui_event = zero_struct;
      ui_event.kind = kind;
      ui_event.key = event->key;
      ui_event.modifiers = event->modifiers;
      ui_event.string = event->character ? Str8From32(UI_BuildArena(), Str32(&event->character, 1)) : Str8Zero();
      ui_event.pos = event->pos;
      ui_event.timestamp_us = event->timestamp_us;
      ui_event.delta_2f32 = event->delta;
      UI_EventListPush(UI_BuildArena(), &ui_events, &ui_event);
   }

   return ui_events;
}

internal UI_Box*
_FR_BuildRectBoxV(UI_BoxFlags flags, Rng2F32 rect, char* fmt, va_list args) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 key = PushStr8FV(scratch.arena, fmt, args);
   // A box's fixed place is from its parent's top left: the parent's rect is
   // this frame's if it was built here too, and the root's is the window's.
   Vec2F32 origin = UI_TopParent()->rect.p0;
   UI_SetNextRect(R2F32P(rect.min.x - origin.x, rect.min.y - origin.y, rect.max.x - origin.x, rect.max.y - origin.y));
   UI_Box* box = UI_BuildBoxFromString(flags, key);
   // The layout puts it at rect too, from its fixed place, but only at
   // UI_EndBuild: its signal would be asked of where it was last frame - a
   // frame behind a piece of the slide that has just moved.
   box->rect = rect;
   ScratchEnd(scratch);
   return box;
}

internal UI_Box*
FR_BuildRectBox(UI_BoxFlags flags, Rng2F32 rect, char* fmt, ...) {
   va_list args;
   va_start(args, fmt);
   UI_Box* box = _FR_BuildRectBoxV(flags, rect, fmt, args);
   va_end(args);
   return box;
}

typedef struct _FR_Edge _FR_Edge;
struct _FR_Edge {
   Rng2F32 rect;
   F32     thick;
};

// Drawn at the rect it was given, not the box's: the layout puts a box on
// whole pixels, and an outline around a piece of the slide is where the
// piece is, which is not.
internal void
_FR_DrawEdge(UI_Box* box, void* user_data) {
   _FR_Edge* edge = (_FR_Edge*)user_data;
   Rng2F32   r = edge->rect;
   F32       t = edge->thick;
   Vec4F32   c = box->border_color;
   Core_D_Rect(R2F32P(r.min.x, r.min.y, r.max.x, r.min.y + t), R2F32P(0, 0, 1, 1), c);
   Core_D_Rect(R2F32P(r.min.x, r.max.y - t, r.max.x, r.max.y), R2F32P(0, 0, 1, 1), c);
   Core_D_Rect(R2F32P(r.min.x, r.min.y, r.min.x + t, r.max.y), R2F32P(0, 0, 1, 1), c);
   Core_D_Rect(R2F32P(r.max.x - t, r.min.y, r.max.x, r.max.y), R2F32P(0, 0, 1, 1), c);
}

internal UI_Box*
FR_BuildEdge(Rng2F32 rect, F32 thick, Vec4F32 color, char* fmt, ...) {
   va_list args;
   va_start(args, fmt);
   UI_Box* box = _FR_BuildRectBoxV(0, rect, fmt, args);
   va_end(args);
   // On the build arena: it is drawn after the build, and gone with it.
   _FR_Edge* edge = PushArray(UI_BuildArena(), _FR_Edge, 1);
   edge->rect = rect;
   edge->thick = thick;
   box->border_color = color;
   UI_BoxEquipCustomDraw(box, _FR_DrawEdge, edge);
   return box;
}

internal F32
FR_UIScale(Vec2F32 window_dim) {
   return Clamp(1.0f, window_dim.y / 900.0f, 2.0f);
}

internal F32
FR_UIFontSize(F32 px) {
   // As text.c's _TXT_PointsFromPx: the UI hands its size to the same font
   // cache, which takes points, 72 to an inch where pixels are 96.
   return Max((F32)(S32)(px * 0.75f), 1.0f);
}

internal void
FR_BeginUIBuild(App_State* app_state, UI_State* ui_state, WM_Window window, U64* last_frame_us) {
   Vec2F32      window_dim = Dim2F32(Core_WM_ClientRectFromWindow(window));
   UI_EventList ui_events = _FR_UIEventsFromEvents(app_state, ui_state, window);
   // The colours, by tag. A box asks under the tags pushed with UI_ColorTag,
   // and the pattern with the most of them wins; a pattern counts only if
   // the box has all of its tags. The first part is the editor's palette
   // (ui_colors.h), for what the basic widgets ask for; the rest are the
   // slideshow's own colours, the ones its hand-drawn screens use.
   static UI_ThemePattern theme_patterns[] = {
       //  tags          slot                      colour
       {"", UI_ColorSlot_Text, {{0.84f, 0.86f, 0.90f, 1.00f}}},
       {"", UI_ColorSlot_Background, UI_BG_1},
       {"", UI_ColorSlot_Border, UI_TRANSPARENT},
       {"", UI_ColorSlot_Selection, UI_ACCENT_BG},
       {"", UI_ColorSlot_Cursor, UI_FG},
       {"selected", UI_ColorSlot_Background, UI_ACCENT_BG},
       {"weak", UI_ColorSlot_Text, UI_FG_WEAK},
       {"soft", UI_ColorSlot_Text, UI_FG_SOFT},
       {"error", UI_ColorSlot_Text, UI_ERROR},
       {"warning", UI_ColorSlot_Text, UI_WARNING},
       {"highlight", UI_ColorSlot_Text, UI_HIGHLIGHT},
       {"button", UI_ColorSlot_Background, UI_BG_4},
       {"button", UI_ColorSlot_Text, UI_FG},
       {"popup", UI_ColorSlot_Background, UI_BG_3},
       {"popup", UI_ColorSlot_Border, UI_LINE},
       {"menu", UI_ColorSlot_Background, UI_TRANSPARENT},
       {"scroll track", UI_ColorSlot_Background, UI_TRANSPARENT},
       {"scroll thumb", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.18f}}},

       //- bgz: the slideshow's panels: the open screen (open\open.c), the
       // edit screen's Format panel and status line (edit\edit.c)
       {"backdrop", UI_ColorSlot_Background, {{0.00f, 0.00f, 0.00f, 0.60f}}}, // over what is behind the open screen
       {"panel", UI_ColorSlot_Background, {{0.11f, 0.115f, 0.135f, 1.00f}}},
       {"panel_bar", UI_ColorSlot_Background, {{0.16f, 0.17f, 0.20f, 1.00f}}}, // title, foot, status line
       {"header", UI_ColorSlot_Background, {{0.14f, 0.15f, 0.17f, 1.00f}}},    // the Format panel's
       {"header hot", UI_ColorSlot_Background, {{0.17f, 0.18f, 0.21f, 1.00f}}},
       {"row", UI_ColorSlot_Background, UI_TRANSPARENT},                       // a list's row: the panel shows through
       {"row hot", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.07f}}},   // under the mouse
       {"dim", UI_ColorSlot_Text, {{0.84f, 0.86f, 0.90f, 0.55f}}},             // what a row is, hints, keys
       {"notice", UI_ColorSlot_Text, {{1.00f, 0.85f, 0.50f, 1.00f}}},          // FR_Notice's
       {"deck", UI_ColorSlot_Text, {{1.00f, 0.80f, 0.40f, 1.00f}}},            // a deck to open
       {"folder", UI_ColorSlot_Text, {{0.55f, 0.75f, 1.00f, 1.00f}}},
       {"action", UI_ColorSlot_Text, {{0.60f, 0.85f, 0.60f, 1.00f}}},          // Browse, New deck, Present
       {"syntax", UI_ColorSlot_Text, {{1.00f, 0.80f, 0.40f, 1.00f}}},          // a Format line, as it is written
       {"picture", UI_ColorSlot_Text, {{0.60f, 0.85f, 0.60f, 1.00f}}},         // a picture's name

       //- bgz: the edit screen's preview (edit\edit.c, caret\caret.c,
       // place\place.c, slide_video\slide_video.c). What is of the slide itself - the
       // caret, a piece's outline, the wash under the mouse, a block's scroll
       // bar - is in the slide's theme's colours instead, to show on it.
       {"focus", UI_ColorSlot_Border, {{0.26f, 0.60f, 1.00f, 1.00f}}},       // the box of the piece the cursor is in
       {"focus", UI_ColorSlot_Background, {{0.26f, 0.60f, 1.00f, 0.35f}}},   // what is selected, on the slide
       {"guide", UI_ColorSlot_Background, {{1.00f, 0.25f, 0.65f, 0.90f}}},   // what Shift lines a drag up with: no theme has it
       {"guide", UI_ColorSlot_Border, {{1.00f, 0.25f, 0.65f, 0.54f}}},       // and the box the line is from
       {"handle", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 1.00f}}},  // inside a corner's handle
       {"strip", UI_ColorSlot_Background, {{0.00f, 0.00f, 0.00f, 0.62f}}},   // a video's controls: dark over any video
       {"strip", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.95f}}},
       {"strip hot", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.18f}}},
       {"strip muted", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.50f}}},
       {"strip track", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.25f}}},
       {"strip played", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.95f}}},
       {"preview", UI_ColorSlot_Background, {{0.125f, 0.13f, 0.15f, 1.00f}}}, // around the slide
       {"divider", UI_ColorSlot_Background, {{0.00f, 0.00f, 0.00f, 0.50f}}},  // where the text ends and the preview begins
       {"thumb", UI_ColorSlot_Border, {{1.00f, 0.80f, 0.40f, 1.00f}}},        // the frame of the small slide showing
       {"thumb hot", UI_ColorSlot_Border, {{1.00f, 1.00f, 1.00f, 0.35f}}},    // and of one under the mouse
       {"add", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.07f}}},      // the + that adds a slide
       {"add", UI_ColorSlot_Text, {{0.84f, 0.86f, 0.90f, 0.55f}}},
       {"add hot", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.16f}}},
       {"add hot", UI_ColorSlot_Text, {{0.84f, 0.86f, 0.90f, 1.00f}}},

       //- bgz: the presentation's overlays (_FR_BuildPresentation): white on
       // dark panels, since they may be over a bright picture
       {"present", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.90f}}},        // "No slides"
       {"present hint", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.55f}}},
       {"present play", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.50f}}}, // the bar that fills as a slide's turn runs out
       {"info", UI_ColorSlot_Background, {{0.00f, 0.00f, 0.00f, 0.55f}}},     // the slide's number
       {"info", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.85f}}},
       {"notes", UI_ColorSlot_Background, {{0.02f, 0.02f, 0.03f, 0.86f}}},
       {"notes", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.95f}}},
       {"notes none", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.50f}}},     // a slide with no notes says how to add one
       {"notes next", UI_ColorSlot_Text, {{1.00f, 0.85f, 0.50f, 0.90f}}},     // what comes next
       {"keys", UI_ColorSlot_Background, {{0.02f, 0.02f, 0.03f, 0.88f}}},     // the list of keys
       {"keys", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.95f}}},
       {"keys what", UI_ColorSlot_Text, {{1.00f, 1.00f, 1.00f, 0.70f}}},
       {"notice", UI_ColorSlot_Background, {{0.00f, 0.00f, 0.00f, 0.75f}}},

       //- bgz: the presenter's window (presenter\presenter.c), all of it under
       // this tag
       {"presenter", UI_ColorSlot_Background, {{0.09f, 0.095f, 0.11f, 1.00f}}},
       {"presenter", UI_ColorSlot_Text, {{0.92f, 0.93f, 0.95f, 1.00f}}},
       {"presenter dim", UI_ColorSlot_Text, {{0.92f, 0.93f, 0.95f, 0.55f}}},
       {"presenter clock", UI_ColorSlot_Text, {{1.00f, 0.85f, 0.50f, 1.00f}}},
       {"presenter button", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.10f}}},
       {"presenter button hot", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.22f}}},
       {"presenter button off", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.04f}}},
       {"presenter button off", UI_ColorSlot_Text, {{0.92f, 0.93f, 0.95f, 0.35f}}},
       {"presenter end", UI_ColorSlot_Background, {{1.00f, 1.00f, 1.00f, 0.04f}}}, // where the next slide would be, after the last
       {"presenter hot", UI_ColorSlot_Border, {{1.00f, 1.00f, 1.00f, 0.45f}}},     // the next slide's frame under the mouse
       {"presenter now", UI_ColorSlot_Border, {{1.00f, 0.85f, 0.50f, 1.00f}}},     // the bar under the small slide showing
       {"presenter thumb hot", UI_ColorSlot_Border, {{1.00f, 1.00f, 1.00f, 0.60f}}}, // and under one under the mouse
   };
   static UI_Theme theme = {theme_patterns, ArrayCount(theme_patterns)};

   // The UI's icons - scroll arrows, carets, check boxes - are single
   // characters of raddebugger's icon font, the ones raddbg uses.
   UI_IconInfo icon_info = {0};
   icon_info.icon_font = app_state->ui_icon_font;
   icon_info.icon_kind_text_map[UI_IconKind_RightArrow] = Str8Lit("8");
   icon_info.icon_kind_text_map[UI_IconKind_DownArrow] = Str8Lit("0");
   icon_info.icon_kind_text_map[UI_IconKind_LeftArrow] = Str8Lit("7");
   icon_info.icon_kind_text_map[UI_IconKind_UpArrow] = Str8Lit("9");
   icon_info.icon_kind_text_map[UI_IconKind_RightCaret] = Str8Lit(">");
   icon_info.icon_kind_text_map[UI_IconKind_DownCaret] = Str8Lit("v");
   icon_info.icon_kind_text_map[UI_IconKind_LeftCaret] = Str8Lit("<");
   icon_info.icon_kind_text_map[UI_IconKind_UpCaret] = Str8Lit("^");
   icon_info.icon_kind_text_map[UI_IconKind_CheckHollow] = Str8Lit("!");
   icon_info.icon_kind_text_map[UI_IconKind_CheckFilled] = Str8Lit("1");
   icon_info.icon_kind_text_map[UI_IconKind_WindowMinimize] = Str8Lit(")");
   icon_info.icon_kind_text_map[UI_IconKind_WindowMaximize] = Str8Lit("n");
   icon_info.icon_kind_text_map[UI_IconKind_WindowRestore] = Str8Lit("(");
   icon_info.icon_kind_text_map[UI_IconKind_WindowClose] = Str8Lit("x");
   icon_info.icon_kind_text_map[UI_IconKind_FolderClosed] = Str8Lit("C");
   icon_info.icon_kind_text_map[UI_IconKind_FolderOpen] = Str8Lit("B");
   icon_info.icon_kind_text_map[UI_IconKind_File] = Str8Lit("f");

   // The time since the last build, for hover fades and scroll glides. A
   // long gap - the first frame, or a wait inside a drag of the window -
   // counts as one frame at 60 a second, so nothing jumps.
   U64 now_us = NowTimeUS();
   F32 dt = 1.0f / 60.0f;
   if (*last_frame_us != 0 && now_us - *last_frame_us < 250000) {
      dt = (F32)(now_us - *last_frame_us) / 1000000.0f;
   }
   *last_frame_us = now_us;

   // Hover and press come in fast, as raddebugger's do: the scroll bar's thumb
   // widens under the mouse by them (_UI_DrawScrollThumb). The rest is not
   // used here.
   UI_AnimationInfo animation_info = {0};
   animation_info.hot_animation_rate = 1.0f - PowF32(2.0f, -60.0f * dt);
   animation_info.active_animation_rate = animation_info.hot_animation_rate;
   // The events live on the build arena until UI_EndBuild: a copy of the list
   // head there too, which UI_BeginBuild keeps.
   UI_EventList* events = PushArray(UI_BuildArena(), UI_EventList, 1);
   *events = ui_events;
   UI_BeginBuild(window, events, &icon_info, &theme, &animation_info, dt, dt);

   // The slideshow's regular font, at the hand-drawn screens' text size
   // (15 pixels at FR_UIScale 1, as the edit screen's), and drawn as they
   // draw it (text.c's _TXT_RASTER_FLAGS), so the two look alike side by
   // side. Panels push their own sizes and fonts where they differ.
   FNT_Tag font = app_state->fonts.regular;
   UI_PushFont(font);
   UI_PushFontSize(FR_UIFontSize((F32)(S32)(15.0f * FR_UIScale(window_dim))));
   UI_PushTextRasterFlags(_TXT_RASTER_FLAGS);
   UI_SetLabelFont(font, _TXT_RASTER_FLAGS);

   // A ctx menu builds with the stack's bottom value, which is whatever was
   // last pushed on the empty stack. Pinned here, or a UI_PrefWidth(UI_Pct(..))
   // anywhere earlier in the frame makes the menu's root as wide as the window
   // (the editor's frame.c).
   UI_PushPrefWidth(UI_TopPrefWidth());
   UI_PushPrefHeight(UI_TopPrefHeight());
}

internal void
FR_DrawUITree(UI_State* ui_state) {
   for (UI_Box* box = UI_RootFromState(ui_state); !UI_BoxIsNil(box);) {
      UI_BoxRec rec = UI_BoxRecDfPost(box, &ui_nil_box);

      // Under the box, down and right of it, soft. raddebugger's sizes. Not
      // for an empty box: the UI's own ctx menu root always has the flag.
      if ((box->flags & UI_BoxFlag_DrawDropShadow) && box->rect.x1 > box->rect.x0 && box->rect.y1 > box->rect.y0) {
         Vec4F32 shadow_color = {{0, 0, 0, 0.40f}};

         // A box that is fading takes its shadow with it.
         if (box->flags & UI_BoxFlag_DrawBackground) {
            shadow_color.w *= Min(1.0f, box->background_color.w);
         }
         Rng2F32       shadow_rect = R2F32P(box->rect.x0 - 6, box->rect.y0 - 2, box->rect.x1 + 6, box->rect.y1 + 10);
         R_Rect2DInst* inst = Core_D_Rect(shadow_rect, R2F32(V2F32(0, 0), V2F32(1, 1)), shadow_color);
         inst->edge_softness = 12.0f;
         for EachEnumVal(Corner, corner) {
            inst->corner_radii[corner] = box->corner_radii[corner] + 6.0f;
         }
      }

      if (box->flags & UI_BoxFlag_DrawBackground) {
         R_Rect2DInst* inst = Core_D_Rect(box->rect, R2F32(V2F32(0, 0), V2F32(1, 1)), box->background_color);
         MemoryCopyArray(inst->corner_radii, box->corner_radii);
      }

      // A pixel outside the rect, so a rounded corner's is that much wider.
      if (box->flags & UI_BoxFlag_DrawBorder) {
         R_Rect2DInst* inst = Core_D_Rect(Pad2F32(box->rect, 1.0f), R2F32(V2F32(0, 0), V2F32(1, 1)), box->border_color);
         inst->border_thickness = 1.0f;
         for EachEnumVal(Corner, corner) {
            inst->corner_radii[corner] = box->corner_radii[corner] > 0 ? box->corner_radii[corner] + 1.0f : 0;
         }
      }

      if (box->flags & UI_BoxFlag_DrawText) {
         Vec2F32 text_pos = UI_BoxTextPosition(box);
         F32     max_x = box->rect.x1 - text_pos.x;
         FNT_Run ellipses_run = {0};
         Core_D_TruncatedFancyRunList(text_pos, &box->display_fruns, max_x, ellipses_run);
      }

      if (box->custom_draw != 0) {
         box->custom_draw(box, box->custom_draw_user_data);
      }

      // A UI_BoxFlag_Clip box keeps everything under it inside its rect, and
      // inside any clip above it. Pushed here, on the way down to its
      // children, and popped below once the walk has left them.
      if (box->flags & UI_BoxFlag_Clip) {
         Rng2F32 top_clip = Core_D_TopClip();
         Rng2F32 new_clip = box->rect;

         // A zero clip is "none", not a rect at the origin to intersect with.
         if (top_clip.x1 != 0 || top_clip.y1 != 0) {
            new_clip = Intersect2F32(new_clip, top_clip);
         }
         Core_D_PushClip(new_clip);
      }

      // Pop the clip of every box this step finishes: this one, unless it has
      // children still to draw (the last of them pops it), and each ancestor
      // the walk climbs out of - rec.pop_count of them.
      {
         S32 pop_idx = 0;
         for (UI_Box* b = box; !UI_BoxIsNil(b) && pop_idx <= rec.pop_count; b = b->parent) {
            pop_idx += 1;
            if (b == box && rec.push_count != 0) {
               continue;
            }
            if (b->flags & UI_BoxFlag_Clip) {
               Core_D_PopClip();
            }
         }
      }

      box = rec.next;
   }
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
   // What is logged this frame is gathered, and written at the frame's end.
   LogSelect(app_state->log);
   LogScopeBegin();

   Vec2F32 window_dim = Dim2F32(Core_WM_ClientRectFromWindow(app_state->window));

   // Before input: the edit screen measures text to know what a click is on.
   _FR_BeginText(app_state, window_dim);

   // Before the UI build too: the edit screen lays its slide out there, into
   // a bucket of its own, to know where its pieces are this frame.
   Core_D_BeginFrame();

   // The presenter's window is for presenting: it goes when the text comes
   // back. Its keys are made the main window's before anything reads them.
   PRES_Input(app_state);
   if (app_state->presenter_open && app_state->is_editing) {
      app_state->toggle_presenter = 1;
   }

   // The open screen is over either screen, and while it is up the input is
   // all its own.
   if (!app_state->show_open && Core_WM_KeyPress(&app_state->events, app_state->window, WM_Modifier_Ctrl, WM_Key_O)) {
      OPEN_Show(app_state);
   }

   // Input can change which screen is up, so that is asked again for drawing.
   if (app_state->show_open) {
      OPEN_Input(app_state, window_dim);
   } else if (app_state->is_editing) {
      EDIT_Input(app_state, window_dim);
   } else {
      _FR_PresentInput(app_state);
   }

   // After the input, which may have changed the slide or the screen.
   COPY_Update(app_state);
   SVID_Update(app_state);

   // The UI tree, built after the screens' own input, against the screen
   // that input left up. UI_EndBuild lays it out; it is drawn below, over
   // what the screens draw by hand.
   //
   // What is built first is drawn on top and asked for clicks first, so the
   // open screen comes first: its backdrop takes every click and turn of the
   // wheel that misses its panel. Under it, the edit screen's panels are
   // built only to be seen.
   {
      FR_BeginUIBuild(app_state, app_state->ui_state, app_state->window, &app_state->ui_last_frame_us);

      // What the panels show is text as it is - paths, names, "## Heading" -
      // never a box's key: the UI would show only what is before a ##.
      UI_FlagsAdd(UI_BoxFlag_DisableIDString) {
         if (app_state->show_open) {
            OPEN_Build(app_state, window_dim);
         }
         // Asked after: a deck opened from the open screen is up already.
         if (app_state->is_editing) {
            EDIT_Build(app_state, window_dim, !app_state->show_open);
         } else {
            _FR_BuildPresentation(app_state, window_dim);
         }
      }

      UI_EndBuild();
   }

   D_Bucket* bucket = Core_D_BucketMake();
   Core_D_BucketScope(bucket) {
      // The background, which is all there is behind a slide that does not
      // fill the window.
      Core_D_Rect(R2F32P(0, 0, window_dim.x, window_dim.y), R2F32P(0, 0, 1, 1),
                  VIEW_ColorsFromTheme(app_state->deck->theme).background);

      // Over it, all there is: the open screen, and the edit screen or the
      // presentation, the slide among it.
      FR_DrawUITree(app_state->ui_state);
   }
   Core_D_SubmitBucket(app_state->window, app_state->window_eqp, bucket);

   FR_WriteLogs(app_state, LogScopeEnd(app_state->frame_arena));
}

////////////////////////////////
//~ bgz: the open screen
//
// How a deck is picked without leaving the program, or giving it anything
// when it is started: a panel over whichever screen is up, with a line for
// each thing that can be opened.
//
//    +----------------------------------------------+
//    |  Open a deck                                 |
//    |                                              |
//    |  Opened lately                               |
//    |    C:\talks\intro\slides.md                  |
//    |  C:\talks                                    |
//    |    ..                    the folder above    |
//    |    Browse...             Windows' window     |
//    |    New deck here         makes slides.md     |
//    |    Present the pictures  12 in this folder   |
//    |    intro\                                    |
//    |    notes.md                                  |
//    |                                              |
//    |  Click to open. Esc closes.                  |
//    +----------------------------------------------+
//
// The lines are the decks opened lately, then one folder: the folder above
// it, Windows' own open-file window (bgz's ShPickFile), what can be made of
// the folder itself - a new text deck in it, or its pictures as a deck - and
// the folders and text decks in it. Clicking a
// folder goes into it. Clicking anything else opens it and closes the panel.
//
//    open the panel   Ctrl+O, on either screen
//    close it         Esc, or Ctrl+O again
//    folder above     Backspace, or the .. line
//    through the list the mouse wheel

////////////////////////////////
//~ bgz: the rows

// How often the folder is read again while the panel is up.
#define _OPEN_SCAN_SECONDS 1.0f

// How many of the decks opened lately get a line.
#define _OPEN_RECENTS_SHOWN 5

internal void
_OPEN_PushRow(Arena* arena, OPEN_Row* rows, U64* row_count, U64 row_cap, OPEN_RowKind kind, String8 label, String8 path) {
   if (*row_count < row_cap) {
      OPEN_Row* row = &rows[*row_count];
      row->kind = kind;
      row->label = PushStr8Copy(arena, label);
      row->path = PushStr8Copy(arena, path);
      *row_count += 1;
   }
}

// The name a new deck in folder gets: slides.md, or with that taken
// slides2.md, and so on.
internal String8
_OPEN_NewDeckName(Arena* arena, String8 folder) {
   String8 result = Str8Lit("slides.md");
   Temp    scratch = ScratchBegin(&arena, 1);
   for (U64 n = 2; n < 1000; n += 1) {
      if (!FilePathExists(PushStr8F(scratch.arena, "%S\\%S", folder, result))) {
         break;
      }
      result = PushStr8F(arena, "slides%llu.md", (unsigned long long)n);
   }
   ScratchEnd(scratch);
   return result;
}

// folder\name - but a drive's own folder, C:\ , has its \ already.
internal String8
_OPEN_PathInFolder(Arena* arena, String8 folder, String8 name) {
   B32 has_slash = (folder.size != 0 && folder.str[folder.size - 1] == '\\');
   return PushStr8F(arena, "%S%s%S", folder, has_slash ? "" : "\\", name);
}

// The folder a folder is in. A drive's own folder is in the list of drives:
// the empty string.
internal String8
_OPEN_FolderAbove(String8 folder) {
   String8 result = {0};
   if (folder.size > 3) {
      result = Str8ChopLastSlash(folder);
      // C:\name is in C:\ , not in C: - which is wherever on C the program
      // happens to be.
      if (result.size == 2 && result.str[1] == ':') {
         result = Str8Prefix(folder, 3);
      }
   }
   return result;
}

// Makes the rows again, from the decks opened lately and the folder as it
// is now.
internal void
_OPEN_Scan(App_State* app_state) {
   Temp scratch = ScratchBegin(0, 0);

   // The folder's name outlasts the arena it is in being cleared.
   String8 folder_before = PushStr8Copy(scratch.arena, app_state->open_folder);
   if (app_state->open_arena == 0) {
      app_state->open_arena = ArenaAlloc();
   }
   Arena* arena = app_state->open_arena;
   ArenaClear(arena);
   String8 folder = PushStr8Copy(arena, folder_before);
   app_state->open_folder = folder;

   //- bgz: what is in the folder
   String8List folder_names = {0};
   String8List deck_names = {0};
   U64         picture_count = 0;
   {
      FileIter* it = FileIterBegin(scratch.arena, folder, FileIterFlag_SkipHiddenFiles);
      for (FileInfo info = {0}; FileIterNext(scratch.arena, it, &info);) {
         if (info.props.flags & FilePropertyFlag_IsFolder) {
            Str8ListPush(scratch.arena, &folder_names, info.name);
         } else if (_DECK_NameIsTextDeck(info.name)) {
            Str8ListPush(scratch.arena, &deck_names, info.name);
         } else if (IMG_NameIsImage(info.name)) {
            picture_count += 1;
         }
      }
      FileIterEnd(it);
   }
   String8Array folders = Str8ArrayFromList(scratch.arena, &folder_names);
   String8Array decks = Str8ArrayFromList(scratch.arena, &deck_names);
   _DECK_SortNames(folders);
   _DECK_SortNames(decks);

   //- bgz: the rows
   U64       recent_count = Min(app_state->recents.count, _OPEN_RECENTS_SHOWN);
   U64       row_cap = recent_count + folders.count + decks.count + 9;
   OPEN_Row* rows = PushArray(arena, OPEN_Row, row_cap);
   U64       row_count = 0;

   if (recent_count != 0) {
      _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Label, Str8Lit("Opened lately"), Str8Zero());
      for (U64 i = 0; i < recent_count; i += 1) {
         _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Recent, app_state->recents.v[i],
                       app_state->recents.v[i]);
      }
   }

   if (folder.size == 0) {
      // The list of drives: each is a folder, and that is all there is.
      _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Label, Str8Lit("Drives"), Str8Zero());
      for (U64 i = 0; i < folders.count; i += 1) {
         String8 drive = PushStr8F(scratch.arena, "%S\\", folders.v[i]);
         _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Folder, drive, drive);
      }
   } else {
      _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Label, folder, Str8Zero());
      _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Up, Str8Lit(".."), _OPEN_FolderAbove(folder));

      _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Browse, Str8Lit("Browse..."), folder);
      String8 new_name = _OPEN_NewDeckName(scratch.arena, folder);
      _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_New, Str8Lit("New deck here"),
                    _OPEN_PathInFolder(scratch.arena, folder, new_name));
      if (picture_count != 0) {
         String8 label = PushStr8F(scratch.arena, "Present the %llu picture%s in this folder",
                                   (unsigned long long)picture_count, picture_count == 1 ? "" : "s");
         _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Present, label, folder);
      }

      for (U64 i = 0; i < folders.count; i += 1) {
         _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Folder, PushStr8F(scratch.arena, "%S\\", folders.v[i]),
                       _OPEN_PathInFolder(scratch.arena, folder, folders.v[i]));
      }
      for (U64 i = 0; i < decks.count; i += 1) {
         _OPEN_PushRow(arena, rows, &row_count, row_cap, OPEN_RowKind_Deck, decks.v[i],
                       _OPEN_PathInFolder(scratch.arena, folder, decks.v[i]));
      }
   }

   app_state->open_rows = rows;
   app_state->open_row_count = row_count;
   app_state->open_scanned_us = NowTimeUS();

   ScratchEnd(scratch);
}

internal void
_OPEN_GoToFolder(App_State* app_state, String8 folder) {
   // folder may be in the arena the scan clears: the scan copies it first.
   app_state->open_folder = folder;
   app_state->open_first_row = 0;
   _OPEN_Scan(app_state);
}

internal void
OPEN_Show(App_State* app_state) {
   Temp scratch = ScratchBegin(0, 0);

   // It starts in the folder of the deck that is open, where the next deck
   // wanted is most likely to be near. With no deck, where the program was
   // started.
   String8 folder = Str8Lit(".");
   if (app_state->deck != 0 && app_state->deck->folder.size != 0) {
      folder = app_state->deck->folder;
   }
   folder = FR_WholePath(scratch.arena, folder);

   app_state->show_open = 1;
   _OPEN_GoToFolder(app_state, folder);

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: layout

typedef struct _OPEN_Layout _OPEN_Layout;
struct _OPEN_Layout {
   F32     ui;    // how many pixels a unit is, as on the edit screen
   F32     ui_px; // the size of the text
   Rng2F32 panel;
   Rng2F32 title;
   Rng2F32 rows;  // where the rows are, all that fit
   Rng2F32 foot;
   F32     row_h;
   U64     rows_fit;
};

internal _OPEN_Layout
_OPEN_LayoutFromWindowDim(Vec2F32 window_dim) {
   _OPEN_Layout result = {0};

   F32 ui = Clamp(1.0f, window_dim.y / 900.0f, 2.0f);
   result.ui = ui;
   result.ui_px = (F32)(S32)(16.0f * ui);
   result.row_h = (F32)(S32)(30.0f * ui);

   F32 margin = 40.0f * ui;
   F32 w = Clamp(Min(320.0f * ui, window_dim.x), window_dim.x - margin * 2.0f, 900.0f * ui);
   F32 h = Max(window_dim.y - margin * 2.0f, Min(200.0f * ui, window_dim.y));
   F32 x = (F32)(S32)((window_dim.x - w) * 0.5f);
   F32 y = (F32)(S32)((window_dim.y - h) * 0.5f);
   result.panel = R2F32P(x, y, x + w, y + h);

   F32 title_h = (F32)(S32)(52.0f * ui);
   F32 foot_h = (F32)(S32)(40.0f * ui);
   result.title = R2F32P(x, y, x + w, y + title_h);
   result.foot = R2F32P(x, y + h - foot_h, x + w, y + h);
   result.rows = R2F32P(x, result.title.max.y, x + w, Max(result.foot.min.y, result.title.max.y));
   result.rows_fit = (U64)(Dim2F32(result.rows).y / result.row_h);

   return result;
}

internal Rng2F32
_OPEN_RowRect(_OPEN_Layout* layout, U64 shown_idx) {
   F32 y = layout->rows.min.y + layout->row_h * (F32)shown_idx;
   return R2F32P(layout->rows.min.x, y, layout->rows.max.x, y + layout->row_h);
}

internal U64
_OPEN_MaxFirstRow(App_State* app_state, _OPEN_Layout* layout) {
   return (app_state->open_row_count > layout->rows_fit) ? app_state->open_row_count - layout->rows_fit : 0;
}

////////////////////////////////
//~ bgz: input

// What a click on a row does.
internal void
_OPEN_PickRow(App_State* app_state, OPEN_Row* row) {
   Temp scratch = ScratchBegin(0, 0);

   // The row is in the arena a scan clears, and opening can lead to one.
   String8 path = PushStr8Copy(scratch.arena, row->path);

   switch (row->kind) {
      case OPEN_RowKind_Label: {
      } break;

      case OPEN_RowKind_Up:
      case OPEN_RowKind_Folder: {
         _OPEN_GoToFolder(app_state, path);
      } break;

      case OPEN_RowKind_Browse: {
         // bgz's shell: the system's window, which does not return until it
         // is answered. Empty: every file is listed - a deck is .md or .txt,
         // and a picture opens its folder.
         String8 picked = ShPickFile(scratch.arena, Str8Lit("Open a deck"), path, Str8Zero());
         if (picked.size != 0 && FR_OpenPath(app_state, picked)) {
            app_state->show_open = 0;
         }
      } break;

      case OPEN_RowKind_New: {
         // Asked first, so that a deck that cannot be opened now - unsaved
         // changes in the one that is - does not leave a file behind.
         B32 can_open = !(app_state->editor != 0 && ED_HasUnsavedChanges(app_state->editor));
         if (can_open && !WriteDataToFilePath(path, Str8Lit("# Title\n"))) {
            FR_Notice(app_state, Str8Lit("Could not make a file in this folder"));
         } else if (FR_OpenPath(app_state, path)) {
            app_state->show_open = 0;
            // Its one line, the title, is ready to be typed over.
            if (app_state->editor != 0 && app_state->editor->size >= 7) {
               ED_SetCursor(app_state->editor, 7);
               app_state->editor->mark = 2;
            }
         }
      } break;

      case OPEN_RowKind_Recent:
      case OPEN_RowKind_Present:
      case OPEN_RowKind_Deck: {
         if (FR_OpenPath(app_state, path)) {
            app_state->show_open = 0;
         }
      } break;
   }

   ScratchEnd(scratch);
}

internal void
OPEN_Input(App_State* app_state, Vec2F32 window_dim) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;
   _OPEN_Layout  layout = _OPEN_LayoutFromWindowDim(window_dim);

   U64 scan_us = (U64)(_OPEN_SCAN_SECONDS * 1000000.0f);
   if (app_state->open_scanned_us == 0 || NowTimeUS() - app_state->open_scanned_us >= scan_us) {
      _OPEN_Scan(app_state);
   }

   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc) || Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_O)) {
      app_state->show_open = 0;
      return;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F11) || Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Return)) {
      app_state->toggle_fullscreen = 1;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Backspace) && app_state->open_folder.size != 0) {
      Temp scratch = ScratchBegin(0, 0);
      _OPEN_GoToFolder(app_state, PushStr8Copy(scratch.arena, _OPEN_FolderAbove(app_state->open_folder)));
      ScratchEnd(scratch);
   }

   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         FR_RequestQuit(app_state);
      }

      // The wheel moves through the rows, three a notch.
      if (event->kind == WM_EventKind_Scroll && event->delta.y != 0) {
         U64 max_first = _OPEN_MaxFirstRow(app_state, &layout);
         if (event->delta.y > 0) {
            app_state->open_first_row = Min(app_state->open_first_row + 3, max_first);
         } else {
            app_state->open_first_row = (app_state->open_first_row > 3) ? app_state->open_first_row - 3 : 0;
         }
      }

      // Something dropped on the window is opened, as it is anywhere else.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         if (FR_OpenPath(app_state, event->strings.first->string)) {
            app_state->show_open = 0;
            return;
         }
      }

      if (event->kind == WM_EventKind_Press && event->key == WM_Key_LeftMouseButton) {
         if (!Contains2F32(layout.panel, event->pos)) {
            // A click beside the panel closes it.
            app_state->show_open = 0;
            return;
         }
         for (U64 i = 0; i < layout.rows_fit; i += 1) {
            U64 row_idx = app_state->open_first_row + i;
            if (row_idx < app_state->open_row_count && Contains2F32(_OPEN_RowRect(&layout, i), event->pos)) {
               _OPEN_PickRow(app_state, &app_state->open_rows[row_idx]);
               break;
            }
         }
         // One click a frame: the rows may not be the ones it was aimed at.
         break;
      }
   }

   app_state->open_first_row = Min(app_state->open_first_row, _OPEN_MaxFirstRow(app_state, &layout));
}

////////////////////////////////
//~ bgz: drawing

internal void
OPEN_Draw(App_State* app_state, Vec2F32 window_dim) {
   TXT_Fonts*   fonts = &app_state->fonts;
   _OPEN_Layout layout = _OPEN_LayoutFromWindowDim(window_dim);
   F32          ui = layout.ui;
   F32          px = layout.ui_px;
   Vec2F32      mouse = Core_WM_MouseFromWindow(app_state->window);
   Temp         scratch = ScratchBegin(0, 0);

   Vec4F32 color_text = V4F32(0.84f, 0.86f, 0.90f, 1.0f);
   Vec4F32 color_dim = V4F32(0.84f, 0.86f, 0.90f, 0.55f);
   Vec4F32 color_deck = V4F32(1.00f, 0.80f, 0.40f, 1.0f);
   Vec4F32 color_folder = V4F32(0.55f, 0.75f, 1.00f, 1.0f);
   Vec4F32 color_action = V4F32(0.60f, 0.85f, 0.60f, 1.0f);

   // What is behind goes dark, and the panel is on top of that.
   Core_D_Rect(R2F32P(0, 0, window_dim.x, window_dim.y), R2F32P(0, 0, 1, 1), V4F32(0, 0, 0, 0.6f));
   Core_D_Rect(layout.panel, R2F32P(0, 0, 1, 1), V4F32(0.11f, 0.115f, 0.135f, 1.0f));
   Core_D_Rect(layout.title, R2F32P(0, 0, 1, 1), V4F32(0.16f, 0.17f, 0.20f, 1.0f));
   Core_D_Rect(layout.foot, R2F32P(0, 0, 1, 1), V4F32(0.16f, 0.17f, 0.20f, 1.0f));

   Core_D_PushClip(layout.panel);

   F32 pad = 18.0f * ui;
   F32 indent = 22.0f * ui;
   F32 text_h = TXT_LineHeight(fonts->regular, px);

   //- bgz: the title
   {
      F32 title_px = (F32)(S32)(20.0f * ui);
      F32 title_h = TXT_LineHeight(fonts->bold, title_px);
      TXT_Draw(fonts->bold, title_px,
               V2F32(layout.title.min.x + pad, layout.title.min.y + (Dim2F32(layout.title).y - title_h) * 0.5f), color_text,
               Str8Lit("Open a deck"));
   }

   //- bgz: the rows
   for (U64 i = 0; i < layout.rows_fit; i += 1) {
      U64 row_idx = app_state->open_first_row + i;
      if (row_idx >= app_state->open_row_count) {
         break;
      }
      OPEN_Row* row = &app_state->open_rows[row_idx];
      Rng2F32   rect = _OPEN_RowRect(&layout, i);
      F32       y = rect.min.y + (layout.row_h - text_h) * 0.5f;

      if (row->kind == OPEN_RowKind_Label) {
         TXT_Draw(fonts->bold, px, V2F32(rect.min.x + pad, y), color_dim, row->label);
         continue;
      }

      if (Contains2F32(rect, mouse)) {
         Core_D_Rect(rect, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.07f));
      }

      // What the row is, by its colour - and for the ones that are not a
      // name, said beside it.
      Vec4F32 color = color_text;
      String8 what = {0};
      switch (row->kind) {
         default: {
         } break;
         case OPEN_RowKind_Recent:
         case OPEN_RowKind_Deck: {
            color = color_deck;
         } break;
         case OPEN_RowKind_Folder: {
            color = color_folder;
         } break;
         case OPEN_RowKind_Up: {
            color = color_folder;
            what = (row->path.size != 0) ? PushStr8F(scratch.arena, "the folder above: %S", row->path) : Str8Lit("the list of drives");
         } break;
         case OPEN_RowKind_Browse: {
            color = color_action;
            what = Str8Lit("Windows' own open window");
         } break;
         case OPEN_RowKind_New: {
            color = color_action;
            what = PushStr8F(scratch.arena, "makes %S, ready to write in", Str8SkipLastSlash(row->path));
         } break;
         case OPEN_RowKind_Present: {
            color = color_action;
            what = Str8Lit("as a slideshow, in name order");
         } break;
      }

      F32 x = rect.min.x + pad + indent;
      TXT_Draw(fonts->regular, px, V2F32(x, y), color, row->label);
      if (what.size != 0) {
         F32 label_w = TXT_Width(fonts->regular, px, row->label);
         TXT_Draw(fonts->regular, px, V2F32(x + label_w + 18.0f * ui, y), color_dim, what);
      }
   }

   //- bgz: the foot: a notice if one is up, else what can be done
   {
      F32     y = layout.foot.min.y + (Dim2F32(layout.foot).y - text_h) * 0.5f;
      String8 notice = FR_CurrentNotice(app_state);
      if (notice.size != 0) {
         TXT_Draw(fonts->regular, px, V2F32(layout.foot.min.x + pad, y), V4F32(1.0f, 0.85f, 0.5f, 1.0f), notice);
      } else {
         String8 left = Str8Lit("Click to open. Backspace: folder above. Esc closes.");
         TXT_Draw(fonts->regular, px, V2F32(layout.foot.min.x + pad, y), color_dim, left);

         if (app_state->open_row_count > layout.rows_fit) {
            U64     last = Min(app_state->open_first_row + layout.rows_fit, app_state->open_row_count);
            String8 more = PushStr8F(scratch.arena, "%llu-%llu of %llu, wheel for more",
                                     (unsigned long long)(app_state->open_first_row + 1), (unsigned long long)last,
                                     (unsigned long long)app_state->open_row_count);
            F32     more_w = TXT_Width(fonts->regular, px, more);
            TXT_Draw(fonts->regular, px, V2F32(layout.foot.max.x - pad - more_w, y), color_dim, more);
         }
      }
   }

   Core_D_PopClip();
   ScratchEnd(scratch);
}

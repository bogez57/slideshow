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
// The rows are made here (_Open_Scan) and the panel is built with the UI
// layer (Open_Build, called from FrameWork's UI build): a dimmed backdrop
// over the whole window, and on it the panel, its rows a UI_ScrollRows list.
// Open_Input has the keys; the clicks and the wheel are the panel's, and
// while it is up nothing under it gets either. Neither changes anything
// itself: a key or a click pushes a state op (Open_Show, Open_Close,
// Open_GoToFolder, Open_NewDeck, and app_state.c's App_OpenPath).
//
//    open the panel   Ctrl+O, on either screen
//    close it         Esc, Ctrl+O again, or a click beside the panel
//    folder above     Backspace, or the .. line
//    through the list the mouse wheel, or the scroll bar

////////////////////////////////
//~ bgz: the rows

// How often the folder is read again while the panel is up.
#define _OPEN_SCAN_SECONDS 1.0f

// How many of the decks opened lately get a line.
#define _OPEN_RECENTS_SHOWN 5

// The panel's own, which is not app state: the rows, made again from the
// folder every so often, and where they are scrolled to. Written here with
// no state op, as the editor project's views write theirs.
typedef struct _Open_View _Open_View;
struct _Open_View {
   Arena*        arena; // the rows
   Open_Row*     rows;
   U64           row_count;
   UI_ScrollRows scroll;     // where the rows are scrolled to (Open_Build)
   U64           scanned_us; // when the folder was last read. 0: read it now
};

PRIVATE _Open_View _open_view;

PRIVATE void
_Open_PushRow(Arena* arena, Open_Row* rows, U64* row_count, U64 row_cap, Open_RowKind kind, String8 label, String8 path) {
   if (*row_count < row_cap) {
      Open_Row* row = &rows[*row_count];
      row->kind = kind;
      row->label = PushStr8Copy(arena, label);
      row->path = PushStr8Copy(arena, path);
      *row_count += 1;
   }
}

// The name a new deck in folder gets: slides.md, or with that taken
// slides2.md, and so on.
PRIVATE String8
_Open_NewDeckName(Arena* arena, String8 folder) {
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
PRIVATE String8
_Open_PathInFolder(Arena* arena, String8 folder, String8 name) {
   B32 has_slash = (folder.size != 0 && folder.str[folder.size - 1] == '\\');
   return PushStr8F(arena, "%S%s%S", folder, has_slash ? "" : "\\", name);
}

// The folder a folder is in. A drive's own folder is in the list of drives:
// the empty string.
PRIVATE String8
_Open_FolderAbove(String8 folder) {
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
PRIVATE void
_Open_Scan(const App_State* app_state) {
   Temp scratch = ScratchBegin(0, 0);

   if (_open_view.arena == 0) {
      _open_view.arena = ArenaAlloc();
   }
   Arena* arena = _open_view.arena;
   ArenaClear(arena);
   String8 folder = app_state->open_folder;

   //- bgz: what is in the folder
   String8List folder_names = {0};
   String8List deck_names = {0};
   U64         picture_count = 0;
   {
      FileIter* it = FileIterBegin(scratch.arena, folder, FileIterFlag_SkipHiddenFiles);
      for (FileInfo info = {0}; FileIterNext(scratch.arena, it, &info);) {
         if (info.props.flags & FilePropertyFlag_IsFolder) {
            Str8ListPush(scratch.arena, &folder_names, info.name);
         } else if (Deck_NameIsTextDeck(info.name)) {
            Str8ListPush(scratch.arena, &deck_names, info.name);
         } else if (Image_NameIsImage(info.name)) {
            picture_count += 1;
         }
      }
      FileIterEnd(it);
   }
   String8Array folders = Str8ArrayFromList(scratch.arena, &folder_names);
   String8Array decks = Str8ArrayFromList(scratch.arena, &deck_names);
   Deck_SortNames(folders);
   Deck_SortNames(decks);

   //- bgz: the rows
   U64       recent_count = Min(app_state->recents.count, _OPEN_RECENTS_SHOWN);
   U64       row_cap = recent_count + folders.count + decks.count + 9;
   Open_Row* rows = PushArray(arena, Open_Row, row_cap);
   U64       row_count = 0;

   if (recent_count != 0) {
      _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Label, Str8Lit("Opened lately"), Str8Zero());
      for (U64 i = 0; i < recent_count; i += 1) {
         _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Recent, app_state->recents.v[i],
                       app_state->recents.v[i]);
      }
   }

   if (folder.size == 0) {
      // The list of drives: each is a folder, and that is all there is.
      _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Label, Str8Lit("Drives"), Str8Zero());
      for (U64 i = 0; i < folders.count; i += 1) {
         String8 drive = PushStr8F(scratch.arena, "%S\\", folders.v[i]);
         _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Folder, drive, drive);
      }
   } else {
      _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Label, folder, Str8Zero());
      _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Up, Str8Lit(".."), _Open_FolderAbove(folder));

      _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Browse, Str8Lit("Browse..."), folder);
      String8 new_name = _Open_NewDeckName(scratch.arena, folder);
      _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_New, Str8Lit("New deck here"),
                    _Open_PathInFolder(scratch.arena, folder, new_name));
      if (picture_count != 0) {
         String8 label = PushStr8F(scratch.arena, "Present the %llu picture%s in this folder",
                                   (unsigned long long)picture_count, picture_count == 1 ? "" : "s");
         _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Present, label, folder);
      }

      for (U64 i = 0; i < folders.count; i += 1) {
         _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Folder, PushStr8F(scratch.arena, "%S\\", folders.v[i]),
                       _Open_PathInFolder(scratch.arena, folder, folders.v[i]));
      }
      for (U64 i = 0; i < decks.count; i += 1) {
         _Open_PushRow(arena, rows, &row_count, row_cap, Open_RowKind_Deck, decks.v[i],
                       _Open_PathInFolder(scratch.arena, folder, decks.v[i]));
      }
   }

   _open_view.rows = rows;
   _open_view.row_count = row_count;
   _open_view.scanned_us = NowTimeUS();

   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: state ops

void
Open_GoToFolder(App_State* app_state, StateOp_List* ops, Open_GoToFolderOpParams* params) {
   UnusedVariable(ops);
   Temp scratch = ScratchBegin(0, 0);

   // The folder asked for may be in the arena cleared here.
   String8 folder = PushStr8Copy(scratch.arena, params->folder);
   if (app_state->open_arena == 0) {
      app_state->open_arena = ArenaAlloc();
   }
   ArenaClear(app_state->open_arena);
   app_state->open_folder = PushStr8Copy(app_state->open_arena, folder);

   // A new folder's list starts at its top, and is read when next built.
   MemoryZeroStruct(&_open_view.scroll);
   _open_view.scanned_us = 0;

   ScratchEnd(scratch);
}
STATE_OP(Open_GoToFolder)

void
Open_Show(App_State* app_state, StateOp_List* ops) {
   Temp scratch = ScratchBegin(0, 0);

   // It starts in the folder of the deck that is open, where the next deck
   // wanted is most likely to be near. With no deck, where the program was
   // started.
   String8 folder = Str8Lit(".");
   if (app_state->deck != 0 && app_state->deck->folder.size != 0) {
      folder = app_state->deck->folder;
   }

   app_state->show_open = 1;
   Open_GoToFolderOpParams go = {0};
   go.folder = FR_WholePath(scratch.arena, folder);
   Open_GoToFolder(app_state, ops, &go);

   ScratchEnd(scratch);
}
STATE_OP_NO_PARAMS(Open_Show)

void
Open_Close(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   app_state->show_open = 0;
}
STATE_OP_NO_PARAMS(Open_Close)

// A new text deck at path, opened with its one line, the title, ready to be
// typed over.
void
Open_NewDeck(App_State* app_state, StateOp_List* ops, Open_NewDeckOpParams* params) {
   // Asked first, so that a deck that cannot be opened now - unsaved
   // changes in the one that is - does not leave a file behind.
   B32 can_open = !(app_state->editor != 0 && Editor_HasUnsavedChanges(app_state->editor));
   if (can_open && !WriteDataToFilePath(params->path, Str8Lit("# Title\n"))) {
      StateOp_App_Notice(ops)->notice = Str8Lit("Could not make a file in this folder");
   } else if (FR_OpenPath(app_state, params->path)) {
      app_state->show_open = 0;
      if (app_state->editor != 0 && app_state->editor->size >= 7) {
         Editor_SetCursor(app_state->editor, 7);
         app_state->editor->mark = 2;
      }
   }
}
STATE_OP(Open_NewDeck)

////////////////////////////////
//~ bgz: layout

typedef struct _Open_Layout _Open_Layout;
struct _Open_Layout {
   F32     ui;    // how many pixels a unit is, as on the edit screen
   F32     ui_px; // the size of the text
   Rng2F32 panel;
   Rng2F32 title;
   Rng2F32 rows;  // where the list of rows is
   Rng2F32 foot;
   F32     row_h;
};

PRIVATE _Open_Layout
_Open_LayoutFromWindowDim(Vec2F32 window_dim) {
   _Open_Layout result = {0};

   F32 ui = FR_UIScale(window_dim);
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

   return result;
}

////////////////////////////////
//~ bgz: input

// What a click on a row does: the op for it. The row is in the arena a scan
// clears, so the op is given a copy of its path that lasts until it has run.
PRIVATE void
_Open_PickRow(StateOp_List* ops, Open_Row* row) {
   String8 path = PushStr8Copy(ops->arena, row->path);

   switch (row->kind) {
      case Open_RowKind_Label: {
      } break;

      case Open_RowKind_Up:
      case Open_RowKind_Folder: {
         StateOp_Open_GoToFolder(ops)->folder = path;
      } break;

      case Open_RowKind_Browse: {
         // bgz's shell: the system's window, which does not return until it
         // is answered. Empty: every file is listed - a deck is .md or .txt,
         // and a picture opens its folder.
         String8 picked = ShPickFile(ops->arena, Str8Lit("Open a deck"), path, Str8Zero());
         if (picked.size != 0) {
            StateOp_App_OpenPath(ops)->path = picked;
         }
      } break;

      case Open_RowKind_New: {
         StateOp_Open_NewDeck(ops)->path = path;
      } break;

      case Open_RowKind_Recent:
      case Open_RowKind_Present:
      case Open_RowKind_Deck: {
         StateOp_App_OpenPath(ops)->path = path;
      } break;
   }
}

// The keys, and what comes to the window rather than to the panel: a close, a
// file dropped. The clicks and the wheel are the panel's (Open_Build).
void
Open_Input(const App_State* app_state, StateOp_List* ops) {
   WM_EventList* events = app_state->events;
   WM_Window     window = app_state->window;

   if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc) || Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_O)) {
      StateOp_Open_Close(ops);
      return;
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_F11) || Core_WM_KeyPress(events, window, WM_Modifier_Alt, WM_Key_Return)) {
      StateOp_App_ToggleFullscreen(ops);
   }
   if (Core_WM_KeyPress(events, window, 0, WM_Key_Backspace) && app_state->open_folder.size != 0) {
      StateOp_Open_GoToFolder(ops)->folder = PushStr8Copy(ops->arena, _Open_FolderAbove(app_state->open_folder));
   }

   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         StateOp_App_RequestQuit(ops);
      }

      // Something dropped on the window is opened, as it is anywhere else.
      if (event->kind == WM_EventKind_FileDrop && event->strings.first != 0) {
         StateOp_App_OpenPath(ops)->path = PushStr8Copy(ops->arena, event->strings.first->string);
         return;
      }
   }
}

////////////////////////////////
//~ bgz: the panel, in the UI layer
//
//    backdrop    the whole window, dimmed: a click on it closes the panel, and
//                it takes every click and turn of the wheel that misses the
//                panel, so nothing underneath gets them
//       panel    title, the rows (UI_ScrollRows), foot
//
// Built first in the frame's UI build, so it is drawn over everything else
// and asked for clicks first.

// A strip across the panel with a line of text in it: the title, the foot.
PRIVATE void
_Open_BuildBar(Rng2F32 rect, F32 pad, String8 color_tag, String8 text, char* key) {
   UI_SetNextPrefWidth(UI_Px(Dim2F32(rect).x, 1));
   UI_SetNextPrefHeight(UI_Px(Dim2F32(rect).y, 1));
   UI_SetNextChildLayoutAxis(Axis2_X);
   UI_SetNextColorTag(Str8Lit("panel_bar"));
   UI_Box* bar = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clip, "###open %s", key);
   UI_Parent(bar) UI_PrefHeight(UI_Pct(1, 1)) {
      UI_Spacer(UI_Px(pad, 1));
      UI_SetNextPrefWidth(UI_Pct(1, 0));
      UI_SetNextColorTag(color_tag);
      UI_Label(text);
   }
}

// A path that is wider than room, px tall in font, with its front left off -
// "...\talks\intro\slides.md" - at a \ , as much of its end as fits. The
// end is what tells decks apart: the decks opened lately are often in
// folders deep under the same one, and cut off at the right, every one of
// their lines reads the same. A path that fits is as it is.
PRIVATE String8
_Open_FitPath(Arena* arena, FNT_Tag font, F32 px, String8 path, F32 room) {
   String8 result = path;
   if (Text_Width(font, px, path) > room) {
      String8 dots = Str8Lit("...");
      // The last part alone, if nothing longer fits: cut at the right then,
      // as any line too long for the panel is.
      result = PushStr8F(arena, "%S\\%S", dots, Str8SkipLastSlash(path));
      for (U64 i = 1; i < path.size; i += 1) {
         if (path.str[i - 1] == '\\' || path.str[i - 1] == '/') {
            String8 tail = Str8Skip(path, i);
            String8 fitted = PushStr8F(arena, "%S\\%S", dots, tail);
            if (Text_Width(font, px, fitted) <= room) {
               result = fitted;
               break;
            }
         }
      }
   }
   return result;
}

void
Open_Build(const App_State* app_state, Vec2F32 window_dim) {
   _Open_Layout layout = _Open_LayoutFromWindowDim(window_dim);
   F32          ui = layout.ui;
   F32          pad = 18.0f * ui;
   F32          indent = 22.0f * ui;
   F32          panel_w = Dim2F32(layout.panel).x;
   FNT_Tag      regular = app_state->fonts.regular;
   Temp         scratch = ScratchBegin(0, 0);

   // The rows, read again every so often, and at once for a folder just gone
   // into.
   U64 scan_us = (U64)(_OPEN_SCAN_SECONDS * 1000000.0f);
   if (_open_view.scanned_us == 0 || NowTimeUS() - _open_view.scanned_us >= scan_us) {
      _Open_Scan(app_state);
   }

   // The row clicked, if one was.
   Open_Row* picked = 0;

   UI_SetNextRect(R2F32P(0, 0, window_dim.x, window_dim.y));
   UI_SetNextColorTag(Str8Lit("backdrop"));
   UI_Box* backdrop = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clickable | UI_BoxFlag_Scroll,
                                             "###open backdrop");

   UI_Parent(backdrop) UI_FontSize(FR_UIFontSize(layout.ui_px)) {
      UI_SetNextFixedX(layout.panel.min.x);
      UI_SetNextFixedY(layout.panel.min.y);
      UI_SetNextPrefWidth(UI_Px(Dim2F32(layout.panel).x, 1));
      UI_SetNextPrefHeight(UI_Px(Dim2F32(layout.panel).y, 1));
      UI_SetNextChildLayoutAxis(Axis2_Y);
      UI_SetNextColorTag(Str8Lit("panel"));
      UI_Box* panel = UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground | UI_BoxFlag_Clickable | UI_BoxFlag_Scroll |
                                                 UI_BoxFlag_Clip,
                                             "###open panel");

      UI_Parent(panel) {
         //- bgz: the title
         UI_Font(app_state->fonts.bold) UI_FontSize(FR_UIFontSize((F32)(S32)(20.0f * ui))) {
            _Open_BuildBar(layout.title, pad, Str8Lit(""), Str8Lit("Open a deck"), "title");
         }

         //- bgz: the rows
         //
         // What a row is, by its colour - and for the ones that are not a
         // name, said beside it. A label row says what the rows under it are,
         // and takes no clicks.
         UI_ScrollRowsFlags scroll_flags = UI_ScrollRowsFlag_RowClicks;
         UI_ScrollRowsFrame rows_frame = UI_ScrollRowsBegin(&_open_view.scroll, Dim2F32(layout.rows),
                                                            (S64)_open_view.row_count, layout.row_h, 0,
                                                            scroll_flags, "###open rows");
         UI_ScrollRowsEnd(&_open_view.scroll, &rows_frame, 0, scroll_flags);

         UI_Parent(rows_frame.rows_box) UI_ColorTag(Str8Lit("row")) {
            for (S64 row_idx = rows_frame.first_row; row_idx < rows_frame.last_row; row_idx += 1) {
               Open_Row* row = &_open_view.rows[row_idx];
               B32       is_label = (row->kind == Open_RowKind_Label);

               UI_BoxFlags flags = is_label ? 0 : (UI_BoxFlag_Clickable | UI_BoxFlag_DrawBackground);
               UI_Box*     row_box = UI_ListRow(layout.row_h, 0, flags, "###open row %lld", row_idx);

               String8 color_tag = Str8Lit("");
               String8 what = {0};
               switch (row->kind) {
                  case Open_RowKind_Label: {
                     color_tag = Str8Lit("dim");
                  } break;
                  case Open_RowKind_Recent:
                  case Open_RowKind_Deck: {
                     color_tag = Str8Lit("deck");
                  } break;
                  case Open_RowKind_Folder: {
                     color_tag = Str8Lit("folder");
                  } break;
                  case Open_RowKind_Up: {
                     color_tag = Str8Lit("folder");
                     String8 before = Str8Lit("the folder above: ");
                     F32     used = pad + indent + Text_Width(regular, layout.ui_px, row->label) + 18.0f * ui +
                                Text_Width(regular, layout.ui_px, before) + pad;
                     what = (row->path.size != 0)
                                ? PushStr8F(scratch.arena, "%S%S", before,
                                            _Open_FitPath(scratch.arena, regular, layout.ui_px, row->path, panel_w - used))
                                : Str8Lit("the list of drives");
                  } break;
                  case Open_RowKind_Browse: {
                     color_tag = Str8Lit("action");
                     what = Str8Lit("Windows' own open window");
                  } break;
                  case Open_RowKind_New: {
                     color_tag = Str8Lit("action");
                     what = PushStr8F(scratch.arena, "makes %S, ready to write in", Str8SkipLastSlash(row->path));
                  } break;
                  case Open_RowKind_Present: {
                     color_tag = Str8Lit("action");
                     what = Str8Lit("as a slideshow, in name order");
                  } break;
               }

               UI_Parent(row_box) UI_PrefHeight(UI_Pct(1, 1)) UI_Font(is_label ? app_state->fonts.bold : UI_TopFont()) {
                  UI_Spacer(UI_Px(is_label ? pad : pad + indent, 1));
                  // A whole path - a deck opened lately, the folder the rows
                  // are of - with as much of its end as fits.
                  String8 label = row->label;
                  if (row->kind == Open_RowKind_Recent) {
                     label = _Open_FitPath(scratch.arena, regular, layout.ui_px, label, panel_w - (pad + indent) - pad);
                  } else if (is_label) {
                     label = _Open_FitPath(scratch.arena, app_state->fonts.bold, layout.ui_px, label, panel_w - pad * 2.0f);
                  }
                  UI_SetNextPrefWidth(UI_TextDim(1, 1));
                  UI_SetNextColorTag(color_tag);
                  UI_Label(label);
                  if (what.size != 0) {
                     UI_Spacer(UI_Px(18.0f * ui, 1));
                     UI_SetNextPrefWidth(UI_TextDim(1, 0));
                     UI_SetNextColorTag(Str8Lit("dim"));
                     UI_Label(what);
                  }
               }

               if (!is_label) {
                  UI_Signal signal = UI_SignalFromBox(row_box);
                  if (UI_Hovering(signal)) {
                     row_box->background_color = UI_ThemeColor(Str8Lit("hot"), UI_ColorSlot_Background);
                  }
                  if (UI_Clicked(signal)) {
                     picked = row;
                  }
               }
            }
         }

         //- bgz: the foot: a notice if one is up, else what can be done
         String8 notice = FR_CurrentNotice(app_state);
         if (notice.size != 0) {
            _Open_BuildBar(layout.foot, pad, Str8Lit("notice"), notice, "foot");
         } else {
            _Open_BuildBar(layout.foot, pad, Str8Lit("dim"),
                           Str8Lit("Click to open. Wheel or bar to scroll. Backspace: folder above. Esc closes."), "foot");
         }
      }

      // After the rows have had theirs: a click or the wheel on the rest of
      // the panel stops here, and does not close it.
      UI_SignalFromBox(panel);
   }

   // Last: whatever missed the panel. A click closes it.
   UI_Signal backdrop_signal = UI_SignalFromBox(backdrop);
   if (picked != 0) {
      _Open_PickRow(app_state->op_list, picked);
   } else if (UI_Clicked(backdrop_signal)) {
      StateOp_Open_Close(app_state->op_list);
   }

   ScratchEnd(scratch);
}

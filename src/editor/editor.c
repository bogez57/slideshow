////////////////////////////////
//~ bgz: editor
//
// See editor.h.

// Typing or deleting that follows the last within this long is undone with it.
#define _ED_UNDO_JOIN_US 1000000

// Room between the box's edge and the text, as a part of the font's size.
#define _ED_PAD 0.7f

////////////////////////////////
//~ bgz: memory

// Text as the editor keeps it, from text as it comes from a file or the
// clipboard: no \r, four spaces for a tab (the font cache has no width for
// one), and no other control characters. On arena.
internal String8
_ED_CleanText(Arena* arena, String8 string) {
   String8 result = {0};
   result.str = PushArrayNoZero(arena, U8, string.size * 4 + 1);

   for (U64 i = 0; i < string.size; i += 1) {
      U8 c = string.str[i];
      if (c == '\t') {
         for (U64 j = 0; j < 4; j += 1) {
            result.str[result.size] = ' ';
            result.size += 1;
         }
      } else if (c == '\n' || c >= 32) {
         if (c != 127) {
            result.str[result.size] = c;
            result.size += 1;
         }
      }
   }

   return result;
}

internal void
_ED_Reserve(ED_Editor* editor, U64 size) {
   if (size > editor->cap) {
      U64 cap = Max(editor->cap * 2, 4096);
      while (cap < size) cap *= 2;

      // The text moves to new room twice the size, and its old room stays
      // in the arena, unused: all the old rooms together are smaller than
      // the new one.
      U8* text = PushArrayNoZero(editor->arena, U8, cap);
      if (editor->size != 0) {
         MemoryCopy(text, editor->text, editor->size);
      }
      editor->text = text;
      editor->cap = cap;
   }
}

////////////////////////////////
//~ bgz: undo

internal ED_Snapshot
_ED_SnapshotMake(ED_Editor* editor) {
   ED_Snapshot result = {0};
   result.text = PushArrayNoZero(editor->history_arena, U8, editor->size + 1);
   result.size = editor->size;
   MemoryCopy(result.text, editor->text, editor->size);
   result.cursor = editor->cursor;
   result.mark = editor->mark;
   return result;
}

internal void
_ED_SnapshotRestore(ED_Editor* editor, ED_Snapshot* snapshot) {
   _ED_Reserve(editor, snapshot->size);
   MemoryCopy(editor->text, snapshot->text, snapshot->size);
   editor->size = snapshot->size;
   editor->cursor = snapshot->cursor;
   editor->mark = snapshot->mark;
}

// A snapshot let go of leaves its text in the history arena. Once the arena
// holds this much, and several times what the snapshots still wanted come
// to, those are copied to the spare arena, this one is cleared, and the two
// change places.
#define _ED_HISTORY_TIDY_SIZE MB(4)

internal void
_ED_HistoryTidy(ED_Editor* editor) {
   U64 wanted_size = 0;
   for (U64 i = 0; i < editor->undo_count; i += 1) wanted_size += editor->undo[i].size + 1;
   for (U64 i = 0; i < editor->redo_count; i += 1) wanted_size += editor->redo[i].size + 1;

   if (wanted_size == 0) {
      ArenaClear(editor->history_arena);
   } else if (ArenaPos(editor->history_arena) > Max(_ED_HISTORY_TIDY_SIZE, wanted_size * 4)) {
      Arena*       to = editor->history_spare;
      ED_Snapshot* stacks[] = {editor->undo, editor->redo};
      U64          counts[] = {editor->undo_count, editor->redo_count};
      for (U64 stack_idx = 0; stack_idx < ArrayCount(stacks); stack_idx += 1) {
         for (U64 i = 0; i < counts[stack_idx]; i += 1) {
            ED_Snapshot* snapshot = &stacks[stack_idx][i];
            U8*          text = PushArrayNoZero(to, U8, snapshot->size + 1);
            MemoryCopy(text, snapshot->text, snapshot->size);
            snapshot->text = text;
         }
      }
      ArenaClear(editor->history_arena);
      editor->history_spare = editor->history_arena;
      editor->history_arena = to;
   }
}

// Pushes onto one of the two stacks, letting go of the oldest when full.
internal void
_ED_SnapshotPush(ED_Snapshot* stack, U64* count, ED_Snapshot snapshot) {
   if (*count == ED_MAX_UNDO) {
      MemoryCopy(stack, stack + 1, sizeof(stack[0]) * (ED_MAX_UNDO - 1));
      *count -= 1;
   }
   stack[*count] = snapshot;
   *count += 1;
}

internal void
_ED_SnapshotsClear(ED_Snapshot* stack, U64* count) {
   UnusedVariable(stack);
   *count = 0;
}

// Called before every change to the text, with what kind it will be.
internal void
_ED_BeforeEdit(ED_Editor* editor, ED_EditKind kind) {
   U64 now_us = NowTimeUS();

   B32 joins_last = (kind != ED_EditKind_Other && kind == editor->last_edit_kind &&
                     now_us - editor->last_edit_us < _ED_UNDO_JOIN_US);
   if (!joins_last) {
      _ED_SnapshotPush(editor->undo, &editor->undo_count, _ED_SnapshotMake(editor));
   }
   _ED_SnapshotsClear(editor->redo, &editor->redo_count);
   _ED_HistoryTidy(editor);

   editor->last_edit_kind = kind;
   editor->last_edit_us = now_us;
}

// What undo and redo both do: from is taken from, and what is there now is
// put on to.
internal void
_ED_StepHistory(ED_Editor* editor, ED_Snapshot* from, U64* from_count, ED_Snapshot* to, U64* to_count) {
   if (*from_count != 0) {
      _ED_SnapshotPush(to, to_count, _ED_SnapshotMake(editor));

      *from_count -= 1;
      _ED_SnapshotRestore(editor, &from[*from_count]);
      _ED_HistoryTidy(editor);

      editor->edit_count += 1;
      editor->last_edit_kind = ED_EditKind_None;
      editor->want_column = -1;
      editor->scroll_to_cursor = 1;
   }
}

////////////////////////////////
//~ bgz: positions

internal U64
_ED_SelectionMin(ED_Editor* editor) {
   return Min(editor->cursor, editor->mark);
}

internal U64
_ED_SelectionMax(ED_Editor* editor) {
   return Max(editor->cursor, editor->mark);
}

// A byte in the middle of a UTF-8 character, which the cursor never rests on.
internal B32
_ED_IsContinuation(U8 c) {
   return (c & 0xC0) == 0x80;
}

internal U64
_ED_PrevChar(ED_Editor* editor, U64 pos) {
   if (pos > 0) {
      pos -= 1;
      while (pos > 0 && _ED_IsContinuation(editor->text[pos])) pos -= 1;
   }
   return pos;
}

internal U64
_ED_NextChar(ED_Editor* editor, U64 pos) {
   if (pos < editor->size) {
      pos += 1;
      while (pos < editor->size && _ED_IsContinuation(editor->text[pos])) pos += 1;
   }
   return pos;
}

// The start and end of the line pos is on, as the file has lines. The end is
// where its \n is, or the end of the text.
internal U64
_ED_LineStart(ED_Editor* editor, U64 pos) {
   while (pos > 0 && editor->text[pos - 1] != '\n') pos -= 1;
   return pos;
}

internal U64
_ED_LineEnd(ED_Editor* editor, U64 pos) {
   while (pos < editor->size && editor->text[pos] != '\n') pos += 1;
   return pos;
}

internal B32
_ED_IsWordChar(U8 c) {
   return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

// The start of the word before pos, and the end of the one after: what
// Ctrl+Left and Ctrl+Right move to.
internal U64
_ED_WordLeft(ED_Editor* editor, U64 pos) {
   while (pos > 0 && !_ED_IsWordChar(editor->text[pos - 1])) pos -= 1;
   while (pos > 0 && _ED_IsWordChar(editor->text[pos - 1])) pos -= 1;
   return pos;
}

internal U64
_ED_WordRight(ED_Editor* editor, U64 pos) {
   while (pos < editor->size && !_ED_IsWordChar(editor->text[pos])) pos += 1;
   while (pos < editor->size && _ED_IsWordChar(editor->text[pos])) pos += 1;
   return pos;
}

// How many characters there are from start to pos.
internal U64
_ED_ColumnCount(ED_Editor* editor, U64 start, U64 pos) {
   U64 result = 0;
   for (U64 i = start; i < pos && i < editor->size; i += 1) {
      if (!_ED_IsContinuation(editor->text[i])) {
         result += 1;
      }
   }
   return result;
}

////////////////////////////////
//~ bgz: rows

// Fills rows, if it is not 0, and returns how many there are. A line longer
// than max_columns is cut into rows after the last space that fits, or at
// max_columns if it has none. A space a row is cut after stays on that row.
internal U64
_ED_FillRows(ED_Editor* editor, U64 max_columns, ED_Row* rows) {
   U64 count = 0;
   U64 line_start = 0;

   for (;;) {
      U64 line_end = _ED_LineEnd(editor, line_start);

      U64 pos = line_start;
      do {
         // As many characters as fit, noting where the last space ends.
         U64 end = pos;
         U64 columns = 0;
         U64 after_space = pos;
         while (end < line_end && columns < max_columns) {
            if (editor->text[end] == ' ') {
               after_space = end + 1;
            }
            end = _ED_NextChar(editor, end);
            columns += 1;
         }
         end = Min(end, line_end);

         B32 is_line_end = (end == line_end);
         if (!is_line_end && after_space > pos) {
            end = after_space;
         }

         if (rows != 0) {
            rows[count].start = pos;
            rows[count].end = end;
            rows[count].line_start = (pos == line_start);
            rows[count].line_end = is_line_end;
         }
         count += 1;
         pos = end;
      } while (pos < line_end);

      if (line_end >= editor->size) {
         break;
      }
      line_start = line_end + 1;
   }

   return count;
}

internal ED_RowArray
_ED_Layout(Arena* arena, ED_Editor* editor, U64 max_columns) {
   ED_RowArray result = {0};
   max_columns = Max(max_columns, 8);
   result.count = _ED_FillRows(editor, max_columns, 0);
   result.v = PushArray(arena, ED_Row, result.count);
   _ED_FillRows(editor, max_columns, result.v);
   return result;
}

// The row pos is drawn on: the last that starts at or before it. A position
// where a long line is cut is the start of the row after the cut.
internal U64
_ED_RowFromPos(ED_RowArray rows, U64 pos) {
   U64 result = 0;
   for (U64 i = 0; i < rows.count; i += 1) {
      if (rows.v[i].start <= pos) {
         result = i;
      } else {
         break;
      }
   }
   return result;
}

// The position at a column of a row, or the row's last if it is shorter. On
// a row a line is cut after, the last is before the space at its end: after
// it is the next row's first.
internal U64
_ED_PosFromRowColumn(ED_Editor* editor, ED_Row row, U64 column) {
   U64 last = row.end;
   if (!row.line_end && last > row.start) {
      last = _ED_PrevChar(editor, last);
   }

   U64 pos = row.start;
   for (U64 i = 0; i < column && pos < last; i += 1) {
      pos = _ED_NextChar(editor, pos);
   }
   return Min(pos, last);
}

// The box, measured.
typedef struct _ED_Metrics _ED_Metrics;
struct _ED_Metrics {
   Rng2F32 text_rect; // the box less its padding
   F32     char_w;
   F32     row_h;
   U64     columns;      // that fit across
   U64     rows_visible; // that fit down, whole
};

internal _ED_Metrics
_ED_MetricsFromBox(ED_Box box) {
   _ED_Metrics result = {0};
   F32         pad = box.px * _ED_PAD;

   result.text_rect = R2F32P(box.rect.min.x + pad, box.rect.min.y + pad * 0.5f, box.rect.max.x - pad, box.rect.max.y);
   result.char_w = Max(TXT_Width(box.font, box.px, Str8Lit("M")), 1.0f);
   result.row_h = Max(TXT_LineHeight(box.font, box.px), 1.0f);

   Vec2F32 dim = Dim2F32(result.text_rect);
   result.columns = (dim.x > 0) ? (U64)(dim.x / result.char_w) : 0;
   result.rows_visible = (dim.y > 0) ? (U64)(dim.y / result.row_h) : 0;
   result.rows_visible = Max(result.rows_visible, 1);
   return result;
}

internal U64
_ED_PosFromPoint(ED_Editor* editor, ED_RowArray rows, _ED_Metrics* metrics, Vec2F32 point) {
   F32 y = point.y - metrics->text_rect.min.y + editor->scroll_y;
   F32 x = point.x - metrics->text_rect.min.x;

   U64 row_idx = (y > 0) ? (U64)(y / metrics->row_h) : 0;
   row_idx = Min(row_idx, rows.count - 1);

   // The nearest gap between characters, not the character under the point.
   U64 column = (x > 0) ? (U64)(x / metrics->char_w + 0.5f) : 0;

   return _ED_PosFromRowColumn(editor, rows.v[row_idx], column);
}

////////////////////////////////
//~ bgz: changing the text

// Puts string in place of start to end, and moves the cursor and mark as the
// text around them moves. Nothing else: no undo, no selection.
internal void
_ED_Splice(ED_Editor* editor, U64 start, U64 end, String8 string) {
   start = Min(start, editor->size);
   end = Clamp(start, end, editor->size);

   U64 removed = end - start;
   U64 new_size = editor->size - removed + string.size;
   _ED_Reserve(editor, new_size + 1);

   // MemoryCopy is memmove: the two may overlap.
   MemoryCopy(editor->text + start + string.size, editor->text + end, editor->size - end);
   if (string.size != 0) {
      MemoryCopy(editor->text + start, string.str, string.size);
   }
   editor->size = new_size;

   U64* positions[] = {&editor->cursor, &editor->mark};
   for (U64 i = 0; i < ArrayCount(positions); i += 1) {
      U64 pos = *positions[i];
      if (pos >= end) {
         pos = pos - removed + string.size;
      } else if (pos > start) {
         pos = start + string.size;
      }
      *positions[i] = pos;
   }

   editor->edit_count += 1;
   editor->want_column = -1;
   editor->scroll_to_cursor = 1;
}

internal void
ED_Insert(ED_Editor* editor, String8 string, ED_EditKind kind) {
   U64 start = _ED_SelectionMin(editor);
   U64 end = _ED_SelectionMax(editor);

   // Typing over a selection is its own step to undo.
   _ED_BeforeEdit(editor, (start != end) ? ED_EditKind_Other : kind);
   _ED_Splice(editor, start, end, string);
   editor->cursor = start + string.size;
   editor->mark = editor->cursor;
}

internal void
ED_InsertLine(ED_Editor* editor, String8 string, U64 select_start, U64 select_end) {
   U64 line_start = _ED_LineStart(editor, editor->cursor);
   U64 line_end = _ED_LineEnd(editor, editor->cursor);

   B32 line_is_blank = 1;
   for (U64 i = line_start; i < line_end; i += 1) {
      if (editor->text[i] != ' ') {
         line_is_blank = 0;
         break;
      }
   }

   _ED_BeforeEdit(editor, ED_EditKind_Other);

   U64 string_start = 0;
   if (line_is_blank) {
      _ED_Splice(editor, line_start, line_end, string);
      string_start = line_start;
   } else {
      _ED_Splice(editor, line_end, line_end, Str8Lit("\n"));
      _ED_Splice(editor, line_end + 1, line_end + 1, string);
      string_start = line_end + 1;
   }

   editor->mark = string_start + Min(select_start, string.size);
   editor->cursor = string_start + Min(select_end, string.size);
}

internal void
ED_ReplaceRange(ED_Editor* editor, U64 start, U64 end, String8 string) {
   start = Min(start, editor->size);
   end = Clamp(start, end, editor->size);

   _ED_BeforeEdit(editor, ED_EditKind_Other);
   _ED_Splice(editor, start, end, string);
   editor->cursor = start + string.size;
   editor->mark = editor->cursor;
}

internal void
ED_SetCursor(ED_Editor* editor, U64 pos) {
   editor->cursor = Min(pos, editor->size);
   editor->mark = editor->cursor;
   editor->want_column = -1;
   editor->scroll_to_cursor = 1;
}

// Removes the selection, or with none, from the cursor to pos.
internal void
_ED_Delete(ED_Editor* editor, U64 pos) {
   U64 start = _ED_SelectionMin(editor);
   U64 end = _ED_SelectionMax(editor);
   if (start == end) {
      start = Min(editor->cursor, pos);
      end = Max(editor->cursor, pos);
   }
   if (start != end) {
      _ED_BeforeEdit(editor, ED_EditKind_Delete);
      _ED_Splice(editor, start, end, Str8Zero());
      editor->cursor = start;
      editor->mark = start;
   }
}

// How long the bullet at the start of a line is, with the spaces before it
// and the one after: "  - " is 4. 0 if the line is not a bullet.
internal U64
_ED_BulletPrefixSize(ED_Editor* editor, U64 line_start, U64 line_end) {
   U64 pos = line_start;
   while (pos < line_end && editor->text[pos] == ' ') pos += 1;

   U64 result = 0;
   if (pos + 1 < line_end && (editor->text[pos] == '-' || editor->text[pos] == '*') && editor->text[pos + 1] == ' ') {
      result = pos + 2 - line_start;
   } else if (pos + 1 == line_end && (editor->text[pos] == '-' || editor->text[pos] == '*')) {
      // A bullet with nothing typed after it yet, and its space trimmed.
      result = pos + 1 - line_start;
   }
   return result;
}

// Return. On a bullet it starts the next bullet, at the same level - and on
// a bullet with nothing in it, takes the bullet away instead, which is how a
// list is left.
internal void
_ED_Newline(ED_Editor* editor) {
   B32 handled = 0;

   if (editor->cursor == editor->mark) {
      U64 line_start = _ED_LineStart(editor, editor->cursor);
      U64 line_end = _ED_LineEnd(editor, editor->cursor);
      U64 prefix_size = _ED_BulletPrefixSize(editor, line_start, line_end);

      if (prefix_size != 0 && editor->cursor >= line_start + prefix_size) {
         B32 bullet_is_empty = 1;
         for (U64 i = line_start + prefix_size; i < line_end; i += 1) {
            if (editor->text[i] != ' ') {
               bullet_is_empty = 0;
               break;
            }
         }

         if (bullet_is_empty) {
            _ED_BeforeEdit(editor, ED_EditKind_Other);
            _ED_Splice(editor, line_start, line_end, Str8Zero());
            editor->cursor = line_start;
            editor->mark = line_start;
         } else {
            // "\n" and the line's own bullet, copied before the text moves.
            U8  prefix[256];
            U64 size = 0;
            prefix[size] = '\n';
            size += 1;
            for (U64 i = 0; i < prefix_size && size < sizeof(prefix) - 1; i += 1) {
               prefix[size] = editor->text[line_start + i];
               size += 1;
            }
            if (prefix[size - 1] != ' ') {
               prefix[size] = ' ';
               size += 1;
            }
            ED_Insert(editor, Str8(prefix, size), ED_EditKind_Other);
         }
         handled = 1;
      }
   }

   if (!handled) {
      ED_Insert(editor, Str8Lit("\n"), ED_EditKind_Other);
   }
}

// Tab and Shift+Tab. On a bullet they move it a level in or out, wherever in
// the line the cursor is. Elsewhere Tab types two spaces.
internal void
_ED_Tab(ED_Editor* editor, B32 is_shift) {
   U64 line_start = _ED_LineStart(editor, editor->cursor);
   U64 line_end = _ED_LineEnd(editor, editor->cursor);
   B32 is_bullet = (_ED_BulletPrefixSize(editor, line_start, line_end) != 0);

   if (is_shift) {
      U64 spaces = 0;
      while (spaces < 2 && line_start + spaces < line_end && editor->text[line_start + spaces] == ' ') spaces += 1;
      if (spaces != 0) {
         _ED_BeforeEdit(editor, ED_EditKind_Other);
         _ED_Splice(editor, line_start, line_start + spaces, Str8Zero());
      }
   } else if (is_bullet && editor->cursor == editor->mark) {
      _ED_BeforeEdit(editor, ED_EditKind_Other);

      // The cursor moves along with the text it is in.
      _ED_Splice(editor, line_start, line_start, Str8Lit("  "));
   } else {
      ED_Insert(editor, Str8Lit("  "), ED_EditKind_Type);
   }
}

////////////////////////////////
//~ bgz: files

internal void
_ED_Read(ED_Editor* editor) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 data = DataFromFilePath(scratch.arena, editor->file);

   // A byte order mark, which Notepad puts at the start of a UTF-8 file.
   if (data.size >= 3 && data.str[0] == 0xEF && data.str[1] == 0xBB && data.str[2] == 0xBF) {
      data = Str8Skip(data, 3);
   }

   editor->file_crlf = (Str8FindNeedle(data, 0, Str8Lit("\r\n"), 0) < data.size);

   String8 text = _ED_CleanText(scratch.arena, data);
   editor->size = 0; // nothing of the old text to take along to new room
   _ED_Reserve(editor, text.size + 1);
   MemoryCopy(editor->text, text.str, text.size);
   editor->size = text.size;

   editor->file_modified = PropertiesFromFilePath(editor->file).modified;
   editor->cursor = Min(editor->cursor, editor->size);
   while (editor->cursor > 0 && editor->cursor < editor->size && _ED_IsContinuation(editor->text[editor->cursor])) {
      editor->cursor -= 1;
   }
   editor->mark = editor->cursor;
   editor->want_column = -1;
   editor->scroll_to_cursor = 1;

   editor->edit_count += 1;
   editor->saved_count = editor->edit_count;

   _ED_SnapshotsClear(editor->undo, &editor->undo_count);
   _ED_SnapshotsClear(editor->redo, &editor->redo_count);
   _ED_HistoryTidy(editor);
   editor->last_edit_kind = ED_EditKind_None;

   ScratchEnd(scratch);
}

internal ED_Editor*
ED_Open(String8 file) {
   Arena*     arena = ArenaAlloc();
   ED_Editor* editor = PushArray(arena, ED_Editor, 1);
   editor->arena = arena;
   editor->history_arena = ArenaAlloc();
   editor->history_spare = ArenaAlloc();
   editor->file = PushStr8Copy(arena, file);
   _ED_Reserve(editor, 1);
   _ED_Read(editor);
   return editor;
}

internal void
ED_Close(ED_Editor* editor) {
   ArenaRelease(editor->history_arena);
   ArenaRelease(editor->history_spare);
   ArenaRelease(editor->arena); // the editor itself is on it
}

internal String8
ED_Text(ED_Editor* editor) {
   return Str8(editor->text, editor->size);
}

internal B32
ED_HasUnsavedChanges(ED_Editor* editor) {
   return editor->edit_count != editor->saved_count;
}

internal B32
ED_Save(ED_Editor* editor) {
   String8 data = ED_Text(editor);

   // The file's own line endings back.
   Temp scratch = ScratchBegin(0, 0);
   if (editor->file_crlf) {
      U8* crlf_text = PushArrayNoZero(scratch.arena, U8, editor->size * 2 + 1);
      U64 size = 0;
      for (U64 i = 0; i < editor->size; i += 1) {
         if (editor->text[i] == '\n') {
            crlf_text[size] = '\r';
            size += 1;
         }
         crlf_text[size] = editor->text[i];
         size += 1;
      }
      data = Str8(crlf_text, size);
   }

   B32 result = WriteDataToFilePath(editor->file, data);
   if (result) {
      editor->saved_count = editor->edit_count;
      editor->file_modified = PropertiesFromFilePath(editor->file).modified;
   }

   ScratchEnd(scratch);
   return result;
}

internal B32
ED_FileHasChanged(ED_Editor* editor) {
   // A file that is gone for a moment - some editors save by writing a new
   // file and renaming it - reads as modified at 0, and has not changed.
   DenseTime modified = PropertiesFromFilePath(editor->file).modified;
   B32       result = (modified != 0 && modified != editor->file_modified);
   return result;
}

internal void
ED_Reload(ED_Editor* editor) {
   _ED_Read(editor);
}

////////////////////////////////
//~ bgz: input

// Moves the cursor to pos, taking the mark along unless selecting.
internal void
_ED_MoveTo(ED_Editor* editor, U64 pos, B32 is_selecting) {
   editor->cursor = Min(pos, editor->size);
   if (!is_selecting) {
      editor->mark = editor->cursor;
   }
   editor->scroll_to_cursor = 1;
}

// Up or down by row_delta rows, keeping to the column the cursor was in
// before the first such move - through rows too short to have it.
internal void
_ED_MoveRows(ED_Editor* editor, ED_RowArray rows, S64 row_delta, B32 is_selecting) {
   U64 row_idx = _ED_RowFromPos(rows, editor->cursor);
   if (editor->want_column < 0) {
      editor->want_column = (S64)_ED_ColumnCount(editor, rows.v[row_idx].start, editor->cursor);
   }

   S64 target = (S64)row_idx + row_delta;
   target = Clamp(0, target, (S64)rows.count - 1);

   S64 want_column = editor->want_column;
   _ED_MoveTo(editor, _ED_PosFromRowColumn(editor, rows.v[target], (U64)want_column), is_selecting);
   editor->want_column = want_column;
}

internal void
ED_Input(ED_Editor* editor, Arena* arena, WM_EventList* events, WM_Window window, ED_Box box) {
   _ED_Metrics metrics = _ED_MetricsFromBox(box);
   Vec2F32     mouse = Core_WM_MouseFromWindow(window);
   B32         mouse_is_over = Contains2F32(box.rect, mouse);
   B32         had_input = 0;

   WM_Event* next_event = 0;
   for (WM_Event* event = events->first; event != 0; event = next_event) {
      next_event = event->next;
      B32 used = 0;

      // The rows are laid out again for each event: the one before it may
      // have changed the text.
      ED_RowArray rows = _ED_Layout(arena, editor, metrics.columns);

      B32 is_ctrl = !!(event->modifiers & WM_Modifier_Ctrl);
      B32 is_shift = !!(event->modifiers & WM_Modifier_Shift);
      B32 is_alt = !!(event->modifiers & WM_Modifier_Alt);

      if (event->kind == WM_EventKind_Text) {
         // Return comes as text too, and is handled as a key below. Ctrl with
         // Alt is AltGr, which types characters on some keyboards.
         U32 c = event->character;
         if (c >= 32 && c != 127 && (!is_ctrl || is_alt)) {
            U8  utf8[4];
            U64 size = 0;
            if (c < 0x80) {
               utf8[0] = (U8)c;
               size = 1;
            } else if (c < 0x800) {
               utf8[0] = (U8)(0xC0 | (c >> 6));
               utf8[1] = (U8)(0x80 | (c & 0x3F));
               size = 2;
            } else if (c < 0xD800 || (c >= 0xE000 && c < 0x10000)) {
               utf8[0] = (U8)(0xE0 | (c >> 12));
               utf8[1] = (U8)(0x80 | ((c >> 6) & 0x3F));
               utf8[2] = (U8)(0x80 | (c & 0x3F));
               size = 3;
            }
            // Half of a pair that makes a character outside the first 65536
            // (an emoji) is dropped: the font here would not draw it.
            if (size != 0) {
               ED_Insert(editor, Str8(utf8, size), ED_EditKind_Type);
            }
         }
         used = 1;
      } else if (event->kind == WM_EventKind_Scroll) {
         if (mouse_is_over) {
            editor->scroll_y += event->delta.y / 120.0f * metrics.row_h * 3.0f;
            used = 1;
         }
      } else if (event->kind == WM_EventKind_Press) {
         used = 1;
         switch (event->key) {
         case WM_Key_LeftMouseButton: {
            if (Contains2F32(box.rect, event->pos)) {
               _ED_MoveTo(editor, _ED_PosFromPoint(editor, rows, &metrics, event->pos), is_shift);
               editor->want_column = -1;
               editor->is_dragging = 1;
            } else {
               used = 0;
            }
         } break;

         case WM_Key_Left: {
            if (!is_shift && !is_ctrl && editor->cursor != editor->mark) {
               _ED_MoveTo(editor, _ED_SelectionMin(editor), 0);
            } else {
               _ED_MoveTo(editor, is_ctrl ? _ED_WordLeft(editor, editor->cursor) : _ED_PrevChar(editor, editor->cursor), is_shift);
            }
            editor->want_column = -1;
         } break;

         case WM_Key_Right: {
            if (!is_shift && !is_ctrl && editor->cursor != editor->mark) {
               _ED_MoveTo(editor, _ED_SelectionMax(editor), 0);
            } else {
               _ED_MoveTo(editor, is_ctrl ? _ED_WordRight(editor, editor->cursor) : _ED_NextChar(editor, editor->cursor), is_shift);
            }
            editor->want_column = -1;
         } break;

         case WM_Key_Up: {
            _ED_MoveRows(editor, rows, -1, is_shift);
         } break;

         case WM_Key_Down: {
            _ED_MoveRows(editor, rows, +1, is_shift);
         } break;

         case WM_Key_PageUp: {
            _ED_MoveRows(editor, rows, -(S64)metrics.rows_visible, is_shift);
         } break;

         case WM_Key_PageDown: {
            _ED_MoveRows(editor, rows, +(S64)metrics.rows_visible, is_shift);
         } break;

         case WM_Key_Home: {
            U64 pos = 0;
            if (!is_ctrl) {
               pos = rows.v[_ED_RowFromPos(rows, editor->cursor)].start;
            }
            _ED_MoveTo(editor, pos, is_shift);
            editor->want_column = -1;
         } break;

         case WM_Key_End: {
            U64 pos = editor->size;
            if (!is_ctrl) {
               // Past any column the row has, which is its last position.
               pos = _ED_PosFromRowColumn(editor, rows.v[_ED_RowFromPos(rows, editor->cursor)], max_U64);
            }
            _ED_MoveTo(editor, pos, is_shift);
            editor->want_column = -1;
         } break;

         case WM_Key_Backspace: {
            _ED_Delete(editor, is_ctrl ? _ED_WordLeft(editor, editor->cursor) : _ED_PrevChar(editor, editor->cursor));
         } break;

         case WM_Key_Delete: {
            _ED_Delete(editor, is_ctrl ? _ED_WordRight(editor, editor->cursor) : _ED_NextChar(editor, editor->cursor));
         } break;

         case WM_Key_Return: {
            _ED_Newline(editor);
         } break;

         case WM_Key_Tab: {
            _ED_Tab(editor, is_shift);
         } break;

         default: {
            used = 0;
            if (is_ctrl && !is_alt) {
               used = 1;
               switch (event->key) {
               case WM_Key_A: {
                  editor->mark = 0;
                  editor->cursor = editor->size;
               } break;

               case WM_Key_C:
               case WM_Key_X: {
                  U64 start = _ED_SelectionMin(editor);
                  U64 end = _ED_SelectionMax(editor);
                  if (start != end) {
                     Core_WM_SetClipboardText(Str8(editor->text + start, end - start));
                     if (event->key == WM_Key_X) {
                        ED_Insert(editor, Str8Zero(), ED_EditKind_Other);
                     }
                  }
               } break;

               case WM_Key_V: {
                  String8 text = _ED_CleanText(arena, Core_WM_GetClipboardText(arena));
                  if (text.size != 0) {
                     ED_Insert(editor, text, ED_EditKind_Other);
                  }
               } break;

               case WM_Key_Z: {
                  if (is_shift) {
                     _ED_StepHistory(editor, editor->redo, &editor->redo_count, editor->undo, &editor->undo_count);
                  } else {
                     _ED_StepHistory(editor, editor->undo, &editor->undo_count, editor->redo, &editor->redo_count);
                  }
               } break;

               case WM_Key_Y: {
                  _ED_StepHistory(editor, editor->redo, &editor->redo_count, editor->undo, &editor->undo_count);
               } break;

               default: {
                  used = 0;
               } break;
               }
            }
         } break;
         }
      }

      if (used) {
         Core_WM_EatEvent(events, event);
         had_input = 1;
      }
   }

   // A drag: the selection follows the mouse for as long as the button that
   // went down in the text stays down, over the box or not.
   if (editor->is_dragging) {
      if (Core_WM_KeyIsDown(WM_Key_LeftMouseButton)) {
         ED_RowArray rows = _ED_Layout(arena, editor, metrics.columns);
         U64         pos = _ED_PosFromPoint(editor, rows, &metrics, mouse);
         if (pos != editor->cursor) {
            editor->cursor = pos;
            editor->scroll_to_cursor = 1;
            had_input = 1;
         }
      } else {
         editor->is_dragging = 0;
      }
   }

   if (had_input) {
      editor->last_input_us = NowTimeUS();
   }
}

////////////////////////////////
//~ bgz: drawing

internal void
ED_Draw(ED_Editor* editor, Arena* arena, ED_Box box) {
   _ED_Metrics metrics = _ED_MetricsFromBox(box);
   ED_RowArray rows = _ED_Layout(arena, editor, metrics.columns);
   F32         view_h = Dim2F32(metrics.text_rect).y;

   //- bgz: scrolling
   U64 cursor_row = _ED_RowFromPos(rows, editor->cursor);
   if (editor->scroll_to_cursor) {
      editor->scroll_to_cursor = 0;
      F32 cursor_top = (F32)cursor_row * metrics.row_h;
      F32 cursor_bottom = cursor_top + metrics.row_h;
      if (cursor_top < editor->scroll_y) {
         editor->scroll_y = cursor_top;
      }
      if (cursor_bottom > editor->scroll_y + view_h) {
         editor->scroll_y = cursor_bottom - view_h;
      }
   }
   // The last row can be scrolled up to the top of the box, no further.
   F32 max_scroll = (F32)(rows.count - 1) * metrics.row_h;
   editor->scroll_y = Clamp(0, editor->scroll_y, max_scroll);
   // Whole pixels, or every row of text is blurred.
   editor->scroll_y = (F32)(S32)editor->scroll_y;

   Core_D_Rect(box.rect, R2F32P(0, 0, 1, 1), V4F32(0.085f, 0.09f, 0.105f, 1.0f));
   Core_D_PushClip(box.rect);

   U64 first_row = (U64)(editor->scroll_y / metrics.row_h);
   U64 row_opl = Min(rows.count, first_row + metrics.rows_visible + 2);

   U64 selection_min = _ED_SelectionMin(editor);
   U64 selection_max = _ED_SelectionMax(editor);

   for (U64 row_idx = first_row; row_idx < row_opl; row_idx += 1) {
      ED_Row row = rows.v[row_idx];
      F32    x = metrics.text_rect.min.x;
      F32    y = metrics.text_rect.min.y + (F32)row_idx * metrics.row_h - editor->scroll_y;

      //- bgz: the cursor's row, lit a little
      if (row_idx == cursor_row && selection_min == selection_max) {
         Core_D_Rect(R2F32P(box.rect.min.x, y, box.rect.max.x, y + metrics.row_h), R2F32P(0, 0, 1, 1),
                     V4F32(1, 1, 1, 0.035f));
      }

      //- bgz: the selection
      if (selection_min != selection_max && selection_min <= row.end && selection_max >= row.start) {
         U64 from = Max(selection_min, row.start);
         U64 to = Min(selection_max, row.end);
         F32 x0 = x + (F32)_ED_ColumnCount(editor, row.start, from) * metrics.char_w;
         F32 x1 = x + (F32)_ED_ColumnCount(editor, row.start, to) * metrics.char_w;
         // A selected line ending shows as half a character past the row.
         if (selection_max > row.end && row.line_end) {
            x1 += metrics.char_w * 0.5f;
         }
         if (x1 > x0) {
            Core_D_Rect(R2F32P(x0, y, x1, y + metrics.row_h), R2F32P(0, 0, 1, 1), V4F32(0.25f, 0.45f, 0.85f, 0.55f));
         }
      }

      //- bgz: the text, colored by what its line starts with
      if (row.end > row.start) {
         U64 line_start = _ED_LineStart(editor, row.start);
         U64 line_end = _ED_LineEnd(editor, row.start);
         U64 first = line_start;
         while (first < line_end && editor->text[first] == ' ') first += 1;
         String8 line = Str8(editor->text + first, line_end - first);

         Vec4F32 color = V4F32(0.84f, 0.86f, 0.90f, 1.0f);
         if (line.size >= 2 && line.str[0] == '#' && line.str[1] != '#') {
            color = V4F32(1.00f, 0.80f, 0.40f, 1.0f); // a new slide
         } else if (line.size >= 3 && line.str[0] == '-' && line.str[1] == '-' && line.str[2] == '-') {
            color = V4F32(1.00f, 0.80f, 0.40f, 1.0f); // a new slide
         } else if (line.size >= 1 && line.str[0] == '#') {
            color = V4F32(0.45f, 0.72f, 1.00f, 1.0f); // a heading
         } else if (line.size >= 2 && line.str[0] == '!' && line.str[1] == '[') {
            color = V4F32(0.55f, 0.85f, 0.60f, 1.0f); // a picture
         } else if (line.size >= 2 && line.str[0] == '@') {
            color = V4F32(0.80f, 0.62f, 0.95f, 1.0f); // @theme, @layout
         }

         TXT_Draw(box.font, box.px, V2F32(x, y), color, Str8(editor->text + row.start, row.end - row.start));
      }

      //- bgz: the cursor: solid just after input, then blinking
      if (row_idx == cursor_row) {
         U64 since_input_us = NowTimeUS() - editor->last_input_us;
         B32 is_on = (since_input_us < 500000) || ((since_input_us / 500000) % 2 == 0);
         if (is_on) {
            F32 cursor_x = x + (F32)_ED_ColumnCount(editor, row.start, editor->cursor) * metrics.char_w;
            F32 cursor_w = Max(box.px * 0.11f, 1.5f);
            Core_D_Rect(R2F32P(cursor_x, y, cursor_x + cursor_w, y + metrics.row_h), R2F32P(0, 0, 1, 1),
                        V4F32(1, 1, 1, 0.95f));
         }
      }
   }

   //- bgz: where in the text the box is, down its right edge
   F32 content_h = (F32)rows.count * metrics.row_h;
   if (content_h > view_h && view_h > 0) {
      F32 total_h = content_h + view_h; // scrolling goes a box past the end
      F32 bar_w = Max(box.px * 0.25f, 3.0f);
      F32 bar_h = Max(view_h * view_h / total_h, bar_w * 4.0f);
      F32 bar_y = box.rect.min.y + (Dim2F32(box.rect).y - bar_h) * (editor->scroll_y / Max(max_scroll, 1.0f));
      Core_D_Rect(R2F32P(box.rect.max.x - bar_w - 2.0f, bar_y, box.rect.max.x - 2.0f, bar_y + bar_h), R2F32P(0, 0, 1, 1),
                  V4F32(1, 1, 1, 0.18f));
   }

   Core_D_PopClip();
}

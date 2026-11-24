////////////////////////////////
//~ bgz: vim
//
// The keys, as the editor project has them. A count before a key (3j) is
// not one of them: Shift and Ctrl with a key are the larger steps.
//
//    command and visual mode
//    h l              a character left and right, within the line (the
//                     arrows too)
//    j k              a line down and up. Shift: 10. Ctrl: 45 (Page Down
//                     and Page Up too)
//    w b              a word on and back, within the line. Shift: 5. Ctrl: 20
//    0                the line's start (Home too). End: its last character
//    gg  G            the first line, and the last
//    v  V             visual mode, and visual mode by whole lines. v again
//                     leaves it
//    i  a  A  o       insert mode: at the cursor, after it, at the line's
//                     end, on a new line under this one
//    Esc              back to command mode
//
//    command mode
//    x                takes out the character under the cursor
//    D  Y             cuts and copies from the cursor to the line's end
//    dd dj dk         cuts the line, and with it the one under or over
//    yy yj yk         copies them
//    p                pastes: whole lines on a new line under this one, anything
//                     else after the cursor
//    P                pastes into the line at the cursor, whatever was copied
//
//    visual mode
//    d x              cuts what is selected
//    c                cuts it, and insert mode
//    y                copies it
//
// What is cut or copied is on the system's clipboard. Whole lines there end
// with a \n, which is how p knows them.
//
// In command mode the cursor is on a character, drawn as a block over it:
// never past a line's last one, but at the start of a line with nothing in
// it. In insert and visual mode it is between characters, as the editor has
// it. What is selected in visual mode is the editor's own selection, mark to
// cursor, so nothing is selected until the cursor moves.
//
// Keys with Ctrl that are not vim's stay the editor's in every mode: undo
// and redo, select all, copy, cut and paste.

#define _VIM_LINES_SHIFT 10
#define _VIM_LINES_CTRL  45
#define _VIM_WORDS_SHIFT 5
#define _VIM_WORDS_CTRL  20

////////////////////////////////
//~ bgz: positions

// The line's last character, where the command mode's cursor stops: the
// line's start, for a line with nothing in it.
PRIVATE U64
_Vim_LineLastChar(Editor_Editor* editor, U64 pos) {
   U64 line_start = Editor_LineStart(editor, pos);
   U64 line_end = Editor_LineEnd(editor, pos);
   U64 result = line_start;
   if (line_end > line_start) {
      result = Editor_PrevChar(editor, line_end);
   }
   return result;
}

// What words are made of, what is between them, and everything else, which
// is a word of its own: in "a->b", w stops at -> and then at b.
typedef enum _Vim_CharClass {
   _Vim_CharClass_Space,
   _Vim_CharClass_Word,
   _Vim_CharClass_Other,
} _Vim_CharClass;

PRIVATE _Vim_CharClass
_Vim_CharClassFromByte(U8 c) {
   _Vim_CharClass result = _Vim_CharClass_Other;
   if (c == ' ' || c == '\n') {
      result = _Vim_CharClass_Space;
   } else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80) {
      result = _Vim_CharClass_Word;
   }
   return result;
}

// The start of the next word in pos's line. With none, the line's last
// character.
PRIVATE U64
_Vim_NextWordStart(Editor_Editor* editor, U64 pos) {
   U64 line_end = Editor_LineEnd(editor, pos);
   U64 scan = pos;

   if (scan < line_end) {
      _Vim_CharClass start_class = _Vim_CharClassFromByte(editor->text[scan]);
      if (start_class != _Vim_CharClass_Space) {
         while (scan < line_end && _Vim_CharClassFromByte(editor->text[scan]) == start_class) scan += 1;
      }
   }
   while (scan < line_end && _Vim_CharClassFromByte(editor->text[scan]) == _Vim_CharClass_Space) scan += 1;

   if (scan >= line_end) {
      scan = _Vim_LineLastChar(editor, pos);
   }
   return scan;
}

// The start of the word before pos in its line - the word pos is in, from
// anywhere but its first letter. With none, the line's start.
PRIVATE U64
_Vim_PrevWordStart(Editor_Editor* editor, U64 pos) {
   U64 line_start = Editor_LineStart(editor, pos);
   U64 scan = pos;

   while (scan > line_start && _Vim_CharClassFromByte(editor->text[scan - 1]) == _Vim_CharClass_Space) scan -= 1;
   if (scan > line_start) {
      _Vim_CharClass class_here = _Vim_CharClassFromByte(editor->text[scan - 1]);
      while (scan > line_start && _Vim_CharClassFromByte(editor->text[scan - 1]) == class_here) scan -= 1;
   }
   return scan;
}

// How many characters there are from the line's start to pos.
PRIVATE U64
_Vim_Column(Editor_Editor* editor, U64 pos) {
   U64 result = 0;
   for (U64 at = Editor_LineStart(editor, pos); at < pos; at = Editor_NextChar(editor, at)) {
      result += 1;
   }
   return result;
}

////////////////////////////////
//~ bgz: the mode and the cursor

// The mode, and how the editor shows it: a block for the cursor in command
// mode, whole lines selected in visual mode by lines.
PRIVATE void
_Vim_SetMode(App_State* app_state, Vim_Mode mode, B32 selects_lines) {
   Vim_State* vim = &app_state->vim;
   vim->mode = mode;
   vim->selects_lines = (mode == Vim_Mode_Visual && selects_lines);
   vim->pending_size = 0;

   Editor_Editor* editor = app_state->editor;
   if (editor != 0) {
      editor->block_cursor = (mode == Vim_Mode_Command);
      editor->selects_lines = vim->selects_lines;
      editor->last_input_us = NowTimeUS();
   }
}

// The cursor to pos. The mark comes along, unless in visual mode, where it
// is the other end of what is selected.
PRIVATE void
_Vim_MoveTo(App_State* app_state, U64 pos) {
   Editor_Editor* editor = app_state->editor;
   U64            mark = editor->mark;
   Editor_SetCursor(editor, pos);
   if (app_state->vim.mode == Vim_Mode_Visual) {
      editor->mark = Min(mark, editor->size);
   }
}

// Command mode's cursor, back onto a character if it is past its line's
// last one. Nothing while something is selected.
PRIVATE void
_Vim_KeepOnChar(App_State* app_state) {
   Editor_Editor* editor = app_state->editor;
   if (app_state->vim.mode == Vim_Mode_Command && editor->cursor == editor->mark) {
      U64 last = _Vim_LineLastChar(editor, editor->cursor);
      if (editor->cursor > last) {
         S64 want_column = editor->want_column;
         Editor_SetCursor(editor, last);
         editor->want_column = want_column;
      }
   }
}

// Down by line_delta lines, or up, keeping to the column the cursor was in
// before the first such move - through lines too short to have it.
PRIVATE void
_Vim_MoveLines(App_State* app_state, S64 line_delta) {
   Editor_Editor* editor = app_state->editor;
   if (editor->want_column < 0) {
      editor->want_column = (S64)_Vim_Column(editor, editor->cursor);
   }
   S64 want_column = editor->want_column;

   U64 line_start = Editor_LineStart(editor, editor->cursor);
   for (S64 i = 0; i < line_delta; i += 1) {
      U64 line_end = Editor_LineEnd(editor, line_start);
      if (line_end >= editor->size) break;
      line_start = line_end + 1;
   }
   for (S64 i = 0; i > line_delta; i -= 1) {
      if (line_start == 0) break;
      line_start = Editor_LineStart(editor, line_start - 1);
   }

   // Visual mode's cursor may be past the last character, at the line's end.
   U64 stop = (app_state->vim.mode == Vim_Mode_Visual) ? Editor_LineEnd(editor, line_start) : _Vim_LineLastChar(editor, line_start);
   U64 pos = line_start;
   for (S64 column = 0; column < want_column && pos < stop; column += 1) {
      pos = Editor_NextChar(editor, pos);
   }

   _Vim_MoveTo(app_state, pos);
   editor->want_column = want_column;
}

String8
Vim_ModeName(const App_State* app_state) {
   String8 result = Str8Lit("COMMAND");
   if (app_state->vim.mode == Vim_Mode_Insert) {
      result = Str8Lit("INSERT");
   } else if (app_state->vim.mode == Vim_Mode_Visual) {
      result = app_state->vim.selects_lines ? Str8Lit("VISUAL LINE") : Str8Lit("VISUAL");
   }
   return result;
}

String8
Vim_ModeTag(const App_State* app_state) {
   String8 result = Str8Lit("vim command");
   if (app_state->vim.mode == Vim_Mode_Insert) {
      result = Str8Lit("vim insert");
   } else if (app_state->vim.mode == Vim_Mode_Visual) {
      result = Str8Lit("vim visual");
   }
   return result;
}

B32
Vim_WantsEsc(const App_State* app_state) {
   B32 result = (app_state->editor != 0 && (app_state->vim.mode != Vim_Mode_Command || app_state->vim.pending_size != 0));
   return result;
}

void
Vim_EditorOpened(App_State* app_state) {
   _Vim_SetMode(app_state, Vim_Mode_Command, 0);
}

void
Vim_StartTyping(App_State* app_state) {
   if (app_state->vim.mode != Vim_Mode_Insert) {
      _Vim_SetMode(app_state, Vim_Mode_Insert, 0);
   }
}

void
Vim_AfterTextMouse(App_State* app_state) {
   Editor_Editor* editor = app_state->editor;
   if (editor != 0) {
      B32 has_selection = (editor->cursor != editor->mark);
      if (app_state->vim.mode == Vim_Mode_Command && has_selection) {
         _Vim_SetMode(app_state, Vim_Mode_Visual, 0);
      } else if (app_state->vim.mode == Vim_Mode_Visual && !has_selection) {
         _Vim_SetMode(app_state, Vim_Mode_Command, 0);
      }
      _Vim_KeepOnChar(app_state);
   }
}

////////////////////////////////
//~ bgz: cut, copy and paste

// The text from start to end onto the clipboard - as whole lines, with a \n
// after it, which is how a paste knows them: so a line with nothing in it
// is still something to copy.
PRIVATE void
_Vim_CopyRange(App_State* app_state, U64 start, U64 end, B32 is_lines) {
   Editor_Editor* editor = app_state->editor;
   if (end > start || (end == start && is_lines)) {
      U64 size = end - start;
      U8* text = PushArrayNoZero(app_state->frame_arena, U8, size + 1);
      MemoryCopy(text, editor->text + start, size);
      if (is_lines) {
         text[size] = '\n';
         size += 1;
      }
      Core_WM_SetClipboardText(Str8(text, size));
   }
}

// dd, dj, dk and yy, yj, yk: the cursor's line, and with line_offset the one
// under it (+1) or over it (-1), copied - and with is_cut, taken out.
PRIVATE void
_Vim_Lines(App_State* app_state, S64 line_offset, B32 is_cut) {
   Editor_Editor* editor = app_state->editor;
   if (editor->size == 0) {
      return;
   }

   U64 start = Editor_LineStart(editor, editor->cursor);
   U64 end = Editor_LineEnd(editor, editor->cursor);
   // On the last line dj is dd, and on the first so is dk.
   if (line_offset > 0 && end < editor->size) {
      end = Editor_LineEnd(editor, end + 1);
   }
   if (line_offset < 0 && start > 0) {
      start = Editor_LineStart(editor, start - 1);
   }

   _Vim_CopyRange(app_state, start, end, 1);

   if (is_cut) {
      U64 cursor_after = start;
      if (end < editor->size) {
         // The \n after them too, so the lines go and are not left empty.
         end += 1;
      } else if (start > 0) {
         // The text's last lines have none after them: the one before goes.
         start -= 1;
         cursor_after = Editor_LineStart(editor, start);
      }
      Editor_ReplaceRange(editor, start, end, Str8Zero());
      Editor_SetCursor(editor, cursor_after);
   }
}

// D and Y: from the cursor to the end of its line.
PRIVATE void
_Vim_ToLineEnd(App_State* app_state, B32 is_cut) {
   Editor_Editor* editor = app_state->editor;
   U64            start = editor->cursor;
   U64            end = Editor_LineEnd(editor, start);
   if (start < end) {
      _Vim_CopyRange(app_state, start, end, 0);
      if (is_cut) {
         Editor_ReplaceRange(editor, start, end, Str8Zero());
         // Back onto what is now the line's last character.
         Editor_SetCursor(editor, _Vim_LineLastChar(editor, start));
      }
   }
}

// What is selected in visual mode, copied - and with is_cut, taken out.
// Whole lines are copied as dd copies them. Returns where the cursor goes.
PRIVATE U64
_Vim_Selection(App_State* app_state, B32 is_cut) {
   Editor_Editor* editor = app_state->editor;
   U64            start = Editor_SelectionMin(editor);
   U64            end = Editor_SelectionMax(editor);
   U64            result = start;

   if (editor->selects_lines) {
      // The selection has the \n after its last line, for taking the lines
      // out. The copy has the lines, and one \n after them.
      U64 last_line_end = Editor_LineEnd(editor, Max(editor->cursor, editor->mark));
      _Vim_CopyRange(app_state, start, last_line_end, 1);
      if (is_cut && end == last_line_end && start > 0) {
         // The text's last lines have none after them: the one before goes.
         start -= 1;
         result = Editor_LineStart(editor, start);
      }
   } else {
      _Vim_CopyRange(app_state, start, end, 0);
   }

   if (is_cut && end > start) {
      Editor_ReplaceRange(editor, start, end, Str8Zero());
   }
   return result;
}

// p and P. What the clipboard has that ends in a \n is whole lines, and p
// puts them on a new line under the cursor's; anything else goes in after
// the character the cursor is on. P (in_line) puts either into the line at
// the cursor, without the \n.
PRIVATE void
_Vim_Paste(App_State* app_state, B32 in_line) {
   Editor_Editor* editor = app_state->editor;
   String8        text = Editor_CleanText(app_state->frame_arena, Core_WM_GetClipboardText(app_state->frame_arena));

   if (in_line && text.size != 0 && text.str[text.size - 1] == '\n') {
      text.size -= 1;
   }
   if (text.size == 0) {
      return;
   }

   U64 line_end = Editor_LineEnd(editor, editor->cursor);
   B32 is_lines = (!in_line && text.str[text.size - 1] == '\n');
   if (is_lines) {
      U64 at = line_end;
      if (line_end < editor->size) {
         at = line_end + 1;
      } else {
         // The text's last line has no \n to go after: the one at the end
         // of what is pasted is put at its start instead.
         MemoryCopy(text.str + 1, text.str, text.size - 1);
         text.str[0] = '\n';
      }
      Editor_ReplaceRange(editor, at, at, text);
      Editor_SetCursor(editor, (at == line_end) ? at + 1 : at);
   } else {
      U64 at = editor->cursor;
      if (!in_line && at < line_end) {
         at = Editor_NextChar(editor, at);
      }
      Editor_ReplaceRange(editor, at, at, text);
      // On the last character pasted.
      Editor_SetCursor(editor, Editor_PrevChar(editor, at + text.size));
   }
}

////////////////////////////////
//~ bgz: state ops

PRIVATE S64
_Vim_Step(B32 is_shift, B32 is_ctrl, S64 bare, S64 with_shift, S64 with_ctrl) {
   // Ctrl first: with both held it is the larger step.
   S64 result = is_ctrl ? with_ctrl : is_shift ? with_shift : bare;
   return result;
}

// A key press in command or visual mode. Returns whether it was vim's.
PRIVATE B32
_Vim_Press(App_State* app_state, WM_Event* event) {
   Vim_State*     vim = &app_state->vim;
   Editor_Editor* editor = app_state->editor;
   B32            is_ctrl = !!(event->modifiers & WM_Modifier_Ctrl);
   B32            is_shift = !!(event->modifiers & WM_Modifier_Shift);
   B32            is_visual = (vim->mode == Vim_Mode_Visual);
   B32            used = 1;

   U64 line_start = Editor_LineStart(editor, editor->cursor);
   U64 line_end = Editor_LineEnd(editor, editor->cursor);

   // A key that Ctrl makes the editor's, or the edit screen's, and no step
   // of vim's.
   B32 is_step_key = (event->key == WM_Key_J || event->key == WM_Key_K || event->key == WM_Key_W || event->key == WM_Key_B);
   if (is_ctrl && !is_step_key) {
      // But not its keys that change the text.
      B32 changes_text = (event->key == WM_Key_Backspace || event->key == WM_Key_Delete || event->key == WM_Key_Return ||
                          event->key == WM_Key_Tab);
      return changes_text;
   }

   switch (event->key) {
   case WM_Key_H:
   case WM_Key_Left: {
      if (editor->cursor > line_start) {
         _Vim_MoveTo(app_state, Editor_PrevChar(editor, editor->cursor));
      }
   } break;

   case WM_Key_L:
   case WM_Key_Right: {
      U64 stop = is_visual ? line_end : _Vim_LineLastChar(editor, editor->cursor);
      if (editor->cursor < stop) {
         _Vim_MoveTo(app_state, Editor_NextChar(editor, editor->cursor));
      }
   } break;

   case WM_Key_J:
   case WM_Key_Down: {
      _Vim_MoveLines(app_state, +_Vim_Step(is_shift, is_ctrl, 1, _VIM_LINES_SHIFT, _VIM_LINES_CTRL));
   } break;

   case WM_Key_K:
   case WM_Key_Up: {
      _Vim_MoveLines(app_state, -_Vim_Step(is_shift, is_ctrl, 1, _VIM_LINES_SHIFT, _VIM_LINES_CTRL));
   } break;

   case WM_Key_PageDown: {
      _Vim_MoveLines(app_state, +_VIM_LINES_CTRL);
   } break;

   case WM_Key_PageUp: {
      _Vim_MoveLines(app_state, -_VIM_LINES_CTRL);
   } break;

   case WM_Key_W: {
      S64 steps = _Vim_Step(is_shift, is_ctrl, 1, _VIM_WORDS_SHIFT, _VIM_WORDS_CTRL);
      U64 pos = editor->cursor;
      for (S64 i = 0; i < steps; i += 1) pos = _Vim_NextWordStart(editor, pos);
      _Vim_MoveTo(app_state, pos);
   } break;

   case WM_Key_B: {
      S64 steps = _Vim_Step(is_shift, is_ctrl, 1, _VIM_WORDS_SHIFT, _VIM_WORDS_CTRL);
      U64 pos = editor->cursor;
      for (S64 i = 0; i < steps; i += 1) pos = _Vim_PrevWordStart(editor, pos);
      _Vim_MoveTo(app_state, pos);
   } break;

   case WM_Key_0:
   case WM_Key_Home: {
      // Shift+0 is a ), which is no key of vim's.
      if (!is_shift || event->key == WM_Key_Home) {
         _Vim_MoveTo(app_state, line_start);
      }
   } break;

   case WM_Key_End: {
      _Vim_MoveTo(app_state, is_visual ? line_end : _Vim_LineLastChar(editor, editor->cursor));
   } break;

   case WM_Key_G: {
      // G is the last line. A g alone is the start of gg: typing's (Vim_KeyInput).
      if (is_shift) {
         _Vim_MoveTo(app_state, Editor_LineStart(editor, editor->size));
      }
   } break;

   case WM_Key_V: {
      if (is_shift) {
         _Vim_SetMode(app_state, Vim_Mode_Visual, 1);
      } else if (is_visual) {
         Editor_SetCursor(editor, editor->cursor);
         _Vim_SetMode(app_state, Vim_Mode_Command, 0);
      } else {
         _Vim_SetMode(app_state, Vim_Mode_Visual, 0);
      }
   } break;

   case WM_Key_I: {
      Editor_SetCursor(editor, editor->cursor);
      _Vim_SetMode(app_state, Vim_Mode_Insert, 0);
   } break;

   case WM_Key_A: {
      // After the character the cursor is on - or with Shift, the line's last.
      U64 pos = is_shift ? line_end : editor->cursor;
      if (!is_shift && !is_visual && pos < line_end) {
         pos = Editor_NextChar(editor, pos);
      }
      Editor_SetCursor(editor, pos);
      _Vim_SetMode(app_state, Vim_Mode_Insert, 0);
   } break;

   case WM_Key_O: {
      Editor_ReplaceRange(editor, line_end, line_end, Str8Lit("\n"));
      _Vim_SetMode(app_state, Vim_Mode_Insert, 0);
   } break;

   case WM_Key_X:
   case WM_Key_D:
   case WM_Key_C: {
      if (is_visual) {
         U64 pos = _Vim_Selection(app_state, 1);
         Editor_SetCursor(editor, pos);
         _Vim_SetMode(app_state, (event->key == WM_Key_C) ? Vim_Mode_Insert : Vim_Mode_Command, 0);
      } else if (event->key == WM_Key_X) {
         // Not the \n: x on a line with nothing in it does not join lines.
         if (editor->cursor < line_end) {
            U64 at = editor->cursor;
            Editor_ReplaceRange(editor, at, Editor_NextChar(editor, at), Str8Zero());
            // What came after it is under the cursor now - or it was the
            // line's last, and the cursor steps back.
            Editor_SetCursor(editor, Min(at, _Vim_LineLastChar(editor, at)));
         }
      } else if (event->key == WM_Key_D && is_shift) {
         _Vim_ToLineEnd(app_state, 1);
      }
      // A d alone is the start of dd, dj, dk: typing's (Vim_KeyInput). A c
      // alone is nothing.
   } break;

   case WM_Key_Y: {
      if (is_visual) {
         U64 pos = _Vim_Selection(app_state, 0);
         Editor_SetCursor(editor, pos);
         _Vim_SetMode(app_state, Vim_Mode_Command, 0);
      } else if (is_shift) {
         _Vim_ToLineEnd(app_state, 0);
      }
      // A y alone is the start of yy, yj, yk.
   } break;

   case WM_Key_P: {
      if (!is_visual) {
         _Vim_Paste(app_state, is_shift);
      }
   } break;

   // The editor's keys that change the text: not in these modes.
   case WM_Key_Backspace:
   case WM_Key_Delete:
   case WM_Key_Return:
   case WM_Key_Tab: {
   } break;

   default: {
      used = 0;
   } break;
   }

   return used;
}

// Something typed in command mode: the start of a command of more than one
// key, or the rest of one.
PRIVATE void
_Vim_Typed(App_State* app_state, U32 character) {
   Vim_State* vim = &app_state->vim;

   B32 starts_command = (character == 'd' || character == 'y' || character == 'g');
   if (vim->pending_size == 0 && !starts_command) {
      return;
   }
   if (vim->pending_size == ArrayCount(vim->pending) || character > 127) {
      vim->pending_size = 0;
      return;
   }

   vim->pending[vim->pending_size] = (U8)character;
   vim->pending_size += 1;
   String8 command = Str8(vim->pending, vim->pending_size);
   if (command.size == 1) {
      return;
   }

   // Run if it is one, and dropped whole if it is not: its last key is not
   // tried again by itself.
   vim->pending_size = 0;
   if (Str8Match(command, Str8Lit("gg"), 0)) {
      _Vim_MoveTo(app_state, 0);
   } else if (Str8Match(command, Str8Lit("dd"), 0)) {
      _Vim_Lines(app_state, 0, 1);
   } else if (Str8Match(command, Str8Lit("dj"), 0)) {
      _Vim_Lines(app_state, +1, 1);
   } else if (Str8Match(command, Str8Lit("dk"), 0)) {
      _Vim_Lines(app_state, -1, 1);
   } else if (Str8Match(command, Str8Lit("yy"), 0)) {
      _Vim_Lines(app_state, 0, 0);
   } else if (Str8Match(command, Str8Lit("yj"), 0)) {
      _Vim_Lines(app_state, +1, 0);
   } else if (Str8Match(command, Str8Lit("yk"), 0)) {
      _Vim_Lines(app_state, -1, 0);
   }
}

void
Vim_KeyInput(App_State* app_state, StateOp_List* ops, Vim_KeyInputOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   Vim_State* vim = &app_state->vim;

   U64 cursor_before = editor->cursor;
   U64 mark_before = editor->mark;
   U64 edit_count_before = editor->edit_count;
   B32 had_input = 0;

   // The key that began insert mode types its letter after it, in the same
   // frame: that letter is not for the text.
   B32 drops_text = 0;

   WM_Event* next_event = 0;
   for (WM_Event* event = app_state->events->first; event != 0; event = next_event) {
      next_event = event->next;
      B32 used = 0;
      B32 is_ctrl = !!(event->modifiers & WM_Modifier_Ctrl);
      B32 is_alt = !!(event->modifiers & WM_Modifier_Alt);

      if (event->kind == WM_EventKind_WindowLoseFocus) {
         vim->pending_size = 0;
      }

      if (vim->mode == Vim_Mode_Insert) {
         used = (drops_text && event->kind == WM_EventKind_Text);
      } else if (event->kind == WM_EventKind_Text) {
         // Nothing is typed into the text in these modes.
         used = 1;
         if (vim->mode == Vim_Mode_Command && !is_ctrl && !is_alt) {
            _Vim_Typed(app_state, event->character);
         }
      } else if (event->kind == WM_EventKind_Press && !is_alt && event->key < WM_Key_LeftMouseButton) {
         if (vim->pending_size != 0 && !is_ctrl) {
            // The rest of a command is read as it is typed, and its key
            // does nothing of its own: the j of dj does not move the cursor.
            used = 1;
         } else {
            vim->pending_size = 0;
            used = _Vim_Press(app_state, event);
            drops_text |= (used && vim->mode == Vim_Mode_Insert);
         }
      }

      if (used) {
         Core_WM_EatEvent(app_state->events, event);
         had_input = 1;
      }
   }

   //- bgz: the rest is the editor's: all of it in insert mode, and in the
   // others what vim has no use for (undo, copy, paste)
   Editor_Input(editor, app_state->frame_arena, app_state->events, params->box);

   // Only if something happened, since it may move the cursor: pieces of the slide stay picked for as long
   // as the cursor is where it was (edit\edit.h).
   B32 changed = (editor->cursor != cursor_before || editor->mark != mark_before || editor->edit_count != edit_count_before);
   if (changed || had_input) {
      if (vim->mode == Vim_Mode_Command && editor->cursor != editor->mark) {
         // Something selected with the editor's keys (Ctrl+A).
         _Vim_SetMode(app_state, Vim_Mode_Visual, 0);
      }
      _Vim_KeepOnChar(app_state);
   }
   if (had_input) {
      editor->last_input_us = NowTimeUS();
   }
}
STATE_OP(Vim_KeyInput)

void
Vim_Escape(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   Vim_State*     vim = &app_state->vim;
   if (editor == 0) {
      return;
   }

   if (vim->mode == Vim_Mode_Insert) {
      // Back onto the character just typed, but not off the line.
      U64 pos = Min(editor->cursor, editor->mark);
      if (pos > Editor_LineStart(editor, pos)) {
         pos = Editor_PrevChar(editor, pos);
      }
      Editor_SetCursor(editor, pos);
      _Vim_SetMode(app_state, Vim_Mode_Command, 0);
      _Vim_KeepOnChar(app_state);
   } else if (vim->mode == Vim_Mode_Visual) {
      Editor_SetCursor(editor, editor->cursor);
      _Vim_SetMode(app_state, Vim_Mode_Command, 0);
      _Vim_KeepOnChar(app_state);
   }
   vim->pending_size = 0;
}
STATE_OP_NO_PARAMS(Vim_Escape)

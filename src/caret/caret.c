////////////////////////////////
//~ bgz: the edit screen: typing into the slide
//
// The words of the slide in the preview are typed into where they are. A
// click on a title or a block of words puts the cursor at the letter
// clicked, and a caret shows there, on the slide. What is typed goes in at
// it - the keys are the editor's, as ever (editor.c): the slide's words and
// the text on the left are the same text, with the one cursor, and the
// caret on the slide is only where that cursor is among the words as drawn.
//
//    put the caret    click in the words, or in the room under them: it
//                     goes to the nearest letter. Shift+click selects from
//                     where it was
//    select           drag across the words (place\place.c). A double
//                     click takes a word, a triple click its line. Past
//                     the top or bottom of a block with more words than
//                     room, its words move on under the mouse
//    see the box     the block the caret is in has a thin blue box around
//                     the room its words have. They wrap at its width
//    more words       than the room is tall: they stay the size they are,
//                     and the room shows the part of them the caret is in,
//                     moving down as the typing does. The mouse's wheel,
//                     over the block, moves it too
//
// A letter is found on the slide, and a point on the slide among the
// letters, by the runs the view gives (View_Run): each line or piece of
// words as drawn, with where in the text its letters are. Some of the text
// is not drawn - the "- " of a bullet, the ** of bold, an @ line - and a
// cursor there has its caret at the nearest letter that is, or none.
//
// How far down a block's words its room shows (Deck_Box's scroll) is not in
// the deck's text: it is kept here, for the slide showing, and put on the
// slide only while the preview draws it. Presenting shows every block from
// its top.

// The caret: where the cursor is on the slide.
typedef struct _Caret_Place _Caret_Place;
struct _Caret_Place {
   B32            is_found;
   Rng2F32        rect; // no width: the line it is on, down
   Deck_PlaceKind kind; // whose words it is in
   U64            idx;
};

// Whether the runs kept from the last frame are of the text as it is.
PRIVATE B32
_Caret_RunsAreGood(const App_State* app_state) {
   return Place_BoxesAreGood(app_state);
}

// How far into the run the text position pos is, across: pos is in it.
PRIVATE F32
_Caret_XInRun(String8 text, View_Run* run, U64 pos) {
   String8 before = Str8Substr(text, R1U64(run->text_range.min, Min(pos, run->text_range.max)));
   return run->rect.min.x + Text_Width(run->font, run->px, before);
}

// Where on the slide the text position pos is, by runs. Not found when the
// line pos is on has nothing drawn and is not an empty line of a block.
PRIVATE _Caret_Place
_Caret_PlaceFromPos(const App_State* app_state, View_Run* runs, U64 run_count, U64 pos) {
   _Caret_Place result = {0};
   String8      text = Editor_Text(app_state->editor);
   pos = Min(pos, text.size);

   U64 line_start = pos;
   while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
   U64 line_end = pos;
   while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;

   // Of the runs of this line of the text: the last that starts at or
   // before pos, and the first of them all.
   View_Run* at = 0;
   View_Run* first = 0;
   View_Run* above = 0; // the last run of the lines before this one
   for (U64 i = 0; i < run_count; i += 1) {
      View_Run* run = &runs[i];
      if (run->text_range.min >= line_start && run->text_range.max <= line_end) {
         if (first == 0 || run->text_range.min < first->text_range.min) {
            first = run;
         }
         if (run->text_range.min <= pos && (at == 0 || run->text_range.min >= at->text_range.min)) {
            at = run;
         }
      } else if (run->text_range.max < line_start && (above == 0 || run->text_range.max >= above->text_range.max)) {
         above = run;
      }
   }

   if (at != 0) {
      F32 x = _Caret_XInRun(text, at, pos);
      if (pos > at->text_range.max) {
         // Past its last letter: in the spaces after it, or in the ** or `
         // marks around bold words and code, which are not drawn. Spaces
         // typed at the end of a line are not in any run either: the caret
         // moves on by each space all the same, or it would stand still
         // while they are typed - and stays put across a mark.
         U64 spaces = 0;
         B32 is_gap = 1;
         for (U64 i = at->text_range.max; i < pos && is_gap; i += 1) {
            spaces += (text.str[i] == ' ');
            is_gap = (text.str[i] == ' ' || text.str[i] == '*' || text.str[i] == '`');
         }
         if (is_gap) {
            x += Text_Width(at->font, at->px, Str8Lit(" ")) * (F32)spaces;
         }
      }
      result.is_found = 1;
      result.rect = R2F32P(x, at->rect.min.y, x, at->rect.max.y);
      result.kind = at->kind;
      result.idx = at->idx;
   } else if (first != 0) {
      // Before the line's first letter: in its "- ", or its "## ".
      result.is_found = 1;
      result.rect = R2F32P(first->rect.min.x, first->rect.min.y, first->rect.min.x, first->rect.max.y);
      result.kind = first->kind;
      result.idx = first->idx;
   } else if (above != 0 && above->kind == Deck_PlaceKind_Text) {
      // A line with nothing on it, in a block: a line being started. The
      // caret is under the block's last words before it, at the block's
      // left - if the cursor is in that block, and not in what follows it.
      B32 is_blank = 1;
      for (U64 i = line_start; i < line_end && is_blank; i += 1) {
         is_blank = (text.str[i] == ' ' || text.str[i] == '\t');
      }
      U64            slide_idx = 0;
      Deck_PlaceKind kind = Deck_PlaceKind_Title;
      U64            idx = 0;
      if (is_blank && Look_Target(app_state, &slide_idx, &kind, &idx) && kind == Deck_PlaceKind_Text && idx == above->idx) {
         F32 left = above->rect.min.x;
         for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
            View_Box* box = &g_edit_view.boxes[i];
            if (box->kind == Deck_PlaceKind_Text && box->idx == idx) {
               // Where its first letter will be: in the middle or at the
               // right, when the slide or the block's @align puts every
               // line there.
               Deck_Slide* on_slide = &app_state->deck->slides[app_state->slide_idx];
               Deck_Align  align = (idx < on_slide->box_count) ? Deck_AlignOf(on_slide, &on_slide->boxes[idx].look) : Deck_Align_Left;
               F32         across = (align == Deck_Align_Right) ? 1.0f : (align == Deck_Align_Center) ? 0.5f : 0.0f;
               left = box->room.min.x + (box->room.max.x - box->room.min.x) * across;
            }
         }
         F32 line_h = above->rect.max.y - above->rect.min.y;
         F32 top = above->rect.max.y + line_h * 0.3f;
         result.is_found = 1;
         result.rect = R2F32P(left, top, left, top + line_h);
         result.kind = kind;
         result.idx = idx;
      }
   }
   return result;
}

// The text position of the letter nearest to x in the run: before it or
// after it, whichever edge is nearer.
PRIVATE U64
_Caret_PosInRun(String8 text, View_Run* run, F32 x) {
   U64 result = run->text_range.min;
   if (x >= run->rect.max.x) {
      result = run->text_range.max;
   } else if (x > run->rect.min.x) {
      F32 before_x = run->rect.min.x;
      for (U64 pos = run->text_range.min; pos < Min(run->text_range.max, text.size);) {
         // A letter at a time: all the bytes of one that has several.
         U64 next = pos + 1;
         while (next < run->text_range.max && next < text.size && (text.str[next] & 0xC0) == 0x80) next += 1;
         F32 after_x = _Caret_XInRun(text, run, next);
         if (x < after_x) {
            result = (x - before_x < after_x - x) ? pos : next;
            break;
         }
         before_x = after_x;
         result = next;
         pos = next;
      }
   }
   return result;
}

// The box of the title's or of a block's words that pos is in the room of:
// 0 for none, or when what is there is a picture. As a press there would
// pick (Place_BoxAt), and past that the room under a block's words.
PRIVATE View_Box*
_Caret_WordsBoxAt(const App_State* app_state, Vec2F32 pos) {
   View_Box* result = Place_BoxAt(app_state, pos);
   if (result == 0) {
      for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
         View_Box* box = &g_edit_view.boxes[i];
         if (box->kind != Deck_PlaceKind_Picture && Contains2F32(box->room, pos)) {
            result = box;
         }
      }
   }
   if (result != 0 && result->kind == Deck_PlaceKind_Picture) {
      result = 0;
   }
   return result;
}

// The cursor to the letter of box's words nearest to pos, which need not be
// in the box. keeps_mark: the mark stays where it is, so what is between it
// and the cursor is selected. whole_lines: only of the lines that show whole,
// if any do.
PRIVATE B32
_Caret_ClickIn(App_State* app_state, View_Box* box, Vec2F32 pos, B32 keeps_mark, B32 whole_lines) {
   B32 result = 0;
   if (box != 0) {
      // The nearest of its runs that show: first by how far up or down the
      // point is from the run's line, then by how far across.
      View_Run* nearest = 0;
      F32       nearest_far = 0;
      // Twice through, the first time without the lines cut by the room's
      // edge, when whole_lines asks for that and some are whole.
      for (U64 k = 0; k < g_edit_view.run_count * 2; k += 1) {
         if (k == g_edit_view.run_count && nearest != 0) {
            break;
         }
         View_Run* run = &g_edit_view.runs[k % g_edit_view.run_count];
         B32       is_whole = (run->rect.min.y >= box->room.min.y - 1.0f && run->rect.max.y <= box->room.max.y + 1.0f);
         if (run->kind != box->kind || run->idx != box->idx || run->rect.max.y <= box->room.min.y ||
             run->rect.min.y >= box->room.max.y || (k < g_edit_view.run_count && whole_lines && !is_whole)) {
            continue;
         }
         F32 down = (pos.y < run->rect.min.y) ? run->rect.min.y - pos.y : (pos.y >= run->rect.max.y) ? pos.y - run->rect.max.y : 0;
         F32 across = (pos.x < run->rect.min.x) ? run->rect.min.x - pos.x : (pos.x > run->rect.max.x) ? pos.x - run->rect.max.x : 0;
         F32 how_far = down * 4096.0f + across;
         // Of two as near - one line's last piece and the next's first -
         // the later: a point under all the words goes to their end.
         if (nearest == 0 || how_far <= nearest_far) {
            nearest = run;
            nearest_far = how_far;
         }
      }
      Editor_Editor* editor = app_state->editor;
      if (nearest != 0 && nearest->text_range.max <= editor->size) {
         U64 mark = editor->mark;
         Editor_SetCursor(editor, _Caret_PosInRun(Editor_Text(editor), nearest, pos.x));
         if (keeps_mark) {
            editor->mark = mark;
         }
         editor->last_input_us = NowTimeUS();
         g_caret_view.focus_hidden = 0;
         // In the slide's words, to type: not vim's command mode.
         Vim_StartTyping(app_state);
         result = 1;
      }
   }
   return result;
}

B32
Caret_Click(App_State* app_state, Vec2F32 pos, B32 keeps_mark) {
   if (!_Caret_RunsAreGood(app_state) || !Contains2F32(g_edit_view.page, pos)) {
      return 0;
   }
   return _Caret_ClickIn(app_state, _Caret_WordsBoxAt(app_state, pos), pos, keeps_mark, 0);
}

// The mouse held down from a press at the words of the title or of a block
// (kind, idx), and now at pos: what is between where it went down and here
// is selected, as in the text pane, to be copied, cut or typed over. It
// stays with the words it began in, wherever the mouse goes: past the end of
// a line is the line's end, off the box is the nearest of its lines. Past
// the top or bottom of a block with more words than room, the words move
// under the mouse, faster the further it is.
//
// How fast is by the time since the last frame, not a step each frame: at
// a step a frame it went with the screen's refresh, and a twentieth of a
// notch a frame at the edge was some sixteen lines a second at 60 - a block
// of fifteen lines went past in under a second, too fast to stop on a line.
// Now at the edge three of its lines a second, and ten more for each line
// height further out, as far as forty.
B32
Caret_DragTo(const App_State* app_state, Deck_PlaceKind kind, U64 idx, Vec2F32 pos) {
   B32 result = 0;
   if (_Caret_RunsAreGood(app_state)) {
      View_Box* box = 0;
      for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
         if (g_edit_view.boxes[i].kind == kind && g_edit_view.boxes[i].idx == idx && kind != Deck_PlaceKind_Picture) {
            box = &g_edit_view.boxes[i];
         }
      }
      if (box != 0) {
         F32 scale = View_ScaleFromDim(Dim2F32(g_edit_view.page));
         B32 is_past = 0;
         if (kind == Deck_PlaceKind_Text && scale > 0) {
            F32 past = (pos.y < box->room.min.y) ? pos.y - box->room.min.y : (pos.y > box->room.max.y) ? pos.y - box->room.max.y : 0;
            is_past = (past != 0);
            if (is_past) {
               // Its lines' height: the first run of its words.
               F32 line_h = 0;
               for (U64 i = 0; i < g_edit_view.run_count && line_h <= 0; i += 1) {
                  View_Run* run = &g_edit_view.runs[i];
                  if (run->kind == kind && run->idx == idx) {
                     line_h = run->rect.max.y - run->rect.min.y;
                  }
               }
               line_h = (line_h > 0) ? line_h : 30.0f * scale;
               F32 lines_per_s = Min(3.0f + 10.0f * AbsF32(past) / line_h, 40.0f);
               F32 pixels = lines_per_s * line_h * UI_Dt();
               // In notches of the wheel, which Caret_ScrollBlock moves 96
               // page units each.
               Caret_ScrollBlock(app_state, idx, ((past < 0) ? -pixels : pixels) / scale / 96.0f);
               result = 1;
            }
         }
         // Past the room, to the line at its edge that shows whole. To one
         // cut by the edge, the room moved on to show the caret's line
         // (Caret_Follow), and the cursor went to the next cut one: the
         // words ran on a line every few frames, whatever the speed above.
         Caret_DragSelectOpParams* select = StateOp_Caret_DragSelect(app_state->op_list);
         select->kind = kind;
         select->idx = idx;
         select->pos = pos;
         select->whole_lines = is_past;
      }
   }
   return result;
}

////////////////////////////////
//~ bgz: state ops
//
// What the mouse does to the cursor and the selection, from the runs as the
// preview last laid them out: nothing when the text has changed since.

// A press in the room of the title's or a block's words: the cursor to the
// nearest letter.
void
Caret_PressRoom(App_State* app_state, StateOp_List* ops, Caret_PressRoomOpParams* params) {
   UnusedVariable(ops);
   // The cursor is in the words now: no piece is picked whole. Said so
   // here: with the cursor already where the click puts it, nothing else
   // would show that they are not (Edit_PickedIsGood).
   g_edit_view.picked_count = 0;
   Caret_Click(app_state, params->pos, params->keeps_mark);
}
STATE_OP(Caret_PressRoom)

// The mouse held from a press at a piece's words, now at pos: the cursor to
// the letter there, the mark left where it is (Caret_DragTo).
void
Caret_DragSelect(App_State* app_state, StateOp_List* ops, Caret_DragSelectOpParams* params) {
   UnusedVariable(ops);
   if (_Caret_RunsAreGood(app_state) && params->kind != Deck_PlaceKind_Picture) {
      View_Box* box = 0;
      for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
         if (g_edit_view.boxes[i].kind == params->kind && g_edit_view.boxes[i].idx == params->idx) {
            box = &g_edit_view.boxes[i];
         }
      }
      _Caret_ClickIn(app_state, box, params->pos, 1, params->whole_lines);
   }
}
STATE_OP(Caret_DragSelect)

// 1 for a byte of a word: a letter, a digit, or part of a letter of several
// bytes. What a double click takes in.
PRIVATE B32
_Caret_IsWordByte(U8 byte) {
   return ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') || (byte >= '0' && byte <= '9') || byte == '_' || byte == '\'' ||
           byte >= 0x80);
}

// A double click: the word the cursor is in or beside is selected.
void
Caret_SelectWord(App_State* app_state, StateOp_List* ops) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   String8 text = Editor_Text(editor);
   U64        min = Min(editor->cursor, text.size);
   U64        max = min;
   while (min > 0 && _Caret_IsWordByte(text.str[min - 1])) min -= 1;
   while (max < text.size && _Caret_IsWordByte(text.str[max])) max += 1;
   if (max > min) {
      Editor_SetCursor(editor, max);
      editor->mark = min;
      editor->last_input_us = NowTimeUS();
   }
}
STATE_OP_NO_PARAMS(Caret_SelectWord)

// A triple click: the words of the line of text the cursor is in - a
// paragraph, an item of a list, the title - as the slide has them, so
// without the "- " or "# " they start with in the text.
void
Caret_SelectLine(App_State* app_state, StateOp_List* ops, Caret_SelectLineOpParams* params) {
   UnusedVariable(ops);
   if (!_Caret_RunsAreGood(app_state)) {
      return;
   }
   Editor_Editor* editor = app_state->editor;
   String8        text = Editor_Text(editor);
   Deck_PlaceKind kind = params->kind;
   U64            idx = params->idx;
   U64 at = Min(editor->cursor, text.size);
   U64 line_min = at;
   U64 line_max = at;
   while (line_min > 0 && text.str[line_min - 1] != '\n') line_min -= 1;
   while (line_max < text.size && text.str[line_max] != '\n' && text.str[line_max] != '\r') line_max += 1;
   B32 has_any = 0;
   U64 min = 0;
   U64 max = 0;
   for (U64 i = 0; i < g_edit_view.run_count; i += 1) {
      View_Run* run = &g_edit_view.runs[i];
      if (run->kind != kind || run->idx != idx || run->text_range.min < line_min || run->text_range.max > line_max) {
         continue;
      }
      min = has_any ? Min(min, run->text_range.min) : run->text_range.min;
      max = has_any ? Max(max, run->text_range.max) : run->text_range.max;
      has_any = 1;
   }
   if (has_any && max > min) {
      Editor_SetCursor(editor, max);
      editor->mark = min;
      editor->last_input_us = NowTimeUS();
   }
}
STATE_OP(Caret_SelectLine)

// The scroll of the block a box is of: its own if it is placed, the one
// the others share if not.
PRIVATE F32*
_Caret_ScrollOf(Deck_Slide* slide, U64 block_idx) {
   F32* result = &g_caret_view.body_scroll;
   if (block_idx < slide->box_count && slide->boxes[block_idx].place.is_set) {
      result = &g_caret_view.scroll[block_idx];
   }
   return result;
}

void
Caret_ScrollToTop(void) {
   MemoryZeroArray(g_caret_view.scroll);
   g_caret_view.body_scroll = 0;
}

void
Caret_ScrollBlock(const App_State* app_state, U64 block_idx, F32 delta) {
   if (_Caret_RunsAreGood(app_state)) {
      // Some three lines of the body a notch (down the words when over 0).
      // Kept to the words there are when the slide is next laid out
      // (Caret_Follow).
      Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
      F32*        scroll = _Caret_ScrollOf(slide, block_idx);
      *scroll = Max(*scroll + delta * 96.0f, 0.0f);
   }
}

// The most a box's block can be moved down its words, in page units: none
// when they fit. scale is pixels to a page unit.
PRIVATE F32
_Caret_MaxScroll(View_Box* box, F32 scale) {
   F32 track_h = box->bar_track.max.y - box->bar_track.min.y;
   return (scale > 0 && box->words_height > track_h) ? (box->words_height - track_h) / scale : 0.0f;
}

// The scroll of box's block for its bar's thumb with its top grab pixels
// above mouse_y.
PRIVATE void
_Caret_DragBar(const App_State* app_state, View_Box* box, F32 mouse_y) {
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   F32         scale = View_ScaleFromDim(Dim2F32(g_edit_view.page));
   F32         track_h = box->bar_track.max.y - box->bar_track.min.y;
   F32         thumb_h = box->bar_thumb.max.y - box->bar_thumb.min.y;
   F32         part = (track_h > thumb_h) ? (mouse_y - g_caret_view.bar_grab - box->bar_track.min.y) / (track_h - thumb_h) : 0.0f;
   *_Caret_ScrollOf(slide, box->idx) = Clamp(0.0f, part, 1.0f) * _Caret_MaxScroll(box, scale);
}

void
Caret_BeforeLayout(const App_State* app_state) {
   if (!g_caret_view.bar_drag) {
      return;
   }
   // Held while the box pressed - its thumb, or its track - is the UI's
   // active one: the bar's track does not move as its thumb does, and its
   // box is there every frame the block has more words than room. Its
   // place is the last layout's, which is where the mouse was taken to it.
   View_Box* box = 0;
   for (U64 i = 0; i < g_edit_view.box_count && app_state->slide_idx < app_state->deck->slide_count; i += 1) {
      View_Box* candidate = &g_edit_view.boxes[i];
      if (candidate->kind == Deck_PlaceKind_Text && candidate->idx == g_caret_view.bar_block && candidate->words_height > 0) {
         box = candidate;
      }
   }
   if (box == 0 || app_state->show_open || !UI_KeyMatch(UI_ActiveKey(UI_MouseButtonKind_Left), g_caret_view.bar_key)) {
      g_caret_view.bar_drag = 0;
   } else {
      // The window's mouse: the UI's is off the window when the window is
      // not in front and the mouse has not moved for a moment.
      _Caret_DragBar(app_state, box, Core_WM_MouseFromWindow(app_state->window).y);
   }
}

// A bar's thumb, as the UI draws its own scroll bars' (_UI_DrawScrollThumb,
// ui_basic_widgets.c, which is private to the UI): a rounded bar against the
// right edge, thin, and the bar's whole width under the mouse or held. In the
// box's background colour.
PRIVATE void
_Caret_DrawThumb(UI_Box* box, void* user_data) {
   UnusedVariable(user_data);
   Rng2F32 rect = box->rect;
   F32     full_w = rect.x1 - rect.x0;
   F32     w = Max(2.0f, (4.0f + (full_w - 4.0f) * Max(box->hot_t, box->active_t)) - 2.0f);
   F32     x1 = rect.x1 - 1.0f;

   R_Rect2DInst* inst = Core_D_Rect(R2F32P(x1 - w, rect.y0 + 2.0f, x1, rect.y1 - 2.0f), R2F32(V2F32(0, 0), V2F32(1, 1)), box->background_color);

   for EachEnumVal(Corner, corner) {
      inst->corner_radii[corner] = w * 0.5f;
   }
}

B32
Caret_BuildBars(const App_State* app_state, F32 ui, B32 interactive) {
   B32 result = 0;
   if (!_Caret_RunsAreGood(app_state)) {
      return 0;
   }
   UI_BoxFlags flags = interactive ? (UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll) : 0;
   // The UI's own scroll bar's look (_Caret_DrawThumb): a thin thumb
   // with no track, the bar's whole width under the mouse or held. In the
   // colour of the slide's words, the one colour sure to show on its theme.
   Vec4F32 color = View_ColorsFromTheme(app_state->deck->theme).body;
   color.w = 0.55f;
   F32 bar_w = 10.0f * ui;

   for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
      View_Box* box = &g_edit_view.boxes[i];
      if (box->kind != Deck_PlaceKind_Text || box->words_height <= 0) {
         continue;
      }
      // The thumb is drawn against the bar's right edge: there, where the
      // view puts it, a gap from the block's room.
      Rng2F32 bar = R2F32P(box->bar_track.max.x + 1.0f - bar_w, box->bar_track.min.y, box->bar_track.max.x + 1.0f,
                           box->bar_track.max.y);
      Rng2F32 thumb_rect = R2F32P(bar.min.x, box->bar_thumb.min.y, bar.max.x, box->bar_thumb.max.y);

      // The thumb first, over the track and asked first.
      UI_Box* thumb = FR_BuildRectBox(flags, thumb_rect, "###caret thumb %llu", (unsigned long long)box->idx);
      thumb->background_color = color;
      UI_BoxEquipCustomDraw(thumb, _Caret_DrawThumb, 0);
      UI_Box* track = FR_BuildRectBox(flags, bar, "###caret bar %llu", (unsigned long long)box->idx);
      if (!interactive) {
         continue;
      }

      //- bgz: a press takes hold of the bar: of its thumb where the mouse
      // is, or, on the track beside the thumb, of the thumb's middle - which
      // then comes to the mouse
      UI_Signal thumb_signal = UI_SignalFromBox(thumb);
      UI_Signal track_signal = UI_SignalFromBox(track);
      UI_Box*   pressed = (thumb_signal.f & UI_SignalFlag_LeftPressed) ? thumb : (track_signal.f & UI_SignalFlag_LeftPressed) ? track : 0;
      if (pressed != 0) {
         Vec2F32 pos = UI_DragStartMouse();
         g_caret_view.bar_drag = 1;
         g_caret_view.bar_block = box->idx;
         g_caret_view.bar_key = pressed->key;
         g_caret_view.bar_grab = (pressed == thumb) ? pos.y - box->bar_thumb.min.y : (box->bar_thumb.max.y - box->bar_thumb.min.y) * 0.5f;
         _Caret_DragBar(app_state, box, pos.y);
         result = 1;
      }
      S16 scroll = thumb_signal.scroll.y + track_signal.scroll.y;
      if (scroll != 0) {
         Caret_ScrollBlock(app_state, box->idx, (F32)scroll / FR_UI_SCROLL_PER_NOTCH);
         result = 1;
      }
   }
   return result;
}

B32
Caret_BuildRooms(const App_State* app_state, F32 ui, B32 interactive) {
   B32 result = 0;
   if (!interactive || !_Caret_RunsAreGood(app_state)) {
      return 0;
   }
   // Under the pieces' own boxes, which are built before: the room under a
   // block's words, and beside the title's.
   for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
      View_Box* box = &g_edit_view.boxes[i];
      if (box->kind == Deck_PlaceKind_Picture) {
         continue;
      }
      // A block's, no further down than its words: a block with no @place
      // line shares the body's room, and the last of them has all of it
      // down to the bottom margin - where a click then went into the block,
      // far under its words, when it was meant for the slide's open space,
      // to let go of what is picked. As the box drawn around it
      // (Caret_BuildLook). A placed block's box is its room.
      Rng2F32 room_rect = box->room;
      if (box->kind == Deck_PlaceKind_Text) {
         room_rect.max.y = Min(box->room.max.y, Max(box->rect.max.y, room_rect.min.y));
      }
      UI_Box*   room = FR_BuildRectBox(UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll, Intersect2F32(room_rect, g_edit_view.page),
                                       "###caret room %d %llu", (int)box->kind, (unsigned long long)box->idx);
      // A click here goes to the nearest letter, but the mouse is the bar
      // that text is typed at only right at the words (place\place.c) - and
      // while words are selected by a drag from there, wherever it goes.
      B32 is_selecting = (g_place_view.drag.state == Place_DragState_Selecting);
      room->hover_cursor = (is_selecting || Place_IsAtWords(box->kind, box->idx, Core_WM_MouseFromWindow(app_state->window)))
                               ? WM_Cursor_IBar
                               : WM_Cursor_Pointer;
      UI_Signal signal = UI_SignalFromBox(room);
      // The room reaches past the words' own boxes: beside the title's
      // words, and a little to the left of a block's first letters. A press
      // there that is still at the words selects, as one on them does.
      if ((signal.f & UI_SignalFlag_LeftPressed) && !Place_PressWords(app_state, signal, box->kind, box->idx, ui)) {
         g_place_view.click_count = 0;
         Caret_PressRoomOpParams* press = StateOp_Caret_PressRoom(app_state->op_list);
         press->pos = UI_DragStartMouse();
         press->keeps_mark = !!(signal.event_flags & WM_Modifier_Shift);
      }
      result |= !!(signal.f & UI_SignalFlag_LeftPressed);
      result |= Place_SelectHeld(app_state, signal);
      if (signal.scroll.y != 0 && box->kind == Deck_PlaceKind_Text) {
         Caret_ScrollBlock(app_state, box->idx, (F32)signal.scroll.y / FR_UI_SCROLL_PER_NOTCH);
         result = 1;
      }
   }
   return result;
}

void
Caret_PresentAfterDraw(const App_State* app_state, View_HitList* hits) {
   Caret_AfterDrawOf(app_state, app_state->slide_idx);
   MemoryCopyArray(g_caret_view.present_boxes, hits->boxes);
   g_caret_view.present_box_count = hits->box_count;
   g_caret_view.present_page = hits->page;
}

B32
Caret_PresentWheel(const App_State* app_state, Vec2F32 pos, F32 delta) {
   B32 result = 0;
   if (app_state->slide_idx >= app_state->deck->slide_count || g_caret_view.scroll_slide != app_state->slide_idx) {
      return 0;
   }
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   F32         scale = View_ScaleFromDim(Dim2F32(g_caret_view.present_page));
   for (U64 i = 0; i < g_caret_view.present_box_count && !result; i += 1) {
      View_Box* box = &g_caret_view.present_boxes[i];
      if (box->kind != Deck_PlaceKind_Text || box->idx >= slide->box_count || box->words_height <= 0) {
         continue;
      }
      // Over the room the bar is of, or the bar: the track is as tall as
      // that room, which for blocks that share one is the body's.
      Rng2F32 over = R2F32P(box->room.min.x, box->bar_track.min.y, box->bar_track.max.x, box->bar_track.max.y);
      if (Contains2F32(over, pos)) {
         F32* scroll = _Caret_ScrollOf(slide, box->idx);
         *scroll = Clamp(0.0f, *scroll + delta * 96.0f, _Caret_MaxScroll(box, scale));
         result = 1;
      }
   }
   return result;
}

// Scrolls put on a slide's blocks: the shared one, and one for each placed
// block. 0 for scrolls takes them all off.
PRIVATE void
_Caret_PutScroll(Deck_Slide* slide, F32 body_scroll, F32* scrolls) {
   slide->body_scroll = (scrolls != 0) ? body_scroll : 0;
   for (U64 i = 0; i < slide->box_count; i += 1) {
      slide->boxes[i].scroll = (scrolls != 0 && slide->boxes[i].place.is_set) ? scrolls[i] : 0;
   }
}

void
Caret_BeforeDraw(const App_State* app_state) {
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   // Another slide starts from its top. The one left keeps how far down it
   // was, for as long as it fades out (Caret_BeforeDrawOf).
   if (g_caret_view.scroll_slide != app_state->slide_idx) {
      g_caret_view.left_slide = g_caret_view.scroll_slide;
      MemoryCopyArray(g_caret_view.left_scroll, g_caret_view.scroll);
      g_caret_view.left_body_scroll = g_caret_view.body_scroll;
      g_caret_view.scroll_slide = app_state->slide_idx;
      MemoryZeroArray(g_caret_view.scroll);
      g_caret_view.body_scroll = 0;
   }
   _Caret_PutScroll(slide, g_caret_view.body_scroll, g_caret_view.scroll);
}

void
Caret_BeforeDrawOf(const App_State* app_state, U64 slide_idx) {
   if (slide_idx < app_state->deck->slide_count) {
      Deck_Slide* slide = &app_state->deck->slides[slide_idx];
      if (slide_idx == g_caret_view.scroll_slide) {
         _Caret_PutScroll(slide, g_caret_view.body_scroll, g_caret_view.scroll);
      } else if (slide_idx == g_caret_view.left_slide && slide_idx != app_state->slide_idx) {
         // Not if it has just been come back to: that starts from its top.
         _Caret_PutScroll(slide, g_caret_view.left_body_scroll, g_caret_view.left_scroll);
      }
   }
}

void
Caret_AfterDrawOf(const App_State* app_state, U64 slide_idx) {
   if (slide_idx < app_state->deck->slide_count) {
      _Caret_PutScroll(&app_state->deck->slides[slide_idx], 0, 0);
   }
}

// The runs of a hit list, as an array on arena.
PRIVATE View_Run*
_Caret_RunsFromHits(Arena* arena, View_HitList* hits, U64* count_out) {
   View_Run* result = PushArray(arena, View_Run, Max(hits->run_count, 1));
   U64       count = 0;
   for (View_Run* run = hits->first_run; run != 0; run = run->next) {
      result[count] = *run;
      result[count].next = 0;
      count += 1;
   }
   *count_out = count;
   return result;
}

// The caret among runs, and the box of the piece the cursor is in: the
// caret's, or with no caret - the cursor on an @ line - the piece that line
// is about. 0 for none.
// Whether the cursor is on the line of one of the slide's pictures: the
// line of its path, ![](name).
PRIVATE B32
_Caret_CursorIsOnPictureLine(const App_State* app_state) {
   B32        result = 0;
   Deck_Deck* deck = app_state->deck;
   Editor_Editor* editor = app_state->editor;
   if (editor != 0 && app_state->slide_idx < deck->slide_count) {
      String8 text = Editor_Text(editor);
      U64     cursor = Min(editor->cursor, text.size);
      U64     line_start = cursor;
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 line_end = cursor;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;

      Deck_Slide* slide = &deck->slides[app_state->slide_idx];
      for (U64 i = 0; i < slide->picture_count && !result; i += 1) {
         Rng1U64 range = slide->pictures[i].range;
         result = (range.max != 0 && range.min >= line_start && range.min <= line_end);
      }
   }
   return result;
}

PRIVATE View_Box*
_Caret_BoxOfCursor(const App_State* app_state, View_Box* boxes, U64 box_count, View_Run* runs, U64 run_count, _Caret_Place* caret_out) {
   *caret_out = _Caret_PlaceFromPos(app_state, runs, run_count, app_state->editor->cursor);
   Deck_PlaceKind kind = caret_out->kind;
   U64            idx = caret_out->idx;
   B32            has_piece = caret_out->is_found;
   // A picture's line is the picture's, not the words': the edit screen
   // puts the box around the picture (Edit_Build). Without this, the line
   // of a picture just clicked or dropped would have the box around the
   // title or a block of words instead - the piece Look_Target picks for
   // a line that is not about words - and the picture none.
   if (!has_piece && !_Caret_CursorIsOnPictureLine(app_state)) {
      U64 slide_idx = 0;
      has_piece = (Look_Target(app_state, &slide_idx, &kind, &idx) && slide_idx == app_state->slide_idx);
   }
   View_Box* result = 0;
   for (U64 i = 0; i < box_count && has_piece; i += 1) {
      if (boxes[i].kind == kind && boxes[i].idx == idx) {
         result = &boxes[i];
      }
   }
   return result;
}

B32
Caret_Follow(const App_State* app_state, View_HitList* hits) {
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   Editor_Editor*  editor = app_state->editor;
   // Of the cursor, which is in the text as it is: nothing if the slide
   // drawn is of the text as it was.
   if (editor->edit_count != app_state->parsed_edit_count) {
      return 0;
   }

   Temp         scratch = ScratchBegin(0, 0);
   U64          run_count = 0;
   View_Run*    runs = _Caret_RunsFromHits(scratch.arena, hits, &run_count);
   _Caret_Place caret = {0};
   View_Box*    box = _Caret_BoxOfCursor(app_state, hits->boxes, hits->box_count, runs, run_count, &caret);
   Deck_PlaceKind kind = caret.kind;
   U64            idx = caret.idx;
   F32            scale = View_ScaleFromDim(Dim2F32(hits->page));
   F32            body_scroll_was = g_caret_view.body_scroll;
   F32            scroll_was[DECK_MAX_BOXES];
   MemoryCopyArray(scroll_was, g_caret_view.scroll);

   //- bgz: the room follows the caret, when the cursor has gone somewhere
   // or the text has changed - not all the time: the wheel moves it too
   B32 has_moved = (editor->cursor != g_caret_view.seen_cursor || editor->edit_count != g_caret_view.seen_edit_count);
   g_caret_view.seen_cursor = editor->cursor;
   g_caret_view.seen_edit_count = editor->edit_count;
   if (has_moved && caret.is_found && box != 0 && kind == Deck_PlaceKind_Text && scale > 0) {
      F32* scroll = _Caret_ScrollOf(slide, idx);
      // The room of the blocks that share one is the body's: from the first
      // of them that shows to the bottom of the last.
      Rng2F32 room = box->room;
      if (!slide->boxes[idx].place.is_set) {
         for (U64 i = 0; i < hits->box_count; i += 1) {
            View_Box* other = &hits->boxes[i];
            if (other->kind == Deck_PlaceKind_Text && other->idx < slide->box_count && !slide->boxes[other->idx].place.is_set) {
               room.min.y = Min(room.min.y, other->room.min.y);
               room.max.y = Max(room.max.y, other->room.max.y);
            }
         }
      }
      if (caret.rect.max.y > room.max.y) {
         *scroll += (caret.rect.max.y - room.max.y) / scale;
      } else if (caret.rect.min.y < room.min.y) {
         *scroll = Max(*scroll - (room.min.y - caret.rect.min.y) / scale, 0.0f);
      }
   }

   //- bgz: no further down than the words go: a block whose words end
   // above its room's bottom is moved back down to them, as words are
   // deleted, or after the wheel. The caret on an empty line under the
   // words counts as words.
   for (U64 i = 0; i < hits->box_count && scale > 0; i += 1) {
      View_Box* text_box = &hits->boxes[i];
      if (text_box->kind != Deck_PlaceKind_Text || text_box->idx >= slide->box_count) {
         continue;
      }
      B32 is_placed = slide->boxes[text_box->idx].place.is_set;
      // Of the blocks that share a room, the last is the one that ends it.
      B32 is_last_shared = !is_placed;
      for (U64 k = i + 1; k < hits->box_count && is_last_shared; k += 1) {
         View_Box* later = &hits->boxes[k];
         if (later->kind == Deck_PlaceKind_Text && later->idx < slide->box_count && !slide->boxes[later->idx].place.is_set) {
            is_last_shared = 0;
         }
      }
      if (is_placed || is_last_shared) {
         F32* scroll = _Caret_ScrollOf(slide, text_box->idx);
         F32  words_max_y = text_box->words_max_y;
         if (caret.is_found && kind == Deck_PlaceKind_Text && idx == text_box->idx) {
            words_max_y = Max(words_max_y, caret.rect.max.y);
         }
         F32 empty = text_box->room.max.y - words_max_y;
         if (*scroll > 0 && empty > 0) {
            *scroll = Max(*scroll - empty / scale, 0.0f);
         }
      }
   }
   ScratchEnd(scratch);

   B32 result = (g_caret_view.body_scroll != body_scroll_was || !MemoryMatchArray(g_caret_view.scroll, scroll_was));
   return result;
}

void
Caret_KeepRuns(View_HitList* hits) {
   // On the arena the hits are kept on, which the layout has just cleared.
   g_edit_view.runs = _Caret_RunsFromHits(g_edit_view.hits_arena, hits, &g_edit_view.run_count);
}

B32
Caret_BuildLook(const App_State* app_state, F32 ui) {
   Deck_Deck*  deck = app_state->deck;
   Editor_Editor*  editor = app_state->editor;
   if (!_Caret_RunsAreGood(app_state) || editor->edit_count != app_state->parsed_edit_count) {
      return 0;
   }
   Deck_Slide*    slide = &deck->slides[app_state->slide_idx];
   _Caret_Place   caret = {0};
   View_Box*      box = _Caret_BoxOfCursor(app_state, g_edit_view.boxes, g_edit_view.box_count, g_edit_view.runs,
                                           g_edit_view.run_count, &caret);
   Deck_PlaceKind kind = caret.kind;
   U64            idx = caret.idx;
   if (box != 0) {
      kind = box->kind;
      idx = box->idx;
   }

   //- bgz: the box and the caret. Not after Esc, until the cursor goes
   // somewhere or a piece is clicked.
   if (g_caret_view.focus_hidden && editor->cursor != g_caret_view.focus_hidden_cursor) {
      g_caret_view.focus_hidden = 0;
   }
   B32 result = 0;
   if (box != 0 && kind != Deck_PlaceKind_Picture && !g_caret_view.focus_hidden) {
      result = 1;

      //- bgz: the caret, on top: solid just after input, then blinking, as
      // the editor's is. In the colour of the words, which shows on the theme.
      // Not in a box picked whole (edit\edit.c): Delete takes all of it out,
      // not a letter at the caret.
      if (caret.is_found && caret.rect.min.y >= box->room.min.y - 1.0f && caret.rect.max.y <= box->room.max.y + 1.0f &&
          !Edit_PieceIsPicked(app_state, kind, idx)) {
         U64 since_input_us = NowTimeUS() - editor->last_input_us;
         B32 is_on = (since_input_us < 500000) || ((since_input_us / 500000) % 2 == 0);
         if (is_on) {
            View_Colors colors = View_ColorsFromTheme(deck->theme);
            Vec4F32     color = (kind == Deck_PlaceKind_Title) ? colors.title : colors.body;
            Deck_Look*  look = Look_Of(slide, kind, idx);
            if (look->has_color) {
               color = look->color;
            }
            F32 width = Max((F32)(S32)(1.5f * ui), 1.0f);
            F32 x = (F32)(S32)caret.rect.min.x;
            UI_SetNextBackgroundColor(color);
            FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(x, caret.rect.min.y, x + width, caret.rect.max.y), "###caret caret");
         }
      }

      UI_ColorTag(Str8Lit("focus")) {
         //- bgz: what is selected, washed in the blue of the box: the part of
         // each run that is in the selection, where the run shows
         String8 text = Editor_Text(editor);
         U64     select_min = Min(editor->cursor, editor->mark);
         U64     select_max = Max(editor->cursor, editor->mark);
         for (U64 i = 0; i < g_edit_view.run_count && select_min != select_max; i += 1) {
            View_Run* run = &g_edit_view.runs[i];
            U64       from = Max(run->text_range.min, select_min);
            U64       to = Min(run->text_range.max, select_max);
            if (from < to) {
               Rng2F32 washed = R2F32P(_Caret_XInRun(text, run, from), run->rect.min.y, _Caret_XInRun(text, run, to), run->rect.max.y);
               // On to the next word of the line, if that is selected too:
               // the space between them, so the wash is not in pieces.
               View_Run* next = (i + 1 < g_edit_view.run_count) ? &g_edit_view.runs[i + 1] : 0;
               if (next != 0 && to == run->text_range.max && select_max > next->text_range.min && next->rect.min.y == run->rect.min.y &&
                   next->kind == run->kind && next->idx == run->idx && next->rect.min.x > washed.max.x) {
                  washed.max.x = next->rect.min.x;
               }
               for (U64 k = 0; k < g_edit_view.box_count; k += 1) {
                  if (g_edit_view.boxes[k].kind == run->kind && g_edit_view.boxes[k].idx == run->idx) {
                     washed = Intersect2F32(washed, g_edit_view.boxes[k].room);
                  }
               }
               if (washed.max.x > washed.min.x && washed.max.y > washed.min.y) {
                  FR_BuildRectBox(UI_BoxFlag_DrawBackground, washed, "###caret wash %llu", (unsigned long long)i);
               }
            }
         }

         //- bgz: a thin box around the room its words have, in a blue that
         // is the same on every theme: it is the editor's mark, not part of
         // the slide. Just outside the room, where the words wrap - and so
         // beside, not a little way off, the outline the mouse's box has
         // (place\place.c), which is just inside it: one line, not two side
         // by side.
         F32     thick = Max(1.0f, (F32)(S32)(1.5f * ui));
         //
         // A block with no @place line shares the body's room, and the last
         // of them has all of it down to the bottom margin: around that the
         // box ran to the bottom of the slide under a line or two of words.
         // So as tall as the words (the block's own box) - and the caret's
         // line, on a new line under them - and no taller than the room. A
         // placed block's box is its room: nothing changes for it.
         Rng2F32 around = box->room;
         if (kind == Deck_PlaceKind_Text) {
            around.max.y = Min(box->room.max.y, Max(box->rect.max.y, caret.is_found ? caret.rect.max.y : 0.0f));
         }
         Rng2F32 rect = Intersect2F32(Pad2F32(around, thick), g_edit_view.page);
         if (rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
            FR_BuildEdge(rect, thick, UI_ColorFromSlot(UI_ColorSlot_Border), "###caret focus");
         }
      }
   }
   return result;
}

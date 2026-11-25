////////////////////////////////
//~ bgz: the edit screen: the size and the colour of a slide's words
//
// The title's words, and those of each block of the body, can be given a
// size and a colour of their own. Which words: the ones the cursor is in -
// the title when it is on the title's line, else the block of the body it
// is in. That is the piece with the thin blue box around it in the preview.
//
//    larger           Ctrl+]   a tenth larger than they are drawn now
//    smaller          Ctrl+[   a tenth smaller
//    against the left Ctrl+Shift+L   an "@align left" line
//    in the middle    Ctrl+Shift+E   "@align center"
//    against the right Ctrl+Shift+R  "@align right"
//    the Format panel its @size line has smaller, larger and usual to
//                     click, and its @color line the colours with names
//                     (deck.h) and the theme's
//
// What comes of it is a line of the deck's text, as with a piece moved by
// the mouse (place\place.c): "@size title 1.21", "@color orange"
// (deck.h), over the one the piece has or as a new line beside it. The text
// is still all there is to a deck: the line can be changed by hand - any
// size, any #rrggbb - and Ctrl+Z takes a change back. Usual and the theme's
// take the line out again.
//
// With words selected in a block, all of these are of the lines the
// selection is in, each its own ("the lines selected", below). A right-click
// on words selected has them in a menu (edit\edit.c).
//
// Larger and smaller go from the size the words are drawn at, which the
// preview knows (View_Box's text_size).

// How much one step of larger or smaller is.
#define _LOOK_STEP 1.1f

// The piece the cursor is in: 0 if there is none whose words can be sized -
// no slides, or a slide with neither a title nor a body. The deck has to be
// of the text as it is: FR_SyncDeck first.
B32
Look_Target(const App_State* app_state, U64* slide_idx_out, Deck_PlaceKind* kind_out, U64* idx_out) {
   Deck_Deck* deck = app_state->deck;
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || deck->slide_count == 0) {
      return 0;
   }
   String8     text = Editor_Text(editor);
   U64         cursor = Min(editor->cursor, text.size);
   U64         slide_idx = Deck_SlideFromTextPos(deck, cursor);
   Deck_Slide* slide = &deck->slides[slide_idx];

   // The line the cursor is on.
   U64 line_start = cursor;
   while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
   U64 line_end = cursor;
   while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
#define _Look_OnLine(pos) ((pos) >= line_start && (pos) <= line_end)

   // The title's: its own line, and the lines that are about it.
   B32 has_title = (slide->title_range.max != 0);
   B32 on_title = has_title && (_Look_OnLine(slide->title_range.min) ||
                                (slide->title_look.has_size && _Look_OnLine(slide->title_look.size_line.min)) ||
                                (slide->title_look.has_color && _Look_OnLine(slide->title_look.color_line.min)) ||
                                (slide->places[Deck_PlaceKind_Title].has_line &&
                                 _Look_OnLine(slide->places[Deck_PlaceKind_Title].line.min)));
#undef _Look_OnLine

   // Else the block the cursor is in: the last that begins at or before it.
   U64 block_idx = 0;
   for (U64 i = 1; i < slide->box_count; i += 1) {
      if (slide->boxes[i].text_pos <= cursor) {
         block_idx = i;
      }
   }
   B32 has_block = (slide->box_count != 0 && slide->boxes[block_idx].item_count != 0);

   B32 result = 1;
   *slide_idx_out = slide_idx;
   if (on_title || (!has_block && has_title)) {
      *kind_out = Deck_PlaceKind_Title;
      *idx_out = 0;
   } else if (has_block) {
      *kind_out = Deck_PlaceKind_Text;
      *idx_out = block_idx;
   } else {
      result = 0;
   }
   return result;
}

Deck_Look*
Look_Of(Deck_Slide* slide, Deck_PlaceKind kind, U64 idx) {
   Deck_Look* result = &slide->title_look;
   if (kind == Deck_PlaceKind_Text && idx < slide->box_count) {
      result = &slide->boxes[idx].look;
   }
   return result;
}

// Writes the piece's @size line, or with has_size 0 takes it out.
PRIVATE void
_Look_WriteSize(Editor_Editor* editor, Deck_Slide* slide, Deck_PlaceKind kind, U64 idx, B32 has_size, F32 size) {
   Deck_Look* look = Look_Of(slide, kind, idx);
   if (has_size) {
      Temp    scratch = ScratchBegin(0, 0);
      // A block's without saying whose: an @size line is its block's words'
      // unless it says title.
      String8 line = (kind == Deck_PlaceKind_Title) ? PushStr8F(scratch.arena, "@size title %.2f", size)
                                                    : PushStr8F(scratch.arena, "@size %.2f", size);
      Edit_PutLineFor(editor, slide, kind, idx, look->has_size, look->size_line, line);
      ScratchEnd(scratch);
   } else if (look->has_size) {
      Edit_TakeOutLine(editor, look->size_line);
   }
}

// Writes the piece's @color line, or with nothing for a colour takes it out.
PRIVATE void
_Look_WriteColor(Editor_Editor* editor, Deck_Slide* slide, Deck_PlaceKind kind, U64 idx, String8 color) {
   Deck_Look* look = Look_Of(slide, kind, idx);
   if (color.size != 0) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 line = (kind == Deck_PlaceKind_Title) ? PushStr8F(scratch.arena, "@color title %S", color)
                                                    : PushStr8F(scratch.arena, "@color %S", color);
      Edit_PutLineFor(editor, slide, kind, idx, look->has_color, look->color_line, line);
      ScratchEnd(scratch);
   } else if (look->has_color) {
      Edit_TakeOutLine(editor, look->color_line);
   }
}

// Writes the piece's @align line, or with align Deck_Align_COUNT takes it
// out.
PRIVATE void
_Look_WriteAlign(Editor_Editor* editor, Deck_Slide* slide, Deck_PlaceKind kind, U64 idx, U64 align) {
   Deck_Look* look = Look_Of(slide, kind, idx);
   if (align < Deck_Align_COUNT) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 name = Deck_AlignName((Deck_Align)align);
      String8 line = (kind == Deck_PlaceKind_Title) ? PushStr8F(scratch.arena, "@align title %S", name)
                                                    : PushStr8F(scratch.arena, "@align %S", name);
      Edit_PutLineFor(editor, slide, kind, idx, look->has_align, look->align_line, line);
      ScratchEnd(scratch);
   } else if (look->has_align) {
      Edit_TakeOutLine(editor, look->align_line);
   }
}

// The size a step from size is: direction over 0 larger, under 0 smaller.
// To two places, as it is written, and never the same as it was.
PRIVATE F32
_Look_SteppedSize(F32 size, S32 direction) {
   F32 result = (direction > 0) ? size * _LOOK_STEP : size / _LOOK_STEP;
   result = (F32)(S32)(result * 100.0f + 0.5f) / 100.0f;
   result = Clamp(DECK_SIZE_MIN, result, DECK_SIZE_MAX);
   return result;
}

// The size the piece's words are drawn at, as a part of their usual size:
// what the preview drew them at, if it is of the text as it is and of this
// slide, else what the piece's @size line says, else 1.
PRIVATE F32
_Look_SizeNow(const App_State* app_state, U64 slide_idx, Deck_PlaceKind kind, U64 idx) {
   Deck_Look* look = Look_Of(&app_state->deck->slides[slide_idx], kind, idx);
   F32        result = look->has_size ? look->size : 1.0f;
   if (!look->has_size && Place_BoxesAreGood(app_state) && slide_idx == app_state->slide_idx) {
      for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
         View_Box* box = &g_edit_view.boxes[i];
         if (box->kind == kind && box->idx == idx && box->text_size > 0) {
            result = box->text_size;
         }
      }
   }
   return result;
}

////////////////////////////////
//~ bgz: the lines selected
//
// With words selected in a block, a change of look is of the lines the
// selection is in and not of the block: each gets a line of its own above
// it, "@size line 1.5" (deck.h). Not when the selection is in every line
// of the block: that is the block, and is written as the block's - a text
// box just drawn has its one line selected, to be typed over.

#define _LOOK_MAX_LINES 64

// The lines of the slide's idx-th block the selection is in, in the order
// of the text, into items: how many. 0 with nothing selected, for a title,
// and when they are all of the block's.
PRIVATE U64
_Look_SelectedItems(const App_State* app_state, Deck_Slide* slide, Deck_PlaceKind kind, U64 idx, Deck_Item** items) {
   Editor_Editor* editor = app_state->editor;
   U64            count = 0;
   if (editor->cursor == editor->mark || kind != Deck_PlaceKind_Text || idx >= slide->box_count) {
      return 0;
   }
   String8   text = Editor_Text(editor);
   U64       sel_min = Min(editor->cursor, editor->mark);
   U64       sel_max = Max(editor->cursor, editor->mark);
   Deck_Box* block = &slide->boxes[idx];
   Deck_Item* item = block->first_item;
   for (U64 i = 0; i < block->item_count && item != 0; i += 1, item = item->next) {
      U64 line_start = Min(item->text_range.min, text.size);
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 line_end = Min(item->text_range.min, text.size);
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
      // Not a line the selection only ends at the start of: whole lines
      // selected reach to the start of the one after.
      if (sel_min <= line_end && sel_max > line_start && count < _LOOK_MAX_LINES) {
         items[count] = item;
         count += 1;
      }
   }
   return (count == block->item_count) ? 0 : count;
}

// Puts string where [start, end) of the text was, and moves the two
// positions with it: the cursor and the mark, which the editor is given
// back when all the lines are written, so that what was selected still is.
PRIVATE void
_Look_Replace(Editor_Editor* editor, U64 start, U64 end, String8 string, U64* cursor, U64* mark) {
   U64* points[2] = {cursor, mark};
   for (U64 i = 0; i < ArrayCount(points); i += 1) {
      if (*points[i] >= end) {
         *points[i] = *points[i] - (end - start) + string.size;
      } else if (*points[i] > start) {
         *points[i] = start + string.size;
      }
   }
   Editor_ReplaceRange(editor, start, end, string);
}

// One line's own line of a kind - written over the one it has (has_line,
// old_line), or above the line of words; or with nothing to write, the one
// it has taken out.
PRIVATE void
_Look_WriteItemLine(Editor_Editor* editor, Deck_Item* item, B32 has_line, Rng1U64 old_line, String8 line, U64* cursor, U64* mark) {
   String8 text = Editor_Text(editor);
   if (line.size != 0 && has_line && old_line.max <= text.size) {
      _Look_Replace(editor, old_line.min, old_line.max, line, cursor, mark);
   } else if (line.size != 0) {
      Temp scratch = ScratchBegin(0, 0);
      U64  line_start = Min(item->text_range.min, text.size);
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      _Look_Replace(editor, line_start, line_start, PushStr8F(scratch.arena, "%S\n", line), cursor, mark);
      ScratchEnd(scratch);
   } else if (has_line && old_line.max <= text.size) {
      U64 start = old_line.min;
      U64 end = old_line.max;
      while (start > 0 && text.str[start - 1] != '\n') start -= 1;
      while (end < text.size && text.str[end] != '\n') end += 1;
      end = Min(end + 1, text.size);
      _Look_Replace(editor, start, end, Str8Lit(""), cursor, mark);
   }
}

// What a change of the lines selected is.
typedef enum _Look_Change {
   _Look_Change_Size,  // direction: larger, smaller, or 0 for the block's again
   _Look_Change_Color, // name: a colour's, or none for the block's again
   _Look_Change_Align, // align: a Deck_Align, or Deck_Align_COUNT for the block's again
} _Look_Change;

// Each of the lines its own line above it, the last first - so where the
// ones before it are in the text is not changed under them - as one step to
// undo, and the selection kept. 1 if a size could not go further.
PRIVATE B32
_Look_ChangeItems(App_State* app_state, U64 slide_idx, U64 idx, Deck_Item** items, U64 count, _Look_Change change, S32 direction,
                  String8 name, U64 align) {
   Editor_Editor* editor = app_state->editor;
   B32            at_end = 0;
   Temp           scratch = ScratchBegin(0, 0);
   F32            block_size = _Look_SizeNow(app_state, slide_idx, Deck_PlaceKind_Text, idx);
   U64            cursor = editor->cursor;
   U64            mark = editor->mark;
   U64            undo_before = editor->undo_count;
   for (U64 i = count; i > 0; i -= 1) {
      Deck_Item* item = items[i - 1];
      Deck_Look* look = &item->look;
      if (change == _Look_Change_Size) {
         String8 line = {0};
         if (direction != 0) {
            F32 now = look->has_size ? look->size : block_size;
            F32 size = _Look_SteppedSize(now, direction);
            at_end |= (size == now);
            line = PushStr8F(scratch.arena, "@size line %.2f", size);
         }
         _Look_WriteItemLine(editor, item, look->has_size, look->size_line, line, &cursor, &mark);
      } else if (change == _Look_Change_Color) {
         String8 line = (name.size != 0) ? PushStr8F(scratch.arena, "@color line %S", name) : Str8Lit("");
         _Look_WriteItemLine(editor, item, look->has_color, look->color_line, line, &cursor, &mark);
      } else {
         String8 line = (align < Deck_Align_COUNT) ? PushStr8F(scratch.arena, "@align line %S", Deck_AlignName((Deck_Align)align))
                                                   : Str8Lit("");
         _Look_WriteItemLine(editor, item, look->has_align, look->align_line, line, &cursor, &mark);
      }
   }
   Editor_JoinUndoSince(editor, undo_before);
   editor->cursor = Min(cursor, editor->size);
   editor->mark = Min(mark, editor->size);
   ScratchEnd(scratch);
   return at_end;
}

////////////////////////////////
//~ bgz: state ops
//
// Each writes a line of the text, or takes one out. The deck is made from
// the text again when the ops have run (_FR_UpdateAppState): made here first
// too, since an op before this one may have changed the text.

// The pieces an op with has_piece is about, into kinds and idxs
// (VIEW_MAX_BOXES of each): how many. With with_picked those picked whole
// that have words, else the one of kind and idx.
PRIVATE U64
_Look_Pieces(const App_State* app_state, B32 with_picked, Deck_PlaceKind kind, U64 idx, Deck_PlaceKind* kinds, U64* idxs) {
   U64 count = 0;
   if (with_picked && Edit_PickedIsGood(app_state)) {
      for (U64 i = 0; i < g_edit_view.picked_count; i += 1) {
         if (g_edit_view.picked_kinds[i] != Deck_PlaceKind_Picture) {
            kinds[count] = g_edit_view.picked_kinds[i];
            idxs[count] = g_edit_view.picked_idxs[i];
            count += 1;
         }
      }
   } else if (kind != Deck_PlaceKind_Picture) {
      kinds[0] = kind;
      idxs[0] = idx;
      count = 1;
   }
   return count;
}

// One step of the size of a slide's piece's words, written. 1 if it could
// not go further that way.
PRIVATE B32
_Look_StepPiece(App_State* app_state, U64 slide_idx, Deck_PlaceKind kind, U64 idx, S32 direction) {
   B32         result = 0;
   Deck_Slide* slide = &app_state->deck->slides[slide_idx];
   if (direction == 0) {
      _Look_WriteSize(app_state->editor, slide, kind, idx, 0, 0);
   } else {
      F32 now = _Look_SizeNow(app_state, slide_idx, kind, idx);
      F32 size = _Look_SteppedSize(now, direction);
      result = (size == now);
      _Look_WriteSize(app_state->editor, slide, kind, idx, 1, size);
   }
   return result;
}

void
Look_StepSize(App_State* app_state, StateOp_List* ops, Look_StepSizeOpParams* params) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   S32 direction = params->direction;
   B32 at_end = 0;

   if (params->has_piece) {
      //- bgz: a piece of the preview's slide, or all of those picked whole
      // that have words: one step to undo
      U64 slide_idx = app_state->slide_idx;
      if (slide_idx >= app_state->deck->slide_count) {
         return;
      }
      B32            was_picked = Edit_PickedIsGood(app_state);
      Deck_PlaceKind kinds[VIEW_MAX_BOXES];
      U64            idxs[VIEW_MAX_BOXES];
      U64            count = _Look_Pieces(app_state, params->with_picked, params->kind, params->idx, kinds, idxs);
      U64 undo_before = editor->undo_count;
      for (U64 i = 0; i < count; i += 1) {
         if (i != 0) {
            FR_SyncDeck(app_state);
         }
         at_end |= _Look_StepPiece(app_state, slide_idx, kinds[i], idxs[i], direction);
      }
      Editor_JoinUndoSince(editor, undo_before);
      Edit_KeepPicked(app_state, was_picked);
   } else {
      //- bgz: the piece the cursor is in
      U64            slide_idx = 0;
      Deck_PlaceKind kind = Deck_PlaceKind_Title;
      U64            idx = 0;
      if (!Look_Target(app_state, &slide_idx, &kind, &idx)) {
         StateOp_App_Notice(ops)->notice = Str8Lit("Put the cursor in the words to size: a title, or a line of the body");
         return;
      }
      // The lines selected, each its own - or the piece.
      Deck_Item* items[_LOOK_MAX_LINES];
      U64        item_count = _Look_SelectedItems(app_state, &app_state->deck->slides[slide_idx], kind, idx, items);
      if (item_count != 0) {
         at_end = _Look_ChangeItems(app_state, slide_idx, idx, items, item_count, _Look_Change_Size, direction, Str8Lit(""), 0);
      } else {
         at_end = _Look_StepPiece(app_state, slide_idx, kind, idx, direction);
      }
   }

   if (at_end) {
      StateOp_App_Notice(ops)->notice = (direction > 0) ? Str8Lit("That is as large as words get")
                                                        : Str8Lit("That is as small as words get");
   }
   editor->last_input_us = NowTimeUS();
}
STATE_OP(Look_StepSize)

void
Look_SetColor(App_State* app_state, StateOp_List* ops, Look_SetColorOpParams* params) {
   if (app_state->editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   U64     named_idx = params->named_idx;
   String8 name = (named_idx < DECK_NAMED_COLOR_COUNT) ? Deck_NamedColorFromIdx(named_idx).name : Str8Lit("");

   if (params->has_piece) {
      //- bgz: a piece of the preview's slide, or all of those picked whole
      // that have words: one step to undo
      Editor_Editor* editor = app_state->editor;
      U64            piece_slide_idx = app_state->slide_idx;
      if (piece_slide_idx >= app_state->deck->slide_count) {
         return;
      }
      B32            was_picked = Edit_PickedIsGood(app_state);
      Deck_PlaceKind kinds[VIEW_MAX_BOXES];
      U64            idxs[VIEW_MAX_BOXES];
      U64            count = _Look_Pieces(app_state, params->with_picked, params->kind, params->idx, kinds, idxs);
      U64            undo_before = editor->undo_count;
      for (U64 i = 0; i < count; i += 1) {
         FR_SyncDeck(app_state);
         if (piece_slide_idx < app_state->deck->slide_count) {
            _Look_WriteColor(editor, &app_state->deck->slides[piece_slide_idx], kinds[i], idxs[i], name);
         }
      }
      Editor_JoinUndoSince(editor, undo_before);
      Edit_KeepPicked(app_state, was_picked);
      editor->last_input_us = NowTimeUS();
      return;
   }

   //- bgz: the piece the cursor is in, or the lines selected
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   if (!Look_Target(app_state, &slide_idx, &kind, &idx)) {
      StateOp_App_Notice(ops)->notice = Str8Lit("Put the cursor in the words to colour: a title, or a line of the body");
      return;
   }
   Deck_Item* items[_LOOK_MAX_LINES];
   U64        item_count = _Look_SelectedItems(app_state, &app_state->deck->slides[slide_idx], kind, idx, items);
   if (item_count != 0) {
      _Look_ChangeItems(app_state, slide_idx, idx, items, item_count, _Look_Change_Color, 0, name, 0);
   } else {
      _Look_WriteColor(app_state->editor, &app_state->deck->slides[slide_idx], kind, idx, name);
   }
   app_state->editor->last_input_us = NowTimeUS();
}
STATE_OP(Look_SetColor)

void
Look_SetAlign(App_State* app_state, StateOp_List* ops, Look_SetAlignOpParams* params) {
   Editor_Editor* editor = app_state->editor;
   if (editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   U64 align = params->align;

   if (params->has_piece) {
      //- bgz: a piece of the preview's slide, or all of those picked whole
      // that have words: one step to undo
      U64 slide_idx = app_state->slide_idx;
      if (slide_idx >= app_state->deck->slide_count) {
         return;
      }
      B32            was_picked = Edit_PickedIsGood(app_state);
      Deck_PlaceKind kinds[VIEW_MAX_BOXES];
      U64            idxs[VIEW_MAX_BOXES];
      U64            count = _Look_Pieces(app_state, params->with_picked, params->kind, params->idx, kinds, idxs);
      U64 undo_before = editor->undo_count;
      for (U64 i = 0; i < count; i += 1) {
         FR_SyncDeck(app_state);
         if (slide_idx < app_state->deck->slide_count) {
            _Look_WriteAlign(editor, &app_state->deck->slides[slide_idx], kinds[i], idxs[i], align);
         }
      }
      Editor_JoinUndoSince(editor, undo_before);
      Edit_KeepPicked(app_state, was_picked);
   } else {
      //- bgz: the piece the cursor is in, or the lines selected
      U64            slide_idx = 0;
      Deck_PlaceKind kind = Deck_PlaceKind_Title;
      U64            idx = 0;
      if (!Look_Target(app_state, &slide_idx, &kind, &idx)) {
         StateOp_App_Notice(ops)->notice = Str8Lit("Put the cursor in the words to line up: a title, or a line of the body");
         return;
      }
      Deck_Item* items[_LOOK_MAX_LINES];
      U64        item_count = _Look_SelectedItems(app_state, &app_state->deck->slides[slide_idx], kind, idx, items);
      if (item_count != 0) {
         _Look_ChangeItems(app_state, slide_idx, idx, items, item_count, _Look_Change_Align, 0, Str8Lit(""), align);
      } else {
         _Look_WriteAlign(editor, &app_state->deck->slides[slide_idx], kind, idx, align);
      }
   }
   editor->last_input_us = NowTimeUS();
}
STATE_OP(Look_SetAlign)

// The look the Format panel is about: the first of the lines selected's
// own, where there are lines selected, else the piece's. is_lines_out: which.
PRIVATE Deck_Look*
_Look_Shown(const App_State* app_state, U64 slide_idx, Deck_PlaceKind kind, U64 idx, B32* is_lines_out) {
   Deck_Slide* slide = &app_state->deck->slides[slide_idx];
   Deck_Item*  items[_LOOK_MAX_LINES];
   U64         item_count = _Look_SelectedItems(app_state, slide, kind, idx, items);
   *is_lines_out = (item_count != 0);
   return (item_count != 0) ? &items[0]->look : Look_Of(slide, kind, idx);
}

String8
Look_DescribeAlign(Arena* arena, const App_State* app_state) {
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   String8        result = Str8Lit("which side the words the cursor is in are against");
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       Look_Target(app_state, &slide_idx, &kind, &idx)) {
      B32        is_lines = 0;
      Deck_Look* look = _Look_Shown(app_state, slide_idx, kind, idx, &is_lines);
      char*      whose = is_lines ? "the lines selected" : (kind == Deck_PlaceKind_Title) ? "title" : "this block";
      if (look->has_align) {
         result = PushStr8F(arena, "%s: %S", whose, Deck_AlignName(look->align));
      } else {
         result = PushStr8F(arena, "%s: as the %s", whose, is_lines ? "block" : "layout");
      }
   }
   return result;
}

B32
Look_AlignIsNow(const App_State* app_state, U64 align) {
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   B32            result = 0;
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       Look_Target(app_state, &slide_idx, &kind, &idx)) {
      B32        is_lines = 0;
      Deck_Look* look = _Look_Shown(app_state, slide_idx, kind, idx, &is_lines);
      result = (align >= Deck_Align_COUNT) ? !look->has_align : (look->has_align && (U64)look->align == align);
   }
   return result;
}

String8
Look_Describe(Arena* arena, const App_State* app_state, B32 is_color) {
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   String8        result = is_color ? Str8Lit("the colour of the words the cursor is in")
                                    : Str8Lit("the size of the words the cursor is in");
   // Only of a deck that is of the text as it is: this is asked while the
   // screen is built, when it may not be.
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       Look_Target(app_state, &slide_idx, &kind, &idx)) {
      B32        is_lines = 0;
      Deck_Look* look = _Look_Shown(app_state, slide_idx, kind, idx, &is_lines);
      // Whose words first, and short: what there is to click takes most of
      // the row, and the rest of this is cut off where it does not fit.
      char*      whose = is_lines ? "the lines selected" : (kind == Deck_PlaceKind_Title) ? "title" : "this block";
      if (is_color && look->has_color) {
         // As the line has it: what is after the kind's name.
         String8 text = Editor_Text(app_state->editor);
         String8 line = Str8Substr(text, R1U64(Min(look->color_line.min, text.size), Min(look->color_line.max, text.size)));
         U64     value_start = line.size;
         while (value_start > 0 && line.str[value_start - 1] != ' ' && line.str[value_start - 1] != '\t') value_start -= 1;
         result = PushStr8F(arena, "%s: %S", whose, Str8Skip(line, value_start));
      } else if (is_color) {
         result = PushStr8F(arena, "%s: %s", whose, is_lines ? "the block's" : "the theme's");
      } else if (look->has_size) {
         result = PushStr8F(arena, "%s: %.2f times", whose, look->size);
      } else {
         result = PushStr8F(arena, "%s: usual. Ctrl+] larger, Ctrl+[ smaller", whose);
      }
   }
   return result;
}

B32
Look_ColorIsNow(const App_State* app_state, U64 named_idx) {
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   B32            result = 0;
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       Look_Target(app_state, &slide_idx, &kind, &idx)) {
      B32        is_lines = 0;
      Deck_Look* look = _Look_Shown(app_state, slide_idx, kind, idx, &is_lines);
      if (named_idx >= DECK_NAMED_COLOR_COUNT) {
         result = !look->has_color;
      } else if (look->has_color) {
         Vec4F32 color = Deck_NamedColorFromIdx(named_idx).color;
         result = (color.x == look->color.x && color.y == look->color.y && color.z == look->color.z);
      }
   }
   return result;
}

B32
Look_SizeIsUsual(const App_State* app_state) {
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   B32            result = 0;
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       Look_Target(app_state, &slide_idx, &kind, &idx)) {
      B32 is_lines = 0;
      result = !_Look_Shown(app_state, slide_idx, kind, idx, &is_lines)->has_size;
   }
   return result;
}

void
Look_Input(const App_State* app_state, StateOp_List* ops) {
   WM_EventList* events = app_state->events;
   WM_Window     window = app_state->window;
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_RightBracket)) {
      StateOp_Look_StepSize(ops)->direction = +1;
   }
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_LeftBracket)) {
      StateOp_Look_StepSize(ops)->direction = -1;
   }
   // Against the left, in the middle, against the right: of the pieces
   // picked whole, or where the cursor is.
   WM_Modifiers ctrl_shift = WM_Modifier_Ctrl | WM_Modifier_Shift;
   struct {
      WM_Key     key;
      Deck_Align align;
   } aligns[] = {{WM_Key_L, Deck_Align_Left}, {WM_Key_E, Deck_Align_Center}, {WM_Key_R, Deck_Align_Right}};
   for (U64 i = 0; i < ArrayCount(aligns); i += 1) {
      if (Core_WM_KeyPress(events, window, ctrl_shift, aligns[i].key)) {
         Look_SetAlignOpParams* set = StateOp_Look_SetAlign(ops);
         set->align = aligns[i].align;
         if (Edit_PickedIsGood(app_state)) {
            set->has_piece = 1;
            set->with_picked = 1;
         }
      }
   }
}

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
//    the Format panel its @size line has smaller, larger and usual to
//                     click, and its @color line the colours with names
//                     (deck.h) and the theme's
//
// What comes of it is a line of the deck's text, as with a piece moved by
// the mouse (place\place.c): "@size title 1.21", "@color text orange"
// (deck.h), over the one the piece has or as a new line beside it. The text
// is still all there is to a deck: the line can be changed by hand - any
// size, any #rrggbb - and Ctrl+Z takes a change back. Usual and the theme's
// take the line out again.
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
      String8 line = PushStr8F(scratch.arena, "@size %S %.2f", Deck_PlaceName(kind), size);
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
      String8 line = PushStr8F(scratch.arena, "@color %S %S", Deck_PlaceName(kind), color);
      Edit_PutLineFor(editor, slide, kind, idx, look->has_color, look->color_line, line);
      ScratchEnd(scratch);
   } else if (look->has_color) {
      Edit_TakeOutLine(editor, look->color_line);
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
//~ bgz: state ops
//
// Each writes a line of the text, or takes one out. The deck is made from
// the text again when the ops have run (_FR_UpdateAppState): made here first
// too, since an op before this one may have changed the text.

void
Look_StepSize(App_State* app_state, StateOp_List* ops, Look_StepSizeOpParams* params) {
   if (app_state->editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   S32            direction = params->direction;
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   if (!Look_Target(app_state, &slide_idx, &kind, &idx)) {
      StateOp_App_Notice(ops)->notice = Str8Lit("Put the cursor in the words to size: a title, or a line of the body");
      return;
   }
   Deck_Slide* slide = &app_state->deck->slides[slide_idx];
   if (direction == 0) {
      _Look_WriteSize(app_state->editor, slide, kind, idx, 0, 0);
   } else {
      F32 now = _Look_SizeNow(app_state, slide_idx, kind, idx);
      F32 size = _Look_SteppedSize(now, direction);
      if (size == now) {
         StateOp_App_Notice(ops)->notice = (direction > 0) ? Str8Lit("That is as large as words get")
                                                           : Str8Lit("That is as small as words get");
      }
      _Look_WriteSize(app_state->editor, slide, kind, idx, 1, size);
   }
   app_state->editor->last_input_us = NowTimeUS();
}
STATE_OP(Look_StepSize)

void
Look_SetColor(App_State* app_state, StateOp_List* ops, Look_SetColorOpParams* params) {
   if (app_state->editor == 0) {
      return;
   }
   FR_SyncDeck(app_state);
   U64            named_idx = params->named_idx;
   U64            slide_idx = 0;
   Deck_PlaceKind kind = Deck_PlaceKind_Title;
   U64            idx = 0;
   if (!Look_Target(app_state, &slide_idx, &kind, &idx)) {
      StateOp_App_Notice(ops)->notice = Str8Lit("Put the cursor in the words to colour: a title, or a line of the body");
      return;
   }
   String8 name = (named_idx < DECK_NAMED_COLOR_COUNT) ? Deck_NamedColorFromIdx(named_idx).name : Str8Lit("");
   _Look_WriteColor(app_state->editor, &app_state->deck->slides[slide_idx], kind, idx, name);
   app_state->editor->last_input_us = NowTimeUS();
}
STATE_OP(Look_SetColor)

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
      Deck_Look* look = Look_Of(&app_state->deck->slides[slide_idx], kind, idx);
      // Whose words first, and short: what there is to click takes most of
      // the row, and the rest of this is cut off where it does not fit.
      char*      whose = (kind == Deck_PlaceKind_Title) ? "title" : "this block";
      if (is_color && look->has_color) {
         // As the line has it: what is after the kind's name.
         String8 text = Editor_Text(app_state->editor);
         String8 line = Str8Substr(text, R1U64(Min(look->color_line.min, text.size), Min(look->color_line.max, text.size)));
         U64     value_start = line.size;
         while (value_start > 0 && line.str[value_start - 1] != ' ' && line.str[value_start - 1] != '\t') value_start -= 1;
         result = PushStr8F(arena, "%s: %S", whose, Str8Skip(line, value_start));
      } else if (is_color) {
         result = PushStr8F(arena, "%s: the theme's", whose);
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
      Deck_Look* look = Look_Of(&app_state->deck->slides[slide_idx], kind, idx);
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
      result = !Look_Of(&app_state->deck->slides[slide_idx], kind, idx)->has_size;
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
}

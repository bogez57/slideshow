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
// preview knows (VIEW_Box's text_size).

// How much one step of larger or smaller is.
#define _LOOK_STEP 1.1f

// The piece the cursor is in: 0 if there is none whose words can be sized -
// no slides, or a slide with neither a title nor a body. The deck has to be
// of the text as it is: FR_SyncDeck first.
internal B32
_LOOK_Target(App_State* app_state, U64* slide_idx_out, DECK_PlaceKind* kind_out, U64* idx_out) {
   DECK_Deck* deck = app_state->deck;
   ED_Editor* editor = app_state->editor;
   if (editor == 0 || deck->slide_count == 0) {
      return 0;
   }
   String8     text = ED_Text(editor);
   U64         cursor = Min(editor->cursor, text.size);
   U64         slide_idx = DECK_SlideFromTextPos(deck, cursor);
   DECK_Slide* slide = &deck->slides[slide_idx];

   // The line the cursor is on.
   U64 line_start = cursor;
   while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
   U64 line_end = cursor;
   while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;
#define _LOOK_OnLine(pos) ((pos) >= line_start && (pos) <= line_end)

   // The title's: its own line, and the lines that are about it.
   B32 has_title = (slide->title_range.max != 0);
   B32 on_title = has_title && (_LOOK_OnLine(slide->title_range.min) ||
                                (slide->title_look.has_size && _LOOK_OnLine(slide->title_look.size_line.min)) ||
                                (slide->title_look.has_color && _LOOK_OnLine(slide->title_look.color_line.min)) ||
                                (slide->places[DECK_PlaceKind_Title].has_line &&
                                 _LOOK_OnLine(slide->places[DECK_PlaceKind_Title].line.min)));
#undef _LOOK_OnLine

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
      *kind_out = DECK_PlaceKind_Title;
      *idx_out = 0;
   } else if (has_block) {
      *kind_out = DECK_PlaceKind_Text;
      *idx_out = block_idx;
   } else {
      result = 0;
   }
   return result;
}

internal DECK_Look*
_LOOK_Of(DECK_Slide* slide, DECK_PlaceKind kind, U64 idx) {
   DECK_Look* result = &slide->title_look;
   if (kind == DECK_PlaceKind_Text && idx < slide->box_count) {
      result = &slide->boxes[idx].look;
   }
   return result;
}

// Writes the piece's @size line, or with has_size 0 takes it out.
internal void
_LOOK_WriteSize(ED_Editor* editor, DECK_Slide* slide, DECK_PlaceKind kind, U64 idx, B32 has_size, F32 size) {
   DECK_Look* look = _LOOK_Of(slide, kind, idx);
   if (has_size) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 line = PushStr8F(scratch.arena, "@size %S %.2f", DECK_PlaceName(kind), size);
      _EDIT_PutLineFor(editor, slide, kind, idx, look->has_size, look->size_line, line);
      ScratchEnd(scratch);
   } else if (look->has_size) {
      _EDIT_TakeOutLine(editor, look->size_line);
   }
}

// Writes the piece's @color line, or with nothing for a colour takes it out.
internal void
_LOOK_WriteColor(ED_Editor* editor, DECK_Slide* slide, DECK_PlaceKind kind, U64 idx, String8 color) {
   DECK_Look* look = _LOOK_Of(slide, kind, idx);
   if (color.size != 0) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 line = PushStr8F(scratch.arena, "@color %S %S", DECK_PlaceName(kind), color);
      _EDIT_PutLineFor(editor, slide, kind, idx, look->has_color, look->color_line, line);
      ScratchEnd(scratch);
   } else if (look->has_color) {
      _EDIT_TakeOutLine(editor, look->color_line);
   }
}

// The size a step from size is: direction over 0 larger, under 0 smaller.
// To two places, as it is written, and never the same as it was.
internal F32
_LOOK_SteppedSize(F32 size, S32 direction) {
   F32 result = (direction > 0) ? size * _LOOK_STEP : size / _LOOK_STEP;
   result = (F32)(S32)(result * 100.0f + 0.5f) / 100.0f;
   result = Clamp(DECK_SIZE_MIN, result, DECK_SIZE_MAX);
   return result;
}

// The size the piece's words are drawn at, as a part of their usual size:
// what the preview drew them at, if it is of the text as it is and of this
// slide, else what the piece's @size line says, else 1.
internal F32
_LOOK_SizeNow(App_State* app_state, U64 slide_idx, DECK_PlaceKind kind, U64 idx) {
   DECK_Look* look = _LOOK_Of(&app_state->deck->slides[slide_idx], kind, idx);
   F32        result = look->has_size ? look->size : 1.0f;
   if (!look->has_size && _PLACE_BoxesAreGood(app_state) && slide_idx == app_state->slide_idx) {
      for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
         VIEW_Box* box = &app_state->preview_boxes[i];
         if (box->kind == kind && box->idx == idx && box->text_size > 0) {
            result = box->text_size;
         }
      }
   }
   return result;
}

// After a change: the deck made again from the text, so that what is drawn
// this frame is of the text as it now is.
internal void
_LOOK_Changed(App_State* app_state) {
   app_state->editor->last_input_us = NowTimeUS();
   FR_SyncDeck(app_state);
   app_state->slide_idx = DECK_SlideFromTextPos(app_state->deck, app_state->editor->cursor);
}

internal void
LOOK_StepSize(App_State* app_state, S32 direction) {
   FR_SyncDeck(app_state);
   U64            slide_idx = 0;
   DECK_PlaceKind kind = DECK_PlaceKind_Title;
   U64            idx = 0;
   if (!_LOOK_Target(app_state, &slide_idx, &kind, &idx)) {
      FR_Notice(app_state, Str8Lit("Put the cursor in the words to size: a title, or a line of the body"));
      return;
   }
   DECK_Slide* slide = &app_state->deck->slides[slide_idx];
   if (direction == 0) {
      _LOOK_WriteSize(app_state->editor, slide, kind, idx, 0, 0);
   } else {
      F32 now = _LOOK_SizeNow(app_state, slide_idx, kind, idx);
      F32 size = _LOOK_SteppedSize(now, direction);
      if (size == now) {
         FR_Notice(app_state, (direction > 0) ? Str8Lit("That is as large as words get")
                                              : Str8Lit("That is as small as words get"));
      }
      _LOOK_WriteSize(app_state->editor, slide, kind, idx, 1, size);
   }
   _LOOK_Changed(app_state);
}

internal void
LOOK_SetColor(App_State* app_state, U64 named_idx) {
   FR_SyncDeck(app_state);
   U64            slide_idx = 0;
   DECK_PlaceKind kind = DECK_PlaceKind_Title;
   U64            idx = 0;
   if (!_LOOK_Target(app_state, &slide_idx, &kind, &idx)) {
      FR_Notice(app_state, Str8Lit("Put the cursor in the words to colour: a title, or a line of the body"));
      return;
   }
   String8 name = (named_idx < DECK_NAMED_COLOR_COUNT) ? DECK_NamedColorFromIdx(named_idx).name : Str8Lit("");
   _LOOK_WriteColor(app_state->editor, &app_state->deck->slides[slide_idx], kind, idx, name);
   _LOOK_Changed(app_state);
}

internal String8
LOOK_Describe(Arena* arena, App_State* app_state, B32 is_color) {
   U64            slide_idx = 0;
   DECK_PlaceKind kind = DECK_PlaceKind_Title;
   U64            idx = 0;
   String8        result = is_color ? Str8Lit("the colour of the words the cursor is in")
                                    : Str8Lit("the size of the words the cursor is in");
   // Only of a deck that is of the text as it is: this is asked while the
   // screen is built, when it may not be.
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       _LOOK_Target(app_state, &slide_idx, &kind, &idx)) {
      DECK_Look* look = _LOOK_Of(&app_state->deck->slides[slide_idx], kind, idx);
      // Whose words first, and short: what there is to click takes most of
      // the row, and the rest of this is cut off where it does not fit.
      char*      whose = (kind == DECK_PlaceKind_Title) ? "title" : "this block";
      if (is_color && look->has_color) {
         // As the line has it: what is after the kind's name.
         String8 text = ED_Text(app_state->editor);
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

internal B32
LOOK_ColorIsNow(App_State* app_state, U64 named_idx) {
   U64            slide_idx = 0;
   DECK_PlaceKind kind = DECK_PlaceKind_Title;
   U64            idx = 0;
   B32            result = 0;
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       _LOOK_Target(app_state, &slide_idx, &kind, &idx)) {
      DECK_Look* look = _LOOK_Of(&app_state->deck->slides[slide_idx], kind, idx);
      if (named_idx >= DECK_NAMED_COLOR_COUNT) {
         result = !look->has_color;
      } else if (look->has_color) {
         Vec4F32 color = DECK_NamedColorFromIdx(named_idx).color;
         result = (color.x == look->color.x && color.y == look->color.y && color.z == look->color.z);
      }
   }
   return result;
}

internal B32
LOOK_SizeIsUsual(App_State* app_state) {
   U64            slide_idx = 0;
   DECK_PlaceKind kind = DECK_PlaceKind_Title;
   U64            idx = 0;
   B32            result = 0;
   if (app_state->editor != 0 && app_state->parsed_edit_count == app_state->editor->edit_count &&
       _LOOK_Target(app_state, &slide_idx, &kind, &idx)) {
      result = !_LOOK_Of(&app_state->deck->slides[slide_idx], kind, idx)->has_size;
   }
   return result;
}

internal void
LOOK_Input(App_State* app_state) {
   WM_EventList* events = &app_state->events;
   WM_Window     window = app_state->window;
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_RightBracket)) {
      LOOK_StepSize(app_state, +1);
   }
   if (Core_WM_KeyPress(events, window, WM_Modifier_Ctrl, WM_Key_LeftBracket)) {
      LOOK_StepSize(app_state, -1);
   }
}

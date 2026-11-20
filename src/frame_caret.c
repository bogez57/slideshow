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
//    see the box      the block the caret is in has a thin blue box around
//                     the room its words have. They wrap at its width
//    more words       than the room is tall: they stay the size they are,
//                     and the room shows the part of them the caret is in,
//                     moving down as the typing does. The mouse's wheel,
//                     over the block, moves it too
//
// A letter is found on the slide, and a point on the slide among the
// letters, by the runs the view gives (VIEW_Run): each line or piece of
// words as drawn, with where in the text its letters are. Some of the text
// is not drawn - the "- " of a bullet, the ** of bold, an @ line - and a
// cursor there has its caret at the nearest letter that is, or none.
//
// How far down a block's words its room shows (DECK_Box's scroll) is not in
// the deck's text: it is kept here, for the slide showing, and put on the
// slide only while the preview draws it. Presenting shows every block from
// its top.

// The caret: where the cursor is on the slide.
typedef struct _CARET_Place _CARET_Place;
struct _CARET_Place {
   B32            is_found;
   Rng2F32        rect; // no width: the line it is on, down
   DECK_PlaceKind kind; // whose words it is in
   U64            idx;
};

// Whether the runs kept from the last frame are of the text as it is.
internal B32
_CARET_RunsAreGood(App_State* app_state) {
   return _PLACE_BoxesAreGood(app_state);
}

// How far into the run the text position pos is, across: pos is in it.
internal F32
_CARET_XInRun(String8 text, VIEW_Run* run, U64 pos) {
   String8 before = Str8Substr(text, R1U64(run->text_range.min, Min(pos, run->text_range.max)));
   return run->rect.min.x + TXT_Width(run->font, run->px, before);
}

// Where on the slide the text position pos is, by runs. Not found when the
// line pos is on has nothing drawn and is not an empty line of a block.
internal _CARET_Place
_CARET_PlaceFromPos(App_State* app_state, VIEW_Run* runs, U64 run_count, U64 pos) {
   _CARET_Place result = {0};
   String8      text = ED_Text(app_state->editor);
   pos = Min(pos, text.size);

   U64 line_start = pos;
   while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
   U64 line_end = pos;
   while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;

   // Of the runs of this line of the text: the last that starts at or
   // before pos, and the first of them all.
   VIEW_Run* at = 0;
   VIEW_Run* first = 0;
   VIEW_Run* above = 0; // the last run of the lines before this one
   for (U64 i = 0; i < run_count; i += 1) {
      VIEW_Run* run = &runs[i];
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
      F32 x = _CARET_XInRun(text, at, pos);
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
            x += TXT_Width(at->font, at->px, Str8Lit(" ")) * (F32)spaces;
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
   } else if (above != 0 && above->kind == DECK_PlaceKind_Text) {
      // A line with nothing on it, in a block: a line being started. The
      // caret is under the block's last words before it, at the block's
      // left - if the cursor is in that block, and not in what follows it.
      B32 is_blank = 1;
      for (U64 i = line_start; i < line_end && is_blank; i += 1) {
         is_blank = (text.str[i] == ' ' || text.str[i] == '\t');
      }
      U64            slide_idx = 0;
      DECK_PlaceKind kind = DECK_PlaceKind_Title;
      U64            idx = 0;
      if (is_blank && _LOOK_Target(app_state, &slide_idx, &kind, &idx) && kind == DECK_PlaceKind_Text && idx == above->idx) {
         F32 left = above->rect.min.x;
         for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
            VIEW_Box* box = &app_state->preview_boxes[i];
            if (box->kind == DECK_PlaceKind_Text && box->idx == idx) {
               // Where its first letter will be: in the middle, when the
               // slide puts every line there.
               B32 centered = (app_state->deck->slides[app_state->slide_idx].layout == DECK_Layout_Center);
               left = centered ? (box->room.min.x + box->room.max.x) * 0.5f : box->room.min.x;
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
internal U64
_CARET_PosInRun(String8 text, VIEW_Run* run, F32 x) {
   U64 result = run->text_range.min;
   if (x >= run->rect.max.x) {
      result = run->text_range.max;
   } else if (x > run->rect.min.x) {
      F32 before_x = run->rect.min.x;
      for (U64 pos = run->text_range.min; pos < Min(run->text_range.max, text.size);) {
         // A letter at a time: all the bytes of one that has several.
         U64 next = pos + 1;
         while (next < run->text_range.max && next < text.size && (text.str[next] & 0xC0) == 0x80) next += 1;
         F32 after_x = _CARET_XInRun(text, run, next);
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
// pick (_PLACE_BoxAt), and past that the room under a block's words.
internal VIEW_Box*
_CARET_WordsBoxAt(App_State* app_state, Vec2F32 pos) {
   VIEW_Box* result = _PLACE_BoxAt(app_state, pos);
   if (result == 0) {
      for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
         VIEW_Box* box = &app_state->preview_boxes[i];
         if (box->kind != DECK_PlaceKind_Picture && Contains2F32(box->room, pos)) {
            result = box;
         }
      }
   }
   if (result != 0 && result->kind == DECK_PlaceKind_Picture) {
      result = 0;
   }
   return result;
}

internal B32
CARET_Click(App_State* app_state, Vec2F32 pos, B32 keeps_mark) {
   B32 result = 0;
   if (!_CARET_RunsAreGood(app_state) || !Contains2F32(app_state->preview_page, pos)) {
      return 0;
   }
   VIEW_Box* box = _CARET_WordsBoxAt(app_state, pos);
   if (box != 0) {
      // The nearest of its runs that show: first by how far up or down the
      // point is from the run's line, then by how far across.
      VIEW_Run* nearest = 0;
      F32       nearest_far = 0;
      for (U64 i = 0; i < app_state->preview_run_count; i += 1) {
         VIEW_Run* run = &app_state->preview_runs[i];
         if (run->kind != box->kind || run->idx != box->idx || run->rect.max.y <= box->room.min.y ||
             run->rect.min.y >= box->room.max.y) {
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
      ED_Editor* editor = app_state->editor;
      if (nearest != 0 && nearest->text_range.max <= editor->size) {
         U64 mark = editor->mark;
         ED_SetCursor(editor, _CARET_PosInRun(ED_Text(editor), nearest, pos.x));
         if (keeps_mark) {
            editor->mark = mark;
         }
         editor->last_input_us = NowTimeUS();
         app_state->preview_focus_hidden = 0;
         result = 1;
      }
   }
   return result;
}

// The scroll of the block a box is of: its own if it is placed, the one
// the others share if not.
internal F32*
_CARET_ScrollOf(App_State* app_state, DECK_Slide* slide, U64 block_idx) {
   F32* result = &app_state->caret_body_scroll;
   if (block_idx < slide->box_count && slide->boxes[block_idx].place.is_set) {
      result = &app_state->caret_scroll[block_idx];
   }
   return result;
}

internal B32
CARET_Wheel(App_State* app_state, Vec2F32 pos, F32 delta) {
   B32 result = 0;
   if (_CARET_RunsAreGood(app_state) && Contains2F32(app_state->preview_page, pos)) {
      VIEW_Box* box = _CARET_WordsBoxAt(app_state, pos);
      if (box != 0 && box->kind == DECK_PlaceKind_Text) {
         // Some three lines of the body a notch (delta is in notches, down
         // the words when over 0). Kept to the words there are when the
         // slide is next drawn (CARET_Follow).
         DECK_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
         F32*        scroll = _CARET_ScrollOf(app_state, slide, box->idx);
         *scroll = Max(*scroll + delta * 96.0f, 0.0f);
         result = 1;
      }
   }
   return result;
}

internal void
CARET_BeforeDraw(App_State* app_state) {
   DECK_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   // Another slide starts from its top.
   if (app_state->caret_scroll_slide != app_state->slide_idx) {
      app_state->caret_scroll_slide = app_state->slide_idx;
      MemoryZeroArray(app_state->caret_scroll);
      app_state->caret_body_scroll = 0;
   }
   slide->body_scroll = app_state->caret_body_scroll;
   for (U64 i = 0; i < slide->box_count; i += 1) {
      slide->boxes[i].scroll = slide->boxes[i].place.is_set ? app_state->caret_scroll[i] : 0;
   }
}

// The runs of a hit list, as an array on arena.
internal VIEW_Run*
_CARET_RunsFromHits(Arena* arena, VIEW_HitList* hits, U64* count_out) {
   VIEW_Run* result = PushArray(arena, VIEW_Run, Max(hits->run_count, 1));
   U64       count = 0;
   for (VIEW_Run* run = hits->first_run; run != 0; run = run->next) {
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
internal VIEW_Box*
_CARET_BoxOfCursor(App_State* app_state, VIEW_HitList* hits, VIEW_Run* runs, U64 run_count, _CARET_Place* caret_out) {
   *caret_out = _CARET_PlaceFromPos(app_state, runs, run_count, app_state->editor->cursor);
   DECK_PlaceKind kind = caret_out->kind;
   U64            idx = caret_out->idx;
   B32            has_piece = caret_out->is_found;
   if (!has_piece) {
      U64 slide_idx = 0;
      has_piece = (_LOOK_Target(app_state, &slide_idx, &kind, &idx) && slide_idx == app_state->slide_idx);
   }
   VIEW_Box* result = 0;
   for (U64 i = 0; i < hits->box_count && has_piece; i += 1) {
      if (hits->boxes[i].kind == kind && hits->boxes[i].idx == idx) {
         result = &hits->boxes[i];
      }
   }
   return result;
}

internal B32
CARET_Follow(App_State* app_state, VIEW_HitList* hits) {
   DECK_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   ED_Editor*  editor = app_state->editor;
   // Of the cursor, which is in the text as it is: nothing if the slide
   // drawn is of the text as it was.
   if (editor->edit_count != app_state->parsed_edit_count) {
      return 0;
   }

   Temp         scratch = ScratchBegin(0, 0);
   U64          run_count = 0;
   VIEW_Run*    runs = _CARET_RunsFromHits(scratch.arena, hits, &run_count);
   _CARET_Place caret = {0};
   VIEW_Box*    box = _CARET_BoxOfCursor(app_state, hits, runs, run_count, &caret);
   DECK_PlaceKind kind = caret.kind;
   U64            idx = caret.idx;
   F32            scale = VIEW_ScaleFromDim(Dim2F32(hits->page));
   F32            body_scroll_was = app_state->caret_body_scroll;
   F32            scroll_was[DECK_MAX_BOXES];
   MemoryCopyArray(scroll_was, app_state->caret_scroll);

   //- bgz: the room follows the caret, when the cursor has gone somewhere
   // or the text has changed - not all the time: the wheel moves it too
   B32 has_moved = (editor->cursor != app_state->caret_seen_cursor || editor->edit_count != app_state->caret_seen_edit_count);
   app_state->caret_seen_cursor = editor->cursor;
   app_state->caret_seen_edit_count = editor->edit_count;
   if (has_moved && caret.is_found && box != 0 && kind == DECK_PlaceKind_Text && scale > 0) {
      F32* scroll = _CARET_ScrollOf(app_state, slide, idx);
      // The room of the blocks that share one is the body's: from the first
      // of them that shows to the bottom of the last.
      Rng2F32 room = box->room;
      if (!slide->boxes[idx].place.is_set) {
         for (U64 i = 0; i < hits->box_count; i += 1) {
            VIEW_Box* other = &hits->boxes[i];
            if (other->kind == DECK_PlaceKind_Text && other->idx < slide->box_count && !slide->boxes[other->idx].place.is_set) {
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
      VIEW_Box* text_box = &hits->boxes[i];
      if (text_box->kind != DECK_PlaceKind_Text || text_box->idx >= slide->box_count) {
         continue;
      }
      B32 is_placed = slide->boxes[text_box->idx].place.is_set;
      // Of the blocks that share a room, the last is the one that ends it.
      B32 is_last_shared = !is_placed;
      for (U64 k = i + 1; k < hits->box_count && is_last_shared; k += 1) {
         VIEW_Box* later = &hits->boxes[k];
         if (later->kind == DECK_PlaceKind_Text && later->idx < slide->box_count && !slide->boxes[later->idx].place.is_set) {
            is_last_shared = 0;
         }
      }
      if (is_placed || is_last_shared) {
         F32* scroll = _CARET_ScrollOf(app_state, slide, text_box->idx);
         F32  words_max_y = text_box->words_max_y;
         if (caret.is_found && kind == DECK_PlaceKind_Text && idx == text_box->idx) {
            words_max_y = Max(words_max_y, caret.rect.max.y);
         }
         F32 empty = text_box->room.max.y - words_max_y;
         if (*scroll > 0 && empty > 0) {
            *scroll = Max(*scroll - empty / scale, 0.0f);
         }
      }
   }
   ScratchEnd(scratch);

   B32 result = (app_state->caret_body_scroll != body_scroll_was || !MemoryMatchArray(app_state->caret_scroll, scroll_was));
   return result;
}

internal B32
CARET_AfterDraw(App_State* app_state, VIEW_HitList* hits, F32 ui) {
   DECK_Deck*  deck = app_state->deck;
   DECK_Slide* slide = &deck->slides[app_state->slide_idx];
   ED_Editor*  editor = app_state->editor;

   //- bgz: the scroll is the preview's, not the deck's
   slide->body_scroll = 0;
   for (U64 i = 0; i < slide->box_count; i += 1) {
      slide->boxes[i].scroll = 0;
   }

   //- bgz: the runs, kept for the next frame's clicks
   // On the arena the hits are on, which EDIT_Draw has just cleared.
   app_state->preview_runs = _CARET_RunsFromHits(app_state->preview_hits_arena, hits, &app_state->preview_run_count);

   if (editor->edit_count != app_state->parsed_edit_count) {
      return 0;
   }
   _CARET_Place   caret = {0};
   VIEW_Box*      box = _CARET_BoxOfCursor(app_state, hits, app_state->preview_runs, app_state->preview_run_count, &caret);
   DECK_PlaceKind kind = caret.kind;
   U64            idx = caret.idx;
   if (box != 0) {
      kind = box->kind;
      idx = box->idx;
   }

   //- bgz: the box and the caret. Not after Esc, until the cursor goes
   // somewhere or a piece is clicked.
   if (app_state->preview_focus_hidden && editor->cursor != app_state->preview_focus_hidden_cursor) {
      app_state->preview_focus_hidden = 0;
   }
   B32 result = 0;
   if (box != 0 && kind != DECK_PlaceKind_Picture && !app_state->preview_focus_hidden) {
      // A thin box around the room its words have, in a blue that is the
      // same on every theme: it is the editor's mark, not part of the slide.
      // Just outside the room, where the words wrap - and so beside, not a
      // little way off, the outline the mouse's box has (PLACE_Draw), which
      // is just inside it: one line, not two side by side.
      F32     thick = Max(1.0f, (F32)(S32)(1.5f * ui));
      Vec4F32 blue = V4F32(0.26f, 0.60f, 1.0f, 1.0f);
      Rng2F32 rect = Intersect2F32(Pad2F32(box->room, thick), hits->page);
      if (rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
         Core_D_Rect(R2F32P(rect.min.x, rect.min.y, rect.max.x, rect.min.y + thick), R2F32P(0, 0, 1, 1), blue);
         Core_D_Rect(R2F32P(rect.min.x, rect.max.y - thick, rect.max.x, rect.max.y), R2F32P(0, 0, 1, 1), blue);
         Core_D_Rect(R2F32P(rect.min.x, rect.min.y, rect.min.x + thick, rect.max.y), R2F32P(0, 0, 1, 1), blue);
         Core_D_Rect(R2F32P(rect.max.x - thick, rect.min.y, rect.max.x, rect.max.y), R2F32P(0, 0, 1, 1), blue);
      }
      result = 1;

      //- bgz: what is selected, washed in the same blue: the part of each
      // run that is in the selection, where the run shows
      String8 text = ED_Text(editor);
      U64     select_min = Min(editor->cursor, editor->mark);
      U64     select_max = Max(editor->cursor, editor->mark);
      for (U64 i = 0; i < app_state->preview_run_count && select_min != select_max; i += 1) {
         VIEW_Run* run = &app_state->preview_runs[i];
         U64       from = Max(run->text_range.min, select_min);
         U64       to = Min(run->text_range.max, select_max);
         if (from < to) {
            Rng2F32 washed = R2F32P(_CARET_XInRun(text, run, from), run->rect.min.y, _CARET_XInRun(text, run, to), run->rect.max.y);
            // On to the next word of the line, if that is selected too: the
            // space between them, so the wash is not in pieces.
            VIEW_Run* next = (i + 1 < app_state->preview_run_count) ? &app_state->preview_runs[i + 1] : 0;
            if (next != 0 && to == run->text_range.max && select_max > next->text_range.min && next->rect.min.y == run->rect.min.y &&
                next->kind == run->kind && next->idx == run->idx && next->rect.min.x > washed.max.x) {
               washed.max.x = next->rect.min.x;
            }
            for (U64 k = 0; k < hits->box_count; k += 1) {
               if (hits->boxes[k].kind == run->kind && hits->boxes[k].idx == run->idx) {
                  washed = Intersect2F32(washed, hits->boxes[k].room);
               }
            }
            if (washed.max.x > washed.min.x && washed.max.y > washed.min.y) {
               Core_D_Rect(washed, R2F32P(0, 0, 1, 1), V4F32(0.26f, 0.60f, 1.0f, 0.35f));
            }
         }
      }

      //- bgz: the caret: solid just after input, then blinking, as the
      // editor's is. In the colour of the words, which shows on the theme.
      if (caret.is_found && caret.rect.min.y >= box->room.min.y - 1.0f && caret.rect.max.y <= box->room.max.y + 1.0f) {
         U64 since_input_us = NowTimeUS() - editor->last_input_us;
         B32 is_on = (since_input_us < 500000) || ((since_input_us / 500000) % 2 == 0);
         if (is_on) {
            VIEW_Colors colors = VIEW_ColorsFromTheme(deck->theme);
            Vec4F32     color = (kind == DECK_PlaceKind_Title) ? colors.title : colors.body;
            DECK_Look*  look = _LOOK_Of(slide, kind, idx);
            if (look->has_color) {
               color = look->color;
            }
            F32         width = Max((F32)(S32)(1.5f * ui), 1.0f);
            F32         x = (F32)(S32)caret.rect.min.x;
            Core_D_Rect(R2F32P(x, caret.rect.min.y, x + width, caret.rect.max.y), R2F32P(0, 0, 1, 1), color);
         }
      }
   }
   return result;
}

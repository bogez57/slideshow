////////////////////////////////
//~ bgz: the edit screen: a slide's pictures, title and words, moved and resized with the mouse
//
// In the preview, each of the slide's pictures - or videos - its title and
// each block of its body (every line under the title, in blocks that @box
// lines start) have a box: where VIEW_DrawSlide put them. With the mouse
// over one, its outline and a handle at each corner show.
//
//    move it          press inside the box and drag
//    resize it        drag a corner. A picture keeps its shape. A block of
//                     words takes any shape: its words wrap at the width, and
//                     those that do not fit the height are not shown
//    line it up       hold Shift while dragging: an edge or the middle of
//                     it jumps to the page's edges, margins and middle, and
//                     to the edges and middles of the slide's other boxes,
//                     when it comes near one. A line shows what it is on,
//                     and an outline the other box the line is from
//    put it back      right-click it: where the layout would have it
//    give up a drag   Esc
//
// What comes of it is a line of the deck's text - "@place picture 0.55 0.2
// 0.4 0.6", in parts of the page (deck.h) - written when the button comes
// up: the text is still all there is to a deck, the line can be changed by
// hand, and one Ctrl+Z takes the move back. While the button is down nothing
// is written: the slide of the deck in memory is given the place the mouse
// makes (DECK_Place's is_set, without a line), and is drawn there.
//
// A press that does not go anywhere is a click, as before: the cursor goes
// to the words (frame_edit.c). So a press inside a box is a click, and only
// watched for a drag. A press on a corner is a drag at once.
//
// All of it is UI boxes (src\ui), built in the edit screen's UI build after
// its slide is laid out for the frame (frame_edit.c), so they are where the
// pieces are now:
//
//    PLACE_BuildHits   boxes that are clicked and not drawn: every corner's
//                      handle, every hit of the slide (a line of words, a
//                      picture) as the piece it is of, and every piece's own
//                      box. In that order: what is built first is asked first,
//                      so a corner is before everything, a block's words are
//                      before a picture they are over, and a picture before
//                      the title's or a block's box under it. Their signals
//                      press, right-press, hover and wheel
//    PLACE_BuildLook   boxes that are drawn and not clicked: the outline and
//                      corners of what the mouse is over, the lines Shift lines
//                      up with - after the slide is laid out again for what
//                      the clicks did
//
// A drag is moved before the slide is laid out (PLACE_BeforeLayout), so the
// slide and its outline are where the mouse is in the same frame. It goes on
// while the button is down, wherever the mouse is: a block resized wraps its
// words again, and the box of the line that was pressed on may be gone.

// The side of a corner's handle, and how far the mouse goes with the button
// down before it is a drag and not a click. In UI units.
#define _PLACE_HANDLE     10.0f
#define _PLACE_DRAG_START 4.0f

// With Shift down: how near an edge comes to a line before it jumps to it.
// In UI units. A slide of three pictures, a title and a body has some fifty
// lines across, each tried by an edge and the middle: at 7 the box was on one
// line or another almost the whole way across the page, jumping between
// them, and could hardly be put anywhere else.
#define _PLACE_SNAP 5.0f

// The least the body's box is made by a corner, as parts of the page - or
// what it was already, if a hand-written line made it less.
#define _PLACE_TEXT_MIN_W 0.2f
#define _PLACE_TEXT_MIN_H 0.12f

// The handle at a corner of box: 1 top left, 2 top right, 3 bottom left, 4
// bottom right.
internal Rng2F32
_PLACE_HandleRect(Rng2F32 box, U32 corner, F32 ui) {
   F32 half = (F32)(S32)(_PLACE_HANDLE * ui * 0.5f);
   F32 x = (corner == 1 || corner == 3) ? box.min.x : box.max.x;
   F32 y = (corner == 1 || corner == 2) ? box.min.y : box.max.y;
   return R2F32P(x - half, y - half, x + half, y + half);
}

// Whether the slide's boxes, as laid out, are of the text as it is.
internal B32
_PLACE_BoxesAreGood(App_State* app_state) {
   return (app_state->editor != 0 && app_state->editor->edit_count == app_state->preview_hits_edit_count &&
           !app_state->show_open && app_state->slide_idx < app_state->deck->slide_count &&
           Dim2F32(app_state->preview_page).x > 0 && Dim2F32(app_state->preview_page).y > 0);
}

// The slide's place of a kind: a picture's is the picture's own, and a
// block's of the body the block's own. idx says which (VIEW_Box's).
internal DECK_Place*
_PLACE_PlaceOf(DECK_Slide* slide, DECK_PlaceKind kind, U64 idx) {
   DECK_Place* result = &slide->places[kind];
   if (kind == DECK_PlaceKind_Picture && idx < slide->picture_count) {
      result = &slide->pictures[idx].place;
   }
   if (kind == DECK_PlaceKind_Text && idx < slide->box_count) {
      result = &slide->boxes[idx].place;
   }
   return result;
}

// Which block of the slide's body (DECK_Box) has the line whose words are at
// range: max_U64 for none - the title, or a picture.
internal U64
_PLACE_BlockOfRange(DECK_Slide* slide, Rng1U64 range) {
   U64 result = max_U64;
   for (U64 block_idx = 0; block_idx < slide->box_count && result == max_U64; block_idx += 1) {
      DECK_Box*  block = &slide->boxes[block_idx];
      DECK_Item* item = block->first_item;
      for (U64 i = 0; i < block->item_count && item != 0; i += 1, item = item->next) {
         if (item->text_range.min == range.min && item->text_range.max == range.max) {
            result = block_idx;
            break;
         }
      }
   }
   return result;
}

// Which box a point is on: 0 for none. Where boxes are over each other, a
// point on a line of the body's words is on the block that line is in, one
// on the title's words on the title, and any other on the picture drawn
// last - the one on top - then the title, then the block drawn last.
//
// The words are the ones a click there would go to (_EDIT_PreviewHitAt): of
// two blocks over each other, those of the one drawn later. Not merely
// whichever block's box is last under the point - that box can be mostly
// empty, over the words of another.
internal VIEW_Box*
_PLACE_BoxAt(App_State* app_state, Vec2F32 pos) {
   DECK_Slide* slide = &app_state->deck->slides[app_state->slide_idx];

   // What words the point is on, if any: the title's, or which block's.
   B32 on_title_words = 0;
   U64 words_block = max_U64;
   S64 hit_idx = _EDIT_PreviewHitAt(app_state, pos);
   if (hit_idx >= 0) {
      Rng1U64 range = app_state->preview_hits[hit_idx].text_range;
      on_title_words = (range.min == slide->title_range.min && range.max == slide->title_range.max);
      words_block = _PLACE_BlockOfRange(slide, range);
   }

   VIEW_Box* title = 0;
   VIEW_Box* text = 0;
   VIEW_Box* words_text = 0;
   VIEW_Box* picture = 0;
   for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
      VIEW_Box* box = &app_state->preview_boxes[i];
      if (Contains2F32(box->rect, pos)) {
         if (box->kind == DECK_PlaceKind_Title) {
            title = box;
         } else if (box->kind == DECK_PlaceKind_Text) {
            text = box;
            if (box->idx == words_block) {
               words_text = box;
            }
         } else if (box->idx < slide->picture_count) {
            picture = box;
         }
      }
   }

   VIEW_Box* result = 0;
   if (words_text != 0) {
      result = words_text;
   } else if (title != 0 && on_title_words) {
      result = title;
   } else if (picture != 0) {
      result = picture;
   } else if (title != 0) {
      result = title;
   } else {
      result = text;
   }
   return result;
}

// Writes the slide's @place line for kind - for the idx-th picture, or block
// of the body: over the one it has (before), or as a new line under the
// picture's line, under the title's, or above the block's first.
internal void
_PLACE_WriteLine(ED_Editor* editor, DECK_Slide* slide, DECK_PlaceKind kind, U64 idx, DECK_Place* before, Rng2F32 rect) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 line = PushStr8F(scratch.arena, "@place %S %.3f %.3f %.3f %.3f", DECK_PlaceName(kind), rect.min.x, rect.min.y,
                            rect.max.x - rect.min.x, rect.max.y - rect.min.y);
   _EDIT_PutLineFor(editor, slide, kind, idx, before->has_line, before->line, line);
   ScratchEnd(scratch);
}

// Takes an @place line out of the text, with its line ending.
internal void
_PLACE_RemoveLine(ED_Editor* editor, DECK_Place* place) {
   if (place->has_line) {
      _EDIT_TakeOutLine(editor, place->line);
   }
}

// The least a corner makes the thing dragged, as parts of the page: the
// body's and the title's box more than a picture's. Their words wrap at the
// width and get smaller only so far, so a box much narrower than this has a
// word a line, running down off the page.
internal Vec2F32
_PLACE_MinSize(EDIT_PlaceDrag* drag) {
   B32     is_text = (drag->kind != DECK_PlaceKind_Picture);
   F32     w0 = drag->start.max.x - drag->start.min.x;
   F32     h0 = drag->start.max.y - drag->start.min.y;
   Vec2F32 result = V2F32(DECK_PLACE_MIN, DECK_PLACE_MIN);
   if (is_text) {
      result.x = Min(_PLACE_TEXT_MIN_W, w0);
      result.y = Min(_PLACE_TEXT_MIN_H, h0);
   }
   return result;
}

// Where the mouse has the thing being dragged, in parts of the page.
internal Rng2F32
_PLACE_RectFromMouse(EDIT_PlaceDrag* drag, Rng2F32 page, Vec2F32 mouse) {
   Vec2F32 page_dim = Dim2F32(page);
   F32     dx = (mouse.x - drag->press_mouse.x) / page_dim.x;
   F32     dy = (mouse.y - drag->press_mouse.y) / page_dim.y;
   Rng2F32 start = drag->start;
   F32     w0 = start.max.x - start.min.x;
   F32     h0 = start.max.y - start.min.y;
   Rng2F32 result = start;

   if (drag->corner == 0) {
      // All of it, kept on the page - if it was all on the page.
      if (start.min.x >= 0 && start.max.x <= 1.0f) {
         dx = Clamp(-start.min.x, dx, 1.0f - start.max.x);
      }
      if (start.min.y >= 0 && start.max.y <= 1.0f) {
         dy = Clamp(-start.min.y, dy, 1.0f - start.max.y);
      }
      result = R2F32P(start.min.x + dx, start.min.y + dy, start.max.x + dx, start.max.y + dy);
   } else {
      // The corner across from the one held stays where it is.
      B32 is_left = (drag->corner == 1 || drag->corner == 3);
      B32 is_top = (drag->corner == 1 || drag->corner == 2);
      F32 anchor_x = is_left ? start.max.x : start.min.x;
      F32 anchor_y = is_top ? start.max.y : start.min.y;

      Vec2F32 min_size = _PLACE_MinSize(drag);
      F32     min_w = min_size.x;
      F32     min_h = min_size.y;

      // The most there is room for on the page, from that corner.
      F32 max_w = Max(is_left ? anchor_x : 1.0f - anchor_x, min_w);
      F32 max_h = Max(is_top ? anchor_y : 1.0f - anchor_y, min_h);
      F32 w = Clamp(min_w, w0 + (is_left ? -dx : dx), max_w);
      F32 h = Clamp(min_h, h0 + (is_top ? -dy : dy), max_h);

      if (drag->kind == DECK_PlaceKind_Picture && w0 > 0 && h0 > 0) {
         // A picture keeps its shape: as large as the further of the two
         // ways the mouse has gone asks for, and no larger than fits.
         F32 grow = Max(w / w0, h / h0);
         grow = Min(grow, Min(max_w / w0, max_h / h0));
         grow = Max(grow, DECK_PLACE_MIN / Min(w0, h0));
         w = w0 * grow;
         h = h0 * grow;
      }

      result.min.x = is_left ? anchor_x - w : anchor_x;
      result.max.x = is_left ? anchor_x : anchor_x + w;
      result.min.y = is_top ? anchor_y - h : anchor_y;
      result.max.y = is_top ? anchor_y : anchor_y + h;
   }

   return result;
}

// A line something can be lined up with, as a part of the page across or
// down, and the box of the slide's it is an edge or the middle of: -1 for
// the page's own edges, middle and margins.
typedef struct _PLACE_Line _PLACE_Line;
struct _PLACE_Line {
   F32 at;
   S32 box_idx;
};

// The line nearest value among lines that is within reach, and that moving
// value onto would not move it by less than shift_lo or more than shift_hi:
// those say how far it can go before it is off the page or too small. Its
// index, and how far value is from it (to add) - or -1 if there is none.
// The nearest that can be had, not the nearest: a line just past the page's
// edge must not hide one a little further in.
internal S64
_PLACE_NearestLine(F32 value, _PLACE_Line* lines, U64 line_count, F32 reach, F32 shift_lo, F32 shift_hi, F32* shift_out) {
   S64 result = -1;
   F32 best = reach;
   for (U64 i = 0; i < line_count; i += 1) {
      F32 gap = lines[i].at - value;
      F32 gap_size = (gap < 0) ? -gap : gap;
      if (gap_size <= best && gap >= shift_lo && gap <= shift_hi) {
         best = gap_size;
         *shift_out = gap;
         result = (S64)i;
      }
   }
   return result;
}

// Puts what line_idx of lines is into the drag, across (is_x) or down, to be
// drawn: the line, and the box it came from.
internal void
_PLACE_SetGuide(EDIT_PlaceDrag* drag, B32 is_x, _PLACE_Line* lines, S64 line_idx, Rng2F32* boxes) {
   _PLACE_Line* line = &lines[line_idx];
   B32          has_box = (line->box_idx >= 0);
   Rng2F32      box = has_box ? boxes[line->box_idx] : R2F32P(0, 0, 0, 0);
   if (is_x) {
      drag->has_guide_x = 1;
      drag->guide_x = line->at;
      drag->has_guide_box_x = has_box;
      drag->guide_box_x = box;
   } else {
      drag->has_guide_y = 1;
      drag->guide_y = line->at;
      drag->has_guide_box_y = has_box;
      drag->guide_box_y = box;
   }
}

// With Shift down: rect, where the mouse has the thing dragged, lined up
// with what it is near - the page's edges, margins and middle, and the edges
// and middles of the slide's other boxes. Across and down each on their
// own. A move tries its two edges and its middle; a corner only the edges
// it is moving, and a picture, which keeps its shape, the nearer of the two,
// the other following.
//
// A jump never does what the mouse could not: take something on the page
// off it, or make it smaller than a corner can. Where the line nearest is
// past that, the next nearest is taken, or none.
// What it was lined up with goes into the drag, to be drawn.
internal Rng2F32
_PLACE_LinedUp(App_State* app_state, EDIT_PlaceDrag* drag, Rng2F32 rect, F32 ui) {
   Rng2F32 page = app_state->preview_page;
   Vec2F32 page_dim = Dim2F32(page);
   drag->has_guide_x = 0;
   drag->has_guide_y = 0;
   if (page_dim.x <= 0 || page_dim.y <= 0) {
      return rect;
   }

   //- bgz: the other boxes, and the lines there are, as parts of the page
   Rng2F32     boxes[VIEW_MAX_BOXES];
   _PLACE_Line xs[3 * ArrayCount(boxes) + 5];
   _PLACE_Line ys[3 * ArrayCount(boxes) + 5];
   U64         x_count = 0;
   U64         y_count = 0;
   F32         page_xs[5] = {0.0f, 0.5f, 1.0f, _VIEW_MARGIN_X / VIEW_PAGE_W, 1.0f - _VIEW_MARGIN_X / VIEW_PAGE_W};
   F32         page_ys[5] = {0.0f, 0.5f, 1.0f, _VIEW_MARGIN_TOP / VIEW_PAGE_H, 1.0f - _VIEW_MARGIN_BOTTOM / VIEW_PAGE_H};
   for (U64 i = 0; i < 5; i += 1) {
      xs[x_count++] = (_PLACE_Line){page_xs[i], -1};
      ys[y_count++] = (_PLACE_Line){page_ys[i], -1};
   }
   U64 box_count = 0;
   for (U64 i = 0; i < app_state->preview_box_count && box_count < ArrayCount(boxes); i += 1) {
      VIEW_Box* box = &app_state->preview_boxes[i];
      B32       is_dragged = (box->kind == drag->kind && box->idx == drag->idx);
      if (is_dragged) {
         continue;
      }
      Rng2F32 r = R2F32P((box->rect.min.x - page.min.x) / page_dim.x, (box->rect.min.y - page.min.y) / page_dim.y,
                         (box->rect.max.x - page.min.x) / page_dim.x, (box->rect.max.y - page.min.y) / page_dim.y);
      S32 box_idx = (S32)box_count;
      boxes[box_count++] = r;
      xs[x_count++] = (_PLACE_Line){r.min.x, box_idx};
      xs[x_count++] = (_PLACE_Line){(r.min.x + r.max.x) * 0.5f, box_idx};
      xs[x_count++] = (_PLACE_Line){r.max.x, box_idx};
      ys[y_count++] = (_PLACE_Line){r.min.y, box_idx};
      ys[y_count++] = (_PLACE_Line){(r.min.y + r.max.y) * 0.5f, box_idx};
      ys[y_count++] = (_PLACE_Line){r.max.y, box_idx};
   }

   F32     reach_x = _PLACE_SNAP * ui / page_dim.x;
   F32     reach_y = _PLACE_SNAP * ui / page_dim.y;
   F32     w = rect.max.x - rect.min.x;
   F32     h = rect.max.y - rect.min.y;
   Rng2F32 result = rect;

   if (drag->corner == 0) {
      //- bgz: a move: the nearest of what its edges and its middle are near
      // Kept on the page, as the mouse keeps it - if it was all on the page.
      // One a hand-written line put partly off can go anywhere.
      F32 lo_x = -1.0f, hi_x = 1.0f, lo_y = -1.0f, hi_y = 1.0f;
      if (drag->start.min.x >= 0 && drag->start.max.x <= 1.0f) {
         lo_x = -rect.min.x;
         hi_x = 1.0f - rect.max.x;
      }
      if (drag->start.min.y >= 0 && drag->start.max.y <= 1.0f) {
         lo_y = -rect.min.y;
         hi_y = 1.0f - rect.max.y;
      }
      F32 own_x[3] = {rect.min.x, (rect.min.x + rect.max.x) * 0.5f, rect.max.x};
      F32 own_y[3] = {rect.min.y, (rect.min.y + rect.max.y) * 0.5f, rect.max.y};
      F32 best_x = reach_x;
      F32 best_y = reach_y;
      for (U64 i = 0; i < 3; i += 1) {
         F32 shift = 0;
         S64 line_idx = _PLACE_NearestLine(own_x[i], xs, x_count, best_x, lo_x, hi_x, &shift);
         if (line_idx >= 0) {
            best_x = (shift < 0) ? -shift : shift;
            result.min.x = rect.min.x + shift;
            result.max.x = rect.max.x + shift;
            _PLACE_SetGuide(drag, 1, xs, line_idx, boxes);
         }
         line_idx = _PLACE_NearestLine(own_y[i], ys, y_count, best_y, lo_y, hi_y, &shift);
         if (line_idx >= 0) {
            best_y = (shift < 0) ? -shift : shift;
            result.min.y = rect.min.y + shift;
            result.max.y = rect.max.y + shift;
            _PLACE_SetGuide(drag, 0, ys, line_idx, boxes);
         }
      }
   } else {
      //- bgz: a corner: the edges it moves
      B32     is_left = (drag->corner == 1 || drag->corner == 3);
      B32     is_top = (drag->corner == 1 || drag->corner == 2);
      B32     keeps_shape = (drag->kind == DECK_PlaceKind_Picture && w > 0 && h > 0);
      F32     anchor_x = is_left ? rect.max.x : rect.min.x;
      F32     anchor_y = is_top ? rect.max.y : rect.min.y;
      Vec2F32 min_size = _PLACE_MinSize(drag);

      // How wide and high it can be made: from the least a corner makes it
      // to what there is room for on the page from the corner that stays,
      // as _PLACE_RectFromMouse has it. A picture's width and height go
      // together, so each holds the other to its own.
      F32 lo_w = min_size.x;
      F32 lo_h = min_size.y;
      F32 hi_w = Max(is_left ? anchor_x : 1.0f - anchor_x, lo_w);
      F32 hi_h = Max(is_top ? anchor_y : 1.0f - anchor_y, lo_h);
      if (keeps_shape) {
         F32 lo_w2 = Max(lo_w, lo_h * w / h);
         F32 hi_w2 = Min(hi_w, hi_h * w / h);
         F32 lo_h2 = Max(lo_h, lo_w * h / w);
         F32 hi_h2 = Min(hi_h, hi_w * h / w);
         lo_w = lo_w2, hi_w = hi_w2, lo_h = lo_h2, hi_h = hi_h2;
      }

      // The edge moved goes onto the line; how far it goes is the width's
      // change, or the opposite of it at the left.
      F32 shift_x = 0;
      F32 shift_y = 0;
      S64 line_x = _PLACE_NearestLine(is_left ? rect.min.x : rect.max.x, xs, x_count, reach_x,
                                      is_left ? w - hi_w : lo_w - w, is_left ? w - lo_w : hi_w - w, &shift_x);
      S64 line_y = _PLACE_NearestLine(is_top ? rect.min.y : rect.max.y, ys, y_count, reach_y,
                                      is_top ? h - hi_h : lo_h - h, is_top ? h - lo_h : hi_h - h, &shift_y);

      // A picture: the nearer of the two on the screen, the other way
      // following from the corner that stays.
      if (keeps_shape && line_x >= 0 && line_y >= 0) {
         F32 gap_x = ((shift_x < 0) ? -shift_x : shift_x) * page_dim.x;
         F32 gap_y = ((shift_y < 0) ? -shift_y : shift_y) * page_dim.y;
         if (gap_x <= gap_y) {
            line_y = -1;
         } else {
            line_x = -1;
         }
      }

      F32 new_w = w;
      F32 new_h = h;
      if (line_x >= 0) {
         new_w = w + (is_left ? -shift_x : shift_x);
         if (keeps_shape) {
            new_h = h * new_w / w;
         }
         _PLACE_SetGuide(drag, 1, xs, line_x, boxes);
      }
      if (line_y >= 0) {
         new_h = h + (is_top ? -shift_y : shift_y);
         if (keeps_shape) {
            new_w = w * new_h / h;
         }
         _PLACE_SetGuide(drag, 0, ys, line_y, boxes);
      }
      result.min.x = is_left ? anchor_x - new_w : anchor_x;
      result.max.x = is_left ? anchor_x : anchor_x + new_w;
      result.min.y = is_top ? anchor_y - new_h : anchor_y;
      result.max.y = is_top ? anchor_y : anchor_y + new_h;
      // The line itself, not where adding to the width put it - the same
      // but for the last bit of a float.
      if (line_x >= 0) {
         if (is_left) result.min.x = xs[line_x].at; else result.max.x = xs[line_x].at;
      }
      if (line_y >= 0) {
         if (is_top) result.min.y = ys[line_y].at; else result.max.y = ys[line_y].at;
      }
   }

   return result;
}

internal void
_PLACE_Begin(App_State* app_state, VIEW_Box* from, U32 corner, Vec2F32 pos, EDIT_PlaceDragState state) {
   EDIT_PlaceDrag* drag = &app_state->place_drag;
   Rng2F32         page = app_state->preview_page;
   Vec2F32         page_dim = Dim2F32(page);
   Rng2F32         box = from->rect;
   DECK_PlaceKind  kind = from->kind;
   U64             idx = from->idx;

   MemoryZeroStruct(drag);
   drag->state = state;
   drag->kind = kind;
   drag->idx = idx;
   drag->corner = corner;
   drag->slide_idx = app_state->slide_idx;
   drag->edit_count = app_state->editor->edit_count;
   drag->press_mouse = pos;
   drag->start = R2F32P((box.min.x - page.min.x) / page_dim.x, (box.min.y - page.min.y) / page_dim.y,
                        (box.max.x - page.min.x) / page_dim.x, (box.max.y - page.min.y) / page_dim.y);
   drag->now = drag->start;
   drag->before = *_PLACE_PlaceOf(&app_state->deck->slides[app_state->slide_idx], kind, idx);
}


// The box the slide's piece of kind and idx was laid out in, this frame: 0
// if it has none - a picture behind the words.
internal VIEW_Box*
_PLACE_BoxOf(App_State* app_state, DECK_PlaceKind kind, U64 idx) {
   VIEW_Box* result = 0;
   for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
      if (app_state->preview_boxes[i].kind == kind && app_state->preview_boxes[i].idx == idx) {
         result = &app_state->preview_boxes[i];
      }
   }
   return result;
}

// Which piece of the slide a hit of the preview is of: the title's words,
// a line of a block's, or a picture. 0 if none - a hit is always one of
// those, but the text can be ahead of the slide drawn.
internal B32
_PLACE_PieceOfHit(DECK_Slide* slide, VIEW_Hit* hit, DECK_PlaceKind* kind_out, U64* idx_out) {
   B32 result = 0;
   U64 block_idx = _PLACE_BlockOfRange(slide, hit->text_range);
   if (hit->text_range.min == slide->title_range.min && hit->text_range.max == slide->title_range.max) {
      *kind_out = DECK_PlaceKind_Title;
      *idx_out = 0;
      result = 1;
   } else if (block_idx != max_U64) {
      *kind_out = DECK_PlaceKind_Text;
      *idx_out = block_idx;
      result = 1;
   } else {
      for (U64 i = 0; i < slide->picture_count && !result; i += 1) {
         if (slide->pictures[i].range.min == hit->text_range.min && slide->pictures[i].range.max == hit->text_range.max) {
            *kind_out = DECK_PlaceKind_Picture;
            *idx_out = i;
            result = 1;
         }
      }
   }
   return result;
}

internal void
PLACE_Keys(App_State* app_state) {
   EDIT_PlaceDrag* drag = &app_state->place_drag;
   if (drag->state == EDIT_PlaceDragState_Dragging && Core_WM_KeyPress(&app_state->events, app_state->window, 0, WM_Key_Esc)) {
      DECK_Deck* deck = app_state->deck;
      if (drag->slide_idx < deck->slide_count) {
         *_PLACE_PlaceOf(&deck->slides[drag->slide_idx], drag->kind, drag->idx) = drag->before;
      }
      drag->state = EDIT_PlaceDragState_None;
      // The box held lets go too: its release is no click.
      UI_KillAction();
   }
}

internal void
PLACE_BeforeLayout(App_State* app_state, F32 ui) {
   EDIT_PlaceDrag* drag = &app_state->place_drag;
   ED_Editor*      editor = app_state->editor;
   DECK_Deck*      deck = app_state->deck;
   if (drag->state == EDIT_PlaceDragState_None) {
      return;
   }

   B32 is_same = (editor != 0 && editor->edit_count == drag->edit_count && drag->slide_idx == app_state->slide_idx &&
                  drag->slide_idx < deck->slide_count && !app_state->show_open);
   if (!is_same) {
      // The text has changed under it, or the slide: given up. A deck made
      // again from the text has the place the text says already.
      if (editor != 0 && editor->edit_count == drag->edit_count && drag->slide_idx < deck->slide_count) {
         *_PLACE_PlaceOf(&deck->slides[drag->slide_idx], drag->kind, drag->idx) = drag->before;
      }
      drag->state = EDIT_PlaceDragState_None;
      return;
   }

   DECK_Slide* slide = &deck->slides[drag->slide_idx];
   DECK_Place* place = _PLACE_PlaceOf(slide, drag->kind, drag->idx);
   // The window's mouse, not the UI's: the UI puts it off the window when
   // the window is not the one in front and it has not moved for a moment.
   Vec2F32 mouse = Core_WM_MouseFromWindow(app_state->window);

   // The button held since the press. Not asked of the box pressed: a
   // resized block wraps its words again, and the line pressed on may be
   // gone, and its box with it.
   if (!Core_WM_KeyIsDown(WM_Key_LeftMouseButton)) {
      if (drag->state == EDIT_PlaceDragState_Dragging) {
         *place = drag->before;
         _PLACE_WriteLine(editor, slide, drag->kind, drag->idx, &drag->before, drag->now);
         editor->last_input_us = NowTimeUS();
      }
      drag->state = EDIT_PlaceDragState_None;
      return;
   }

   if (drag->state == EDIT_PlaceDragState_Pressed) {
      F32 reach = _PLACE_DRAG_START * ui;
      F32 gone_x = mouse.x - drag->press_mouse.x;
      F32 gone_y = mouse.y - drag->press_mouse.y;
      if (gone_x > reach || gone_x < -reach || gone_y > reach || gone_y < -reach) {
         drag->state = EDIT_PlaceDragState_Dragging;
      }
   }
   if (drag->state == EDIT_PlaceDragState_Dragging) {
      drag->now = _PLACE_RectFromMouse(drag, app_state->preview_page, mouse);
      // Shift lines it up with what it is near.
      drag->has_guide_x = 0;
      drag->has_guide_y = 0;
      if (Core_WM_KeyIsDown(WM_Key_Shift)) {
         drag->now = _PLACE_LinedUp(app_state, drag, drag->now, ui);
      }
      place->is_set = 1;
      place->rect = drag->now;
   }
}

// One of the boxes of a piece of the slide - kind and idx, has_piece 0 for
// one that is of none - asked for its signal. corner: of a corner's handle,
// else 0. hit_idx: of a hit of the preview, else -1. 1 if what was done to it
// changed the text, the cursor or a block's scroll.
internal B32
_PLACE_Signal(App_State* app_state, UI_Box* box, B32 has_piece, DECK_PlaceKind kind, U64 idx, U32 corner, S64 hit_idx,
              PLACE_Hover* hover) {
   B32       result = 0;
   UI_Signal signal = UI_SignalFromBox(box);
   VIEW_Box* piece = has_piece ? _PLACE_BoxOf(app_state, kind, idx) : 0;

   if (UI_Hovering(signal) && !hover->is_over) {
      hover->is_over = 1;
      hover->has_piece = (piece != 0);
      hover->kind = kind;
      hover->idx = idx;
      hover->corner = corner;
      hover->hit_idx = hit_idx;
   }

   if (signal.f & UI_SignalFlag_LeftPressed) {
      Vec2F32 pos = UI_DragStartMouse();
      if (corner != 0 && piece != 0) {
         _PLACE_Begin(app_state, piece, corner, pos, EDIT_PlaceDragState_Dragging);
      } else {
         // A press is a click until the mouse goes somewhere: to the words,
         // or the picture (frame_edit.c) - and watched, for a drag.
         EDIT_PreviewClick(app_state, pos, !!(signal.event_flags & WM_Modifier_Shift));
         if (piece != 0) {
            _PLACE_Begin(app_state, piece, 0, pos, EDIT_PlaceDragState_Pressed);
         }
      }
      result = 1;
   }

   if ((signal.f & UI_SignalFlag_RightPressed) && piece != 0) {
      DECK_Place* place = _PLACE_PlaceOf(&app_state->deck->slides[app_state->slide_idx], kind, idx);
      if (place->has_line) {
         _PLACE_RemoveLine(app_state->editor, place);
         app_state->editor->last_input_us = NowTimeUS();
         FR_Notice(app_state, Str8Lit("Back where the layout puts it"));
         result = 1;
      }
   }

   // The wheel over a block's words moves them (frame_caret.c). Over
   // anything else of the slide it does nothing - not the block under a
   // picture.
   if (signal.scroll.y != 0 && has_piece && kind == DECK_PlaceKind_Text) {
      CARET_ScrollBlock(app_state, idx, (F32)signal.scroll.y / FR_UI_SCROLL_PER_NOTCH);
      result = 1;
   }

   return result;
}

internal B32
PLACE_BuildCorners(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover) {
   B32 result = 0;
   if (!interactive || !_PLACE_BoxesAreGood(app_state)) {
      return 0;
   }
   UI_BoxFlags flags = UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll;

   // Their handles reach past their boxes, over whatever else is there -
   // a video's strip too. The last drawn, which is on top, first.
   for (U64 i = app_state->preview_box_count; i > 0; i -= 1) {
      VIEW_Box* box = &app_state->preview_boxes[i - 1];
      for (U32 corner = 1; corner <= 4; corner += 1) {
         UI_Box* handle = FR_BuildRectBox(flags, _PLACE_HandleRect(box->rect, corner, ui), "###place corner %d %llu %u",
                                          (int)box->kind, (unsigned long long)box->idx, corner);
         result |= _PLACE_Signal(app_state, handle, 1, box->kind, box->idx, corner, -1, hover);
      }
   }
   return result;
}

internal B32
PLACE_BuildHits(App_State* app_state, F32 ui, B32 interactive, PLACE_Hover* hover) {
   B32 result = 0;
   // Not built at all under the open screen: they are only to be clicked.
   if (!interactive || !_PLACE_BoxesAreGood(app_state)) {
      return 0;
   }
   DECK_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   UI_BoxFlags flags = UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll;
   UnusedVariable(ui);

   //- bgz: the words and the pictures, as drawn: the last drawn first, which
   // is the one on top. A block's words are over a picture under them
   for (U64 i = app_state->preview_hit_count; i > 0; i -= 1) {
      VIEW_Hit*      hit = &app_state->preview_hits[i - 1];
      DECK_PlaceKind kind = DECK_PlaceKind_Title;
      U64            idx = 0;
      B32            has_piece = _PLACE_PieceOfHit(slide, hit, &kind, &idx);
      UI_Box*        box = FR_BuildRectBox(flags, hit->rect, "###place hit %llu", (unsigned long long)(i - 1));
      // Over words the mouse is the bar that text is typed at: a click
      // there puts the caret at the letter (frame_caret.c).
      if (has_piece && kind != DECK_PlaceKind_Picture) {
         box->hover_cursor = WM_Cursor_IBar;
      }
      result |= _PLACE_Signal(app_state, box, has_piece, kind, idx, 0, (S64)(i - 1), hover);
   }

   //- bgz: the pieces' own boxes, where no words are: a picture over the
   // title's or a block's, and the title's over a block's
   DECK_PlaceKind kinds[] = {DECK_PlaceKind_Picture, DECK_PlaceKind_Title, DECK_PlaceKind_Text};
   for (U64 k = 0; k < ArrayCount(kinds); k += 1) {
      for (U64 i = app_state->preview_box_count; i > 0; i -= 1) {
         VIEW_Box* box = &app_state->preview_boxes[i - 1];
         if (box->kind != kinds[k] || (box->kind == DECK_PlaceKind_Picture && box->idx >= slide->picture_count)) {
            continue;
         }
         UI_Box* ui_box = FR_BuildRectBox(flags, box->rect, "###place box %d %llu", (int)box->kind, (unsigned long long)box->idx);
         result |= _PLACE_Signal(app_state, ui_box, 1, box->kind, box->idx, 0, -1, hover);
      }
   }

   return result;
}

internal B32
PLACE_BuildLook(App_State* app_state, F32 ui, PLACE_Hover* hover) {
   B32             result = 0;
   EDIT_PlaceDrag* drag = &app_state->place_drag;
   if (app_state->show_open || app_state->slide_idx >= app_state->deck->slide_count) {
      return 0;
   }

   B32       is_dragging = (drag->state == EDIT_PlaceDragState_Dragging);
   VIEW_Box* over = 0;
   if (is_dragging) {
      over = _PLACE_BoxOf(app_state, drag->kind, drag->idx);
   } else if (_PLACE_BoxesAreGood(app_state) && hover->is_over && hover->has_piece) {
      over = _PLACE_BoxOf(app_state, hover->kind, hover->idx);
   }

   //- bgz: what a drag with Shift down is lined up with: a line the whole
   // way across the page, or down it, and the box each line came from
   if (is_dragging) {
      Rng2F32 page = app_state->preview_page;
      Vec2F32 page_dim = Dim2F32(page);
      F32     guide_thick = Max(1.0f, (F32)(S32)ui);
      UI_ColorTag(Str8Lit("guide")) {
         if (drag->has_guide_x) {
            F32 x = (F32)(S32)(page.min.x + drag->guide_x * page_dim.x - guide_thick * 0.5f);
            x = Clamp(page.min.x, x, page.max.x - guide_thick);
            FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(x, page.min.y, x + guide_thick, page.max.y), "###place guide x");
         }
         if (drag->has_guide_y) {
            F32 y = (F32)(S32)(page.min.y + drag->guide_y * page_dim.y - guide_thick * 0.5f);
            y = Clamp(page.min.y, y, page.max.y - guide_thick);
            FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(page.min.x, y, page.max.x, y + guide_thick), "###place guide y");
         }
         Vec4F32 box_color = UI_ColorFromSlot(UI_ColorSlot_Border);
         for (U32 axis = 0; axis < 2; axis += 1) {
            B32     has_box = (axis == 0) ? (drag->has_guide_x && drag->has_guide_box_x) : (drag->has_guide_y && drag->has_guide_box_y);
            Rng2F32 from = (axis == 0) ? drag->guide_box_x : drag->guide_box_y;
            if (!has_box || (axis == 1 && drag->has_guide_x && drag->has_guide_box_x && MemoryMatchStruct(&from, &drag->guide_box_x))) {
               continue;
            }
            Rng2F32 box = R2F32P((F32)(S32)(page.min.x + from.min.x * page_dim.x), (F32)(S32)(page.min.y + from.min.y * page_dim.y),
                                 (F32)(S32)(page.min.x + from.max.x * page_dim.x), (F32)(S32)(page.min.y + from.max.y * page_dim.y));
            FR_BuildEdge(box, guide_thick, box_color, "###place guide box %u", axis);
         }
      }
   }

   //- bgz: the box the mouse is over, or that is dragged, with its corners.
   // In the theme's accent: it is about the slide's own piece
   if (over != 0) {
      Rng2F32 box = over->rect;
      Vec4F32 accent = VIEW_ColorsFromTheme(app_state->deck->theme).accent;
      F32     thick = Max(1.0f, (F32)(S32)(1.5f * ui));

      for (U32 corner = 1; corner <= 4; corner += 1) {
         Rng2F32 handle = _PLACE_HandleRect(box, corner, ui);
         B32     is_hot = is_dragging ? (drag->corner == corner) : (hover->corner == corner);
         UI_SetNextBackgroundColor(accent);
         UI_Box* outer = FR_BuildRectBox(UI_BoxFlag_DrawBackground, handle, "###place look corner %u", corner);
         UI_Parent(outer) UI_ColorTag(Str8Lit("handle")) {
            Rng2F32 inner = R2F32P(thick, thick, Dim2F32(handle).x - thick, Dim2F32(handle).y - thick);
            if (is_hot) {
               UI_SetNextBackgroundColor(accent);
            }
            UI_SetNextRect(inner);
            UI_BuildBoxFromStringf(UI_BoxFlag_DrawBackground, "###place look corner inner %u", corner);
         }
      }

      Vec4F32 line_color = accent;
      line_color.w = is_dragging ? 1.0f : 0.7f;
      FR_BuildEdge(box, thick, line_color, "###place look outline");
      result = 1;
   }

   return result;
}

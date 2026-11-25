////////////////////////////////
//~ bgz: the edit screen: a slide's pictures, title and words, moved and resized with the mouse
//
// In the preview, each of the slide's pictures - or videos - its title and
// each block of its body (every line under the title, in blocks that @box
// lines start) have a box: where View_DrawSlide put them. With the mouse
// over one, its outline and its handles show: one at each corner, and one in
// the middle of each side long enough to have it clear of them.
//
//    move it          press inside the box and drag
//    resize it        drag a corner, or the handle in the middle of a side:
//                     a side changes only the width, or only the height.
//                     A picture keeps its shape. A block of
//                     words takes any shape: its words wrap at the width, and
//                     those that do not fit the height are not shown
//    line it up       hold Shift while dragging: an edge or the middle of
//                     it jumps to the page's edges, margins and middle, and
//                     to the edges and middles of the slide's other boxes,
//                     when it comes near one. A line shows what it is on,
//                     and an outline the other box the line is from
//    pick it          click it away from its words - a picture anywhere:
//                     Delete then takes it out (edit\edit.c). Ctrl+click
//                     adds more, Shift+click picks all of the kind
//    move several     drag one of several picked: the others go with it,
//                     as far, each kept on the page. One line each, written
//                     together, one step to undo
//    nudge it         the arrow keys, with pieces picked: a small step,
//                     and a larger one with Shift
//    line them up     with several picked, Ctrl+Shift and an arrow: their
//                     left, right, top or bottom edges on one line.
//                     Ctrl+Shift+C their middles on a line down the page,
//                     Ctrl+Shift+M on one across it
//    space them       with three or more, Ctrl+Shift+H across and
//                     Ctrl+Shift+V down: the same room between each two
//    size its words   Ctrl and the wheel over the title or a block
//                     (look\look.c)
//    put it back      right-click it, and "Put back where the layout has it"
//                     in the menu (edit\edit.c)
//    give up a drag   Esc - or F5, which presents
//
// The mouse says which it would be (_Place_HoverCursor): over a handle the
// arrow that resizes the way it does, the four-way arrow while a piece is
// dragged, and the bar that text is typed at right at the title's and blocks'
// words (Place_IsAtWords) - the four-way arrow in the rest of their boxes,
// and over a picture, which is where they are taken hold of to be moved.
//
// What comes of it is a line of the deck's text - "@place picture 0.55 0.2
// 0.4 0.6", in parts of the page (deck.h) - written when the button comes
// up, by a state op (Place_WriteLine): the text is still all there is to a
// deck, the line can be changed by hand, and one Ctrl+Z takes the move back.
// While the button is down nothing is written, and nothing of the deck is
// changed: the drag is the mouse's own (g_place_view.drag), and the slide is
// given the place the mouse makes only while it is drawn (Place_PutDrag,
// Place_TakeDrag), as a block's scroll is (caret\caret.c).
//
// A press that does not go anywhere is a click, as before: the cursor goes
// to the words (edit\edit.c). So a press inside a box is a click, and only
// watched for a drag. A press on a handle is a drag at once.
//
// A press right at the words (Place_IsAtWords) moves nothing: held and
// taken across them it selects them, as in the text pane, to be copied, cut
// or typed over (Place_DragState_Selecting, Caret_DragTo). The rest of
// a box is where it is taken hold of to be moved.
//
// All of it is UI boxes (src\ui), built in the edit screen's UI build after
// its slide is laid out for the frame (edit\edit.c), so they are where the
// pieces are now:
//
//    Place_BuildHits   boxes that are clicked and not drawn: every handle,
//                      every hit of the slide (a line of words, a picture)
//                      as the piece it is of, and every piece's own box. In that order: what is built first is asked first,
//                      so a corner is before everything, a block's words are
//                      before a picture they are over, and a picture before
//                      the title's or a block's box under it. Their signals
//                      press, right-press, hover and wheel
//    Place_BuildLook   boxes that are drawn and not clicked: the outline and
//                      corners of what the mouse is over, the lines Shift lines
//                      up with - after the slide is laid out again for what
//                      the clicks did
//
// A drag is moved before the slide is laid out (Place_BeforeLayout), so the
// slide and its outline are where the mouse is in the same frame. It goes on
// while the button is down, wherever the mouse is: a block resized wraps its
// words again, and the box of the line that was pressed on may be gone.

// The side of a handle, and how far the mouse goes with the button down
// before it is a drag and not a click. In UI units.
#define _PLACE_HANDLE     10.0f
#define _PLACE_DRAG_START 4.0f

// With Shift down: how near an edge comes to a line before it jumps to it.
// In UI units. A slide of three pictures, a title and a body has some fifty
// lines across, each tried by an edge and the middle: at 7 the box was on one
// line or another almost the whole way across the page, jumping between
// them, and could hardly be put anywhere else.
#define _PLACE_SNAP 5.0f

// And how far from a line it is on the mouse takes it before it comes off:
// more than it takes to get on. With the two the same, a mouse held at the
// edge of a line's reach had the box on the line one frame and off it the
// next, and a box between two lines went back and forth between them.
#define _PLACE_SNAP_LEAVE 11.0f

// The least the body's box is made by a handle, as parts of the page - or
// what it was already, if a hand-written line made it less.
#define _PLACE_TEXT_MIN_W 0.2f
#define _PLACE_TEXT_MIN_H 0.12f

// A box's handles, by number (Place_Drag's corner): its corners - 1 top
// left, 2 top right, 3 bottom left, 4 bottom right - and the middles of its
// sides - 5 left, 6 right, 7 top, 8 bottom. A corner moves two edges, a side
// one.
#define _PLACE_HANDLE_COUNT 8

// Which edge a handle moves, across: -1 the left, 1 the right, 0 neither.
PRIVATE S32
_Place_HandleSideX(U32 handle) {
   return (handle == 1 || handle == 3 || handle == 5) ? -1 : (handle == 2 || handle == 4 || handle == 6) ? 1 : 0;
}

// And down: -1 the top, 1 the bottom, 0 neither.
PRIVATE S32
_Place_HandleSideY(U32 handle) {
   return (handle == 1 || handle == 2 || handle == 7) ? -1 : (handle == 3 || handle == 4 || handle == 8) ? 1 : 0;
}

// Where a handle of box is.
PRIVATE Rng2F32
_Place_HandleRect(Rng2F32 box, U32 handle, F32 ui) {
   F32 half = (F32)(S32)(_PLACE_HANDLE * ui * 0.5f);
   S32 side_x = _Place_HandleSideX(handle);
   S32 side_y = _Place_HandleSideY(handle);
   F32 x = (side_x < 0) ? box.min.x : (side_x > 0) ? box.max.x : (F32)(S32)((box.min.x + box.max.x) * 0.5f);
   F32 y = (side_y < 0) ? box.min.y : (side_y > 0) ? box.max.y : (F32)(S32)((box.min.y + box.max.y) * 0.5f);
   return R2F32P(x - half, y - half, x + half, y + half);
}

// Whether box has that handle: a side's is left out on a side too short to
// have it clear of the corners'.
PRIVATE B32
_Place_HasHandle(Rng2F32 box, U32 handle, F32 ui) {
   F32 room = _PLACE_HANDLE * ui * 3.5f;
   B32 result = 1;
   if (handle == 5 || handle == 6) {
      result = (box.max.y - box.min.y >= room);
   } else if (handle == 7 || handle == 8) {
      result = (box.max.x - box.min.x >= room);
   }
   return result;
}

// Whether the slide's boxes, as laid out, are of the text as it is.
B32
Place_BoxesAreGood(const App_State* app_state) {
   return (app_state->editor != 0 && app_state->editor->edit_count == g_edit_view.hits_edit_count &&
           !app_state->show_open && app_state->slide_idx < app_state->deck->slide_count &&
           Dim2F32(g_edit_view.page).x > 0 && Dim2F32(g_edit_view.page).y > 0);
}

// The slide's place of a kind: a picture's is the picture's own, and a
// block's of the body the block's own. idx says which (View_Box's).
PRIVATE Deck_Place*
_Place_PlaceOf(Deck_Slide* slide, Deck_PlaceKind kind, U64 idx) {
   Deck_Place* result = &slide->places[kind];
   if (kind == Deck_PlaceKind_Picture && idx < slide->picture_count) {
      result = &slide->pictures[idx].place;
   }
   if (kind == Deck_PlaceKind_Text && idx < slide->box_count) {
      result = &slide->boxes[idx].place;
   }
   return result;
}

// Which block of the slide's body (Deck_Box) has the line whose words are at
// range: max_U64 for none - the title, or a picture.
PRIVATE U64
_Place_BlockOfRange(Deck_Slide* slide, Rng1U64 range) {
   U64 result = max_U64;
   for (U64 block_idx = 0; block_idx < slide->box_count && result == max_U64; block_idx += 1) {
      Deck_Box*  block = &slide->boxes[block_idx];
      Deck_Item* item = block->first_item;
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
// The words are the ones a click there would go to (Edit_PreviewHitAt): of
// two blocks over each other, those of the one drawn later. Not merely
// whichever block's box is last under the point - that box can be mostly
// empty, over the words of another.
View_Box*
Place_BoxAt(const App_State* app_state, Vec2F32 pos) {
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];

   // What words the point is on, if any: the title's, or which block's.
   B32 on_title_words = 0;
   U64 words_block = max_U64;
   S64 hit_idx = Edit_PreviewHitAt(pos);
   if (hit_idx >= 0) {
      Rng1U64 range = g_edit_view.hits[hit_idx].text_range;
      on_title_words = (range.min == slide->title_range.min && range.max == slide->title_range.max);
      words_block = _Place_BlockOfRange(slide, range);
   }

   View_Box* title = 0;
   View_Box* text = 0;
   View_Box* words_text = 0;
   View_Box* picture = 0;
   for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
      View_Box* box = &g_edit_view.boxes[i];
      if (Contains2F32(box->rect, pos)) {
         if (box->kind == Deck_PlaceKind_Title) {
            title = box;
         } else if (box->kind == Deck_PlaceKind_Text) {
            text = box;
            if (box->idx == words_block) {
               words_text = box;
            }
         } else if (box->idx < slide->picture_count) {
            picture = box;
         }
      }
   }

   View_Box* result = 0;
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
// of the body: over the one it has (before), or as a new line above the
// picture's line, the title's, or the block's first (Edit_PutLineFor).
PRIVATE void
_Place_WriteLine(Editor_Editor* editor, Deck_Slide* slide, Deck_PlaceKind kind, U64 idx, Deck_Place* before, Rng2F32 rect) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 line = PushStr8F(scratch.arena, "@place %S %.3f %.3f %.3f %.3f", Deck_PlaceName(kind), rect.min.x, rect.min.y,
                            rect.max.x - rect.min.x, rect.max.y - rect.min.y);
   Edit_PutLineFor(editor, slide, kind, idx, before->has_line, before->line, line);
   ScratchEnd(scratch);
}

// Takes an @place line out of the text, with its line ending.
PRIVATE void
_Place_RemoveLine(Editor_Editor* editor, Deck_Place* place) {
   if (place->has_line) {
      Edit_TakeOutLine(editor, place->line);
   }
}

// The least a handle makes the thing dragged, as parts of the page: the
// body's and the title's box more than a picture's. Their words wrap at the
// width and get smaller only so far, so a box much narrower than this has a
// word a line, running down off the page.
PRIVATE Vec2F32
_Place_MinSize(Place_Drag* drag) {
   B32     is_text = (drag->kind != Deck_PlaceKind_Picture);
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
PRIVATE Rng2F32
_Place_RectFromMouse(Place_Drag* drag, Rng2F32 page, Vec2F32 mouse) {
   Vec2F32 page_dim = Dim2F32(page);
   F32     dx = (mouse.x - drag->press_mouse.x) / page_dim.x;
   F32     dy = (mouse.y - drag->press_mouse.y) / page_dim.y;
   Rng2F32 start = drag->start;
   F32     w0 = start.max.x - start.min.x;
   F32     h0 = start.max.y - start.min.y;
   Rng2F32 result = start;

   if (drag->corner == 0) {
      // All of it, kept on the page - if it was all on the page. Not on a
      // board, which has no edge.
      B32 is_board = g_edit_view.is_board;
      if (start.min.x >= 0 && start.max.x <= 1.0f && !is_board) {
         dx = Clamp(-start.min.x, dx, 1.0f - start.max.x);
      }
      if (start.min.y >= 0 && start.max.y <= 1.0f && !is_board) {
         dy = Clamp(-start.min.y, dy, 1.0f - start.max.y);
      }
      result = R2F32P(start.min.x + dx, start.min.y + dy, start.max.x + dx, start.max.y + dy);
   } else {
      // The corner across from the one held stays where it is - or, with a
      // side held, the side across from it, and the other two as well.
      S32 side_x = _Place_HandleSideX(drag->corner);
      S32 side_y = _Place_HandleSideY(drag->corner);
      B32 is_left = (side_x < 0);
      B32 is_top = (side_y < 0);
      F32 anchor_x = is_left ? start.max.x : start.min.x;
      F32 anchor_y = is_top ? start.max.y : start.min.y;

      Vec2F32 min_size = _Place_MinSize(drag);
      F32     min_w = min_size.x;
      F32     min_h = min_size.y;

      // The most there is room for on the page, from that corner.
      F32 max_w = Max(is_left ? anchor_x : 1.0f - anchor_x, min_w);
      F32 max_h = Max(is_top ? anchor_y : 1.0f - anchor_y, min_h);
      if (g_edit_view.is_board) {
         // A board has room for as much as an @place line can say.
         max_w = 2.0f;
         max_h = 2.0f;
      }
      F32 w = (side_x != 0) ? Clamp(min_w, w0 + (is_left ? -dx : dx), max_w) : w0;
      F32 h = (side_y != 0) ? Clamp(min_h, h0 + (is_top ? -dy : dy), max_h) : h0;

      B32 is_picture = (drag->kind == Deck_PlaceKind_Picture && w0 > 0 && h0 > 0);
      if (is_picture && (side_x == 0 || side_y == 0)) {
         // A picture keeps its shape, by a side too: the other way grows
         // with it, as much above its middle as below - or left of it as
         // right - and no further than the page from there.
         F32 grow = (side_x != 0) ? w / w0 : h / h0;
         if (side_x != 0) {
            F32 middle = (start.min.y + start.max.y) * 0.5f;
            F32 room = 2.0f * Min(middle, 1.0f - middle);
            if (room >= h0 && !g_edit_view.is_board) {
               grow = Min(grow, room / h0);
            }
         } else {
            F32 middle = (start.min.x + start.max.x) * 0.5f;
            F32 room = 2.0f * Min(middle, 1.0f - middle);
            if (room >= w0 && !g_edit_view.is_board) {
               grow = Min(grow, room / w0);
            }
         }
         grow = Max(grow, DECK_PLACE_MIN / Min(w0, h0));
         w = w0 * grow;
         h = h0 * grow;
      } else if (is_picture) {
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
      // A picture by a side: about its middle, the other way.
      if (is_picture && side_y == 0) {
         F32 middle = (start.min.y + start.max.y) * 0.5f;
         result.min.y = middle - h * 0.5f;
         result.max.y = middle + h * 0.5f;
      }
      if (is_picture && side_x == 0) {
         F32 middle = (start.min.x + start.max.x) * 0.5f;
         result.min.x = middle - w * 0.5f;
         result.max.x = middle + w * 0.5f;
      }
   }

   return result;
}

// A line something can be lined up with, as a part of the page across or
// down, and the box of the slide's it is an edge or the middle of: -1 for
// the page's own edges, middle and margins.
typedef struct _Place_Line _Place_Line;
struct _Place_Line {
   F32 at;
   S32 box_idx;
};

// The line nearest value among lines that is within reach, and that moving
// value onto would not move it by less than shift_lo or more than shift_hi:
// those say how far it can go before it is off the page or too small. Its
// index, and how far value is from it (to add) - or -1 if there is none.
// The nearest that can be had, not the nearest: a line just past the page's
// edge must not hide one a little further in.
PRIVATE S64
_Place_NearestLine(F32 value, _Place_Line* lines, U64 line_count, F32 reach, F32 shift_lo, F32 shift_hi, F32* shift_out) {
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

// The line of lines that is at at, or -1.
PRIVATE S64
_Place_LineAt(_Place_Line* lines, U64 line_count, F32 at) {
   S64 result = -1;
   for (U64 i = 0; i < line_count && result < 0; i += 1) {
      F32 gap = lines[i].at - at;
      if (gap < 0.00005f && gap > -0.00005f) {
         result = (S64)i;
      }
   }
   return result;
}

// The line the drag was on last frame, if it is still one of lines and one
// of values is within reach of it - the further reach, of a line it is
// already on: its index, and how far that value is from it. Else -1.
PRIVATE S64
_Place_StuckLine(B32 has_stuck, F32 stuck, F32* values, U64 value_count, _Place_Line* lines, U64 line_count, F32 reach,
                 F32 shift_lo, F32 shift_hi, F32* shift_out) {
   S64 result = -1;
   S64 line_idx = has_stuck ? _Place_LineAt(lines, line_count, stuck) : -1;
   F32 best = reach;
   for (U64 i = 0; i < value_count && line_idx >= 0; i += 1) {
      F32 gap = lines[line_idx].at - values[i];
      F32 gap_size = (gap < 0) ? -gap : gap;
      if (gap_size <= best && gap >= shift_lo && gap <= shift_hi) {
         best = gap_size;
         *shift_out = gap;
         result = line_idx;
      }
   }
   return result;
}

// Puts what line_idx of lines is into the drag, across (is_x) or down, to be
// drawn: the line, and the box it came from.
PRIVATE void
_Place_SetGuide(Place_Drag* drag, B32 is_x, _Place_Line* lines, S64 line_idx, Rng2F32* boxes) {
   _Place_Line* line = &lines[line_idx];
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
PRIVATE Rng2F32
_Place_LinedUp(Place_Drag* drag, Rng2F32 rect, F32 ui) {
   Rng2F32 page = g_edit_view.page;
   Vec2F32 page_dim = Dim2F32(page);
   drag->has_guide_x = 0;
   drag->has_guide_y = 0;
   if (page_dim.x <= 0 || page_dim.y <= 0) {
      return rect;
   }

   //- bgz: the other boxes, and the lines there are, as parts of the page
   Rng2F32     boxes[VIEW_MAX_BOXES];
   _Place_Line xs[3 * ArrayCount(boxes) + 5];
   _Place_Line ys[3 * ArrayCount(boxes) + 5];
   U64         x_count = 0;
   U64         y_count = 0;
   F32         page_xs[5] = {0.0f, 0.5f, 1.0f, VIEW_MARGIN_X / VIEW_PAGE_W, 1.0f - VIEW_MARGIN_X / VIEW_PAGE_W};
   F32         page_ys[5] = {0.0f, 0.5f, 1.0f, VIEW_MARGIN_TOP / VIEW_PAGE_H, 1.0f - VIEW_MARGIN_BOTTOM / VIEW_PAGE_H};
   for (U64 i = 0; i < 5; i += 1) {
      xs[x_count++] = (_Place_Line){page_xs[i], -1};
      ys[y_count++] = (_Place_Line){page_ys[i], -1};
   }
   U64 box_count = 0;
   for (U64 i = 0; i < g_edit_view.box_count && box_count < ArrayCount(boxes); i += 1) {
      View_Box* box = &g_edit_view.boxes[i];
      B32       is_dragged = (box->kind == drag->kind && box->idx == drag->idx);
      // Nor the others that go with it: they move as it does, and a line
      // of theirs would be one that runs ahead of it.
      for (U64 k = 0; k < drag->group_count && drag->corner == 0; k += 1) {
         is_dragged |= (box->kind == drag->group_kinds[k] && box->idx == drag->group_idxs[k]);
      }
      if (is_dragged) {
         continue;
      }
      Rng2F32 r = R2F32P((box->rect.min.x - page.min.x) / page_dim.x, (box->rect.min.y - page.min.y) / page_dim.y,
                         (box->rect.max.x - page.min.x) / page_dim.x, (box->rect.max.y - page.min.y) / page_dim.y);
      S32 box_idx = (S32)box_count;
      boxes[box_count++] = r;
      xs[x_count++] = (_Place_Line){r.min.x, box_idx};
      xs[x_count++] = (_Place_Line){(r.min.x + r.max.x) * 0.5f, box_idx};
      xs[x_count++] = (_Place_Line){r.max.x, box_idx};
      ys[y_count++] = (_Place_Line){r.min.y, box_idx};
      ys[y_count++] = (_Place_Line){(r.min.y + r.max.y) * 0.5f, box_idx};
      ys[y_count++] = (_Place_Line){r.max.y, box_idx};
   }

   F32     reach_x = _PLACE_SNAP * ui / page_dim.x;
   F32     reach_y = _PLACE_SNAP * ui / page_dim.y;
   F32     leave_x = _PLACE_SNAP_LEAVE * ui / page_dim.x;
   F32     leave_y = _PLACE_SNAP_LEAVE * ui / page_dim.y;
   F32     w = rect.max.x - rect.min.x;
   F32     h = rect.max.y - rect.min.y;
   Rng2F32 result = rect;

   if (drag->corner == 0) {
      //- bgz: a move: the nearest of what its edges and its middle are near
      // Kept on the page, as the mouse keeps it - if it was all on the page.
      // One a hand-written line put partly off can go anywhere.
      F32 lo_x = -1.0f, hi_x = 1.0f, lo_y = -1.0f, hi_y = 1.0f;
      if (drag->start.min.x >= 0 && drag->start.max.x <= 1.0f && !g_edit_view.is_board) {
         lo_x = -rect.min.x;
         hi_x = 1.0f - rect.max.x;
      }
      if (drag->start.min.y >= 0 && drag->start.max.y <= 1.0f && !g_edit_view.is_board) {
         lo_y = -rect.min.y;
         hi_y = 1.0f - rect.max.y;
      }
      F32 own_x[3] = {rect.min.x, (rect.min.x + rect.max.x) * 0.5f, rect.max.x};
      F32 own_y[3] = {rect.min.y, (rect.min.y + rect.max.y) * 0.5f, rect.max.y};
      F32 best_x = reach_x;
      F32 best_y = reach_y;

      // The line it is on already keeps it, each way, while it is near
      // enough: no other is looked for that way.
      F32 stuck_shift = 0;
      S64 stuck_idx = _Place_StuckLine(drag->has_stuck_x, drag->stuck_x, own_x, 3, xs, x_count, leave_x, lo_x, hi_x, &stuck_shift);
      B32 is_stuck_x = (stuck_idx >= 0);
      if (is_stuck_x) {
         result.min.x = rect.min.x + stuck_shift;
         result.max.x = rect.max.x + stuck_shift;
         _Place_SetGuide(drag, 1, xs, stuck_idx, boxes);
      }
      stuck_idx = _Place_StuckLine(drag->has_stuck_y, drag->stuck_y, own_y, 3, ys, y_count, leave_y, lo_y, hi_y, &stuck_shift);
      B32 is_stuck_y = (stuck_idx >= 0);
      if (is_stuck_y) {
         result.min.y = rect.min.y + stuck_shift;
         result.max.y = rect.max.y + stuck_shift;
         _Place_SetGuide(drag, 0, ys, stuck_idx, boxes);
      }

      for (U64 i = 0; i < 3; i += 1) {
         F32 shift = 0;
         S64 line_idx = is_stuck_x ? -1 : _Place_NearestLine(own_x[i], xs, x_count, best_x, lo_x, hi_x, &shift);
         if (line_idx >= 0) {
            best_x = (shift < 0) ? -shift : shift;
            result.min.x = rect.min.x + shift;
            result.max.x = rect.max.x + shift;
            _Place_SetGuide(drag, 1, xs, line_idx, boxes);
         }
         line_idx = is_stuck_y ? -1 : _Place_NearestLine(own_y[i], ys, y_count, best_y, lo_y, hi_y, &shift);
         if (line_idx >= 0) {
            best_y = (shift < 0) ? -shift : shift;
            result.min.y = rect.min.y + shift;
            result.max.y = rect.max.y + shift;
            _Place_SetGuide(drag, 0, ys, line_idx, boxes);
         }
      }
   } else {
      //- bgz: a corner, or a side: the edges it moves
      S32     side_x = _Place_HandleSideX(drag->corner);
      S32     side_y = _Place_HandleSideY(drag->corner);
      B32     is_left = (side_x < 0);
      B32     is_top = (side_y < 0);
      B32     keeps_shape = (drag->kind == Deck_PlaceKind_Picture && w > 0 && h > 0);
      // A picture held by a side grows about its middle the other way
      // (_Place_RectFromMouse): it is left where the mouse has it.
      if (keeps_shape && (side_x == 0 || side_y == 0)) {
         return rect;
      }
      F32     anchor_x = is_left ? rect.max.x : rect.min.x;
      F32     anchor_y = is_top ? rect.max.y : rect.min.y;
      Vec2F32 min_size = _Place_MinSize(drag);

      // How wide and high it can be made: from the least a corner makes it
      // to what there is room for on the page from the corner that stays,
      // as _Place_RectFromMouse has it. A picture's width and height go
      // together, so each holds the other to its own.
      F32 lo_w = min_size.x;
      F32 lo_h = min_size.y;
      F32 hi_w = Max(is_left ? anchor_x : 1.0f - anchor_x, lo_w);
      F32 hi_h = Max(is_top ? anchor_y : 1.0f - anchor_y, lo_h);
      if (g_edit_view.is_board) {
         hi_w = 2.0f;
         hi_h = 2.0f;
      }
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
      S64 line_x = _Place_NearestLine(is_left ? rect.min.x : rect.max.x, xs, x_count, reach_x,
                                      is_left ? w - hi_w : lo_w - w, is_left ? w - lo_w : hi_w - w, &shift_x);
      S64 line_y = _Place_NearestLine(is_top ? rect.min.y : rect.max.y, ys, y_count, reach_y,
                                      is_top ? h - hi_h : lo_h - h, is_top ? h - lo_h : hi_h - h, &shift_y);

      // The line the edge is on already keeps it, while it is near enough.
      {
         F32 edge_x = is_left ? rect.min.x : rect.max.x;
         F32 edge_y = is_top ? rect.min.y : rect.max.y;
         F32 stuck_shift = 0;
         S64 stuck_idx = _Place_StuckLine(drag->has_stuck_x, drag->stuck_x, &edge_x, 1, xs, x_count, leave_x,
                                          is_left ? w - hi_w : lo_w - w, is_left ? w - lo_w : hi_w - w, &stuck_shift);
         if (stuck_idx >= 0) {
            line_x = stuck_idx;
            shift_x = stuck_shift;
         }
         stuck_idx = _Place_StuckLine(drag->has_stuck_y, drag->stuck_y, &edge_y, 1, ys, y_count, leave_y,
                                      is_top ? h - hi_h : lo_h - h, is_top ? h - lo_h : hi_h - h, &stuck_shift);
         if (stuck_idx >= 0) {
            line_y = stuck_idx;
            shift_y = stuck_shift;
         }
      }

      // A side moves one edge: the other way nothing is lined up.
      if (side_x == 0) {
         line_x = -1;
      }
      if (side_y == 0) {
         line_y = -1;
      }

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
         _Place_SetGuide(drag, 1, xs, line_x, boxes);
      }
      if (line_y >= 0) {
         new_h = h + (is_top ? -shift_y : shift_y);
         if (keeps_shape) {
            new_w = w * new_h / h;
         }
         _Place_SetGuide(drag, 0, ys, line_y, boxes);
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

PRIVATE View_Box* _Place_BoxOf(Deck_PlaceKind kind, U64 idx);

PRIVATE void
_Place_Begin(const App_State* app_state, View_Box* from, U32 corner, Vec2F32 pos, Place_DragState state) {
   Place_Drag* drag = &g_place_view.drag;
   Rng2F32         page = g_edit_view.page;
   Vec2F32         page_dim = Dim2F32(page);
   Rng2F32         box = from->rect;
   Deck_PlaceKind  kind = from->kind;
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
   drag->before = *_Place_PlaceOf(&app_state->deck->slides[app_state->slide_idx], kind, idx);

   // Moved, not resized, as one of several picked whole: the others go with
   // it.
   if (corner == 0 && Edit_PieceIsPicked(app_state, kind, idx)) {
      for (U64 i = 0; i < g_edit_view.picked_count; i += 1) {
         Deck_PlaceKind other_kind = g_edit_view.picked_kinds[i];
         U64            other_idx = g_edit_view.picked_idxs[i];
         View_Box*      other = _Place_BoxOf(other_kind, other_idx);
         if ((other_kind == kind && other_idx == idx) || other == 0 || drag->group_count >= VIEW_MAX_BOXES) {
            continue;
         }
         Rng2F32 rect = other->rect;
         drag->group_kinds[drag->group_count] = other_kind;
         drag->group_idxs[drag->group_count] = other_idx;
         drag->group_start[drag->group_count] = R2F32P((rect.min.x - page.min.x) / page_dim.x, (rect.min.y - page.min.y) / page_dim.y,
                                                       (rect.max.x - page.min.x) / page_dim.x, (rect.max.y - page.min.y) / page_dim.y);
         drag->group_count += 1;
      }
   }
}

// Where the i-th of the others a drag moves has been taken: by as much as
// the piece dragged, and kept on the page.
PRIVATE Rng2F32
_Place_GroupRect(Place_Drag* drag, U64 i) {
   Rng2F32 start = drag->group_start[i];
   F32     w = start.max.x - start.min.x;
   F32     h = start.max.y - start.min.y;
   F32     x = start.min.x + (drag->now.min.x - drag->start.min.x);
   F32     y = start.min.y + (drag->now.min.y - drag->start.min.y);
   if (!g_edit_view.is_board) {
      x = Clamp(0.0f, x, Max(1.0f - w, 0.0f));
      y = Clamp(0.0f, y, Max(1.0f - h, 0.0f));
   }
   return R2F32P(x, y, x + w, y + h);
}


// The box the slide's piece of kind and idx was laid out in, this frame: 0
// if it has none - a picture behind the words.
PRIVATE View_Box*
_Place_BoxOf(Deck_PlaceKind kind, U64 idx) {
   View_Box* result = 0;
   for (U64 i = 0; i < g_edit_view.box_count; i += 1) {
      if (g_edit_view.boxes[i].kind == kind && g_edit_view.boxes[i].idx == idx) {
         result = &g_edit_view.boxes[i];
      }
   }
   return result;
}

// Which piece of the slide a hit of the preview is of: the title's words,
// a line of a block's, or a picture. 0 if none - a hit is always one of
// those, but the text can be ahead of the slide drawn.
PRIVATE B32
_Place_PieceOfHit(Deck_Slide* slide, View_Hit* hit, Deck_PlaceKind* kind_out, U64* idx_out) {
   B32 result = 0;
   U64 block_idx = _Place_BlockOfRange(slide, hit->text_range);
   if (hit->text_range.min == slide->title_range.min && hit->text_range.max == slide->title_range.max) {
      *kind_out = Deck_PlaceKind_Title;
      *idx_out = 0;
      result = 1;
   } else if (block_idx != max_U64) {
      *kind_out = Deck_PlaceKind_Text;
      *idx_out = block_idx;
      result = 1;
   } else {
      for (U64 i = 0; i < slide->picture_count && !result; i += 1) {
         if (slide->pictures[i].range.min == hit->text_range.min && slide->pictures[i].range.max == hit->text_range.max) {
            *kind_out = Deck_PlaceKind_Picture;
            *idx_out = i;
            result = 1;
         }
      }
   }
   return result;
}

void
Place_GiveUp(void) {
   // The piece is back where the text has it: nothing was written.
   g_place_view.drag.state = Place_DragState_None;
}

void
Place_PutDrag(const App_State* app_state, U64 slide_idx) {
   Place_Drag*    drag = &g_place_view.drag;
   Deck_Deck*     deck = app_state->deck;
   Editor_Editor* editor = app_state->editor;
   B32            is_moved = (drag->state == Place_DragState_Dragging || drag->state == Place_DragState_Dropped);
   if (is_moved && !drag->is_put && drag->slide_idx == slide_idx && slide_idx < deck->slide_count && editor != 0 &&
       editor->edit_count == drag->edit_count) {
      Deck_Place* place = _Place_PlaceOf(&deck->slides[slide_idx], drag->kind, drag->idx);
      drag->before = *place;
      place->is_set = 1;
      place->rect = drag->now;
      drag->is_put = 1;
      // Only a move takes the others along: a resize is the one piece's.
      for (U64 i = 0; i < drag->group_count && drag->corner == 0; i += 1) {
         Deck_Place* other = _Place_PlaceOf(&deck->slides[slide_idx], drag->group_kinds[i], drag->group_idxs[i]);
         drag->group_before[i] = *other;
         other->is_set = 1;
         other->rect = _Place_GroupRect(drag, i);
      }
   }
}

void
Place_TakeDrag(const App_State* app_state, U64 slide_idx) {
   Place_Drag* drag = &g_place_view.drag;
   Deck_Deck*  deck = app_state->deck;
   if (drag->is_put && drag->slide_idx == slide_idx && slide_idx < deck->slide_count) {
      // The others first: in the order they were put, backwards.
      for (U64 i = (drag->corner == 0) ? drag->group_count : 0; i > 0; i -= 1) {
         *_Place_PlaceOf(&deck->slides[slide_idx], drag->group_kinds[i - 1], drag->group_idxs[i - 1]) = drag->group_before[i - 1];
      }
      *_Place_PlaceOf(&deck->slides[slide_idx], drag->kind, drag->idx) = drag->before;
      drag->is_put = 0;
   }
}

////////////////////////////////
//~ bgz: state ops

// The piece dropped: its @place line written, at the place the mouse left it.
// Not into text that has changed since the button went down.
void
Place_WriteLine(App_State* app_state, StateOp_List* ops, Place_WriteLineOpParams* params) {
   UnusedVariable(ops);
   Place_Drag*    drag = &g_place_view.drag;
   Editor_Editor* editor = app_state->editor;
   Deck_Deck*     deck = app_state->deck;
   if (drag->state == Place_DragState_Dropped) {
      drag->state = Place_DragState_None;
   }
   if (editor != 0 && editor->edit_count == params->edit_count && app_state->parsed_edit_count == editor->edit_count &&
       params->slide_idx < deck->slide_count) {
      Deck_Slide* slide = &deck->slides[params->slide_idx];
      B32         was_picked = Edit_PickedIsGood(app_state);
      U64         undo_before = editor->undo_count;
      _Place_WriteLine(editor, slide, params->kind, params->idx, _Place_PlaceOf(slide, params->kind, params->idx), params->rect);
      // The others picked that went with it: each its line, with the deck
      // made again between, as one step to undo.
      for (U64 i = 0; i < drag->group_count && params->corner == 0; i += 1) {
         // The deck made again is a new one: the one above is gone.
         FR_SyncDeck(app_state);
         deck = app_state->deck;
         slide = &deck->slides[params->slide_idx];
         _Place_WriteLine(editor, slide, drag->group_kinds[i], drag->group_idxs[i],
                          _Place_PlaceOf(slide, drag->group_kinds[i], drag->group_idxs[i]), _Place_GroupRect(drag, i));
      }
      Editor_JoinUndoSince(editor, undo_before);
      drag->group_count = 0;
      editor->last_input_us = NowTimeUS();
      // Picked whole before they were moved, they still are (edit\edit.c).
      Edit_KeepPicked(app_state, was_picked);
   }
}
STATE_OP(Place_WriteLine)

// The pieces picked whole, as laid out: which, and where each is, in parts
// of the page. Into arrays of VIEW_MAX_BOXES: how many. 0 if they are not
// picked any more, or the slide laid out is not of the text as it is.
PRIVATE U64
_Place_PickedRects(const App_State* app_state, Deck_PlaceKind* kinds, U64* idxs, Rng2F32* rects) {
   U64            count = 0;
   Editor_Editor* editor = app_state->editor;
   if (editor == 0 || !Edit_PickedIsGood(app_state) || !Place_BoxesAreGood(app_state) ||
       app_state->parsed_edit_count != editor->edit_count) {
      return 0;
   }
   Rng2F32 page = g_edit_view.page;
   Vec2F32 page_dim = Dim2F32(page);
   for (U64 i = 0; i < g_edit_view.picked_count && count < VIEW_MAX_BOXES; i += 1) {
      View_Box* box = _Place_BoxOf(g_edit_view.picked_kinds[i], g_edit_view.picked_idxs[i]);
      if (box == 0) {
         continue;
      }
      kinds[count] = box->kind;
      idxs[count] = box->idx;
      rects[count] = R2F32P((box->rect.min.x - page.min.x) / page_dim.x, (box->rect.min.y - page.min.y) / page_dim.y,
                            (box->rect.max.x - page.min.x) / page_dim.x, (box->rect.max.y - page.min.y) / page_dim.y);
      count += 1;
   }
   return count;
}

// Writes the @place line of each of those pieces of the slide showing, with
// rects their new places - each kept on the page, its size as it is - as
// one step to undo, and they stay picked.
PRIVATE void
_Place_WriteRects(App_State* app_state, Deck_PlaceKind* kinds, U64* idxs, Rng2F32* rects, U64 count) {
   Editor_Editor* editor = app_state->editor;
   U64            slide_idx = app_state->slide_idx;
   U64            undo_before = editor->undo_count;
   for (U64 i = 0; i < count; i += 1) {
      F32 w = rects[i].max.x - rects[i].min.x;
      F32 h = rects[i].max.y - rects[i].min.y;
      F32 x = Clamp(0.0f, rects[i].min.x, Max(1.0f - w, 0.0f));
      F32 y = Clamp(0.0f, rects[i].min.y, Max(1.0f - h, 0.0f));
      // The deck made again is a new one, after the line before.
      FR_SyncDeck(app_state);
      if (slide_idx >= app_state->deck->slide_count) {
         break;
      }
      Deck_Slide* slide = &app_state->deck->slides[slide_idx];
      if (slide->is_canvas) {
         // Anywhere, on a board.
         x = rects[i].min.x;
         y = rects[i].min.y;
      }
      _Place_WriteLine(editor, slide, kinds[i], idxs[i], _Place_PlaceOf(slide, kinds[i], idxs[i]), R2F32P(x, y, x + w, y + h));
   }
   Editor_JoinUndoSince(editor, undo_before);
   if (count != 0) {
      editor->last_input_us = NowTimeUS();
      Edit_KeepPicked(app_state, 1);
   }
}

// The arrow keys with pieces picked whole: each one's @place line written a
// step over from where it is. From the slide as it was last laid out:
// nothing if the text has changed since - a key held repeats faster than
// the frame, and the next one then waits for the slide laid out again.
void
Place_Nudge(App_State* app_state, StateOp_List* ops, Place_NudgeOpParams* params) {
   UnusedVariable(ops);
   Deck_PlaceKind kinds[VIEW_MAX_BOXES];
   U64            idxs[VIEW_MAX_BOXES];
   Rng2F32        rects[VIEW_MAX_BOXES];
   U64            count = _Place_PickedRects(app_state, kinds, idxs, rects);
   for (U64 i = 0; i < count; i += 1) {
      rects[i] = R2F32P(rects[i].min.x + params->dx, rects[i].min.y + params->dy, rects[i].max.x + params->dx, rects[i].max.y + params->dy);
   }
   _Place_WriteRects(app_state, kinds, idxs, rects, count);
}
STATE_OP(Place_Nudge)

// Several pieces picked whole, lined up or spaced evenly (Place_ArrangeKind).
void
Place_Arrange(App_State* app_state, StateOp_List* ops, Place_ArrangeOpParams* params) {
   Deck_PlaceKind kinds[VIEW_MAX_BOXES];
   U64            idxs[VIEW_MAX_BOXES];
   Rng2F32        rects[VIEW_MAX_BOXES];
   U64            count = _Place_PickedRects(app_state, kinds, idxs, rects);
   B32            is_space = (params->kind == Place_ArrangeKind_SpaceX || params->kind == Place_ArrangeKind_SpaceY);
   if (count < (is_space ? 3u : 2u)) {
      StateOp_App_Notice(ops)->notice = is_space ? Str8Lit("Pick three or more to space them evenly: Ctrl+click adds one")
                                                 : Str8Lit("Pick two or more to line them up: Ctrl+click adds one");
      return;
   }

   // What they all reach to, together.
   Rng2F32 all = rects[0];
   for (U64 i = 1; i < count; i += 1) {
      all.min.x = Min(all.min.x, rects[i].min.x);
      all.min.y = Min(all.min.y, rects[i].min.y);
      all.max.x = Max(all.max.x, rects[i].max.x);
      all.max.y = Max(all.max.y, rects[i].max.y);
   }

   if (!is_space) {
      for (U64 i = 0; i < count; i += 1) {
         F32 w = rects[i].max.x - rects[i].min.x;
         F32 h = rects[i].max.y - rects[i].min.y;
         F32 x = rects[i].min.x;
         F32 y = rects[i].min.y;
         switch (params->kind) {
         case Place_ArrangeKind_Left: x = all.min.x; break;
         case Place_ArrangeKind_Right: x = all.max.x - w; break;
         case Place_ArrangeKind_Top: y = all.min.y; break;
         case Place_ArrangeKind_Bottom: y = all.max.y - h; break;
         case Place_ArrangeKind_CenterX: x = (all.min.x + all.max.x) * 0.5f - w * 0.5f; break;
         case Place_ArrangeKind_CenterY: y = (all.min.y + all.max.y) * 0.5f - h * 0.5f; break;
         default: break;
         }
         rects[i] = R2F32P(x, y, x + w, y + h);
      }
   } else {
      // In the order they are in, across or down, by their middles: the
      // first and the last stay, the rest go between with the same room
      // between each two - less than none, if they are too large for it.
      B32 is_x = (params->kind == Place_ArrangeKind_SpaceX);
      U64 order[VIEW_MAX_BOXES];
      for (U64 i = 0; i < count; i += 1) {
         order[i] = i;
      }
#define _Place_Mid(r) (is_x ? ((r).min.x + (r).max.x) : ((r).min.y + (r).max.y))
      for (U64 i = 1; i < count; i += 1) {
         for (U64 k = i; k > 0 && _Place_Mid(rects[order[k - 1]]) > _Place_Mid(rects[order[k]]); k -= 1) {
            U64 swap = order[k - 1];
            order[k - 1] = order[k];
            order[k] = swap;
         }
      }
#undef _Place_Mid
      F32 sizes = 0;
      for (U64 i = 0; i < count; i += 1) {
         sizes += is_x ? (rects[i].max.x - rects[i].min.x) : (rects[i].max.y - rects[i].min.y);
      }
      Rng2F32 first = rects[order[0]];
      Rng2F32 last = rects[order[count - 1]];
      F32     span = is_x ? (last.max.x - first.min.x) : (last.max.y - first.min.y);
      F32     gap = (span - sizes) / (F32)(count - 1);
      F32     at = is_x ? first.min.x : first.min.y;
      for (U64 i = 0; i < count; i += 1) {
         Rng2F32* rect = &rects[order[i]];
         F32      w = rect->max.x - rect->min.x;
         F32      h = rect->max.y - rect->min.y;
         if (is_x) {
            *rect = R2F32P(at, rect->min.y, at + w, rect->max.y);
            at += w + gap;
         } else {
            *rect = R2F32P(rect->min.x, at, rect->max.x, at + h);
            at += h + gap;
         }
      }
   }
   _Place_WriteRects(app_state, kinds, idxs, rects, count);
}
STATE_OP(Place_Arrange)

// A right-click on a piece of the slide showing: its @place line taken out,
// so that it is back where the layout puts it.
void
Place_RemoveLine(App_State* app_state, StateOp_List* ops, Place_RemoveLineOpParams* params) {
   Editor_Editor* editor = app_state->editor;
   Deck_Deck*     deck = app_state->deck;
   if (editor != 0 && app_state->parsed_edit_count == editor->edit_count && app_state->slide_idx < deck->slide_count) {
      Deck_Place* place = _Place_PlaceOf(&deck->slides[app_state->slide_idx], params->kind, params->idx);
      if (place->has_line) {
         _Place_RemoveLine(editor, place);
         editor->last_input_us = NowTimeUS();
         StateOp_App_Notice(ops)->notice = Str8Lit("Back where the layout puts it");
      }
   }
}
STATE_OP(Place_RemoveLine)

void
Place_Keys(const App_State* app_state) {
   Place_Drag* drag = &g_place_view.drag;
   if (drag->state == Place_DragState_Dragging && Core_WM_KeyPress(app_state->events, app_state->window, 0, WM_Key_Esc)) {
      Place_GiveUp();
      // The box held lets go too: its release is no click.
      UI_KillAction();
   }
}

// Ctrl and the wheel, from the window's events - the UI gives its boxes no
// wheel that has Ctrl with it, so this is not one of their signals: over
// the title or a block of the slide, its words are made larger - the wheel
// turned up - or smaller, a step a notch (look\look.c). Of all those picked
// whole, if it is one of them - or if the mouse is over none, anywhere on
// the window. Pushes ops.
void
Place_CtrlWheel(const App_State* app_state) {
   // On a board Ctrl and the wheel zoom (edit\edit.c): the words are sized
   // by Ctrl+] and Ctrl+[.
   if (!Place_BoxesAreGood(app_state) || g_edit_view.is_board) {
      return;
   }
   for (WM_Event* event = app_state->events->first; event != 0; event = event->next) {
      if (event->kind != WM_EventKind_Scroll || event->modifiers != WM_Modifier_Ctrl || event->delta.y == 0) {
         continue;
      }
      // The window's mouse: where the wheel was turned.
      View_Box*      box = Place_BoxAt(app_state, Core_WM_MouseFromWindow(app_state->window));
      B32            has_target = (box != 0 && box->kind != Deck_PlaceKind_Picture);
      Deck_PlaceKind kind = has_target ? box->kind : Deck_PlaceKind_Text;
      U64            idx = has_target ? box->idx : 0;
      B32            with_picked = has_target && Edit_PieceIsPicked(app_state, kind, idx);
      if (!has_target && box == 0 && Edit_PickedIsGood(app_state)) {
         for (U64 i = 0; i < g_edit_view.picked_count && !has_target; i += 1) {
            if (g_edit_view.picked_kinds[i] != Deck_PlaceKind_Picture) {
               has_target = 1;
               with_picked = 1;
               kind = g_edit_view.picked_kinds[i];
               idx = g_edit_view.picked_idxs[i];
            }
         }
      }
      if (!has_target) {
         continue;
      }
      // A notch is 120, down is over 0. A touchpad's small movements are a
      // step each: no more than a few an event.
      S32 notches = (S32)(event->delta.y / 120.0f + ((event->delta.y > 0) ? 0.5f : -0.5f));
      if (notches == 0) {
         notches = (event->delta.y > 0) ? 1 : -1;
      }
      notches = Clamp(-4, notches, 4);
      for (S32 n = 0; n < ((notches < 0) ? -notches : notches); n += 1) {
         Look_StepSizeOpParams* step = StateOp_Look_StepSize(app_state->op_list);
         step->direction = (notches < 0) ? 1 : -1;
         step->has_piece = 1;
         step->kind = kind;
         step->idx = idx;
         step->with_picked = with_picked;
      }
   }
}

void
Place_BeforeLayout(const App_State* app_state, F32 ui) {
   Place_Drag* drag = &g_place_view.drag;
   Editor_Editor*      editor = app_state->editor;
   Deck_Deck*      deck = app_state->deck;
   // Dropped: drawn where it was left until its line is written.
   if (drag->state == Place_DragState_None || drag->state == Place_DragState_Dropped) {
      return;
   }

   B32 is_same = (editor != 0 && editor->edit_count == drag->edit_count && drag->slide_idx == app_state->slide_idx &&
                  drag->slide_idx < deck->slide_count && !app_state->show_open);
   if (drag->state == Place_DragState_Selecting) {
      // The box that was pressed says where the mouse has gone while it is
      // held (_Place_Signal). Here only its end, should that box have gone
      // with the button still down: nothing was moved, nothing is put back.
      if (!is_same || !Core_WM_KeyIsDown(WM_Key_LeftMouseButton)) {
         drag->state = Place_DragState_None;
      }
      return;
   }
   if (!is_same) {
      // The text has changed under it, or the slide: given up.
      drag->state = Place_DragState_None;
      return;
   }

   // The window's mouse, not the UI's: the UI puts it off the window when
   // the window is not the one in front and it has not moved for a moment.
   Vec2F32 mouse = Core_WM_MouseFromWindow(app_state->window);

   // The button held since the press. Not asked of the box pressed: a
   // resized block wraps its words again, and the line pressed on may be
   // gone, and its box with it.
   if (!Core_WM_KeyIsDown(WM_Key_LeftMouseButton)) {
      if (drag->state == Place_DragState_Dragging) {
         // Its line is written by an op, at the start of the next frame.
         // Until then it stays drawn where it was dropped.
         Place_WriteLineOpParams* write = StateOp_Place_WriteLine(app_state->op_list);
         write->slide_idx = drag->slide_idx;
         write->kind = drag->kind;
         write->idx = drag->idx;
         write->edit_count = drag->edit_count;
         write->rect = drag->now;
         write->corner = drag->corner;
         drag->state = Place_DragState_Dropped;
      } else {
         // A click, not a drag, on one of several picked: that one alone is
         // picked now, as in other editors. The press kept them all, for a
         // drag (edit\edit.c).
         if (drag->state == Place_DragState_Pressed && drag->is_shift_click) {
            Edit_PreviewClickOpParams* click = StateOp_Edit_PreviewClick(app_state->op_list);
            click->pos = drag->press_mouse;
            click->picks = 1;
            click->picks_all = 1;
            click->kind = drag->kind;
            click->idx = drag->idx;
         } else if (drag->state == Place_DragState_Pressed && drag->group_count != 0) {
            Edit_PreviewClickOpParams* click = StateOp_Edit_PreviewClick(app_state->op_list);
            click->pos = drag->press_mouse;
            click->picks = 1;
            click->picks_only = 1;
            click->kind = drag->kind;
            click->idx = drag->idx;
         }
         drag->state = Place_DragState_None;
      }
      return;
   }

   if (drag->state == Place_DragState_Pressed) {
      F32 reach = _PLACE_DRAG_START * ui;
      F32 gone_x = mouse.x - drag->press_mouse.x;
      F32 gone_y = mouse.y - drag->press_mouse.y;
      if (gone_x > reach || gone_x < -reach || gone_y > reach || gone_y < -reach) {
         drag->state = Place_DragState_Dragging;
      }
   }
   if (drag->state == Place_DragState_Dragging) {
      drag->now = _Place_RectFromMouse(drag, g_edit_view.page, mouse);
      // Shift lines it up with what it is near - and keeps it on the line
      // it was on last frame until it is well past it.
      drag->has_stuck_x = drag->has_guide_x;
      drag->has_stuck_y = drag->has_guide_y;
      drag->stuck_x = drag->guide_x;
      drag->stuck_y = drag->guide_y;
      drag->has_guide_x = 0;
      drag->has_guide_y = 0;
      if (Core_WM_KeyIsDown(WM_Key_Shift)) {
         drag->now = _Place_LinedUp(drag, drag->now, ui);
      }
   }
}

// 1 if pos is at the words of the title or a block, as shown: on one of
// their lines, and no further from its letters than half the line is tall.
// A line with no words yet has that much about where they will start. The
// rest of the box - beside a short line, under the last - is not: a press
// there still puts the cursor at the nearest letter, but it is where the box
// is taken hold of to move it, and the mouse says so.
//
// A line reaches a fifth of itself up and down as well. The view leaves a
// little room between the items of a list (0.30 of a line, view.c), and
// without this the mouse turned to the arrow and back for every bullet it
// was moved down across - for a strip too thin to aim a drag at anyway. The
// wider room after an empty line of the file, or above a heading, stays the
// box's, as does the most of what is under the last line.
B32
Place_IsAtWords(Deck_PlaceKind kind, U64 idx, Vec2F32 pos) {
   B32       result = 0;
   View_Box* box = _Place_BoxOf(kind, idx);
   if (box != 0 && kind != Deck_PlaceKind_Picture && Contains2F32(box->room, pos)) {
      for (U64 i = 0; i < g_edit_view.run_count && !result; i += 1) {
         View_Run* run = &g_edit_view.runs[i];
         if (run->kind != kind || run->idx != idx) {
            continue;
         }
         F32 height = run->rect.max.y - run->rect.min.y;
         F32 pad_x = height * 0.5f;
         F32 pad_y = height * 0.2f;
         result = (pos.y >= run->rect.min.y - pad_y && pos.y < run->rect.max.y + pad_y && pos.x >= run->rect.min.x - pad_x &&
                   pos.x < run->rect.max.x + pad_x);
      }
   }
   return result;
}

// The mouse over one of the boxes of a piece of the slide - kind and idx,
// has_piece 0 for one that is of none; corner: of a handle, else 0:
//
//    a handle               the arrow that resizes the way it does
//    the piece dragged      the four-way arrow that moves it
//    the title's or a       the bar that text is typed at: a click puts the
//    block's words          caret at the letter (caret\caret.c)
//    (Place_IsAtWords)
//    the rest of their box, the four-way arrow: it moves
//    and a picture

PRIVATE WM_Cursor
_Place_HoverCursor(const App_State* app_state, B32 has_piece, Deck_PlaceKind kind, U64 idx, U32 corner) {
   Place_Drag* drag = &g_place_view.drag;
   WM_Cursor       result = WM_Cursor_Pointer;
   if (drag->state == Place_DragState_Selecting) {
      // Words are being selected: the bar they are typed at, whatever the
      // mouse is taken across on its way.
      result = WM_Cursor_IBar;
   } else if (corner >= 5) {
      result = (_Place_HandleSideX(corner) != 0) ? WM_Cursor_LeftRight : WM_Cursor_UpDown;
   } else if (corner != 0) {
      result = (corner == 1 || corner == 4) ? WM_Cursor_DownRight : WM_Cursor_UpRight;
   } else if (has_piece && drag->state == Place_DragState_Dragging && drag->kind == kind && drag->idx == idx) {
      result = WM_Cursor_UpDownLeftRight;
   } else if (has_piece && Place_IsAtWords(kind, idx, Core_WM_MouseFromWindow(app_state->window))) {
      result = WM_Cursor_IBar;
   } else if (has_piece) {
      // Where it is taken hold of to be moved: the arrow that says so.
      result = WM_Cursor_UpDownLeftRight;
   }
   return result;
}

B32
Place_PressWords(const App_State* app_state, UI_Signal signal, Deck_PlaceKind kind, U64 idx, F32 ui) {
   Vec2F32 pos = UI_DragStartMouse();
   if (!(signal.f & UI_SignalFlag_LeftPressed) || _Place_BoxOf(kind, idx) == 0 || !Place_IsAtWords(kind, idx, pos)) {
      return 0;
   }
   // Which press of a double or a triple click this is. The UI says when a
   // press follows the last on the same box in a moment - but a box here is
   // a whole line of words, or the room beside them, and a drag that ends
   // with a quick click further along the line was taken for a double
   // click: the word there selected, not the cursor put down. A press counts
   // as the next only near the last one, as Windows has it.
   F32 reach = _PLACE_DRAG_START * ui;
   F32 gone_x = pos.x - g_place_view.click_mouse.x;
   F32 gone_y = pos.y - g_place_view.click_mouse.y;
   B32 is_next = ((signal.f & UI_SignalFlag_LeftDoubleClicked) && g_place_view.click_count != 0 && gone_x <= reach &&
                  gone_x >= -reach && gone_y <= reach && gone_y >= -reach);
   g_place_view.click_count = is_next ? Min(g_place_view.click_count + 1, 3) : 1;
   g_place_view.click_mouse = pos;

   // The cursor to the letter, then: a second press selects the word, a
   // third the line, and the first is held to select by dragging - as in
   // any editor. Nothing is moved. Ops, run in this order.
   StateOp_List*              ops = app_state->op_list;
   Edit_PreviewClickOpParams* click = StateOp_Edit_PreviewClick(ops);
   click->pos = pos;
   click->keeps_mark = !!(signal.event_flags & WM_Modifier_Shift);
   if (g_place_view.click_count == 3) {
      Caret_SelectLineOpParams* line = StateOp_Caret_SelectLine(ops);
      line->kind = kind;
      line->idx = idx;
   } else if (g_place_view.click_count == 2) {
      StateOp_Caret_SelectWord(ops);
   } else {
      _Place_Begin(app_state, _Place_BoxOf(kind, idx), 0, pos, Place_DragState_Selecting);
      g_place_view.drag.select_hit = -1;
   }
   return 1;
}

B32
Place_SelectHeld(const App_State* app_state, UI_Signal signal) {
   // Asked of every box the press can have been on. The box pressed is the
   // UI's active one until the button comes up, wherever the mouse goes, and
   // signals a drag every frame - also when the mouse is still, which past
   // the end of a block keeps its words moving.
   B32             result = 0;
   Place_Drag* drag = &g_place_view.drag;
   if (drag->state == Place_DragState_Selecting && !(signal.f & UI_SignalFlag_LeftPressed)) {
      if (signal.f & UI_SignalFlag_LeftReleased) {
         drag->state = Place_DragState_None;
      } else if (signal.f & UI_SignalFlag_LeftDragging) {
         result = Caret_DragTo(app_state, drag->kind, drag->idx, UI_Mouse());
      }
   }
   return result;
}

// One of the boxes of a piece of the slide - kind and idx, has_piece 0 for
// one that is of none - asked for its signal. corner: of a handle, else 0.
// hit_idx: of a hit of the preview, else -1. 1 if what was done to it changed
// the text, the cursor or a block's scroll.
PRIVATE B32
_Place_Signal(const App_State* app_state, UI_Box* box, B32 has_piece, Deck_PlaceKind kind, U64 idx, U32 corner, S64 hit_idx,
              Place_Hover* hover, F32 ui) {
   B32       result = 0;
   UI_Signal signal = UI_SignalFromBox(box);
   View_Box* piece = has_piece ? _Place_BoxOf(kind, idx) : 0;

   if (UI_Hovering(signal) && !hover->is_over) {
      hover->is_over = 1;
      hover->has_piece = (piece != 0);
      hover->kind = kind;
      hover->idx = idx;
      hover->corner = corner;
      hover->hit_idx = hit_idx;
   }

   B32 ctrl = !!(signal.event_flags & WM_Modifier_Ctrl);
   B32 shift = !!(signal.event_flags & WM_Modifier_Shift);
   if ((signal.f & UI_SignalFlag_LeftPressed) && corner == 0 && piece != 0 && ctrl) {
      // Ctrl: the piece added to those picked whole, or taken out of them
      // (edit\edit.c). Nothing moves, and the cursor stays.
      Edit_PreviewClickOpParams* click = StateOp_Edit_PreviewClick(app_state->op_list);
      click->pos = UI_DragStartMouse();
      click->picks = 1;
      click->adds = 1;
      click->kind = kind;
      click->idx = idx;
      g_place_view.click_count = 0;
      result = 1;
   } else if ((signal.f & UI_SignalFlag_LeftPressed) && corner == 0 && piece != 0 && shift && Edit_PickedIsGood(app_state)) {
      // Shift, with pieces picked: watched for a drag, which Shift lines up
      // - it was no drag at all, with Shift down before the press, and the
      // key had to be found after the button. Let go of with no drag, every
      // piece of the kind of the first is picked (Place_BeforeLayout).
      _Place_Begin(app_state, piece, 0, UI_DragStartMouse(), Place_DragState_Pressed);
      g_place_view.drag.is_shift_click = 1;
      g_place_view.click_count = 0;
      result = 1;
   } else if (signal.f & UI_SignalFlag_LeftPressed) {
      Vec2F32 pos = UI_DragStartMouse();
      if (corner != 0 && piece != 0) {
         _Place_Begin(app_state, piece, corner, pos, Place_DragState_Dragging);
      } else {
         // A press is a click until the mouse goes somewhere: to the words,
         // or the picture (edit\edit.c) - and watched, for a drag. Right at
         // the words the drag selects them instead (Place_PressWords).
         if (piece == 0 || !Place_PressWords(app_state, signal, kind, idx, ui)) {
            g_place_view.click_count = 0;
            Edit_PreviewClickOpParams* click = StateOp_Edit_PreviewClick(app_state->op_list);
            click->pos = pos;
            click->keeps_mark = !!(signal.event_flags & WM_Modifier_Shift);
            // Away from the words, the piece is picked whole: Delete takes
            // it out (edit\edit.c).
            click->picks = (piece != 0);
            click->kind = kind;
            click->idx = idx;
            if (piece != 0) {
               _Place_Begin(app_state, piece, 0, pos, Place_DragState_Pressed);
            }
         } else if (g_place_view.drag.state == Place_DragState_Selecting) {
            g_place_view.drag.select_hit = hit_idx;
         }
      }
      result = 1;
   }
   result |= Place_SelectHeld(app_state, signal);

   // A right-click opens the edit screen's menu on the piece (edit\edit.c):
   // putting it back where the layout has it is a line of that.
   if ((signal.f & UI_SignalFlag_RightPressed) && piece != 0) {
      Edit_OpenMenu(app_state, Core_WM_MouseFromWindow(app_state->window), 1, kind, idx);
   }

   // The wheel over a block's words moves them (caret\caret.c). Over
   // anything else of the slide it does nothing - not the block under a
   // picture.
   // With Ctrl down the wheel sizes words, and does not come here: the UI
   // gives a box no wheel that has Ctrl with it (Place_CtrlWheel).
   if (signal.scroll.y != 0 && has_piece && kind == Deck_PlaceKind_Text) {
      Caret_ScrollBlock(app_state, idx, (F32)signal.scroll.y / FR_UI_SCROLL_PER_NOTCH);
      result = 1;
   }

   return result;
}

B32
Place_BuildCorners(const App_State* app_state, F32 ui, B32 interactive, Place_Hover* hover) {
   B32 result = 0;
   if (!interactive || !Place_BoxesAreGood(app_state)) {
      return 0;
   }
   UI_BoxFlags flags = UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll;

   // Their handles reach past their boxes, over whatever else is there -
   // a video's strip too. The last drawn, which is on top, first.
   for (U64 i = g_edit_view.box_count; i > 0; i -= 1) {
      View_Box* box = &g_edit_view.boxes[i - 1];
      for (U32 corner = 1; corner <= _PLACE_HANDLE_COUNT; corner += 1) {
         if (!_Place_HasHandle(box->rect, corner, ui)) {
            continue;
         }
         UI_Box* handle = FR_BuildRectBox(flags, _Place_HandleRect(box->rect, corner, ui), "###place corner %d %llu %u",
                                          (int)box->kind, (unsigned long long)box->idx, corner);
         handle->hover_cursor = _Place_HoverCursor(app_state, 1, box->kind, box->idx, corner);
         result |= _Place_Signal(app_state, handle, 1, box->kind, box->idx, corner, -1, hover, ui);
      }
   }
   return result;
}

B32
Place_BuildHits(const App_State* app_state, F32 ui, B32 interactive, Place_Hover* hover) {
   B32 result = 0;
   // Not built at all under the open screen: they are only to be clicked.
   if (!interactive || !Place_BoxesAreGood(app_state)) {
      return 0;
   }
   Deck_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   UI_BoxFlags flags = UI_BoxFlag_MouseClickable | UI_BoxFlag_Scroll;

   //- bgz: the words and the pictures, as drawn: the last drawn first, which
   // is the one on top. A block's words are over a picture under them
   for (U64 i = g_edit_view.hit_count; i > 0; i -= 1) {
      View_Hit*      hit = &g_edit_view.hits[i - 1];
      Deck_PlaceKind kind = Deck_PlaceKind_Title;
      U64            idx = 0;
      B32            has_piece = _Place_PieceOfHit(slide, hit, &kind, &idx);
      UI_Box*        box = FR_BuildRectBox(flags, hit->rect, "###place hit %llu", (unsigned long long)(i - 1));
      box->hover_cursor = _Place_HoverCursor(app_state, has_piece, kind, idx, 0);
      result |= _Place_Signal(app_state, box, has_piece, kind, idx, 0, (S64)(i - 1), hover, ui);
   }

   //- bgz: the line words are being selected from, when the drag has moved
   // it out of its block's room: it has no hit now, and its box would be
   // gone - and the UI lets go of a box that is not built, which left the
   // selection stuck where it was, with the button still down. Built
   // anyway, the same box by the same key, with no room to be pressed in:
   // only held, until the button comes up.
   Place_Drag* drag = &g_place_view.drag;
   if (drag->state == Place_DragState_Selecting && drag->select_hit >= (S64)g_edit_view.hit_count) {
      UI_Box* held = FR_BuildRectBox(flags, R2F32P(0, 0, 0, 0), "###place hit %llu", (unsigned long long)drag->select_hit);
      held->hover_cursor = WM_Cursor_IBar;
      result |= Place_SelectHeld(app_state, UI_SignalFromBox(held));
   }

   //- bgz: the pieces' own boxes, where no words are: a picture over the
   // title's or a block's, and the title's over a block's
   Deck_PlaceKind kinds[] = {Deck_PlaceKind_Picture, Deck_PlaceKind_Title, Deck_PlaceKind_Text};
   for (U64 k = 0; k < ArrayCount(kinds); k += 1) {
      for (U64 i = g_edit_view.box_count; i > 0; i -= 1) {
         View_Box* box = &g_edit_view.boxes[i - 1];
         if (box->kind != kinds[k] || (box->kind == Deck_PlaceKind_Picture && box->idx >= slide->picture_count)) {
            continue;
         }
         UI_Box* ui_box = FR_BuildRectBox(flags, box->rect, "###place box %d %llu", (int)box->kind, (unsigned long long)box->idx);
         ui_box->hover_cursor = _Place_HoverCursor(app_state, 1, box->kind, box->idx, 0);
         result |= _Place_Signal(app_state, ui_box, 1, box->kind, box->idx, 0, -1, hover, ui);
      }
   }

   return result;
}

B32
Place_BuildLook(const App_State* app_state, F32 ui, Place_Hover* hover) {
   B32             result = 0;
   Place_Drag* drag = &g_place_view.drag;
   if (app_state->show_open || app_state->slide_idx >= app_state->deck->slide_count) {
      return 0;
   }

   B32       is_dragging = (drag->state == Place_DragState_Dragging || drag->state == Place_DragState_Dropped);
   View_Box* over = 0;
   if (is_dragging) {
      over = _Place_BoxOf(drag->kind, drag->idx);
   } else if (Place_BoxesAreGood(app_state) && hover->is_over && hover->has_piece) {
      over = _Place_BoxOf(hover->kind, hover->idx);
   }

   //- bgz: what a drag with Shift down is lined up with: a line the whole
   // way across the page, or down it, and the box each line came from
   if (is_dragging) {
      Rng2F32 page = g_edit_view.page;
      Vec2F32 page_dim = Dim2F32(page);
      F32     guide_thick = Max(1.0f, (F32)(S32)ui);
      UI_ColorTag(Str8Lit("guide")) {
         if (drag->has_guide_x) {
            F32 x = (F32)(S32)(page.min.x + drag->guide_x * page_dim.x - guide_thick * 0.5f);
            // Across all of a board that shows, where its page is not all there is.
            Rng2F32 span = g_edit_view.is_board ? g_edit_view.board : page;
            x = g_edit_view.is_board ? x : Clamp(page.min.x, x, page.max.x - guide_thick);
            FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(x, span.min.y, x + guide_thick, span.max.y), "###place guide x");
         }
         if (drag->has_guide_y) {
            F32 y = (F32)(S32)(page.min.y + drag->guide_y * page_dim.y - guide_thick * 0.5f);
            Rng2F32 span = g_edit_view.is_board ? g_edit_view.board : page;
            y = g_edit_view.is_board ? y : Clamp(page.min.y, y, page.max.y - guide_thick);
            FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(span.min.x, y, span.max.x, y + guide_thick), "###place guide y");
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
      Vec4F32 accent = View_ColorsFromTheme(app_state->deck->theme).accent;
      F32     thick = Max(1.0f, (F32)(S32)(1.5f * ui));

      for (U32 corner = 1; corner <= _PLACE_HANDLE_COUNT; corner += 1) {
         if (!_Place_HasHandle(box, corner, ui)) {
            continue;
         }
         Rng2F32 handle = _Place_HandleRect(box, corner, ui);
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

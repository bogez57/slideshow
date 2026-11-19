////////////////////////////////
//~ bgz: the edit screen: a slide's picture and its body, moved and resized with the mouse
//
// In the preview, each of the slide's pictures - or videos - its title and
// its body (every line under the title) have a box: where VIEW_DrawSlide
// put them. With the
// mouse over one, its outline and a handle at each corner show.
//
//    move it          press inside the box and drag
//    resize it        drag a corner. A picture keeps its shape. The body's
//                     box takes any shape: its words wrap at the width, and
//                     are made smaller if they do not fit the height
//    line it up       hold Shift while dragging: an edge or the middle of
//                     it jumps to the page's edges, margins and middle, and
//                     to the edges and middles of the slide's other boxes,
//                     when it comes near one. A line shows what it is on
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
// to the words (frame_edit.c). So a press inside a box is not taken from
// the click, only watched. A press on a corner is taken.
//
// The boxes are last frame's, as the hits of the preview are: good only
// while the text is what it was when they were drawn.

// The side of a corner's handle, and how far the mouse goes with the button
// down before it is a drag and not a click. In UI units.
#define _PLACE_HANDLE     10.0f
#define _PLACE_DRAG_START 4.0f

// With Shift down: how near an edge comes to a line before it jumps to it.
// In UI units.
#define _PLACE_SNAP 7.0f

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

// Whether the boxes kept from the last frame are of the text as it is.
internal B32
_PLACE_BoxesAreGood(App_State* app_state) {
   return (app_state->editor != 0 && app_state->editor->edit_count == app_state->preview_hits_edit_count &&
           !app_state->show_open && app_state->slide_idx < app_state->deck->slide_count &&
           Dim2F32(app_state->preview_page).x > 0 && Dim2F32(app_state->preview_page).y > 0);
}

// The slide's place of a kind: a picture's is the picture's own.
internal DECK_Place*
_PLACE_PlaceOf(DECK_Slide* slide, DECK_PlaceKind kind, U64 picture_idx) {
   DECK_Place* result = &slide->places[kind];
   if (kind == DECK_PlaceKind_Picture && picture_idx < slide->picture_count) {
      result = &slide->pictures[picture_idx].place;
   }
   return result;
}

// Which box a point is on: 0 for none. Where boxes are over each other, a
// point on a line of the body's words is on the body, one on the title's
// words on the title, and any other on the picture drawn last - the one on
// top - then the title, then the body.
internal VIEW_Box*
_PLACE_BoxAt(App_State* app_state, Vec2F32 pos) {
   DECK_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
   VIEW_Box*   title = 0;
   VIEW_Box*   text = 0;
   VIEW_Box*   picture = 0;
   for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
      VIEW_Box* box = &app_state->preview_boxes[i];
      if (Contains2F32(box->rect, pos)) {
         if (box->kind == DECK_PlaceKind_Title) {
            title = box;
         } else if (box->kind == DECK_PlaceKind_Text) {
            text = box;
         } else if (box->picture_idx < slide->picture_count) {
            picture = box;
         }
      }
   }

   // What words the point is on, if any.
   B32 on_title_words = 0;
   B32 on_body_words = 0;
   for (U64 i = 0; i < app_state->preview_hit_count; i += 1) {
      VIEW_Hit* hit = &app_state->preview_hits[i];
      if (!Contains2F32(hit->rect, pos)) {
         continue;
      }
      B32 is_picture = 0;
      for (U64 k = 0; k < slide->picture_count; k += 1) {
         is_picture |= (hit->text_range.min == slide->pictures[k].range.min && hit->text_range.max == slide->pictures[k].range.max);
      }
      B32 is_title = (hit->text_range.min == slide->title_range.min && hit->text_range.max == slide->title_range.max);
      on_title_words |= (is_title && !is_picture);
      on_body_words |= (!is_title && !is_picture);
   }

   VIEW_Box* result = 0;
   if (text != 0 && on_body_words) {
      result = text;
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

// Whether pos is on the strip of controls of one of the preview's videos.
internal B32
_PLACE_IsOnVideoStrip(App_State* app_state, F32 ui, Vec2F32 pos) {
   B32 result = 0;
   for (U64 picture_idx = 0; picture_idx < DECK_MAX_PICTURES && !result; picture_idx += 1) {
      Rng2F32    video_rect = {0};
      VID_Video* video = _EDIT_PreviewVideo(app_state, app_state->preview_page, picture_idx, &video_rect);
      result = (video != 0 && FR_VideoStripContains(app_state, video_rect, ui, pos));
   }
   return result;
}

// Writes the slide's @place line for kind - for the picture_idx-th picture,
// if a picture's: over the one it has (before), or as a new line under the
// picture's line, under the title's, or above the body's first.
internal void
_PLACE_WriteLine(ED_Editor* editor, DECK_Slide* slide, DECK_PlaceKind kind, U64 picture_idx, DECK_Place* before, Rng2F32 rect) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 text = ED_Text(editor);
   String8 line = PushStr8F(scratch.arena, "@place %S %.3f %.3f %.3f %.3f", DECK_PlaceName(kind), rect.min.x, rect.min.y,
                            rect.max.x - rect.min.x, rect.max.y - rect.min.y);

   if (before->has_line && before->line.max <= text.size) {
      _EDIT_ReplaceKeepingCursor(editor, before->line.min, before->line.max, line);
   } else {
      // The line of the text the thing is on: the picture's, the title's,
      // or the first of the body's.
      U64 on = slide->text_pos;
      if (kind == DECK_PlaceKind_Picture && picture_idx < slide->picture_count) {
         on = slide->pictures[picture_idx].range.min;
      } else if (kind == DECK_PlaceKind_Title) {
         on = slide->title_range.min;
      } else if (kind == DECK_PlaceKind_Text && slide->first_item != 0) {
         on = slide->first_item->text_range.min;
      }
      on = Min(on, text.size);
      U64 line_start = on;
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 line_end = on;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;

      // The body's goes above its first line, where it does not come between
      // two lines of the body - unless that line is what starts the slide:
      // above it would be the slide before. A picture's goes under the
      // picture's line: it is for the picture last written above it. The
      // title's goes under the title's, which is what starts the slide.
      if (kind == DECK_PlaceKind_Text && line_start > slide->text_pos) {
         _EDIT_ReplaceKeepingCursor(editor, line_start, line_start, PushStr8F(scratch.arena, "%S\n", line));
      } else {
         _EDIT_ReplaceKeepingCursor(editor, line_end, line_end, PushStr8F(scratch.arena, "\n%S", line));
      }
   }

   ScratchEnd(scratch);
}

// Takes an @place line out of the text, with its line ending.
internal void
_PLACE_RemoveLine(ED_Editor* editor, DECK_Place* place) {
   String8 text = ED_Text(editor);
   if (place->has_line && place->line.max <= text.size) {
      U64 start = place->line.min;
      U64 end = place->line.max;
      // All of the line: what is in front of the @ too.
      while (start > 0 && text.str[start - 1] != '\n') start -= 1;
      while (end < text.size && text.str[end] != '\n') end += 1;
      if (end < text.size) {
         end += 1;
      } else if (start > 0) {
         start -= 1;
      }
      _EDIT_ReplaceKeepingCursor(editor, start, end, Str8Lit(""));
   }
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

      // The least it can be: the body's box more than a picture's. Its words
      // wrap at its width and get smaller only so far, so a box much
      // narrower than this has a word a line, running down off the page.
      B32 is_text = (drag->kind != DECK_PlaceKind_Picture);
      F32 min_w = is_text ? Min(_PLACE_TEXT_MIN_W, w0) : DECK_PLACE_MIN;
      F32 min_h = is_text ? Min(_PLACE_TEXT_MIN_H, h0) : DECK_PLACE_MIN;

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

// The line nearest value among lines, if one is within reach: how far value
// is from it (to add), and the line. 0 if none is.
internal B32
_PLACE_NearestLine(F32 value, F32* lines, U64 line_count, F32 reach, F32* shift_out, F32* line_out) {
   B32 result = 0;
   F32 best = reach;
   for (U64 i = 0; i < line_count; i += 1) {
      F32 gap = lines[i] - value;
      F32 gap_size = (gap < 0) ? -gap : gap;
      if (gap_size <= best) {
         best = gap_size;
         *shift_out = gap;
         *line_out = lines[i];
         result = 1;
      }
   }
   return result;
}

// With Shift down: rect, where the mouse has the thing dragged, lined up
// with what it is near - the page's edges, margins and middle, and the edges
// and middles of the slide's other boxes. Across and down each on their
// own. A move tries its two edges and its middle; a corner only the edges
// it is moving, and a picture, which keeps its shape, one of the two.
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

   //- bgz: the lines there are, as parts of the page
   F32 xs[3 * (DECK_MAX_PICTURES + 2) + 5];
   F32 ys[3 * (DECK_MAX_PICTURES + 2) + 5];
   U64 x_count = 0;
   U64 y_count = 0;
   xs[x_count++] = 0.0f;
   xs[x_count++] = 0.5f;
   xs[x_count++] = 1.0f;
   xs[x_count++] = _VIEW_MARGIN_X / VIEW_PAGE_W;
   xs[x_count++] = 1.0f - _VIEW_MARGIN_X / VIEW_PAGE_W;
   ys[y_count++] = 0.0f;
   ys[y_count++] = 0.5f;
   ys[y_count++] = 1.0f;
   ys[y_count++] = _VIEW_MARGIN_TOP / VIEW_PAGE_H;
   ys[y_count++] = 1.0f - _VIEW_MARGIN_BOTTOM / VIEW_PAGE_H;
   for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
      VIEW_Box* box = &app_state->preview_boxes[i];
      B32       is_dragged = (box->kind == drag->kind && (box->kind != DECK_PlaceKind_Picture || box->picture_idx == drag->picture_idx));
      if (is_dragged) {
         continue;
      }
      F32 left = (box->rect.min.x - page.min.x) / page_dim.x;
      F32 right = (box->rect.max.x - page.min.x) / page_dim.x;
      F32 top = (box->rect.min.y - page.min.y) / page_dim.y;
      F32 bottom = (box->rect.max.y - page.min.y) / page_dim.y;
      xs[x_count++] = left;
      xs[x_count++] = (left + right) * 0.5f;
      xs[x_count++] = right;
      ys[y_count++] = top;
      ys[y_count++] = (top + bottom) * 0.5f;
      ys[y_count++] = bottom;
   }

   F32 reach_x = _PLACE_SNAP * ui / page_dim.x;
   F32 reach_y = _PLACE_SNAP * ui / page_dim.y;
   F32 w = rect.max.x - rect.min.x;
   F32 h = rect.max.y - rect.min.y;
   Rng2F32 result = rect;

   if (drag->corner == 0) {
      //- bgz: a move: the nearest of what its edges and its middle are near
      F32 own_x[3] = {rect.min.x, (rect.min.x + rect.max.x) * 0.5f, rect.max.x};
      F32 own_y[3] = {rect.min.y, (rect.min.y + rect.max.y) * 0.5f, rect.max.y};
      F32 best_x = reach_x;
      F32 best_y = reach_y;
      for (U64 i = 0; i < 3; i += 1) {
         F32 shift = 0;
         F32 line = 0;
         if (_PLACE_NearestLine(own_x[i], xs, x_count, best_x, &shift, &line)) {
            best_x = (shift < 0) ? -shift : shift;
            result.min.x = rect.min.x + shift;
            result.max.x = rect.max.x + shift;
            drag->has_guide_x = 1;
            drag->guide_x = line;
         }
         if (_PLACE_NearestLine(own_y[i], ys, y_count, best_y, &shift, &line)) {
            best_y = (shift < 0) ? -shift : shift;
            result.min.y = rect.min.y + shift;
            result.max.y = rect.max.y + shift;
            drag->has_guide_y = 1;
            drag->guide_y = line;
         }
      }
   } else {
      //- bgz: a corner: the edges it moves
      B32 is_left = (drag->corner == 1 || drag->corner == 3);
      B32 is_top = (drag->corner == 1 || drag->corner == 2);
      B32 keeps_shape = (drag->kind == DECK_PlaceKind_Picture && w > 0 && h > 0);
      F32 shift = 0;
      F32 line = 0;
      B32 did_x = 0;
      if (_PLACE_NearestLine(is_left ? rect.min.x : rect.max.x, xs, x_count, reach_x, &shift, &line)) {
         F32 new_w = w + (is_left ? -shift : shift);
         if (new_w >= DECK_PLACE_MIN) {
            did_x = 1;
            drag->has_guide_x = 1;
            drag->guide_x = line;
            if (is_left) result.min.x = line; else result.max.x = line;
            if (keeps_shape) {
               // The other way follows, from the corner that stays.
               F32 new_h = h * new_w / w;
               if (is_top) result.min.y = rect.max.y - new_h; else result.max.y = rect.min.y + new_h;
            }
         }
      }
      if (!(keeps_shape && did_x) && _PLACE_NearestLine(is_top ? rect.min.y : rect.max.y, ys, y_count, reach_y, &shift, &line)) {
         F32 new_h = h + (is_top ? -shift : shift);
         if (new_h >= DECK_PLACE_MIN) {
            drag->has_guide_y = 1;
            drag->guide_y = line;
            if (is_top) result.min.y = line; else result.max.y = line;
            if (keeps_shape) {
               F32 new_w = w * new_h / h;
               if (is_left) result.min.x = rect.max.x - new_w; else result.max.x = rect.min.x + new_w;
            }
         }
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
   U64             picture_idx = from->picture_idx;

   MemoryZeroStruct(drag);
   drag->state = state;
   drag->kind = kind;
   drag->picture_idx = picture_idx;
   drag->corner = corner;
   drag->slide_idx = app_state->slide_idx;
   drag->edit_count = app_state->editor->edit_count;
   drag->press_mouse = pos;
   drag->start = R2F32P((box.min.x - page.min.x) / page_dim.x, (box.min.y - page.min.y) / page_dim.y,
                        (box.max.x - page.min.x) / page_dim.x, (box.max.y - page.min.y) / page_dim.y);
   drag->now = drag->start;
   drag->before = *_PLACE_PlaceOf(&app_state->deck->slides[app_state->slide_idx], kind, picture_idx);
}

// Before the edit screen's own keys and clicks (EDIT_Input), and before
// anything this frame has changed the text.
internal void
PLACE_Input(App_State* app_state, F32 ui) {
   EDIT_PlaceDrag* drag = &app_state->place_drag;
   ED_Editor*      editor = app_state->editor;
   DECK_Deck*      deck = app_state->deck;
   WM_EventList*   events = &app_state->events;

   //- bgz: one under way
   if (drag->state != EDIT_PlaceDragState_None) {
      B32 is_same = (editor != 0 && editor->edit_count == drag->edit_count && drag->slide_idx == app_state->slide_idx &&
                     drag->slide_idx < deck->slide_count && !app_state->show_open);
      if (!is_same) {
         // The text has changed under it, or the slide: given up. A deck
         // made again from the text has the place the text says already.
         if (editor != 0 && editor->edit_count == drag->edit_count && drag->slide_idx < deck->slide_count) {
            *_PLACE_PlaceOf(&deck->slides[drag->slide_idx], drag->kind, drag->picture_idx) = drag->before;
         }
         drag->state = EDIT_PlaceDragState_None;
         return;
      }

      DECK_Slide* slide = &deck->slides[drag->slide_idx];
      Vec2F32     mouse = Core_WM_MouseFromWindow(app_state->window);

      DECK_Place* place = _PLACE_PlaceOf(slide, drag->kind, drag->picture_idx);

      if (drag->state == EDIT_PlaceDragState_Dragging && Core_WM_KeyPress(events, app_state->window, 0, WM_Key_Esc)) {
         *place = drag->before;
         drag->state = EDIT_PlaceDragState_None;
         return;
      }

      if (!Core_WM_KeyIsDown(WM_Key_LeftMouseButton)) {
         if (drag->state == EDIT_PlaceDragState_Dragging) {
            *place = drag->before;
            _PLACE_WriteLine(editor, slide, drag->kind, drag->picture_idx, &drag->before, drag->now);
            editor->last_input_us = NowTimeUS();
         }
         drag->state = EDIT_PlaceDragState_None;
         return;
      }

      if (drag->state == EDIT_PlaceDragState_Pressed) {
         // Not "far": windows.h makes that word nothing.
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
      return;
   }

   //- bgz: one begun, or a box put back
   if (!_PLACE_BoxesAreGood(app_state)) {
      return;
   }
   for (WM_Event* event = events->first; event != 0; event = event->next) {
      if (event->kind != WM_EventKind_Press) {
         continue;
      }

      if (event->key == WM_Key_LeftMouseButton) {
         // A corner first: its handle reaches past the box, and over
         // whatever else is there. Of the box the point is on, if it is on
         // one, then of any - the last drawn, which is on top, first.
         VIEW_Box* on = _PLACE_BoxAt(app_state, event->pos);
         VIEW_Box* held = 0;
         U32       held_corner = 0;
         for (U64 i = app_state->preview_box_count + 1; i > 0 && held == 0; i -= 1) {
            VIEW_Box* box = (i == app_state->preview_box_count + 1) ? on : &app_state->preview_boxes[i - 1];
            for (U32 corner = 1; corner <= 4 && box != 0; corner += 1) {
               if (Contains2F32(_PLACE_HandleRect(box->rect, corner, ui), event->pos)) {
                  held = box;
                  held_corner = corner;
                  break;
               }
            }
         }
         if (held != 0) {
            _PLACE_Begin(app_state, held, held_corner, event->pos, EDIT_PlaceDragState_Dragging);
            Core_WM_EatEvent(events, event);
            break;
         }

         // Inside a box: watched, and left for the click it may be. Not on
         // a video's strip of controls: a press there is the strip's.
         if (on != 0 && !_PLACE_IsOnVideoStrip(app_state, ui, event->pos)) {
            _PLACE_Begin(app_state, on, 0, event->pos, EDIT_PlaceDragState_Pressed);
            break;
         }
      }

      if (event->key == WM_Key_RightMouseButton) {
         VIEW_Box* on = _PLACE_BoxAt(app_state, event->pos);
         if (on != 0) {
            DECK_Place* place = _PLACE_PlaceOf(&deck->slides[app_state->slide_idx], on->kind, on->picture_idx);
            if (place->has_line) {
               _PLACE_RemoveLine(editor, place);
               editor->last_input_us = NowTimeUS();
               FR_Notice(app_state, Str8Lit("Back where the layout puts it"));
            }
            Core_WM_EatEvent(events, event);
            break;
         }
      }
   }
}

// The outline and the corners of the box the mouse is over, or that is
// being dragged. After the preview's slide is drawn and its boxes kept.
// Returns 1 if it drew any: the mouse is on something that can be moved.
internal B32
PLACE_Draw(App_State* app_state, F32 ui, Vec2F32 mouse) {
   B32             result = 0;
   EDIT_PlaceDrag* drag = &app_state->place_drag;
   if (app_state->show_open || app_state->slide_idx >= app_state->deck->slide_count) {
      return 0;
   }

   B32       is_dragging = (drag->state == EDIT_PlaceDragState_Dragging);
   VIEW_Box* over = 0;
   if (is_dragging) {
      for (U64 i = 0; i < app_state->preview_box_count; i += 1) {
         VIEW_Box* candidate = &app_state->preview_boxes[i];
         if (candidate->kind == drag->kind && (drag->kind != DECK_PlaceKind_Picture || candidate->picture_idx == drag->picture_idx)) {
            over = candidate;
         }
      }
   } else if (_PLACE_BoxesAreGood(app_state)) {
      over = _PLACE_BoxAt(app_state, mouse);
      // A corner's handle reaches past its box.
      for (U64 i = app_state->preview_box_count; i > 0 && over == 0; i -= 1) {
         for (U32 corner = 1; corner <= 4; corner += 1) {
            if (Contains2F32(_PLACE_HandleRect(app_state->preview_boxes[i - 1].rect, corner, ui), mouse)) {
               over = &app_state->preview_boxes[i - 1];
               break;
            }
         }
      }
   }

   if (over != 0) {
      Rng2F32 box = over->rect;
      Vec4F32 accent = VIEW_ColorsFromTheme(app_state->deck->theme).accent;
      Vec4F32 line_color = accent;
      line_color.w = is_dragging ? 1.0f : 0.7f;
      F32 thick = Max(1.0f, (F32)(S32)(1.5f * ui));

      Core_D_Rect(R2F32P(box.min.x, box.min.y, box.max.x, box.min.y + thick), R2F32P(0, 0, 1, 1), line_color);
      Core_D_Rect(R2F32P(box.min.x, box.max.y - thick, box.max.x, box.max.y), R2F32P(0, 0, 1, 1), line_color);
      Core_D_Rect(R2F32P(box.min.x, box.min.y, box.min.x + thick, box.max.y), R2F32P(0, 0, 1, 1), line_color);
      Core_D_Rect(R2F32P(box.max.x - thick, box.min.y, box.max.x, box.max.y), R2F32P(0, 0, 1, 1), line_color);

      for (U32 corner = 1; corner <= 4; corner += 1) {
         Rng2F32 handle = _PLACE_HandleRect(box, corner, ui);
         B32     is_hot = (is_dragging ? drag->corner == corner : Contains2F32(handle, mouse));
         Core_D_Rect(handle, R2F32P(0, 0, 1, 1), accent);
         Core_D_Rect(R2F32P(handle.min.x + thick, handle.min.y + thick, handle.max.x - thick, handle.max.y - thick),
                     R2F32P(0, 0, 1, 1), is_hot ? accent : V4F32(1, 1, 1, 1));
      }
      result = 1;
   }

   // What a drag with Shift down is lined up with: a line the whole way
   // across the page, or down it.
   if (is_dragging) {
      Rng2F32 page = app_state->preview_page;
      Vec2F32 page_dim = Dim2F32(page);
      Vec4F32 guide_color = V4F32(1.0f, 0.25f, 0.65f, 0.9f); // not a colour any theme has
      F32     guide_thick = Max(1.0f, (F32)(S32)ui);
      if (drag->has_guide_x) {
         F32 x = (F32)(S32)(page.min.x + drag->guide_x * page_dim.x - guide_thick * 0.5f);
         x = Clamp(page.min.x, x, page.max.x - guide_thick);
         Core_D_Rect(R2F32P(x, page.min.y, x + guide_thick, page.max.y), R2F32P(0, 0, 1, 1), guide_color);
      }
      if (drag->has_guide_y) {
         F32 y = (F32)(S32)(page.min.y + drag->guide_y * page_dim.y - guide_thick * 0.5f);
         y = Clamp(page.min.y, y, page.max.y - guide_thick);
         Core_D_Rect(R2F32P(page.min.x, y, page.max.x, y + guide_thick), R2F32P(0, 0, 1, 1), guide_color);
      }
   }

   return result;
}

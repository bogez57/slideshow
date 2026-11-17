////////////////////////////////
//~ bgz: the edit screen: a slide's picture and its body, moved and resized with the mouse
//
// In the preview, the slide's picture - or video - and its body (every line
// under the title) each have a box: where VIEW_DrawSlide put them. With the
// mouse over one, its outline and a handle at each corner show.
//
//    move it          press inside the box and drag
//    resize it        drag a corner. A picture keeps its shape. The body's
//                     box takes any shape: its words wrap at the width, and
//                     are made smaller if they do not fit the height
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

// Which box a point is on: DECK_PlaceKind_COUNT for neither. Where the body's
// box and the picture's are over each other, a point on a line of words is
// on the body, and any other on the picture.
internal DECK_PlaceKind
_PLACE_KindAt(App_State* app_state, Vec2F32 pos) {
   B32 on_picture = (app_state->preview_has_box[DECK_PlaceKind_Picture] &&
                     Contains2F32(app_state->preview_boxes[DECK_PlaceKind_Picture], pos));
   B32 on_text = (app_state->preview_has_box[DECK_PlaceKind_Text] &&
                  Contains2F32(app_state->preview_boxes[DECK_PlaceKind_Text], pos));

   if (on_picture && on_text) {
      DECK_Slide* slide = &app_state->deck->slides[app_state->slide_idx];
      on_text = 0;
      for (U64 i = 0; i < app_state->preview_hit_count; i += 1) {
         VIEW_Hit* hit = &app_state->preview_hits[i];
         B32       is_picture = (hit->text_range.min == slide->image_range.min && hit->text_range.max == slide->image_range.max);
         B32       is_title = (hit->text_range.min == slide->title_range.min && hit->text_range.max == slide->title_range.max);
         if (!is_picture && !is_title && Contains2F32(hit->rect, pos)) {
            on_text = 1;
            break;
         }
      }
   }

   DECK_PlaceKind result = DECK_PlaceKind_COUNT;
   if (on_text) {
      result = DECK_PlaceKind_Text;
   } else if (on_picture) {
      result = DECK_PlaceKind_Picture;
   }
   return result;
}

// Writes the slide's @place line for kind: over the one it has (before), or
// as a new line beside the picture's line, or the body's first.
internal void
_PLACE_WriteLine(ED_Editor* editor, DECK_Slide* slide, DECK_PlaceKind kind, DECK_Place* before, Rng2F32 rect) {
   Temp    scratch = ScratchBegin(0, 0);
   String8 text = ED_Text(editor);
   String8 line = PushStr8F(scratch.arena, "@place %S %.3f %.3f %.3f %.3f", DECK_PlaceName(kind), rect.min.x, rect.min.y,
                            rect.max.x - rect.min.x, rect.max.y - rect.min.y);

   if (before->has_line && before->line.max <= text.size) {
      _EDIT_ReplaceKeepingCursor(editor, before->line.min, before->line.max, line);
   } else {
      // The line of the text the thing is on: the picture's, or the first of
      // the body's.
      U64 on = (kind == DECK_PlaceKind_Picture) ? slide->image_range.min
                                                : (slide->first_item != 0 ? slide->first_item->text_range.min : slide->text_pos);
      on = Min(on, text.size);
      U64 line_start = on;
      while (line_start > 0 && text.str[line_start - 1] != '\n') line_start -= 1;
      U64 line_end = on;
      while (line_end < text.size && text.str[line_end] != '\n') line_end += 1;

      // The body's goes above its first line, where it does not come between
      // two lines of the body - unless that line is what starts the slide:
      // above it would be the slide before. The picture's goes under the
      // picture's line.
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

      // The most there is room for on the page, from that corner.
      F32 max_w = Max(is_left ? anchor_x : 1.0f - anchor_x, DECK_PLACE_MIN);
      F32 max_h = Max(is_top ? anchor_y : 1.0f - anchor_y, DECK_PLACE_MIN);
      F32 w = Clamp(DECK_PLACE_MIN, w0 + (is_left ? -dx : dx), max_w);
      F32 h = Clamp(DECK_PLACE_MIN, h0 + (is_top ? -dy : dy), max_h);

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

internal void
_PLACE_Begin(App_State* app_state, DECK_PlaceKind kind, U32 corner, Vec2F32 pos, EDIT_PlaceDragState state) {
   EDIT_PlaceDrag* drag = &app_state->place_drag;
   Rng2F32         page = app_state->preview_page;
   Vec2F32         page_dim = Dim2F32(page);
   Rng2F32         box = app_state->preview_boxes[kind];

   MemoryZeroStruct(drag);
   drag->state = state;
   drag->kind = kind;
   drag->corner = corner;
   drag->slide_idx = app_state->slide_idx;
   drag->edit_count = app_state->editor->edit_count;
   drag->press_mouse = pos;
   drag->start = R2F32P((box.min.x - page.min.x) / page_dim.x, (box.min.y - page.min.y) / page_dim.y,
                        (box.max.x - page.min.x) / page_dim.x, (box.max.y - page.min.y) / page_dim.y);
   drag->now = drag->start;
   drag->before = app_state->deck->slides[app_state->slide_idx].places[kind];
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
            deck->slides[drag->slide_idx].places[drag->kind] = drag->before;
         }
         drag->state = EDIT_PlaceDragState_None;
         return;
      }

      DECK_Slide* slide = &deck->slides[drag->slide_idx];
      Vec2F32     mouse = Core_WM_MouseFromWindow(app_state->window);

      if (drag->state == EDIT_PlaceDragState_Dragging && Core_WM_KeyPress(events, app_state->window, 0, WM_Key_Esc)) {
         slide->places[drag->kind] = drag->before;
         drag->state = EDIT_PlaceDragState_None;
         return;
      }

      if (!Core_WM_KeyIsDown(WM_Key_LeftMouseButton)) {
         if (drag->state == EDIT_PlaceDragState_Dragging) {
            slide->places[drag->kind] = drag->before;
            _PLACE_WriteLine(editor, slide, drag->kind, &drag->before, drag->now);
            editor->last_input_us = NowTimeUS();
         }
         drag->state = EDIT_PlaceDragState_None;
         return;
      }

      if (drag->state == EDIT_PlaceDragState_Pressed) {
         F32 far = _PLACE_DRAG_START * ui;
         F32 gone_x = mouse.x - drag->press_mouse.x;
         F32 gone_y = mouse.y - drag->press_mouse.y;
         if (gone_x > far || gone_x < -far || gone_y > far || gone_y < -far) {
            drag->state = EDIT_PlaceDragState_Dragging;
         }
      }
      if (drag->state == EDIT_PlaceDragState_Dragging) {
         drag->now = _PLACE_RectFromMouse(drag, app_state->preview_page, mouse);
         slide->places[drag->kind].is_set = 1;
         slide->places[drag->kind].rect = drag->now;
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
         // whatever else is there.
         B32 began = 0;
         for (U64 kind = 0; kind < DECK_PlaceKind_COUNT && !began; kind += 1) {
            if (!app_state->preview_has_box[kind]) {
               continue;
            }
            for (U32 corner = 1; corner <= 4; corner += 1) {
               if (Contains2F32(_PLACE_HandleRect(app_state->preview_boxes[kind], corner, ui), event->pos)) {
                  _PLACE_Begin(app_state, (DECK_PlaceKind)kind, corner, event->pos, EDIT_PlaceDragState_Dragging);
                  Core_WM_EatEvent(events, event);
                  began = 1;
                  break;
               }
            }
         }
         if (began) {
            break;
         }

         // Inside a box: watched, and left for the click it may be. Not on
         // a video's strip of controls: a press there is the strip's.
         DECK_PlaceKind kind = _PLACE_KindAt(app_state, event->pos);
         Rng2F32        video_rect = {0};
         VID_Video*     video = _EDIT_PreviewVideo(app_state, app_state->preview_page, &video_rect);
         B32            on_strip = (video != 0 && FR_VideoStripContains(app_state, video_rect, ui, event->pos));
         if (kind != DECK_PlaceKind_COUNT && !on_strip) {
            _PLACE_Begin(app_state, kind, 0, event->pos, EDIT_PlaceDragState_Pressed);
            break;
         }
      }

      if (event->key == WM_Key_RightMouseButton) {
         DECK_PlaceKind kind = _PLACE_KindAt(app_state, event->pos);
         if (kind != DECK_PlaceKind_COUNT) {
            DECK_Place* place = &deck->slides[app_state->slide_idx].places[kind];
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

   B32            is_dragging = (drag->state == EDIT_PlaceDragState_Dragging);
   DECK_PlaceKind over = is_dragging ? drag->kind : _PLACE_KindAt(app_state, mouse);
   // A corner's handle reaches past its box.
   for (U64 kind = 0; kind < DECK_PlaceKind_COUNT && !is_dragging && over == DECK_PlaceKind_COUNT; kind += 1) {
      for (U32 corner = 1; corner <= 4 && app_state->preview_has_box[kind]; corner += 1) {
         if (Contains2F32(_PLACE_HandleRect(app_state->preview_boxes[kind], corner, ui), mouse)) {
            over = (DECK_PlaceKind)kind;
            break;
         }
      }
   }

   if (over != DECK_PlaceKind_COUNT && app_state->preview_has_box[over]) {
      Rng2F32 box = app_state->preview_boxes[over];
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

   return result;
}

////////////////////////////////
//~ bgz: view
//
// See view.h. Sizes are in page units until multiplied by the scale.

//- bgz: the page's measurements
#define _VIEW_MARGIN_X      90.0f
#define _VIEW_MARGIN_TOP    64.0f
#define _VIEW_MARGIN_BOTTOM 56.0f

#define _VIEW_TITLE_PX       54.0f
#define _VIEW_BIG_TITLE_PX   76.0f // a slide that is only a title
#define _VIEW_BODY_PX        31.0f
#define _VIEW_HEADING_PX     36.0f
#define _VIEW_BULLET_INDENT  38.0f // per level, and from the bullet to its text
#define _VIEW_PICTURE_GUTTER 48.0f // between the body and a picture beside it

// The smallest the body is shrunk to, as a part of its full size, to get a
// long slide onto the page. Past that it runs off the bottom.
#define _VIEW_MIN_BODY_SHRINK 0.5f

//- bgz: colors
#define _VIEW_COLOR_TITLE(a)   V4F32(1.00f, 1.00f, 1.00f, (a))
#define _VIEW_COLOR_BODY(a)    V4F32(0.88f, 0.89f, 0.92f, (a))
#define _VIEW_COLOR_ACCENT(a)  V4F32(0.36f, 0.66f, 1.00f, (a))

internal F32
VIEW_ScaleFromDim(Vec2F32 dim) {
   F32 result = Min(dim.x / VIEW_PAGE_W, dim.y / VIEW_PAGE_H);
   return result;
}

internal Rng2F32
VIEW_PageRectFromRect(Rng2F32 rect) {
   Vec2F32 dim = Dim2F32(rect);
   F32     scale = VIEW_ScaleFromDim(dim);
   F32     x = rect.min.x + (dim.x - VIEW_PAGE_W * scale) * 0.5f;
   F32     y = rect.min.y + (dim.y - VIEW_PAGE_H * scale) * 0.5f;
   return R2F32P(x, y, x + VIEW_PAGE_W * scale, y + VIEW_PAGE_H * scale);
}

// Where a picture that could not be loaded would have been: a box, with the
// file's name, so a wrong path can be seen and read.
internal void
_VIEW_DrawMissingPicture(TXT_Fonts* fonts, DECK_Slide* slide, Rng2F32 rect, F32 scale, F32 alpha) {
   Vec2F32 dim = Dim2F32(rect);
   if (dim.x > 0 && dim.y > 0) {
      Core_D_Rect(rect, R2F32P(0, 0, 1, 1), V4F32(1, 1, 1, 0.05f * alpha));

      String8 lines[] = {Str8Lit("Picture not found"), Str8SkipLastSlash(slide->image_path)};
      F32     px = 22.0f * scale;
      F32     line_height = TXT_LineHeight(fonts->regular, px);
      F32     y = rect.min.y + (dim.y - line_height * (F32)ArrayCount(lines)) * 0.5f;
      for (U64 i = 0; i < ArrayCount(lines); i += 1) {
         F32 w = TXT_Width(fonts->regular, px, lines[i]);
         TXT_Draw(fonts->regular, px, V2F32(rect.min.x + (dim.x - w) * 0.5f, y), V4F32(1, 1, 1, 0.5f * alpha), lines[i]);
         y += line_height;
      }
   }
}

internal void
VIEW_DrawImageFit(IMG_Image* image, Rng2F32 rect, F32 alpha) {
   Vec2F32 rect_dim = Dim2F32(rect);
   F32     image_w = (F32)image->size.x;
   F32     image_h = (F32)image->size.y;

   if (rect_dim.x > 0 && rect_dim.y > 0 && image_w > 0 && image_h > 0) {
      F32 scale = Min(rect_dim.x / image_w, rect_dim.y / image_h);
      F32 w = image_w * scale;
      F32 h = image_h * scale;
      F32 x = rect.min.x + (rect_dim.x - w) * 0.5f;
      F32 y = rect.min.y + (rect_dim.y - h) * 0.5f;

      // The source rect is in the image's own pixels.
      Core_D_Img(R2F32P(x, y, x + w, y + h), R2F32P(0, 0, image_w, image_h), image->texture, V4F32(1, 1, 1, alpha));
   }
}

// Lines of wrapped text from top down, each starting at x. Returns the y
// under the last. With do_draw 0 nothing is drawn: it only measures.
internal F32
_VIEW_Lines(Arena* arena, FNT_Tag font, F32 px, String8 text, F32 x, F32 y, F32 max_width, Vec4F32 color, B32 do_draw) {
   F32         line_height = TXT_LineHeight(font, px);
   String8List lines = TXT_Wrap(arena, font, px, text, max_width);

   for (String8Node* node = lines.first; node != 0; node = node->next) {
      if (do_draw) {
         TXT_Draw(font, px, V2F32(x, y), color, node->string);
      }
      y += line_height;
   }

   return y;
}

// The slide's items, from rect's top left down, no wider than rect. shrink is
// the part of full size to draw them at. Returns the y under the last.
internal F32
_VIEW_Body(Arena* arena, TXT_Fonts* fonts, DECK_Slide* slide, Rng2F32 rect, F32 scale, F32 shrink, F32 alpha, B32 do_draw) {
   F32 unit = scale * shrink;
   F32 y = rect.min.y;

   F32 body_px = _VIEW_BODY_PX * unit;
   F32 body_line_height = TXT_LineHeight(fonts->regular, body_px);

   for (DECK_Item* item = slide->first_item; item != 0; item = item->next) {
      // Room between items: a little always, more where the file had an empty
      // line - and above a heading, which belongs to what is under it.
      if (item != slide->first_item) {
         y += body_line_height * 0.30f;
         if (item->gap_before || item->kind == DECK_ItemKind_Heading) {
            y += body_line_height * 0.45f;
         }
      }

      switch (item->kind) {
      case DECK_ItemKind_Heading: {
         y = _VIEW_Lines(arena, fonts->bold, _VIEW_HEADING_PX * unit, item->text, rect.min.x, y, Dim2F32(rect).x,
                         _VIEW_COLOR_ACCENT(alpha), do_draw);
      } break;

      case DECK_ItemKind_Bullet: {
         // Inner levels are a little smaller.
         F32 px = body_px * (item->level == 0 ? 1.0f : 0.88f);
         F32 indent = _VIEW_BULLET_INDENT * unit;
         F32 text_x = rect.min.x + indent * (F32)(item->level + 1);

         if (do_draw) {
            // A round dot, level with the middle of the first line.
            F32 dot = px * 0.26f;
            F32 dot_x = text_x - indent * 0.62f;
            F32 dot_y = y + TXT_LineHeight(fonts->regular, px) * 0.54f - dot * 0.5f;

            R_Rect2DInst* inst = Core_D_Rect(R2F32P(dot_x, dot_y, dot_x + dot, dot_y + dot), R2F32P(0, 0, 1, 1),
                                             _VIEW_COLOR_ACCENT(alpha));
            for (U64 i = 0; i < ArrayCount(inst->corner_radii); i += 1) {
               inst->corner_radii[i] = dot * 0.5f;
            }
            inst->edge_softness = 1.0f;
         }

         y = _VIEW_Lines(arena, fonts->regular, px, item->text, text_x, y, rect.max.x - text_x, _VIEW_COLOR_BODY(alpha),
                         do_draw);
      } break;

      default: {
         y = _VIEW_Lines(arena, fonts->regular, body_px, item->text, rect.min.x, y, Dim2F32(rect).x,
                         _VIEW_COLOR_BODY(alpha), do_draw);
      } break;
      }
   }

   return y;
}

internal void
VIEW_DrawSlide(Arena* arena, TXT_Fonts* fonts, DECK_Slide* slide, IMG_Image* image, Rng2F32 rect, F32 alpha) {
   Vec2F32 rect_dim = Dim2F32(rect);
   if (rect_dim.x <= 0 || rect_dim.y <= 0) {
      return;
   }

   Core_D_PushClip(rect);

   F32     scale = VIEW_ScaleFromDim(rect_dim);
   Rng2F32 page = VIEW_PageRectFromRect(rect);
   B32     picture_is_missing = (image == 0 && slide->load_failed);

   //- bgz: only a picture: all of the rect
   if (!DECK_SlideHasText(slide)) {
      if (image != 0) {
         VIEW_DrawImageFit(image, rect, alpha);
      } else if (picture_is_missing) {
         _VIEW_DrawMissingPicture(fonts, slide, page, scale, alpha);
      }
      Core_D_PopClip();
      return;
   }

   //- bgz: the page
   Rng2F32 content = R2F32P(page.min.x + _VIEW_MARGIN_X * scale,
                            page.min.y + _VIEW_MARGIN_TOP * scale,
                            page.max.x - _VIEW_MARGIN_X * scale,
                            page.max.y - _VIEW_MARGIN_BOTTOM * scale);
   F32     content_w = Dim2F32(content).x;

   //- bgz: only a title: large, in the middle
   if (slide->item_count == 0 && image == 0 && slide->image_path.size == 0) {
      F32         px = _VIEW_BIG_TITLE_PX * scale;
      F32         line_height = TXT_LineHeight(fonts->bold, px);
      String8List lines = TXT_Wrap(arena, fonts->bold, px, slide->title, content_w);

      F32 y = (content.min.y + content.max.y) * 0.5f - line_height * (F32)lines.node_count * 0.5f;
      for (String8Node* node = lines.first; node != 0; node = node->next) {
         F32 line_w = TXT_Width(fonts->bold, px, node->string);
         TXT_Draw(fonts->bold, px, V2F32(content.min.x + (content_w - line_w) * 0.5f, y), _VIEW_COLOR_TITLE(alpha),
                  node->string);
         y += line_height;
      }

      // A short bar under it.
      F32 bar_w = 96.0f * scale;
      F32 bar_h = 5.0f * scale;
      F32 bar_x = content.min.x + (content_w - bar_w) * 0.5f;
      y += 14.0f * scale;
      Core_D_Rect(R2F32P(bar_x, y, bar_x + bar_w, y + bar_h), R2F32P(0, 0, 1, 1), _VIEW_COLOR_ACCENT(alpha));
      Core_D_PopClip();
      return;
   }

   //- bgz: the title, at the top
   F32 y = content.min.y;
   if (slide->title.size != 0) {
      y = _VIEW_Lines(arena, fonts->bold, _VIEW_TITLE_PX * scale, slide->title, content.min.x, y, content_w,
                      _VIEW_COLOR_TITLE(alpha), 1);

      F32 bar_w = 72.0f * scale;
      F32 bar_h = 4.0f * scale;
      y += 8.0f * scale;
      Core_D_Rect(R2F32P(content.min.x, y, content.min.x + bar_w, y + bar_h), R2F32P(0, 0, 1, 1),
                  _VIEW_COLOR_ACCENT(alpha));
      y += bar_h + 30.0f * scale;
   }

   //- bgz: under it, the body and the picture
   //
   // The body has the full width, or the left half when there is a picture,
   // which then has the right. With no body the picture has it all. The
   // picture's room is kept even when it is not loaded (image == 0), so the
   // body does not move when it arrives.
   Rng2F32 body_rect = R2F32P(content.min.x, y, content.max.x, content.max.y);
   Rng2F32 picture_rect = body_rect;
   B32     has_picture = (slide->image_path.size != 0);
   if (has_picture && slide->item_count != 0) {
      F32 middle = (content.min.x + content.max.x) * 0.5f;
      F32 half_gutter = _VIEW_PICTURE_GUTTER * scale * 0.5f;
      body_rect.max.x = middle - half_gutter;
      picture_rect.min.x = middle + half_gutter;
   }

   if (image != 0) {
      VIEW_DrawImageFit(image, picture_rect, alpha);
   } else if (picture_is_missing) {
      _VIEW_DrawMissingPicture(fonts, slide, picture_rect, scale, alpha);
   }

   if (slide->item_count != 0) {
      // Full size if it fits above the bottom margin, else the largest of a
      // few smaller sizes that does - or the smallest, if none do.
      F32 shrink = 1.0f;
      for (;;) {
         F32 end_y = _VIEW_Body(arena, fonts, slide, body_rect, scale, shrink, alpha, 0);
         if (end_y <= body_rect.max.y || shrink <= _VIEW_MIN_BODY_SHRINK) {
            break;
         }
         shrink = Max(shrink * 0.88f, _VIEW_MIN_BODY_SHRINK);
      }
      _VIEW_Body(arena, fonts, slide, body_rect, scale, shrink, alpha, 1);
   }

   Core_D_PopClip();
}

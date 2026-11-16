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

// How much of the theme's background is laid over a picture that is behind
// the words, so they can be read on it.
#define _VIEW_BEHIND_SCRIM 0.62f

//- bgz: colors
internal VIEW_Colors
VIEW_ColorsFromTheme(DECK_Theme theme) {
   VIEW_Colors result = {0};
   switch (theme) {
      default:
      case DECK_Theme_Dark: {
         result.background = V4F32(0.05f, 0.05f, 0.06f, 1.0f);
         result.title = V4F32(1.00f, 1.00f, 1.00f, 1.0f);
         result.body = V4F32(0.88f, 0.89f, 0.92f, 1.0f);
         result.accent = V4F32(0.36f, 0.66f, 1.00f, 1.0f);
      } break;
      case DECK_Theme_Light: {
         result.background = V4F32(0.97f, 0.97f, 0.96f, 1.0f);
         result.title = V4F32(0.09f, 0.10f, 0.13f, 1.0f);
         result.body = V4F32(0.22f, 0.24f, 0.28f, 1.0f);
         result.accent = V4F32(0.10f, 0.45f, 0.90f, 1.0f);
      } break;
      case DECK_Theme_Blue: {
         result.background = V4F32(0.05f, 0.10f, 0.21f, 1.0f);
         result.title = V4F32(1.00f, 1.00f, 1.00f, 1.0f);
         result.body = V4F32(0.84f, 0.89f, 0.97f, 1.0f);
         result.accent = V4F32(1.00f, 0.78f, 0.30f, 1.0f);
      } break;
      case DECK_Theme_Paper: {
         result.background = V4F32(0.96f, 0.93f, 0.86f, 1.0f);
         result.title = V4F32(0.20f, 0.14f, 0.09f, 1.0f);
         result.body = V4F32(0.31f, 0.25f, 0.19f, 1.0f);
         result.accent = V4F32(0.74f, 0.30f, 0.14f, 1.0f);
      } break;
   }
   return result;
}

// color, showing through by alpha as well as by its own.
internal Vec4F32
_VIEW_Faded(Vec4F32 color, F32 alpha) {
   color.w *= alpha;
   return color;
}

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
_VIEW_DrawMissingPicture(TXT_Fonts* fonts, VIEW_Colors* colors, DECK_Slide* slide, Rng2F32 rect, F32 scale, F32 alpha) {
   Vec2F32 dim = Dim2F32(rect);
   if (dim.x > 0 && dim.y > 0) {
      Core_D_Rect(rect, R2F32P(0, 0, 1, 1), _VIEW_Faded(colors->body, 0.07f * alpha));

      String8 lines[] = {Str8Lit("Picture not found"), Str8SkipLastSlash(slide->image_path)};
      F32     px = 22.0f * scale;
      F32     line_height = TXT_LineHeight(fonts->regular, px);
      F32     y = rect.min.y + (dim.y - line_height * (F32)ArrayCount(lines)) * 0.5f;
      for (U64 i = 0; i < ArrayCount(lines); i += 1) {
         F32 w = TXT_Width(fonts->regular, px, lines[i]);
         TXT_Draw(fonts->regular, px, V2F32(rect.min.x + (dim.x - w) * 0.5f, y), _VIEW_Faded(colors->body, 0.6f * alpha), lines[i]);
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

// The image filling rect with its shape kept: what does not fit, at the
// sides or at the top and bottom, is left out.
internal void
_VIEW_DrawImageFill(IMG_Image* image, Rng2F32 rect, F32 alpha) {
   Vec2F32 rect_dim = Dim2F32(rect);
   F32     image_w = (F32)image->size.x;
   F32     image_h = (F32)image->size.y;

   if (rect_dim.x > 0 && rect_dim.y > 0 && image_w > 0 && image_h > 0) {
      // The part of the image, in its own pixels, that has rect's shape.
      F32 scale = Max(rect_dim.x / image_w, rect_dim.y / image_h);
      F32 src_w = rect_dim.x / scale;
      F32 src_h = rect_dim.y / scale;
      F32 src_x = (image_w - src_w) * 0.5f;
      F32 src_y = (image_h - src_h) * 0.5f;
      Core_D_Img(rect, R2F32P(src_x, src_y, src_x + src_w, src_y + src_h), image->texture, V4F32(1, 1, 1, alpha));
   }
}

internal void
_VIEW_PushHit(Arena* arena, VIEW_HitList* hits, Rng2F32 rect, Rng1U64 text_range) {
   if (hits != 0 && rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
      VIEW_Hit* hit = PushArray(arena, VIEW_Hit, 1);
      hit->rect = rect;
      hit->text_range = text_range;
      SLLQueuePush(hits->first, hits->last, hit);
      hits->count += 1;
   }
}

// Lines of wrapped text from top down, each starting at x. Returns the y
// under the last. With do_draw 0 nothing is drawn: it only measures.
// centered puts each line in the middle of max_width. widest_out, if not 0,
// gets the width of the widest line drawn.
internal F32
_VIEW_Lines(Arena* arena, FNT_Tag font, F32 px, String8 text, F32 x, F32 y, F32 max_width, Vec4F32 color, B32 do_draw,
            B32 centered, F32* widest_out) {
   F32         line_height = TXT_LineHeight(font, px);
   String8List lines = TXT_Wrap(arena, font, px, text, max_width);
   F32         widest = 0;

   for (String8Node* node = lines.first; node != 0; node = node->next) {
      if (do_draw) {
         F32 line_w = (centered || widest_out != 0) ? TXT_Width(font, px, node->string) : 0;
         // Centered: each line in the middle of the width it could have had.
         F32 line_x = centered ? x + (max_width - line_w) * 0.5f : x;
         TXT_Draw(font, px, V2F32(line_x, y), color, node->string);
         widest = Max(widest, line_w);
      }
      y += line_height;
   }

   if (widest_out != 0) {
      *widest_out = widest;
   }
   return y;
}

////////////////////////////////
//~ bgz: words in bold and in code
//
// In a paragraph or a bullet:
//
//    **these words**   are drawn bold
//    `these`           in the font with every letter the same width, on a
//                      faint box, as code is written. Inside, ** is only **
//
// The marks themselves are not drawn. One with no partner later in the line
// is not a mark: it is drawn as written.

enum {
   _VIEW_Style_Bold = (1 << 0),
   _VIEW_Style_Code = (1 << 1),
};

// A run of letters drawn alike, with no space in it. A word is one, or
// several end to end where the look changes inside it.
typedef struct _VIEW_Piece _VIEW_Piece;
struct _VIEW_Piece {
   _VIEW_Piece* next;
   String8      text;          // a part of the item's own text
   U32          style;
   U32          spaces_before; // 0: it goes on from the piece before, and is never parted from it
   F32          width;
};

typedef struct _VIEW_PieceList _VIEW_PieceList;
struct _VIEW_PieceList {
   _VIEW_Piece* first;
   _VIEW_Piece* last;
};

internal B32
_VIEW_HasLater(String8 text, U64 from, String8 mark) {
   B32 result = 0;
   for (U64 i = from; i + mark.size <= text.size; i += 1) {
      if (MemoryMatch(text.str + i, mark.str, mark.size)) {
         result = 1;
         break;
      }
   }
   return result;
}

internal FNT_Tag
_VIEW_FontFromStyle(TXT_Fonts* fonts, U32 style) {
   FNT_Tag result = fonts->regular;
   if (style & _VIEW_Style_Code) {
      result = fonts->mono;
   } else if (style & _VIEW_Style_Bold) {
      result = fonts->bold;
   }
   return result;
}

// Code is a little smaller: the letters of that font are wide.
internal F32
_VIEW_PxFromStyle(U32 style, F32 px) {
   return (style & _VIEW_Style_Code) ? px * 0.9f : px;
}

internal _VIEW_PieceList
_VIEW_PiecesFromText(Arena* arena, TXT_Fonts* fonts, F32 px, String8 text) {
   _VIEW_PieceList result = {0};

   U32 style = 0;
   U32 spaces = 0;
   U64 piece_start = 0;
   B32 in_piece = 0;
   B32 is_first = 1;

   // One past the end is gone through too, as a space: it ends the last
   // piece.
   for (U64 i = 0; i <= text.size;) {
      U8  c = (i < text.size) ? text.str[i] : ' ';
      U32 new_style = style;
      U64 skip = 0; // the length of a mark here. 0: a letter or a space

      if (i < text.size) {
         if (c == '`') {
            if (style & _VIEW_Style_Code) {
               new_style &= ~_VIEW_Style_Code;
               skip = 1;
            } else if (_VIEW_HasLater(text, i + 1, Str8Lit("`"))) {
               new_style |= _VIEW_Style_Code;
               skip = 1;
            }
         } else if (c == '*' && !(style & _VIEW_Style_Code) && i + 1 < text.size && text.str[i + 1] == '*') {
            if (style & _VIEW_Style_Bold) {
               new_style &= ~_VIEW_Style_Bold;
               skip = 2;
            } else if (_VIEW_HasLater(text, i + 2, Str8Lit("**"))) {
               new_style |= _VIEW_Style_Bold;
               skip = 2;
            }
         }
      }

      B32 is_space = (skip == 0 && c == ' ');
      if (in_piece && (is_space || skip != 0)) {
         _VIEW_Piece* piece = PushArray(arena, _VIEW_Piece, 1);
         piece->text = Str8Substr(text, R1U64(piece_start, i));
         piece->style = style;
         piece->spaces_before = is_first ? 0 : spaces;
         piece->width = TXT_Width(_VIEW_FontFromStyle(fonts, style), _VIEW_PxFromStyle(style, px), piece->text);
         SLLQueuePush(result.first, result.last, piece);
         in_piece = 0;
         is_first = 0;
         spaces = 0;
      }

      if (skip != 0) {
         style = new_style;
         i += skip;
      } else {
         if (is_space) {
            spaces += 1;
         } else if (!in_piece) {
            in_piece = 1;
            piece_start = i;
         }
         i += 1;
      }
   }

   return result;
}

// As _VIEW_Lines, for text that may have words in bold and in code.
internal F32
_VIEW_RichLines(Arena* arena, TXT_Fonts* fonts, VIEW_Colors* colors, F32 px, String8 text, F32 x, F32 y, F32 max_width,
                Vec4F32 color, F32 alpha, B32 do_draw, B32 centered, F32* widest_out) {
   F32             line_height = TXT_LineHeight(fonts->regular, px);
   F32             space_width = TXT_Width(fonts->regular, px, Str8Lit(" "));
   _VIEW_PieceList pieces = _VIEW_PiecesFromText(arena, fonts, px, text);
   F32             widest = 0;

   // A line at a time: first how many pieces go on it, then they are drawn.
   for (_VIEW_Piece* line_first = pieces.first; line_first != 0;) {
      _VIEW_Piece* line_end = line_first; // the first piece not on the line
      F32          line_w = 0;
      while (line_end != 0) {
         // A word: this piece and the ones that go on from it.
         F32          word_w = line_end->width;
         _VIEW_Piece* word_end = line_end->next;
         while (word_end != 0 && word_end->spaces_before == 0) {
            word_w += word_end->width;
            word_end = word_end->next;
         }
         F32 gap_w = (line_end == line_first) ? 0 : space_width * (F32)line_end->spaces_before;
         // The first word of a line always goes on it.
         if (line_end != line_first && line_w + gap_w + word_w > max_width) {
            break;
         }
         line_w += gap_w + word_w;
         line_end = word_end;
      }

      if (do_draw) {
         F32 piece_x = centered ? x + (max_width - line_w) * 0.5f : x;
         for (_VIEW_Piece* piece = line_first; piece != line_end; piece = piece->next) {
            if (piece != line_first) {
               F32 gap_w = space_width * (F32)piece->spaces_before;
               // The space between two words of code is code too: one box
               // under them all.
               if (gap_w != 0 && (piece->style & _VIEW_Style_Code)) {
                  B32 prev_is_code = 0;
                  for (_VIEW_Piece* prev = line_first; prev != piece; prev = prev->next) {
                     prev_is_code = !!(prev->style & _VIEW_Style_Code);
                  }
                  if (prev_is_code) {
                     Core_D_Rect(R2F32P(piece_x, y + line_height * 0.08f, piece_x + gap_w, y + line_height * 0.94f),
                                 R2F32P(0, 0, 1, 1), _VIEW_Faded(colors->body, alpha * 0.13f));
                  }
               }
               piece_x += gap_w;
            }

            FNT_Tag font = _VIEW_FontFromStyle(fonts, piece->style);
            F32     piece_px = _VIEW_PxFromStyle(piece->style, px);
            F32     piece_y = y;
            if (piece->style & _VIEW_Style_Code) {
               Core_D_Rect(R2F32P(piece_x, y + line_height * 0.08f, piece_x + piece->width, y + line_height * 0.94f),
                           R2F32P(0, 0, 1, 1), _VIEW_Faded(colors->body, alpha * 0.13f));
               // Smaller, and a different font: put where its letters stand
               // on the line the others do, near enough.
               piece_y = y + (line_height - TXT_LineHeight(font, piece_px)) * 0.5f;
            }
            TXT_Draw(font, piece_px, V2F32(piece_x, piece_y), color, piece->text);
            piece_x += piece->width;
         }
         widest = Max(widest, line_w);
      }

      y += line_height;
      line_first = line_end;
   }

   if (widest_out != 0) {
      *widest_out = widest;
   }
   return y;
}

// The slide's items, from rect's top left down, no wider than rect. shrink is
// the part of full size to draw them at. Returns the y under the last.
internal F32
_VIEW_Body(Arena* arena, TXT_Fonts* fonts, VIEW_Colors* colors, DECK_Slide* slide, Rng2F32 rect, F32 scale, F32 shrink,
           F32 alpha, B32 do_draw, VIEW_HitList* hits) {
   B32 centered = (slide->layout == DECK_Layout_Center);
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

      // Where the item is drawn, for its hit: from its left - a bullet's dot
      // - to the end of its widest line.
      F32  item_x = rect.min.x;
      F32  item_y = y;
      F32  text_x = rect.min.x;
      F32  widest = 0;
      F32* widest_out = (do_draw && hits != 0) ? &widest : 0;

      switch (item->kind) {
      case DECK_ItemKind_Heading: {
         y = _VIEW_Lines(arena, fonts->bold, _VIEW_HEADING_PX * unit, item->text, rect.min.x, y, Dim2F32(rect).x,
                         _VIEW_Faded(colors->accent, alpha), do_draw, centered, widest_out);
      } break;

      case DECK_ItemKind_Bullet: {
         // Inner levels are a little smaller.
         F32 px = body_px * (item->level == 0 ? 1.0f : 0.88f);
         F32 indent = _VIEW_BULLET_INDENT * unit;
         text_x = rect.min.x + indent * (F32)(item->level + 1);
         item_x = text_x - indent * 0.62f;
         if (centered) {
            // In the middle, a bullet is a line like any other: no dot, and
            // nothing in from the left.
            text_x = rect.min.x;
            item_x = rect.min.x;
         }

         if (do_draw && !centered) {
            // A round dot, level with the middle of the first line. Not under
            // 4 pixels across, or in the small preview it all but vanishes.
            F32 dot = Max(px * 0.28f, 4.0f);
            F32 dot_x = (F32)(S32)(text_x - indent * 0.62f);
            F32 dot_y = (F32)(S32)(y + TXT_LineHeight(fonts->regular, px) * 0.54f - dot * 0.5f);

            R_Rect2DInst* inst = Core_D_Rect(R2F32P(dot_x, dot_y, dot_x + dot, dot_y + dot), R2F32P(0, 0, 1, 1),
                                             _VIEW_Faded(colors->accent, alpha));
            for (U64 i = 0; i < ArrayCount(inst->corner_radii); i += 1) {
               inst->corner_radii[i] = dot * 0.5f;
            }
            // The soft edge is taken out of the dot, so a small dot gets a
            // thin one or there is nothing left of it.
            inst->edge_softness = Min(1.0f, dot * 0.1f);
         }

         // A bullet with no text yet still takes up its line.
         if (item->text.size == 0) {
            y += TXT_LineHeight(fonts->regular, px);
         }
         y = _VIEW_RichLines(arena, fonts, colors, px, item->text, text_x, y, rect.max.x - text_x,
                             _VIEW_Faded(colors->body, alpha), alpha, do_draw, centered, widest_out);
      } break;

      default: {
         y = _VIEW_RichLines(arena, fonts, colors, body_px, item->text, rect.min.x, y, Dim2F32(rect).x,
                             _VIEW_Faded(colors->body, alpha), alpha, do_draw, centered, widest_out);
      } break;
      }

      if (do_draw) {
         // A bullet with no words yet is still something to click: as wide
         // as a few letters.
         F32 hit_w = Max(widest, body_px * 3.0f);
         if (centered) {
            F32 middle = (rect.min.x + rect.max.x) * 0.5f;
            _VIEW_PushHit(arena, hits, R2F32P(middle - hit_w * 0.5f, item_y, middle + hit_w * 0.5f, y), item->text_range);
         } else {
            _VIEW_PushHit(arena, hits, R2F32P(item_x, item_y, text_x + hit_w, y), item->text_range);
         }
      }
   }

   return y;
}

internal void
VIEW_DrawSlide(Arena* arena, TXT_Fonts* fonts, DECK_Theme theme, DECK_Slide* slide, IMG_Image* image, Rng2F32 rect,
               F32 alpha, VIEW_HitList* hits) {
   Vec2F32 rect_dim = Dim2F32(rect);
   if (rect_dim.x <= 0 || rect_dim.y <= 0) {
      return;
   }

   Core_D_PushClip(rect);

   F32     scale = VIEW_ScaleFromDim(rect_dim);
   Rng2F32 page = VIEW_PageRectFromRect(rect);
   B32     picture_is_missing = (image == 0 && slide->load_failed);

   VIEW_Colors colors = VIEW_ColorsFromTheme(theme);
   B32         has_picture = (slide->image_path.size != 0);
   B32         centered = (slide->layout == DECK_Layout_Center);
   B32         picture_is_behind = (has_picture && slide->layout == DECK_Layout_PictureBehind && DECK_SlideHasText(slide));

   //- bgz: only a picture: all of the rect
   if (!DECK_SlideHasText(slide)) {
      if (image != 0) {
         VIEW_DrawImageFit(image, rect, alpha);
      } else if (picture_is_missing) {
         _VIEW_DrawMissingPicture(fonts, &colors, slide, page, scale, alpha);
      }
      if (slide->image_path.size != 0) {
         _VIEW_PushHit(arena, hits, rect, slide->image_range);
      }
      Core_D_PopClip();
      return;
   }

   //- bgz: a picture behind the words: all of the page, under a wash of the
   // background's colour, and no hit of its own - it is under everything
   if (picture_is_behind) {
      if (image != 0) {
         _VIEW_DrawImageFill(image, page, alpha);
         Core_D_Rect(page, R2F32P(0, 0, 1, 1), _VIEW_Faded(colors.background, _VIEW_BEHIND_SCRIM * alpha));
      } else if (picture_is_missing) {
         _VIEW_DrawMissingPicture(fonts, &colors, slide, page, scale, alpha);
      }
   }

   //- bgz: the page
   Rng2F32 content = R2F32P(page.min.x + _VIEW_MARGIN_X * scale,
                            page.min.y + _VIEW_MARGIN_TOP * scale,
                            page.max.x - _VIEW_MARGIN_X * scale,
                            page.max.y - _VIEW_MARGIN_BOTTOM * scale);
   F32     content_w = Dim2F32(content).x;

   //- bgz: only a title: large, in the middle
   if (slide->item_count == 0 && (!has_picture || picture_is_behind)) {
      F32         px = _VIEW_BIG_TITLE_PX * scale;
      F32         line_height = TXT_LineHeight(fonts->bold, px);
      String8List lines = TXT_Wrap(arena, fonts->bold, px, slide->title, content_w);

      F32 y = (content.min.y + content.max.y) * 0.5f - line_height * (F32)lines.node_count * 0.5f;
      F32 title_y = y;
      F32 widest = px * 2.0f; // a title with no words yet is still something to click
      for (String8Node* node = lines.first; node != 0; node = node->next) {
         F32 line_w = TXT_Width(fonts->bold, px, node->string);
         widest = Max(widest, line_w);
         TXT_Draw(fonts->bold, px, V2F32(content.min.x + (content_w - line_w) * 0.5f, y),
                  _VIEW_Faded(colors.title, alpha), node->string);
         y += line_height;
      }
      _VIEW_PushHit(arena, hits,
                    R2F32P(content.min.x + (content_w - widest) * 0.5f, title_y, content.min.x + (content_w + widest) * 0.5f,
                           Max(y, title_y + line_height)),
                    slide->title_range);

      // A short bar under it.
      F32 bar_w = 96.0f * scale;
      F32 bar_h = 5.0f * scale;
      F32 bar_x = content.min.x + (content_w - bar_w) * 0.5f;
      y += 14.0f * scale;
      Core_D_Rect(R2F32P(bar_x, y, bar_x + bar_w, y + bar_h), R2F32P(0, 0, 1, 1), _VIEW_Faded(colors.accent, alpha));
      Core_D_PopClip();
      return;
   }

   //- bgz: the title, at the top
   F32 y = content.min.y;
   if (slide->title.size != 0) {
      F32 title_y = y;
      F32 widest = 0;
      y = _VIEW_Lines(arena, fonts->bold, _VIEW_TITLE_PX * scale, slide->title, content.min.x, y, content_w,
                      _VIEW_Faded(colors.title, alpha), 1, centered, &widest);

      // The bar under it is under its start - or its middle, when centered.
      F32 title_x = centered ? content.min.x + (content_w - widest) * 0.5f : content.min.x;
      _VIEW_PushHit(arena, hits, R2F32P(title_x, title_y, title_x + widest, y), slide->title_range);

      F32 bar_w = 72.0f * scale;
      F32 bar_h = 4.0f * scale;
      F32 bar_x = centered ? content.min.x + (content_w - bar_w) * 0.5f : content.min.x;
      y += 8.0f * scale;
      Core_D_Rect(R2F32P(bar_x, y, bar_x + bar_w, y + bar_h), R2F32P(0, 0, 1, 1), _VIEW_Faded(colors.accent, alpha));
      y += bar_h + 30.0f * scale;
   }

   //- bgz: under it, the body and the picture
   //
   // The body has the full width, or half of it when there is a picture
   // beside it, which then has the other half: which half is the slide's
   // layout. With no body the picture has it all. The
   // picture's room is kept even when it is not loaded (image == 0), so the
   // body does not move when it arrives.
   Rng2F32 body_rect = R2F32P(content.min.x, y, content.max.x, content.max.y);
   Rng2F32 picture_rect = body_rect;
   B32     picture_is_beside = (has_picture && !picture_is_behind);
   if (picture_is_beside && slide->item_count != 0) {
      F32 middle = (content.min.x + content.max.x) * 0.5f;
      F32 half_gutter = _VIEW_PICTURE_GUTTER * scale * 0.5f;
      if (slide->layout == DECK_Layout_PictureLeft) {
         picture_rect.max.x = middle - half_gutter;
         body_rect.min.x = middle + half_gutter;
      } else {
         body_rect.max.x = middle - half_gutter;
         picture_rect.min.x = middle + half_gutter;
      }
   }

   if (picture_is_beside) {
      if (image != 0) {
         // Beside a body, the picture starts level with the body's first
         // line, not in the middle of its half's height: its room is cut
         // down to the picture's own height, from the top.
         if (slide->item_count != 0 && image->size.x > 0 && image->size.y > 0) {
            Vec2F32 room = Dim2F32(picture_rect);
            F32     fit = Min(room.x / (F32)image->size.x, room.y / (F32)image->size.y);
            picture_rect.max.y = picture_rect.min.y + (F32)image->size.y * fit;
         }
         VIEW_DrawImageFit(image, picture_rect, alpha);
      } else if (picture_is_missing) {
         _VIEW_DrawMissingPicture(fonts, &colors, slide, picture_rect, scale, alpha);
      }
      _VIEW_PushHit(arena, hits, picture_rect, slide->image_range);
   }

   if (slide->item_count != 0) {
      // Full size if it fits above the bottom margin, else the largest of a
      // few smaller sizes that does - or the smallest, if none do.
      F32 shrink = 1.0f;
      for (;;) {
         F32 end_y = _VIEW_Body(arena, fonts, &colors, slide, body_rect, scale, shrink, alpha, 0, 0);
         if (end_y <= body_rect.max.y || shrink <= _VIEW_MIN_BODY_SHRINK) {
            break;
         }
         shrink = Max(shrink * 0.88f, _VIEW_MIN_BODY_SHRINK);
      }
      _VIEW_Body(arena, fonts, &colors, slide, body_rect, scale, shrink, alpha, 1, hits);
   }

   Core_D_PopClip();
}

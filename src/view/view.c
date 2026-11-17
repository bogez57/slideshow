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
_VIEW_DrawMissingPicture(TXT_Fonts* fonts, VIEW_Colors* colors, String8 path, Rng2F32 rect, F32 scale, F32 alpha) {
   Vec2F32 dim = Dim2F32(rect);
   if (dim.x > 0 && dim.y > 0) {
      Core_D_Rect(rect, R2F32P(0, 0, 1, 1), _VIEW_Faded(colors->body, 0.07f * alpha));

      String8 lines[] = {Str8Lit("Picture not found"), Str8SkipLastSlash(path)};
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

// Where VIEW_DrawImageFit draws the image in rect. Nothing of a rect - all
// zero - for an image or a rect with no size.
internal Rng2F32
_VIEW_FitRect(IMG_Image* image, Rng2F32 rect) {
   Rng2F32 result = {0};
   Vec2F32 rect_dim = Dim2F32(rect);
   F32     image_w = (F32)image->size.x;
   F32     image_h = (F32)image->size.y;

   if (rect_dim.x > 0 && rect_dim.y > 0 && image_w > 0 && image_h > 0) {
      F32 scale = Min(rect_dim.x / image_w, rect_dim.y / image_h);
      F32 w = image_w * scale;
      F32 h = image_h * scale;
      F32 x = rect.min.x + (rect_dim.x - w) * 0.5f;
      F32 y = rect.min.y + (rect_dim.y - h) * 0.5f;
      result = R2F32P(x, y, x + w, y + h);
   }
   return result;
}

internal void
VIEW_DrawImageFit(IMG_Image* image, Rng2F32 rect, F32 alpha) {
   Rng2F32 fit = _VIEW_FitRect(image, rect);
   if (fit.max.x > fit.min.x) {
      // The source rect is in the image's own pixels.
      Core_D_Img(fit, R2F32P(0, 0, (F32)image->size.x, (F32)image->size.y), image->texture, V4F32(1, 1, 1, alpha));
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
   U32          code_span;     // code: which `...` it is in, counted from 1. 0: not code
   F32          pad_before;    // code: room inside its box before the letters, at the start of a span
   F32          width;         // the letters', with the room before and after them
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

// The room inside a code box, before and after its letters, as a part of the
// text's size.
#define _VIEW_CodePad 0.18f

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
   U32 code_span = 0;

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
               code_span += 1;
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
         piece->code_span = (style & _VIEW_Style_Code) ? code_span : 0;
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

   // A code span's box reaches a little past its letters at each end, and that
   // room is part of the width, so the words beside it are not touched.
   F32 pad = px * _VIEW_CodePad;
   for (_VIEW_Piece *piece = result.first, *prev = 0; piece != 0; prev = piece, piece = piece->next) {
      if (piece->code_span != 0) {
         if (prev == 0 || prev->code_span != piece->code_span) {
            piece->pad_before = pad;
            piece->width += pad;
         }
         if (piece->next == 0 || piece->next->code_span != piece->code_span) {
            piece->width += pad;
         }
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
         // Every piece stands on the regular font's baseline, whatever its
         // font and size. A code box reaches from a little above a capital
         // to a little under the baseline, around the letters of the line.
         F32     baseline = TXT_Baseline(fonts->regular, px);
         F32     cap_h = TXT_CapHeight(fonts->regular, px);
         F32     box_top = y + baseline - cap_h * 1.32f;
         F32     box_bottom = y + baseline + cap_h * 0.42f;
         Vec4F32 box_color = _VIEW_Faded(colors->body, alpha * 0.13f);

         F32 piece_x = centered ? x + (max_width - line_w) * 0.5f : x;
         for (_VIEW_Piece *piece = line_first, *prev = 0; piece != line_end; prev = piece, piece = piece->next) {
            // The space between two words of one piece of code is code too:
            // the box goes on under it, one box under them all.
            F32 box_x = piece_x;
            if (prev != 0) {
               F32 gap_w = space_width * (F32)piece->spaces_before;
               piece_x += gap_w;
               if (piece->code_span == 0 || prev->code_span != piece->code_span) {
                  box_x = piece_x;
               }
            }

            FNT_Tag font = _VIEW_FontFromStyle(fonts, piece->style);
            F32     piece_px = _VIEW_PxFromStyle(piece->style, px);
            F32     piece_y = y + baseline - TXT_Baseline(font, piece_px);
            if (piece->code_span != 0) {
               Core_D_Rect(R2F32P(box_x, box_top, piece_x + piece->width, box_bottom), R2F32P(0, 0, 1, 1), box_color);
            }
            TXT_Draw(font, piece_px, V2F32(piece_x + piece->pad_before, piece_y), color, piece->text);
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
_VIEW_PushBox(VIEW_HitList* hits, DECK_PlaceKind kind, U64 picture_idx, Rng2F32 rect) {
   if (hits != 0 && hits->box_count < ArrayCount(hits->boxes) && rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
      VIEW_Box* box = &hits->boxes[hits->box_count];
      hits->box_count += 1;
      box->kind = kind;
      box->picture_idx = picture_idx;
      box->rect = rect;
   }
}

// An @place line's rect, from parts of the page to pixels.
internal Rng2F32
_VIEW_PlacedRect(Rng2F32 page, DECK_Place* place) {
   Vec2F32 page_dim = Dim2F32(page);
   return R2F32P(page.min.x + place->rect.min.x * page_dim.x, page.min.y + place->rect.min.y * page_dim.y,
                 page.min.x + place->rect.max.x * page_dim.x, page.min.y + place->rect.max.y * page_dim.y);
}

// The room the idx-th of count pictures has, of the room they share: all
// of it for one, else a cell of a grid - side by side in a room wider than
// it is tall, one above the other in a taller one, and in two rows or two
// columns past three.
internal Rng2F32
_VIEW_PictureCell(Rng2F32 room, U64 idx, U64 count, F32 gap) {
   if (count <= 1) {
      return room;
   }
   Vec2F32 dim = Dim2F32(room);
   B32     is_wide = (dim.x >= dim.y);
   U64     along = (count <= 3) ? count : (count + 1) / 2; // cells in the long direction
   U64     across = (count <= 3) ? 1 : 2;
   U64     cols = is_wide ? along : across;
   U64     rows = is_wide ? across : along;
   F32     cell_w = (dim.x - gap * (F32)(cols - 1)) / (F32)cols;
   F32     cell_h = (dim.y - gap * (F32)(rows - 1)) / (F32)rows;
   U64     col = idx % cols;
   U64     row = idx / cols;
   F32     x = room.min.x + (cell_w + gap) * (F32)col;
   F32     y = room.min.y + (cell_h + gap) * (F32)row;
   return R2F32P(x, y, x + Max(cell_w, 1.0f), y + Max(cell_h, 1.0f));
}

// The slide's title, in rect: from its top, no wider than it. With measure
// 0 it is drawn, with the bar under it, and its hit and its box are given.
// Returns the y under the bar.
internal F32
_VIEW_Title(Arena* arena, TXT_Fonts* fonts, VIEW_Colors* colors, DECK_Slide* slide, Rng2F32 rect, F32 scale, B32 is_big,
            B32 centered, F32 alpha, B32 measure, VIEW_HitList* hits) {
   F32 px = (is_big ? _VIEW_BIG_TITLE_PX : _VIEW_TITLE_PX) * scale;
   F32 width = Dim2F32(rect).x;
   F32 line_height = TXT_LineHeight(fonts->bold, px);
   F32 widest = 0;
   F32 y = _VIEW_Lines(arena, fonts->bold, px, slide->title, rect.min.x, rect.min.y, width, _VIEW_Faded(colors->title, alpha),
                       !measure, centered, &widest);
   y = Max(y, rect.min.y + line_height); // a title with no words yet still has its line

   F32 bar_w = (is_big ? 96.0f : 72.0f) * scale;
   F32 bar_h = (is_big ? 5.0f : 4.0f) * scale;
   F32 bar_gap = (is_big ? 14.0f : 8.0f) * scale;
   if (!measure) {
      // A title with no words yet is still something to click.
      widest = Max(widest, px * 2.0f);
      F32 title_x = centered ? rect.min.x + (width - widest) * 0.5f : rect.min.x;
      _VIEW_PushHit(arena, hits, R2F32P(title_x, rect.min.y, title_x + widest, y), slide->title_range);

      // The box to move it by: placed, as wide as the place. Not placed, as
      // wide as the words and a little over, so that they wrap the same
      // once it is.
      Rng2F32 box = R2F32P(rect.min.x, rect.min.y, rect.max.x, y);
      if (!slide->places[DECK_PlaceKind_Title].is_set) {
         F32 slack = 8.0f * scale;
         box.min.x = Max(title_x - (centered ? slack : 0), rect.min.x);
         box.max.x = Min(title_x + widest + slack, rect.max.x);
      }
      _VIEW_PushBox(hits, DECK_PlaceKind_Title, 0, box);

      // The bar under it is under its start - or its middle, when centered.
      F32 bar_x = centered ? rect.min.x + (width - bar_w) * 0.5f : rect.min.x;
      Core_D_Rect(R2F32P(bar_x, y + bar_gap, bar_x + bar_w, y + bar_gap + bar_h), R2F32P(0, 0, 1, 1),
                  _VIEW_Faded(colors->accent, alpha));
   }
   return y + bar_gap + bar_h;
}

internal void
VIEW_DrawSlide(Arena* arena, TXT_Fonts* fonts, DECK_Theme theme, DECK_Slide* slide, Rng2F32 rect, F32 alpha,
               VIEW_HitList* hits) {
   Vec2F32 rect_dim = Dim2F32(rect);
   if (rect_dim.x <= 0 || rect_dim.y <= 0) {
      return;
   }

   Core_D_PushClip(rect);

   F32     scale = VIEW_ScaleFromDim(rect_dim);
   Rng2F32 page = VIEW_PageRectFromRect(rect);
   if (hits != 0) {
      hits->page = page;
   }

   VIEW_Colors colors = VIEW_ColorsFromTheme(theme);
   B32         has_text = DECK_SlideHasText(slide);
   B32         centered = (slide->layout == DECK_Layout_Center);
   DECK_Place* text_place = &slide->places[DECK_PlaceKind_Text];
   DECK_Place* title_place = &slide->places[DECK_PlaceKind_Title];

   //- bgz: which pictures go where
   //
   // With the picture-behind layout the slide's first picture is behind
   // the words. A picture with an @place line is where that says. The rest
   // share the room the layout has for pictures (below).
   U64 behind_idx = (slide->picture_count != 0 && slide->layout == DECK_Layout_PictureBehind && has_text) ? 0 : max_U64;
   U64 shared_count = 0;
   for (U64 i = 0; i < slide->picture_count; i += 1) {
      if (i != behind_idx && !slide->pictures[i].place.is_set) {
         shared_count += 1;
      }
   }

   //- bgz: the picture behind the words: all of the page, under a wash of
   // the background's colour, and no hit of its own - it is under everything
   if (behind_idx != max_U64) {
      DECK_Picture* picture = &slide->pictures[behind_idx];
      if (IMG_IsLoaded(&picture->image)) {
         _VIEW_DrawImageFill(&picture->image, page, alpha);
         Core_D_Rect(page, R2F32P(0, 0, 1, 1), _VIEW_Faded(colors.background, _VIEW_BEHIND_SCRIM * alpha));
      } else if (picture->load_failed) {
         _VIEW_DrawMissingPicture(fonts, &colors, picture->path, page, scale, alpha);
      }
   }

   //- bgz: the page
   Rng2F32 content = R2F32P(page.min.x + _VIEW_MARGIN_X * scale,
                            page.min.y + _VIEW_MARGIN_TOP * scale,
                            page.max.x - _VIEW_MARGIN_X * scale,
                            page.max.y - _VIEW_MARGIN_BOTTOM * scale);
   F32     gutter = _VIEW_PICTURE_GUTTER * scale;

   //- bgz: where the title is
   //
   // Alone on the slide - no body, and no picture but one behind it or
   // placed somewhere - it is large, in the middle. Else it is at the top.
   // An @place line puts it where that says, wrapped at that width; the
   // body stays where the title's own place would have left it.
   B32     has_title = (slide->title.size != 0);
   B32     title_is_big = (has_title && slide->item_count == 0 && shared_count == 0);
   Rng2F32 title_rect = content;
   F32     y = content.min.y;
   if (has_title) {
      if (title_is_big) {
         // Its words are what is in the middle: the bar is under that.
         F32 height = _VIEW_Title(arena, fonts, &colors, slide, content, scale, 1, 1, alpha, 1, 0) - content.min.y - 19.0f * scale;
         title_rect.min.y = Max((content.min.y + content.max.y - height) * 0.5f, content.min.y);
      }
      y = _VIEW_Title(arena, fonts, &colors, slide, title_rect, scale, title_is_big, centered || title_is_big, alpha, 1, 0);
      y += 30.0f * scale;
      if (title_place->is_set) {
         title_rect = _VIEW_PlacedRect(page, title_place);
      }
   }

   //- bgz: the room of the body, and of the pictures that share one
   //
   // The body has the full width under the title, or half of it when there
   // are pictures beside it, which then have the other half: which half is
   // the slide's layout. With no body the pictures have it all - and with
   // no title either, all of the rect: a slide that is only a picture
   // fills the window, not the page. The pictures' room is kept even when
   // they are not loaded, so the body does not move when they arrive.
   Rng2F32 body_rect = R2F32P(content.min.x, y, content.max.x, content.max.y);
   Rng2F32 picture_rect = body_rect;
   if (!has_text) {
      picture_rect = (shared_count == 1) ? rect : R2F32P(page.min.x + gutter, page.min.y + gutter, page.max.x - gutter,
                                                         page.max.y - gutter);
   } else if (shared_count != 0 && slide->item_count != 0) {
      F32 middle = (content.min.x + content.max.x) * 0.5f;
      if (slide->layout == DECK_Layout_PictureLeft) {
         picture_rect.max.x = middle - gutter * 0.5f;
         body_rect.min.x = middle + gutter * 0.5f;
      } else {
         body_rect.max.x = middle - gutter * 0.5f;
         picture_rect.min.x = middle + gutter * 0.5f;
      }
   }
   // An @place line puts the body where it says. The pictures stay where
   // the layout has them: moving the one does not move the other.
   if (text_place->is_set) {
      body_rect = _VIEW_PlacedRect(page, text_place);
   }

   //- bgz: the pictures, under the words
   for (U64 i = 0, shared_idx = 0; i < slide->picture_count; i += 1) {
      if (i == behind_idx) {
         continue;
      }
      DECK_Picture* picture = &slide->pictures[i];
      B32           is_placed = picture->place.is_set;
      B32           is_loaded = IMG_IsLoaded(&picture->image);
      Rng2F32       room = is_placed ? _VIEW_PlacedRect(page, &picture->place)
                                     : _VIEW_PictureCell(picture_rect, shared_idx, shared_count, gutter * 0.5f);
      if (!is_placed) {
         shared_idx += 1;
      }

      // The one picture beside a body starts level with the body's first
      // line, not in the middle of its half's height: its room is cut down
      // to the picture's own height, from the top.
      if (!is_placed && shared_count == 1 && slide->item_count != 0 && is_loaded && picture->image.size.x > 0 &&
          picture->image.size.y > 0) {
         Vec2F32 room_dim = Dim2F32(room);
         F32     fit = Min(room_dim.x / (F32)picture->image.size.x, room_dim.y / (F32)picture->image.size.y);
         room.max.y = room.min.y + (F32)picture->image.size.y * fit;
      }

      Rng2F32 drawn = room;
      if (is_loaded) {
         VIEW_DrawImageFit(&picture->image, room, alpha);
         drawn = _VIEW_FitRect(&picture->image, room);
      } else if (picture->load_failed) {
         // In the page, for the one picture that would fill the window.
         if (!has_text && !is_placed && shared_count == 1) {
            drawn = page;
         }
         _VIEW_DrawMissingPicture(fonts, &colors, picture->path, drawn, scale, alpha);
      }
      // A placed picture's hit is the picture as drawn. One that is not
      // placed keeps all of its room, as before there were places.
      _VIEW_PushHit(arena, hits, is_placed ? drawn : room, picture->range);
      _VIEW_PushBox(hits, DECK_PlaceKind_Picture, i, drawn);
   }

   //- bgz: the title
   if (has_title) {
      _VIEW_Title(arena, fonts, &colors, slide, title_rect, scale, title_is_big, centered || title_is_big, alpha, 0, hits);
   }

   //- bgz: the body
   if (slide->item_count != 0) {
      // Full size if it fits above the bottom margin, else the largest of a
      // few smaller sizes that does - or the smallest, if none do.
      F32 shrink = 1.0f;
      F32 end_y = body_rect.max.y;
      for (;;) {
         end_y = _VIEW_Body(arena, fonts, &colors, slide, body_rect, scale, shrink, alpha, 0, 0);
         if (end_y <= body_rect.max.y || shrink <= _VIEW_MIN_BODY_SHRINK) {
            break;
         }
         shrink = Max(shrink * 0.88f, _VIEW_MIN_BODY_SHRINK);
      }
      _VIEW_Body(arena, fonts, &colors, slide, body_rect, scale, shrink, alpha, 1, hits);

      // The box to move it by. Placed, it is the place. Not placed, it is
      // as tall as the words, not all the room down to the bottom margin:
      // that box would be mostly empty, and would already touch the page's
      // bottom, so it could hardly be dragged down at all. A little over,
      // so that the words still fit at the same size once it is placed.
      Rng2F32 box = body_rect;
      if (!text_place->is_set) {
         box.max.y = Clamp(body_rect.min.y + 1.0f, end_y + 8.0f * scale, body_rect.max.y);
      }
      _VIEW_PushBox(hits, DECK_PlaceKind_Text, 0, box);
   }

   Core_D_PopClip();
}

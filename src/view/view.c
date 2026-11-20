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

// Whose runs of letters are being drawn, for VIEW_HitList's runs: the
// title's or a block's, and where in the deck's text the first letter of
// the text being drawn is.
typedef struct _VIEW_RunsOf _VIEW_RunsOf;
struct _VIEW_RunsOf {
   VIEW_HitList*  hits;
   DECK_PlaceKind kind;
   U64            idx;
   U64            text_pos;
};

// letters is a part of text, drawn at x, y in font at px. Nothing with
// runs_of 0, or no hits to add to.
internal void
_VIEW_PushRun(Arena* arena, _VIEW_RunsOf* runs_of, FNT_Tag font, F32 px, String8 text, String8 letters, F32 x, F32 y, F32 width) {
   if (runs_of != 0 && runs_of->hits != 0) {
      VIEW_Run* run = PushArray(arena, VIEW_Run, 1);
      U64       start = runs_of->text_pos + ((letters.size != 0) ? (U64)(letters.str - text.str) : 0);
      run->rect = R2F32P(x, y, x + width, y + TXT_LineHeight(font, px));
      run->text_range = R1U64(start, start + letters.size);
      run->font = font;
      run->px = px;
      run->kind = runs_of->kind;
      run->idx = runs_of->idx;
      SLLQueuePush(runs_of->hits->first_run, runs_of->hits->last_run, run);
      runs_of->hits->run_count += 1;
   }
}

// Lines of wrapped text from top down, each starting at x. Returns the y
// under the last. With do_draw 0 nothing is drawn: it only measures.
// centered puts each line in the middle of max_width. widest_out, if not 0,
// gets the width of the widest line drawn. runs_of, if not 0, is given a
// run for each line drawn.
internal F32
_VIEW_Lines(Arena* arena, FNT_Tag font, F32 px, String8 text, F32 x, F32 y, F32 max_width, Vec4F32 color, B32 do_draw,
            B32 centered, F32* widest_out, _VIEW_RunsOf* runs_of) {
   F32         line_height = TXT_LineHeight(font, px);
   String8List lines = TXT_Wrap(arena, font, px, text, max_width);
   F32         widest = 0;

   for (String8Node* node = lines.first; node != 0; node = node->next) {
      if (do_draw) {
         F32 line_w = (centered || widest_out != 0 || runs_of != 0) ? TXT_Width(font, px, node->string) : 0;
         // Centered: each line in the middle of the width it could have had.
         F32 line_x = centered ? x + (max_width - line_w) * 0.5f : x;
         TXT_Draw(font, px, V2F32(line_x, y), color, node->string);
         _VIEW_PushRun(arena, runs_of, font, px, text, node->string, line_x, y, line_w);
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

// As _VIEW_Lines, for text that may have words in bold and in code. A run
// is given for each piece: every one as tall as the line, whatever its font.
internal F32
_VIEW_RichLines(Arena* arena, TXT_Fonts* fonts, VIEW_Colors* colors, F32 px, String8 text, F32 x, F32 y, F32 max_width,
                Vec4F32 color, F32 alpha, B32 do_draw, B32 centered, F32* widest_out, _VIEW_RunsOf* runs_of) {
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
            if (runs_of != 0 && runs_of->hits != 0) {
               F32 letters_w = TXT_Width(font, piece_px, piece->text);
               _VIEW_PushRun(arena, runs_of, font, piece_px, text, piece->text, piece_x + piece->pad_before, y, letters_w);
               // As tall as the line, not as its own font.
               runs_of->hits->last_run->rect.max.y = y + line_height;
            }
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

// The items of one block of the slide's body - its box_idx-th - from rect's
// top left down, no wider than rect, at the block's own size if it has one
// (@size). Returns the y under the last. The caller clips: the hits given
// are only of what is inside shown, and the runs of everything.
internal F32
_VIEW_Body(Arena* arena, TXT_Fonts* fonts, VIEW_Colors* colors, DECK_Slide* slide, DECK_Box* box, U64 box_idx, Rng2F32 rect,
           Rng2F32 shown, F32 scale, F32 alpha, VIEW_HitList* hits) {
   B32 centered = (slide->layout == DECK_Layout_Center);
   F32 unit = scale * (box->look.has_size ? box->look.size : 1.0f);
   F32 y = rect.min.y;

   _VIEW_RunsOf runs_of = {hits, DECK_PlaceKind_Text, box_idx, 0};

   // A colour of the block's own (@color) is the colour of all its words,
   // headings too - and of what is drawn from the words' colour, a code
   // box. The bullets' dots keep the theme's.
   VIEW_Colors own_colors = *colors;
   Vec4F32     heading_color = colors->accent;
   Vec4F32     dot_color = colors->accent;
   if (box->look.has_color) {
      own_colors.body = box->look.color;
      heading_color = box->look.color;
   }
   colors = &own_colors;

   F32 body_px = _VIEW_BODY_PX * unit;
   F32 body_line_height = TXT_LineHeight(fonts->regular, body_px);

   DECK_Item* item = box->first_item;
   for (U64 item_idx = 0; item_idx < box->item_count && item != 0; item_idx += 1, item = item->next) {
      // Room between items: a little always, more where the file had an empty
      // line - and above a heading, which belongs to what is under it.
      if (item != box->first_item) {
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
      F32* widest_out = (hits != 0) ? &widest : 0;
      runs_of.text_pos = item->text_range.min;

      switch (item->kind) {
      case DECK_ItemKind_Heading: {
         y = _VIEW_Lines(arena, fonts->bold, _VIEW_HEADING_PX * unit, item->text, rect.min.x, y, Dim2F32(rect).x,
                         _VIEW_Faded(heading_color, alpha), 1, centered, widest_out, &runs_of);
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

         if (!centered) {
            // A round dot, level with the middle of the first line. Not under
            // 4 pixels across, or in the small preview it all but vanishes.
            F32 dot = Max(px * 0.28f, 4.0f);
            F32 dot_x = (F32)(S32)(text_x - indent * 0.62f);
            F32 dot_y = (F32)(S32)(y + TXT_LineHeight(fonts->regular, px) * 0.54f - dot * 0.5f);

            R_Rect2DInst* inst = Core_D_Rect(R2F32P(dot_x, dot_y, dot_x + dot, dot_y + dot), R2F32P(0, 0, 1, 1),
                                             _VIEW_Faded(dot_color, alpha));
            for (U64 i = 0; i < ArrayCount(inst->corner_radii); i += 1) {
               inst->corner_radii[i] = dot * 0.5f;
            }
            // The soft edge is taken out of the dot, so a small dot gets a
            // thin one or there is nothing left of it.
            inst->edge_softness = Min(1.0f, dot * 0.1f);
         }

         // A bullet with no text yet still takes up its line, and has a
         // run with no letters: where the first will go.
         if (item->text.size == 0) {
            F32 empty_x = centered ? (rect.min.x + rect.max.x) * 0.5f : text_x;
            _VIEW_PushRun(arena, &runs_of, fonts->regular, px, item->text, item->text, empty_x, y, 0);
            y += TXT_LineHeight(fonts->regular, px);
         }
         y = _VIEW_RichLines(arena, fonts, colors, px, item->text, text_x, y, rect.max.x - text_x,
                             _VIEW_Faded(colors->body, alpha), alpha, 1, centered, widest_out, &runs_of);
      } break;

      default: {
         y = _VIEW_RichLines(arena, fonts, colors, body_px, item->text, rect.min.x, y, Dim2F32(rect).x,
                             _VIEW_Faded(colors->body, alpha), alpha, 1, centered, widest_out, &runs_of);
      } break;
      }

      {
         // A bullet with no words yet is still something to click: as wide
         // as a few letters.
         F32     hit_w = Max(widest, body_px * 3.0f);
         Rng2F32 hit_rect = R2F32P(item_x, item_y, text_x + hit_w, y);
         if (centered) {
            F32 middle = (rect.min.x + rect.max.x) * 0.5f;
            hit_rect = R2F32P(middle - hit_w * 0.5f, item_y, middle + hit_w * 0.5f, y);
         }
         _VIEW_PushHit(arena, hits, Intersect2F32(hit_rect, shown), item->text_range);
      }
   }

   return y;
}

// The room between two blocks of the body that share the body's room, at
// unit (the scale): a line of the body's words. More than the most
// _VIEW_Body leaves between two of its lines - three quarters of one, at an
// empty line or a heading - so that where one block ends and the next
// begins can be seen, and not so much that they stop reading as one body.
internal F32
_VIEW_BlockGap(TXT_Fonts* fonts, F32 unit) {
   return TXT_LineHeight(fonts->regular, _VIEW_BODY_PX * unit);
}

// The scroll bar of a room with more words than fit it: beside its right
// edge, outside it - the words wrap at the room's width, and a bar inside
// would be over their ends. words_height is how tall all the words are and
// scroll how far down them the room's top is, both in pixels. Drawn, and
// put on the box hits has last been given, if there are more words than
// room; nothing if they fit.
internal void
_VIEW_ScrollBar(VIEW_HitList* hits, VIEW_Colors* colors, Rng2F32 room, Rng2F32 page, F32 words_height, F32 scroll, F32 scale,
                F32 alpha) {
   F32 room_h = room.max.y - room.min.y;
   if (room_h > 0 && words_height > room_h + 0.5f) {
      // A few pixels each at least, on a slide the size of the edit
      // screen's preview, where a page unit is under half a pixel: the box
      // around the block the caret is in, with its corner handles, reaches
      // some pixels past the room, and a bar any nearer is hidden by them.
      // Not on the small slides, where that would be most of the page.
      F32     width = Max(5.0f * scale, Min(3.0f, 12.0f * scale));
      F32     gap = Max(8.0f * scale, Min(8.0f, 30.0f * scale));
      Rng2F32 track = R2F32P(room.max.x + gap, room.min.y, room.max.x + gap + width, room.max.y);
      // On the page: a room at the page's right edge has it inside.
      if (track.max.x > page.max.x) {
         track.min.x -= track.max.x - page.max.x;
         track.max.x = page.max.x;
      }
      F32 thumb_h = Clamp(Min(24.0f * scale, room_h), room_h * room_h / words_height, room_h);
      F32 part = Clamp(0.0f, scroll / (words_height - room_h), 1.0f);
      F32 thumb_y = track.min.y + part * (room_h - thumb_h);
      Rng2F32 thumb = R2F32P(track.min.x, thumb_y, track.max.x, thumb_y + thumb_h);

      if (hits == 0 || !hits->bars_by_caller) {
         R_Rect2DInst* track_inst = Core_D_Rect(track, R2F32P(0, 0, 1, 1), _VIEW_Faded(colors->body, 0.2f * alpha));
         R_Rect2DInst* thumb_inst = Core_D_Rect(thumb, R2F32P(0, 0, 1, 1), _VIEW_Faded(colors->body, 0.55f * alpha));
         for (U64 i = 0; i < ArrayCount(thumb_inst->corner_radii); i += 1) {
            track_inst->corner_radii[i] = width * 0.5f;
            thumb_inst->corner_radii[i] = width * 0.5f;
         }
      }

      if (hits != 0 && hits->box_count != 0) {
         VIEW_Box* box = &hits->boxes[hits->box_count - 1];
         box->bar_track = track;
         box->bar_thumb = thumb;
         box->words_height = words_height;
      }
   }
}

internal void
_VIEW_PushBox(VIEW_HitList* hits, DECK_PlaceKind kind, U64 idx, Rng2F32 rect, F32 text_size, Rng2F32 room, F32 words_max_y) {
   if (hits != 0 && hits->box_count < ArrayCount(hits->boxes) && rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
      VIEW_Box* box = &hits->boxes[hits->box_count];
      hits->box_count += 1;
      box->kind = kind;
      box->idx = idx;
      box->rect = rect;
      box->text_size = text_size;
      box->room = room;
      box->words_max_y = words_max_y;
   }
}

// An @place line's rect, from parts of the page to pixels.
internal Rng2F32
_VIEW_PlacedRect(Rng2F32 page, DECK_Place* place) {
   Vec2F32 page_dim = Dim2F32(page);
   return R2F32P(page.min.x + place->rect.min.x * page_dim.x, page.min.y + place->rect.min.y * page_dim.y,
                 page.min.x + place->rect.max.x * page_dim.x, page.min.y + place->rect.max.y * page_dim.y);
}

// The shape a picture is laid out by: its width over its height. One that is
// not loaded - not yet, or not found - is taken to be 4:3, the shape of the
// box that says it is missing.
internal F32
_VIEW_AspectOf(DECK_Picture* picture) {
   F32 result = 4.0f / 3.0f;
   if (IMG_IsLoaded(&picture->image) && picture->image.size.x > 0 && picture->image.size.y > 0) {
      result = (F32)picture->image.size.x / (F32)picture->image.size.y;
   }
   return result;
}

// Where each of count pictures goes in the room they share, as a grid: rects
// into out, each the picture's own shape, as large as its cell allows.
//
// The number of columns is the one that gives the pictures the most room
// between them, from their shapes: wide pictures beside a body go one above
// the other, in a wide room side by side, and in rows and columns past a
// few. Then each row is only as tall as its tallest picture, and the rows are
// put together with gap between them - not spread over cells as tall as the
// room, which leaves wide pictures far apart - in the middle of the room, or
// from its top with from_top: beside a body, level with its first line, as
// one picture is. A row with fewer pictures than the others has them in the
// middle.
internal void
_VIEW_PictureGrid(Rng2F32 room, F32* aspects, U64 count, F32 gap, B32 from_top, Rng2F32* out) {
   Vec2F32 dim = Dim2F32(room);
   if (count == 0 || dim.x <= 0 || dim.y <= 0) {
      for (U64 i = 0; i < count; i += 1) {
         out[i] = R2F32P(room.min.x, room.min.y, room.min.x, room.min.y);
      }
      return;
   }

   //- bgz: how many columns
   U64 cols = 1;
   F32 best_area = -1.0f;
   for (U64 try_cols = 1; try_cols <= count; try_cols += 1) {
      U64 try_rows = (count + try_cols - 1) / try_cols;
      F32 cell_w = (dim.x - gap * (F32)(try_cols - 1)) / (F32)try_cols;
      F32 cell_h = (dim.y - gap * (F32)(try_rows - 1)) / (F32)try_rows;
      if (cell_w <= 0 || cell_h <= 0) {
         continue;
      }
      F32 area = 0;
      for (U64 i = 0; i < count; i += 1) {
         F32 w = Min(cell_w, cell_h * aspects[i]);
         area += w * (w / aspects[i]);
      }
      // A little more is not worth a grid that is less square: the first
      // that gives the most, by more than a hair, is kept.
      if (area > best_area * 1.01f) {
         best_area = area;
         cols = try_cols;
      }
   }
   U64 rows = (count + cols - 1) / cols;
   F32 cell_w = Max((dim.x - gap * (F32)(cols - 1)) / (F32)cols, 1.0f);
   F32 cell_h = Max((dim.y - gap * (F32)(rows - 1)) / (F32)rows, 1.0f);

   //- bgz: each picture's size in its cell, and each row's height
   F32 row_heights[DECK_MAX_PICTURES] = {0};
   F32 total_h = gap * (F32)(rows - 1);
   for (U64 row = 0; row < rows; row += 1) {
      for (U64 i = row * cols; i < Min((row + 1) * cols, count); i += 1) {
         F32 w = Min(cell_w, cell_h * aspects[i]);
         out[i] = R2F32P(0, 0, w, w / aspects[i]);
         row_heights[row] = Max(row_heights[row], w / aspects[i]);
      }
      total_h += row_heights[row];
   }

   //- bgz: put together
   F32 y = from_top ? room.min.y : room.min.y + (dim.y - total_h) * 0.5f;
   for (U64 row = 0; row < rows; row += 1) {
      U64 first = row * cols;
      U64 end = Min(first + cols, count);
      F32 row_w = gap * (F32)(end - first - 1);
      for (U64 i = first; i < end; i += 1) {
         row_w += out[i].max.x;
      }
      F32 x = room.min.x + (dim.x - row_w) * 0.5f;
      for (U64 i = first; i < end; i += 1) {
         F32 w = out[i].max.x;
         F32 h = out[i].max.y;
         F32 top = y + (row_heights[row] - h) * 0.5f;
         out[i] = R2F32P(x, top, x + w, top + h);
         x += w + gap;
      }
      y += row_heights[row] + gap;
   }
}

// The slide's title, in rect: from its top, no wider than it. With measure
// 0 it is drawn, with the bar under it, and its hit and its box are given.
// Returns the y under the bar.
internal F32
_VIEW_Title(Arena* arena, TXT_Fonts* fonts, VIEW_Colors* colors, DECK_Slide* slide, Rng2F32 rect, F32 scale, B32 is_big,
            B32 centered, F32 alpha, B32 measure, VIEW_HitList* hits) {
   // Its words at a size and in a colour of their own, where the slide says
   // (@size title, @color title). The bar under it stays as it is.
   DECK_Look* look = &slide->title_look;
   F32        size = look->has_size ? look->size : 1.0f;
   Vec4F32    color = look->has_color ? look->color : colors->title;
   F32        px = (is_big ? _VIEW_BIG_TITLE_PX : _VIEW_TITLE_PX) * scale * size;
   F32        width = Dim2F32(rect).x;
   F32 line_height = TXT_LineHeight(fonts->bold, px);
   F32 widest = 0;
   _VIEW_RunsOf runs_of = {measure ? 0 : hits, DECK_PlaceKind_Title, 0, slide->title_range.min};
   F32 y = _VIEW_Lines(arena, fonts->bold, px, slide->title, rect.min.x, rect.min.y, width, _VIEW_Faded(color, alpha),
                       !measure, centered, &widest, &runs_of);
   if (slide->title.size == 0) {
      // No words yet: a run with no letters, where the first will go.
      _VIEW_PushRun(arena, &runs_of, fonts->bold, px, slide->title, slide->title, centered ? rect.min.x + width * 0.5f : rect.min.x,
                    rect.min.y, 0);
   }
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
      // Its room: the width its words wrap at. Its height is theirs: a
      // title is all shown, however long.
      _VIEW_PushBox(hits, DECK_PlaceKind_Title, 0, box, size, R2F32P(rect.min.x, rect.min.y, rect.max.x, y), y);

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
   DECK_Place* title_place = &slide->places[DECK_PlaceKind_Title];

   //- bgz: which pictures go where
   //
   // With the picture-behind layout the slide's first picture is behind
   // the words. The rest have the room the layout keeps for pictures
   // (below). A picture with an @place line is where that says, and the
   // others share the room between them, as a grid - a picture moved out
   // of the grid does not leave a hole in it. The room itself is there for
   // every picture, placed or not: the title and the body do not move when
   // a picture does, and a drag that has only just started does not shift
   // the words under the mouse.
   U64 behind_idx = (slide->picture_count != 0 && slide->layout == DECK_Layout_PictureBehind && has_text) ? 0 : max_U64;
   U64 beside_count = 0; // every picture not behind
   U64 shared_count = 0; // of those, the ones not placed
   for (U64 i = 0; i < slide->picture_count; i += 1) {
      if (i != behind_idx) {
         beside_count += 1;
         shared_count += !slide->pictures[i].place.is_set;
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
   // Alone on the slide - no body, and no picture but one behind it - it is
   // large, in the middle. Else it is at the top. An @place line puts it
   // where that says, wrapped at that width; the body stays where the
   // title's own place would have left it.
   B32     has_title = (slide->title.size != 0);
   B32     title_is_big = (has_title && slide->item_count == 0 && beside_count == 0);
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
      picture_rect = (beside_count == 1) ? rect : R2F32P(page.min.x + gutter, page.min.y + gutter, page.max.x - gutter,
                                                         page.max.y - gutter);
   } else if (beside_count != 0 && slide->item_count != 0) {
      F32 middle = (content.min.x + content.max.x) * 0.5f;
      if (slide->layout == DECK_Layout_PictureLeft) {
         picture_rect.max.x = middle - gutter * 0.5f;
         body_rect.min.x = middle + gutter * 0.5f;
      } else {
         body_rect.max.x = middle - gutter * 0.5f;
         picture_rect.min.x = middle + gutter * 0.5f;
      }
   }

   //- bgz: the pictures, under the words
   //
   // The ones not placed share the room as a grid (above). One alone has all
   // of it.
   Rng2F32 cells[DECK_MAX_PICTURES] = {0};
   if (shared_count > 1) {
      F32 aspects[DECK_MAX_PICTURES] = {0};
      for (U64 i = 0, cell_idx = 0; i < slide->picture_count && cell_idx < DECK_MAX_PICTURES; i += 1) {
         if (i != behind_idx && !slide->pictures[i].place.is_set) {
            aspects[cell_idx] = _VIEW_AspectOf(&slide->pictures[i]);
            cell_idx += 1;
         }
      }
      _VIEW_PictureGrid(picture_rect, aspects, Min(shared_count, DECK_MAX_PICTURES), gutter * 0.5f, slide->item_count != 0,
                        cells);
   }
   for (U64 i = 0, cell_idx = 0; i < slide->picture_count; i += 1) {
      if (i == behind_idx) {
         continue;
      }
      DECK_Picture* picture = &slide->pictures[i];
      B32           is_placed = picture->place.is_set;
      B32           is_loaded = IMG_IsLoaded(&picture->image);
      Rng2F32       room = picture_rect;
      if (is_placed) {
         room = _VIEW_PlacedRect(page, &picture->place);
      } else if (shared_count > 1) {
         room = cells[Min(cell_idx, DECK_MAX_PICTURES - 1)];
         cell_idx += 1;
      }

      // The one picture beside a body starts level with the body's first
      // line, not in the middle of its half's height: its room is cut down
      // to the picture's own height, from the top. (The grid does that for
      // several.)
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
         if (!has_text && !is_placed && beside_count == 1) {
            drawn = page;
         }
         _VIEW_DrawMissingPicture(fonts, &colors, picture->path, drawn, scale, alpha);
      }
      // A placed picture's hit is the picture as drawn. One that is not
      // placed keeps all of its room, as before there were places - which,
      // in a grid, is the picture as drawn too.
      _VIEW_PushHit(arena, hits, is_placed ? drawn : room, picture->range);
      _VIEW_PushBox(hits, DECK_PlaceKind_Picture, i, drawn, 0, drawn, 0);
   }

   //- bgz: the title
   if (has_title) {
      _VIEW_Title(arena, fonts, &colors, slide, title_rect, scale, title_is_big, centered || title_is_big, alpha, 0, hits);
   }

   //- bgz: the body, a block at a time
   //
   // A block with an @place line is where that says. The rest are one under
   // another in the body's room, as one body would be: the pictures stay
   // where the layout has them, and moving a block does not move them.
   //
   // The words stay the size they are. What does not fit a block's room -
   // its place, or the body's room for those that share it - is not shown,
   // and the block's scroll (DECK_Box's, or the slide's body_scroll for the
   // ones that share) says how far down its words the room shows.
   if (slide->item_count != 0) {
      U64 last_shared = max_U64; // the last block that is not placed
      for (U64 i = 0; i < slide->box_count; i += 1) {
         if (slide->boxes[i].item_count != 0 && !slide->boxes[i].place.is_set) {
            last_shared = i;
         }
      }

      F32 y_at = body_rect.min.y - slide->body_scroll * scale;
      for (U64 i = 0; i < slide->box_count; i += 1) {
         DECK_Box* block = &slide->boxes[i];
         if (block->item_count == 0) {
            continue;
         }
         F32 text_size = block->look.has_size ? block->look.size : 1.0f;
         if (block->place.is_set) {
            Rng2F32 room = _VIEW_PlacedRect(page, &block->place);
            Rng2F32 shown = Intersect2F32(room, rect);
            Rng2F32 block_rect = room;
            block_rect.min.y -= block->scroll * scale;
            Core_D_PushClip(shown);
            F32 end_y = _VIEW_Body(arena, fonts, &colors, slide, block, i, block_rect, shown, scale, alpha, hits);
            Core_D_PopClip();
            _VIEW_PushBox(hits, DECK_PlaceKind_Text, i, room, text_size, room, end_y);
            _VIEW_ScrollBar(hits, &colors, room, page, end_y - block_rect.min.y, block->scroll * scale, scale, alpha);
         } else {
            Rng2F32 shown = Intersect2F32(body_rect, rect);
            Rng2F32 block_rect = R2F32P(body_rect.min.x, y_at, body_rect.max.x, body_rect.max.y);
            Core_D_PushClip(shown);
            F32 end_y = _VIEW_Body(arena, fonts, &colors, slide, block, i, block_rect, shown, scale, alpha, hits);
            Core_D_PopClip();
            y_at = end_y + _VIEW_BlockGap(fonts, scale);

            // The box to move it by: as tall as the words, not all the room
            // down to the bottom margin - that box would be mostly empty,
            // and would already touch the page's bottom, so it could hardly
            // be dragged down at all. A little over, so that the words
            // still fit once it is placed. And only what of it is in the
            // body's room.
            Rng2F32 box = block_rect;
            box.min.y = Clamp(body_rect.min.y, block_rect.min.y, body_rect.max.y);
            box.max.y = Clamp(box.min.y + 1.0f, end_y + 8.0f * scale, Max(body_rect.max.y, box.min.y + 1.0f));
            Rng2F32 room = box;
            if (i == last_shared) {
               room.max.y = Max(body_rect.max.y, box.max.y);
            }
            _VIEW_PushBox(hits, DECK_PlaceKind_Text, i, box, text_size, room, end_y);
            if (i == last_shared) {
               // The one bar of the blocks that share the body's room.
               F32 scroll = slide->body_scroll * scale;
               _VIEW_ScrollBar(hits, &colors, body_rect, page, end_y - (body_rect.min.y - scroll), scroll, scale, alpha);
            }
         }
      }
   }

   Core_D_PopClip();
}

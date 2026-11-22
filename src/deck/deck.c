////////////////////////////////
//~ bgz: deck
//
// See deck.h.

// File names in the order a person expects: letters without regard to case,
// and a run of digits as the number it spells, so "slide2" is before
// "slide10". Negative, zero or positive, as strcmp.
PRIVATE S32
_Deck_CompareNames(String8 a, String8 b) {
   U64 i = 0;
   U64 j = 0;

   while (i < a.size && j < b.size) {
      if (CharIsDigit(a.str[i], 10) && CharIsDigit(b.str[j], 10)) {
         // Leading zeros do not count; then the longer run is the larger
         // number, and runs of one length compare digit by digit.
         while (i < a.size && a.str[i] == '0') i += 1;
         while (j < b.size && b.str[j] == '0') j += 1;

         U64 a_start = i;
         U64 b_start = j;
         while (i < a.size && CharIsDigit(a.str[i], 10)) i += 1;
         while (j < b.size && CharIsDigit(b.str[j], 10)) j += 1;

         U64 a_digits = i - a_start;
         U64 b_digits = j - b_start;
         if (a_digits != b_digits) {
            return a_digits < b_digits ? -1 : 1;
         }
         for (U64 k = 0; k < a_digits; k += 1) {
            U8 a_digit = a.str[a_start + k];
            U8 b_digit = b.str[b_start + k];
            if (a_digit != b_digit) {
               return a_digit < b_digit ? -1 : 1;
            }
         }
      }
      else {
         U8 a_char = LowerFromChar(a.str[i]);
         U8 b_char = LowerFromChar(b.str[j]);
         if (a_char != b_char) {
            return a_char < b_char ? -1 : 1;
         }
         i += 1;
         j += 1;
      }
   }

   // One is a prefix of the other: the shorter first.
   U64 a_left = a.size - i;
   U64 b_left = b.size - j;
   return a_left == b_left ? 0 : (a_left < b_left ? -1 : 1);
}

// Insertion sort: a folder of slides is small, and the file system mostly
// hands them over in order already.
void
Deck_SortNames(String8Array names) {
   for (U64 i = 1; i < names.count; i += 1) {
      String8 name = names.v[i];
      U64     slot = i;
      while (slot > 0 && _Deck_CompareNames(name, names.v[slot - 1]) < 0) {
         names.v[slot] = names.v[slot - 1];
         slot -= 1;
      }
      names.v[slot] = name;
   }
}

String8Array
Deck_PictureNamesFromFolder(Arena* arena, String8 folder) {
   String8List names = {0};
   Temp        scratch = ScratchBegin(&arena, 1);

   if (folder.size != 0) {
      // The folder's own images.
      FileIter* it = FileIterBegin(scratch.arena, folder, FileIterFlag_SkipFolders | FileIterFlag_SkipHiddenFiles);
      for (FileInfo info = {0}; FileIterNext(scratch.arena, it, &info);) {
         if ((Image_NameIsImage(info.name) || Video_NameIsVideo(info.name))) {
            Str8ListPush(arena, &names, PushStr8Copy(arena, info.name));
         }
      }
      FileIterEnd(it);

      // And those one folder down: a deck often keeps its pictures in one.
      String8List sub_folders = {0};
      it = FileIterBegin(scratch.arena, folder, FileIterFlag_SkipFiles | FileIterFlag_SkipHiddenFiles);
      for (FileInfo info = {0}; FileIterNext(scratch.arena, it, &info);) {
         Str8ListPush(scratch.arena, &sub_folders, PushStr8Copy(scratch.arena, info.name));
      }
      FileIterEnd(it);

      for (String8Node* node = sub_folders.first; node != 0; node = node->next) {
         String8 sub_folder = PushStr8F(scratch.arena, "%S\\%S", folder, node->string);
         it = FileIterBegin(scratch.arena, sub_folder, FileIterFlag_SkipFolders | FileIterFlag_SkipHiddenFiles);
         for (FileInfo info = {0}; FileIterNext(scratch.arena, it, &info);) {
            if ((Image_NameIsImage(info.name) || Video_NameIsVideo(info.name))) {
               Str8ListPush(arena, &names, PushStr8F(arena, "%S/%S", node->string, info.name));
            }
         }
         FileIterEnd(it);
      }
   }

   String8Array result = Str8ArrayFromList(arena, &names);
   Deck_SortNames(result);

   ScratchEnd(scratch);
   return result;
}

Deck_Deck*
Deck_OpenFolder(String8 folder) {
   Arena*     arena = ArenaAlloc();
   Deck_Deck* deck = PushArray(arena, Deck_Deck, 1);
   deck->arena = arena;

   // No trailing slash: the file iterator adds its own, and takes an empty
   // path to mean the list of drives.
   while (folder.size > 0 && (folder.str[folder.size - 1] == '/' || folder.str[folder.size - 1] == '\\')) {
      folder.size -= 1;
   }
   deck->folder = PushStr8Copy(arena, folder);

   if (deck->folder.size != 0) {
      Temp scratch = ScratchBegin(&arena, 1);

      // The names first, since how many there are is not known until the end.
      String8List names = {0};
      FileIter*   it = FileIterBegin(scratch.arena, deck->folder, FileIterFlag_SkipFolders | FileIterFlag_SkipHiddenFiles);
      for (FileInfo info = {0}; FileIterNext(scratch.arena, it, &info);) {
         if (Image_NameIsImage(info.name)) {
            Str8ListPush(scratch.arena, &names, info.name);
         }
      }
      FileIterEnd(it);

      String8Array sorted = Str8ArrayFromList(scratch.arena, &names);

      Deck_SortNames(sorted);

      deck->slide_count = sorted.count;
      deck->slides = PushArray(arena, Deck_Slide, Max(sorted.count, 1));
      for (U64 i = 0; i < sorted.count; i += 1) {
         deck->slides[i].pictures = PushArray(arena, Deck_Picture, 1);
         deck->slides[i].picture_count = 1;
         deck->slides[i].pictures[0].path = PushStr8F(arena, "%S\\%S", deck->folder, sorted.v[i]);
      }

      ScratchEnd(scratch);
   }

   return deck;
}

////////////////////////////////
//~ bgz: decks from text files

B32
Deck_NameIsTextDeck(String8 name) {
   String8 extension = Str8SkipLastDot(name);
   B32     result = (Str8Match(extension, Str8Lit("md"), StringMatchFlag_CaseInsensitive) ||
                 Str8Match(extension, Str8Lit("txt"), StringMatchFlag_CaseInsensitive));
   return result;
}

// A slide being read: in a list, since how many there are is not known until
// the end of the file.
typedef struct _Deck_SlideNode _Deck_SlideNode;
struct _Deck_SlideNode {
   _Deck_SlideNode* next;
   Deck_Slide       slide;
};

// The string without the spaces and tabs at either end.
PRIVATE String8
_Deck_Trim(String8 string) {
   while (string.size > 0 && (string.str[0] == ' ' || string.str[0] == '\t')) {
      string.str += 1;
      string.size -= 1;
   }
   while (string.size > 0 && (string.str[string.size - 1] == ' ' || string.str[string.size - 1] == '\t')) {
      string.size -= 1;
   }
   return string;
}

Deck_Deck*
Deck_OpenFile(String8 file) {
   Temp scratch = ScratchBegin(0, 0);

   String8 text = DataFromFilePath(scratch.arena, file);

   // A byte order mark, which Notepad puts at the start of a UTF-8 file.
   if (text.size >= 3 && text.str[0] == 0xEF && text.str[1] == 0xBB && text.str[2] == 0xBF) {
      text = Str8Skip(text, 3);
   }

   // The \r of a Windows line ending is taken off each line as it is read.
   Deck_Deck* deck = Deck_OpenText(text, file);

   ScratchEnd(scratch);
   return deck;
}

String8
Deck_ThemeName(Deck_Theme theme) {
   String8 names[Deck_Theme_COUNT] = {Str8Lit("dark"), Str8Lit("light"), Str8Lit("blue"), Str8Lit("paper")};
   return names[(theme < Deck_Theme_COUNT) ? theme : 0];
}

String8
Deck_LayoutName(Deck_Layout layout) {
   String8 names[Deck_Layout_COUNT] = {Str8Lit("picture-right"), Str8Lit("picture-left"), Str8Lit("picture-behind"),
                                       Str8Lit("center")};
   return names[(layout < Deck_Layout_COUNT) ? layout : 0];
}

String8
Deck_PlaceName(Deck_PlaceKind kind) {
   String8 names[Deck_PlaceKind_COUNT] = {Str8Lit("picture"), Str8Lit("text"), Str8Lit("title")};
   return names[(kind < Deck_PlaceKind_COUNT) ? kind : 0];
}

Deck_NamedColor
Deck_NamedColorFromIdx(U64 idx) {
   // Each one readable on a dark theme and on a light one, but for white
   // and black, which are for one of the two.
   Deck_NamedColor colors[DECK_NAMED_COLOR_COUNT] = {
       {Str8LitComp("white"), {{1.00f, 1.00f, 1.00f, 1.0f}}},
       {Str8LitComp("black"), {{0.07f, 0.07f, 0.08f, 1.0f}}},
       {Str8LitComp("gray"), {{0.55f, 0.57f, 0.60f, 1.0f}}},
       {Str8LitComp("red"), {{0.90f, 0.26f, 0.24f, 1.0f}}},
       {Str8LitComp("orange"), {{0.96f, 0.56f, 0.16f, 1.0f}}},
       // Nearer gold than a bright yellow, which on light and paper is hardly seen.
       {Str8LitComp("yellow"), {{0.84f, 0.65f, 0.07f, 1.0f}}},
       {Str8LitComp("green"), {{0.30f, 0.74f, 0.40f, 1.0f}}},
       {Str8LitComp("blue"), {{0.25f, 0.56f, 0.96f, 1.0f}}},
       {Str8LitComp("purple"), {{0.62f, 0.42f, 0.90f, 1.0f}}},
       {Str8LitComp("pink"), {{0.94f, 0.44f, 0.68f, 1.0f}}},
   };
   return colors[(idx < DECK_NAMED_COLOR_COUNT) ? idx : 0];
}

B32
Deck_ColorFromString(String8 string, Vec4F32* color_out) {
   B32 result = 0;
   if (string.size != 0 && string.str[0] == '#' && (string.size == 4 || string.size == 7)) {
      // #rgb is #rrggbb with each digit twice.
      U32 digits[6] = {0};
      B32 all_hex = 1;
      U64 digit_count = string.size - 1;
      for (U64 i = 0; i < digit_count; i += 1) {
         U8 c = string.str[i + 1];
         if (c >= '0' && c <= '9') {
            digits[i] = (U32)(c - '0');
         } else if (c >= 'a' && c <= 'f') {
            digits[i] = (U32)(c - 'a') + 10;
         } else if (c >= 'A' && c <= 'F') {
            digits[i] = (U32)(c - 'A') + 10;
         } else {
            all_hex = 0;
         }
      }
      if (all_hex) {
         F32 parts[3] = {0};
         for (U64 i = 0; i < 3; i += 1) {
            U32 value = (digit_count == 3) ? digits[i] * 17 : digits[i * 2] * 16 + digits[i * 2 + 1];
            parts[i] = (F32)value / 255.0f;
         }
         *color_out = V4F32(parts[0], parts[1], parts[2], 1.0f);
         result = 1;
      }
   } else {
      for (U64 i = 0; i < DECK_NAMED_COLOR_COUNT && !result; i += 1) {
         Deck_NamedColor named = Deck_NamedColorFromIdx(i);
         if (Str8Match(string, named.name, StringMatchFlag_CaseInsensitive)) {
            *color_out = named.color;
            result = 1;
         }
      }
      // As it is also spelled.
      if (!result && Str8Match(string, Str8Lit("grey"), StringMatchFlag_CaseInsensitive)) {
         *color_out = Deck_NamedColorFromIdx(2).color;
         result = 1;
      }
   }
   return result;
}

// The number at the start of *string - 0.25, .5, 1 - which is then moved past
// it and the spaces before it. 0 if there is none there.
PRIVATE B32
_Deck_TakeNumber(String8* string, F32* number_out) {
   U64 i = 0;
   while (i < string->size && (string->str[i] == ' ' || string->str[i] == '\t' || string->str[i] == ',')) i += 1;
   B32 is_negative = (i < string->size && string->str[i] == '-');
   if (is_negative) {
      i += 1;
   }
   F64 value = 0;
   F64 unit = 1.0;
   B32 after_point = 0;
   U64 digit_count = 0;
   for (; i < string->size; i += 1) {
      U8 c = string->str[i];
      if (c >= '0' && c <= '9') {
         digit_count += 1;
         if (after_point) {
            unit *= 0.1;
            value += unit * (F64)(c - '0');
         } else {
            value = value * 10.0 + (F64)(c - '0');
         }
      } else if (c == '.' && !after_point) {
         after_point = 1;
      } else {
         break;
      }
   }
   // A number ends at a space or the end, not in a letter.
   B32 ends_well = (i == string->size || string->str[i] == ' ' || string->str[i] == '\t' || string->str[i] == ',');
   B32 result = (digit_count != 0 && ends_well);
   if (result) {
      *number_out = (F32)(is_negative ? -value : value);
      *string = Str8Skip(*string, i);
   }
   return result;
}

Deck_Deck*
Deck_OpenText(String8 text_to_copy, String8 file) {
   Arena*     arena = ArenaAlloc();
   Deck_Deck* deck = PushArray(arena, Deck_Deck, 1);
   deck->arena = arena;
   deck->file = PushStr8Copy(arena, file);

   // Pictures are found from the folder the file is in. A file with nothing
   // before its name is in the folder the program was started from.
   deck->folder = Str8ChopLastSlash(deck->file);
   if (deck->folder.size == 0) {
      deck->folder = Str8Lit(".");
   }

   Temp scratch = ScratchBegin(&arena, 1);

   // The deck's own copy: titles and items are pieces of it.
   String8 text = PushStr8Copy(arena, text_to_copy);

   _Deck_SlideNode* first_node = 0;
   _Deck_SlideNode* last_node = 0;
   U64              node_count = 0;

   // The slide lines are being added to. 0 at the start and after a ---: the
   // next line with anything on it starts a new slide.
   Deck_Slide* slide = 0;
   B32         gap_before = 0;

   // Where the last --- line began, for the slide it starts: that begins at
   // the rule, not at the first line after it with something on it.
   B32 has_rule_pos = 0;
   U64 rule_pos = 0;

   // An @layout line that came before its slide had anything on it - right
   // under a --- - waits here for the slide.
   B32         has_waiting_layout = 0;
   Deck_Layout waiting_layout = Deck_Layout_PictureRight;
   Rng1U64     waiting_layout_line = {0};
   // The same for an @video line: 1 if it said so, by word.
   B32 waiting_video_loop = 0;
   B32 waiting_video_mute = 0;
   B32 waiting_video_manual = 0;
   // And for @place lines. A picture's waits for the slide's first picture,
   // which can be a few lines on in a slide that has started already.
   Deck_Place waiting_places[Deck_PlaceKind_COUNT] = {0};
   Deck_Place waiting_picture_place = {0};
   // And for @size and @color lines: the title's, and the first block's.
   Deck_Look waiting_title_look = {0};
   Deck_Look waiting_text_look = {0};
   // Whether the last picture line was one past DECK_MAX_PICTURES, passed
   // over: an @place line under it is passed over with it, and not given to
   // the picture before.
   B32 last_picture_passed_over = 0;

   U64 pos = 0;
   while (pos < text.size) {
      //- bgz: the next line, without its line ending
      U64 line_start = pos;
      while (pos < text.size && text.str[pos] != '\n') pos += 1;
      String8 line = Str8Substr(text, R1U64(line_start, pos));
      pos += 1;
      if (line.size > 0 && line.str[line.size - 1] == '\r') {
         line.size -= 1;
      }

      // How far in it starts, a tab counting as four spaces.
      U64 indent = 0;
      for (U64 i = 0; i < line.size && (line.str[i] == ' ' || line.str[i] == '\t'); i += 1) {
         indent += (line.str[i] == '\t') ? 4 : 1;
      }
      line = _Deck_Trim(line);

      //- bgz: what kind of line it is
      if (line.size == 0) {
         gap_before = 1;
         continue;
      }

      B32 is_rule = (line.size >= 3);
      for (U64 i = 0; i < line.size && is_rule; i += 1) {
         is_rule = (line.str[i] == '-');
      }
      if (is_rule) {
         slide = 0;
         gap_before = 0;
         MemoryZeroStruct(&waiting_picture_place);
         last_picture_passed_over = 0;
         has_rule_pos = 1;
         rule_pos = line_start;
         continue;
      }

      //- bgz: @theme, @layout, @video, @place, @box, @size, @color and @note: about the deck or the slide, not on it
      //
      // Only those eight: any other line that starts with an @ is a paragraph.
      // A name that is not one of the names gives the first, as no line would.
      if (line.str[0] == '@') {
         U64 word_end = 0;
         while (word_end < line.size && line.str[word_end] != ' ' && line.str[word_end] != '\t') word_end += 1;
         String8 word = Str8Prefix(line, word_end);
         String8 name = _Deck_Trim(Str8Skip(line, word_end));
         Rng1U64 line_range = R1U64((U64)(line.str - text.str), (U64)(line.str - text.str) + line.size);

         if (Str8Match(word, Str8Lit("@theme"), StringMatchFlag_CaseInsensitive)) {
            if (!deck->has_theme_line) {
               deck->has_theme_line = 1;
               deck->theme_line = line_range;
               for (U64 i = 0; i < Deck_Theme_COUNT; i += 1) {
                  if (Str8Match(name, Deck_ThemeName((Deck_Theme)i), StringMatchFlag_CaseInsensitive)) {
                     deck->theme = (Deck_Theme)i;
                  }
               }
            }
            continue;
         }
         if (Str8Match(word, Str8Lit("@note"), StringMatchFlag_CaseInsensitive)) {
            // A note with no slide to be on - above the first - is dropped.
            if (slide != 0 && name.size != 0) {
               Str8ListPush(arena, &slide->notes, name);
            }
            continue;
         }
         if (Str8Match(word, Str8Lit("@video"), StringMatchFlag_CaseInsensitive)) {
            // Its words, in any order. One that is not known is passed over.
            B32 loop = 0;
            B32 mute = 0;
            B32 manual = 0;
            for (U64 i = 0; i < name.size;) {
               while (i < name.size && (name.str[i] == ' ' || name.str[i] == '\t' || name.str[i] == ',')) i += 1;
               U64 option_start = i;
               while (i < name.size && name.str[i] != ' ' && name.str[i] != '\t' && name.str[i] != ',') i += 1;
               String8 option = Str8Substr(name, R1U64(option_start, i));
               loop |= Str8Match(option, Str8Lit("loop"), StringMatchFlag_CaseInsensitive);
               mute |= Str8Match(option, Str8Lit("mute"), StringMatchFlag_CaseInsensitive);
               manual |= Str8Match(option, Str8Lit("manual"), StringMatchFlag_CaseInsensitive);
            }
            // Several such lines on a slide add up.
            if (slide != 0) {
               slide->video_loop |= loop;
               slide->video_mute |= mute;
               slide->video_manual |= manual;
            } else {
               waiting_video_loop |= loop;
               waiting_video_mute |= mute;
               waiting_video_manual |= manual;
            }
            continue;
         }
         if (Str8Match(word, Str8Lit("@box"), StringMatchFlag_CaseInsensitive)) {
            // Another block of the body, if the one before has anything in
            // it: a slide's first lines are a block already.
            if (slide != 0 && slide->boxes[slide->box_count - 1].item_count != 0 && slide->box_count < DECK_MAX_BOXES) {
               slide->box_count += 1;
               slide->boxes[slide->box_count - 1].text_pos = line_start;
            }
            gap_before = 0;
            continue;
         }
         if (Str8Match(word, Str8Lit("@place"), StringMatchFlag_CaseInsensitive)) {
            // What is placed, then four numbers. A line that is not all of
            // that places nothing. "video" is the picture too.
            U64 kind_end = 0;
            while (kind_end < name.size && name.str[kind_end] != ' ' && name.str[kind_end] != '\t') kind_end += 1;
            String8 kind_name = Str8Prefix(name, kind_end);
            String8 numbers = Str8Skip(name, kind_end);

            Deck_PlaceKind kind = Deck_PlaceKind_COUNT;
            for (U64 i = 0; i < Deck_PlaceKind_COUNT; i += 1) {
               if (Str8Match(kind_name, Deck_PlaceName((Deck_PlaceKind)i), StringMatchFlag_CaseInsensitive)) {
                  kind = (Deck_PlaceKind)i;
               }
            }
            if (Str8Match(kind_name, Str8Lit("video"), StringMatchFlag_CaseInsensitive)) {
               kind = Deck_PlaceKind_Picture;
            }

            F32 x = 0, y = 0, w = 0, h = 0;
            if (kind != Deck_PlaceKind_COUNT && _Deck_TakeNumber(&numbers, &x) && _Deck_TakeNumber(&numbers, &y) &&
                _Deck_TakeNumber(&numbers, &w) && _Deck_TakeNumber(&numbers, &h)) {
               // Kept to sizes that can be seen, and near enough to the page
               // to be found again.
               w = Clamp(DECK_PLACE_MIN, w, 2.0f);
               h = Clamp(DECK_PLACE_MIN, h, 2.0f);
               x = Clamp(-1.0f, x, 1.0f);
               y = Clamp(-1.0f, y, 1.0f);

               // The first such line of a slide - or of a picture - is the
               // one that counts. A picture's is for the picture last
               // written above it.
               Deck_Place* place = (slide != 0) ? &slide->places[kind] : &waiting_places[kind];
               if (kind == Deck_PlaceKind_Text && slide != 0) {
                  place = &slide->boxes[slide->box_count - 1].place;
               }
               if (kind == Deck_PlaceKind_Picture) {
                  place = (slide != 0 && slide->picture_count != 0) ? &slide->pictures[slide->picture_count - 1].place
                                                                    : &waiting_picture_place;
               }
               if (!(kind == Deck_PlaceKind_Picture && last_picture_passed_over) && !place->is_set) {
                  place->is_set = 1;
                  place->rect = R2F32P(x, y, x + w, y + h);
                  place->has_line = 1;
                  place->line = line_range;
               }
            }
            continue;
         }
         B32 is_size = Str8Match(word, Str8Lit("@size"), StringMatchFlag_CaseInsensitive);
         B32 is_color = (Str8Match(word, Str8Lit("@color"), StringMatchFlag_CaseInsensitive) ||
                         Str8Match(word, Str8Lit("@colour"), StringMatchFlag_CaseInsensitive));
         if (is_size || is_color) {
            // Whose words - title, or text - then the size or the colour.
            // Without the first it is the text's. The first such line of a
            // title, or of a block, is the one that counts; one that does
            // not say a size or a colour changes nothing.
            U64 kind_end = 0;
            while (kind_end < name.size && name.str[kind_end] != ' ' && name.str[kind_end] != '\t') kind_end += 1;
            String8 kind_name = Str8Prefix(name, kind_end);
            String8 value = name;
            B32     is_title_look = Str8Match(kind_name, Str8Lit("title"), StringMatchFlag_CaseInsensitive);
            if (is_title_look || Str8Match(kind_name, Str8Lit("text"), StringMatchFlag_CaseInsensitive)) {
               value = _Deck_Trim(Str8Skip(name, kind_end));
            }

            Deck_Look* look = is_title_look ? &waiting_title_look : &waiting_text_look;
            if (slide != 0) {
               look = is_title_look ? &slide->title_look : &slide->boxes[slide->box_count - 1].look;
            }

            F32     size = 0;
            Vec4F32 color = {0};
            String8 size_rest = value;
            if (is_size && !look->has_size && _Deck_TakeNumber(&size_rest, &size) && _Deck_Trim(size_rest).size == 0) {
               look->has_size = 1;
               look->size = Clamp(DECK_SIZE_MIN, size, DECK_SIZE_MAX);
               look->size_line = line_range;
            }
            if (is_color && !look->has_color && Deck_ColorFromString(value, &color)) {
               look->has_color = 1;
               look->color = color;
               look->color_line = line_range;
            }
            continue;
         }
         if (Str8Match(word, Str8Lit("@layout"), StringMatchFlag_CaseInsensitive)) {
            Deck_Layout layout = Deck_Layout_PictureRight;
            for (U64 i = 0; i < Deck_Layout_COUNT; i += 1) {
               if (Str8Match(name, Deck_LayoutName((Deck_Layout)i), StringMatchFlag_CaseInsensitive)) {
                  layout = (Deck_Layout)i;
               }
            }
            if (slide != 0 && !slide->has_layout_line) {
               slide->layout = layout;
               slide->has_layout_line = 1;
               slide->layout_line = line_range;
            } else if (slide == 0 && !has_waiting_layout) {
               has_waiting_layout = 1;
               waiting_layout = layout;
               waiting_layout_line = line_range;
            }
            continue;
         }
      }

      U64 hash_count = 0;
      while (hash_count < line.size && line.str[hash_count] == '#') hash_count += 1;
      // The space after the #s can be left out: "#Title" is a title too. And
      // a # with nothing after it yet is a title being written: the slide
      // starts as soon as the # is typed.
      B32 is_heading = (hash_count > 0);
      B32 is_title = (is_heading && hash_count == 1);

      // A - with nothing after it yet is a bullet being written: it shows as
      // its dot, not as a paragraph that reads "-".
      B32 is_bullet = ((line.size >= 2 && (line.str[0] == '-' || line.str[0] == '*') && line.str[1] == ' ') ||
                       (line.size == 1 && (line.str[0] == '-' || line.str[0] == '*')));

      // ![anything](path) and nothing else on the line.
      String8 image_path = {0};
      if (line.size >= 5 && line.str[0] == '!' && line.str[1] == '[' && line.str[line.size - 1] == ')') {
         U64 path_start = Str8FindNeedle(line, 0, Str8Lit("]("), 0);
         if (path_start < line.size) {
            image_path = _Deck_Trim(Str8Substr(line, R1U64(path_start + 2, line.size - 1)));
         }
      }

      //- bgz: the slide it goes on
      if (slide == 0 || is_title) {
         _Deck_SlideNode* node = PushArray(scratch.arena, _Deck_SlideNode, 1);
         SLLQueuePush(first_node, last_node, node);
         node_count += 1;
         slide = &node->slide;
         slide->text_pos = (has_rule_pos && !is_title) ? rule_pos : line_start;
         gap_before = 0;
         if (is_title) {
            // What waited for a picture waited on the slide before.
            MemoryZeroStruct(&waiting_picture_place);
         }
         last_picture_passed_over = 0;

         if (has_waiting_layout) {
            slide->layout = waiting_layout;
            slide->has_layout_line = 1;
            slide->layout_line = waiting_layout_line;
         }
         slide->video_loop |= waiting_video_loop;
         slide->video_mute |= waiting_video_mute;
         slide->video_manual |= waiting_video_manual;
         MemoryCopyArray(slide->places, waiting_places);
         slide->box_count = 1;
         slide->boxes[0].place = waiting_places[Deck_PlaceKind_Text];
         slide->boxes[0].look = waiting_text_look;
         slide->boxes[0].text_pos = slide->text_pos;
         slide->title_look = waiting_title_look;
      }
      MemoryZeroArray(waiting_places);
      MemoryZeroStruct(&waiting_title_look);
      MemoryZeroStruct(&waiting_text_look);
      has_waiting_layout = 0;
      waiting_video_loop = 0;
      waiting_video_mute = 0;
      waiting_video_manual = 0;
      has_rule_pos = 0;

      //- bgz: add it
      //
      // Where a piece of the line is in the text: it is a piece of text, so
      // by where it starts. One with nothing in it is put at the line's end.
      U64 line_end_pos = (U64)(line.str - text.str) + line.size;
#define _Deck_RangeOf(piece) \
   (((piece).size != 0) ? R1U64((U64)((piece).str - text.str), (U64)((piece).str - text.str) + (piece).size) \
                        : R1U64(line_end_pos, line_end_pos))

      if (is_title) {
         slide->title = _Deck_Trim(Str8Skip(line, hash_count));
         slide->title_range = _Deck_RangeOf(slide->title);
      } else if (image_path.size != 0) {
         if (slide->pictures == 0) {
            slide->pictures = PushArray(arena, Deck_Picture, DECK_MAX_PICTURES);
         }
         last_picture_passed_over = (slide->picture_count >= DECK_MAX_PICTURES);
         slide->pictures_passed_over += last_picture_passed_over;
         if (slide->picture_count < DECK_MAX_PICTURES) {
            Deck_Picture* picture = &slide->pictures[slide->picture_count];
            slide->picture_count += 1;
            picture->range = _Deck_RangeOf(image_path);
            // A path that says where it starts from is used as it is.
            B32 is_absolute = (image_path.str[0] == '/' || image_path.str[0] == '\\' ||
                               (image_path.size >= 2 && image_path.str[1] == ':'));
            if (is_absolute) {
               picture->path = image_path;
            } else {
               picture->path = PushStr8F(arena, "%S\\%S", deck->folder, image_path);
            }
            if (slide->picture_count == 1) {
               picture->place = waiting_picture_place;
               MemoryZeroStruct(&waiting_picture_place);
            }
         }
      } else {
         Deck_Item* item = PushArray(arena, Deck_Item, 1);
         SLLQueuePush(slide->first_item, slide->last_item, item);
         slide->item_count += 1;
         Deck_Box* box = &slide->boxes[slide->box_count - 1];
         if (box->item_count == 0) {
            box->first_item = item;
            gap_before = 0; // a block's first line has nothing above it
         }
         box->item_count += 1;
         item->gap_before = gap_before;

         if (is_heading) {
            item->kind = Deck_ItemKind_Heading;
            item->text = _Deck_Trim(Str8Skip(line, hash_count));
         } else if (is_bullet) {
            item->kind = Deck_ItemKind_Bullet;
            item->text = _Deck_Trim(Str8Skip(line, 1));
            item->level = (U32)Min(indent / 2, DECK_MAX_BULLET_LEVEL);
         } else {
            item->kind = Deck_ItemKind_Text;
            item->text = line;
         }
         item->text_range = _Deck_RangeOf(item->text);
      }
#undef _Deck_RangeOf
      gap_before = 0;
   }

   deck->slide_count = node_count;
   deck->slides = PushArray(arena, Deck_Slide, Max(node_count, 1));
   U64 slide_idx = 0;
   for (_Deck_SlideNode* node = first_node; node != 0; node = node->next) {
      deck->slides[slide_idx] = node->slide;
      slide_idx += 1;
   }

   ScratchEnd(scratch);
   return deck;
}

U64
Deck_SlideFromTextPos(Deck_Deck* deck, U64 text_pos) {
   U64 result = 0;

   for (U64 i = 0; i < deck->slide_count; i += 1) {
      if (deck->slides[i].text_pos <= text_pos) {
         result = i;
      } else {
         break;
      }
   }

   return result;
}

// A picture's image goes from src to dst, with its video if it is one: a
// video goes on playing from where it is.
PRIVATE void
_Deck_MovePictureImage(Deck_Picture* dst, Deck_Picture* src) {
   dst->image = src->image;
   dst->video = src->video;
   MemoryZeroStruct(&src->image);
   src->video = 0;
}

// dst_picture is src_picture, as the deck was before: it gets what that had.
PRIVATE void
_Deck_TakeFromCounterpart(Deck_Picture* dst_picture, Deck_Picture* src_picture) {
   if (Image_IsLoaded(&src_picture->image)) {
      _Deck_MovePictureImage(dst_picture, src_picture);
   } else if (src_picture->load_failed) {
      // A file that could not be loaded is not tried again either.
      dst_picture->load_failed = 1;
   }
}

// Every picture of a deck's slides, in order, each with its slide's title.
typedef struct _Deck_PictureRef _Deck_PictureRef;
struct _Deck_PictureRef {
   Deck_Picture* picture;
   String8       title;
   U32           matched; // 0 not, 1 by title, 2 by order
};

PRIVATE _Deck_PictureRef*
_Deck_PictureRefs(Arena* arena, Deck_Deck* deck, U64* count_out) {
   U64 count = 0;
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      count += deck->slides[i].picture_count;
   }
   _Deck_PictureRef* refs = PushArray(arena, _Deck_PictureRef, count + 1);
   U64               at = 0;
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      for (U64 k = 0; k < deck->slides[i].picture_count; k += 1) {
         refs[at].picture = &deck->slides[i].pictures[k];
         refs[at].title = deck->slides[i].title;
         at += 1;
      }
   }
   *count_out = count;
   return refs;
}

void
Deck_TakeImages(Deck_Deck* dst, Deck_Deck* src) {
   Temp scratch = ScratchBegin(0, 0);

   // Several slides can show the same file - and one slide the same file
   // twice - and each picture must get what it had itself: not what its
   // neighbour had while the neighbour - on the slide showing, perhaps,
   // with its video playing - is left to load again from the start. So
   // first each picture of dst is matched with the one of src it is, and
   // takes only from that one: what that one did not have loaded, it does
   // not have either. Only a picture that matches none takes from what is
   // left over.
   U64               dst_count = 0;
   U64               src_count = 0;
   _Deck_PictureRef* dst_refs = _Deck_PictureRefs(scratch.arena, dst, &dst_count);
   _Deck_PictureRef* src_refs = _Deck_PictureRefs(scratch.arena, src, &src_count);

   //- bgz: the same file on a slide of the same title: the same picture
   //
   // Whatever was added, taken away or moved around it.
   for (U64 i = 0; i < dst_count; i += 1) {
      for (U64 j = 0; j < src_count; j += 1) {
         if (!src_refs[j].matched && Str8Match(src_refs[j].picture->path, dst_refs[i].picture->path, 0) &&
             Str8Match(src_refs[j].title, dst_refs[i].title, 0)) {
            dst_refs[i].matched = 1;
            src_refs[j].matched = 1;
            _Deck_TakeFromCounterpart(dst_refs[i].picture, src_refs[j].picture);
            break;
         }
      }
   }

   //- bgz: then by order, among the pictures of a file not matched yet
   //
   // A title being typed: the first of them left in dst is the first of
   // them left in src, the second the second, and so on.
   for (U64 i = 0; i < dst_count; i += 1) {
      if (dst_refs[i].matched) {
         continue;
      }
      // 1 is matched by title; 2, in this pass, is still counted on both
      // sides, so that nth means the same in dst and in src.
      U64 nth = 0;
      for (U64 k = 0; k < i; k += 1) {
         if (dst_refs[k].matched != 1 && Str8Match(dst_refs[k].picture->path, dst_refs[i].picture->path, 0)) {
            nth += 1;
         }
      }
      for (U64 j = 0, seen = 0; j < src_count; j += 1) {
         if (src_refs[j].matched == 1 || !Str8Match(src_refs[j].picture->path, dst_refs[i].picture->path, 0)) {
            continue;
         }
         if (seen == nth) {
            dst_refs[i].matched = 2;
            src_refs[j].matched = 2;
            _Deck_TakeFromCounterpart(dst_refs[i].picture, src_refs[j].picture);
            break;
         }
         seen += 1;
      }
   }

   //- bgz: then what is left over
   //
   // A picture that is in dst but was not in src - one more of a file than
   // src had, or one that was gone for an edit or two and is back - takes
   // an image of its file that no picture of src took: from a picture of
   // src not matched, and then from src's spares.
   for (U64 i = 0; i < dst_count; i += 1) {
      Deck_Picture* dst_picture = dst_refs[i].picture;
      if (dst_refs[i].matched || Image_IsLoaded(&dst_picture->image) || dst_picture->load_failed) {
         continue;
      }
      for (U64 j = 0; j < src_count + src->spare_count; j += 1) {
         Deck_Picture* src_picture = (j < src_count) ? src_refs[j].picture : &src->spares[j - src_count];
         if (Image_IsLoaded(&src_picture->image) && Str8Match(src_picture->path, dst_picture->path, 0)) {
            _Deck_MovePictureImage(dst_picture, src_picture);
            break;
         }
      }
   }

   //- bgz: and what no picture took, kept for a while
   //
   // An image of a file that a slide of dst still shows: its own slide may
   // be back at the next key. The rest stays in src, and goes with it.
   U64 spare_count = 0;
   for (U64 j = 0; j < src_count + src->spare_count; j += 1) {
      Deck_Picture* src_picture = (j < src_count) ? src_refs[j].picture : &src->spares[j - src_count];
      if (Image_IsLoaded(&src_picture->image)) {
         spare_count += 1;
      }
   }
   spare_count = Min(spare_count, DECK_SPARE_MAX);
   if (spare_count != 0) {
      dst->spares = PushArray(dst->arena, Deck_Picture, spare_count);
   }
   for (U64 j = 0; j < src_count + src->spare_count && dst->spare_count < spare_count; j += 1) {
      Deck_Picture* src_picture = (j < src_count) ? src_refs[j].picture : &src->spares[j - src_count];
      if (!Image_IsLoaded(&src_picture->image)) {
         continue;
      }
      B32 is_shown = 0;
      for (U64 i = 0; i < dst_count && !is_shown; i += 1) {
         is_shown = Str8Match(dst_refs[i].picture->path, src_picture->path, 0);
      }
      if (is_shown) {
         Deck_Picture* spare = &dst->spares[dst->spare_count];
         dst->spare_count += 1;
         spare->path = PushStr8Copy(dst->arena, src_picture->path);
         spare->spared_us = (j < src_count) ? NowTimeUS() : src_picture->spared_us;
         _Deck_MovePictureImage(spare, src_picture);
      }
   }

   ScratchEnd(scratch);
}

B32
Deck_SlideHasText(Deck_Slide* slide) {
   B32 result = (slide->title.size != 0 || slide->item_count != 0);
   return result;
}

////////////////////////////////
//~ bgz: decks of either kind

Deck_Deck*
Deck_OpenPath(String8 path, U64* first_slide_out) {
   Deck_Deck* deck = 0;
   U64        first_slide = 0;

   // No trailing slash, so that the last part of the path is its name.
   while (path.size > 1 && (path.str[path.size - 1] == '/' || path.str[path.size - 1] == '\\')) {
      path.size -= 1;
   }

   FileProperties properties = PropertiesFromFilePath(path);
   B32            is_folder = !!(properties.flags & FilePropertyFlag_IsFolder);

   if (is_folder) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 slides_file = PushStr8F(scratch.arena, "%S\\slides.md", path);
      if (FilePathExists(slides_file)) {
         deck = Deck_OpenFile(slides_file);
      } else {
         deck = Deck_OpenFolder(path);
      }
      ScratchEnd(scratch);
   } else if (Deck_NameIsTextDeck(path)) {
      deck = Deck_OpenFile(path);
   } else if (Image_NameIsImage(path)) {
      // A file with nothing before its name is in the folder the program was
      // started from.
      String8 folder = Str8ChopLastSlash(path);
      String8 name = Str8SkipLastSlash(path);
      if (folder.size == 0) {
         folder = Str8Lit(".");
      }

      deck = Deck_OpenFolder(folder);
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         if (Str8Match(Str8SkipLastSlash(deck->slides[i].pictures[0].path), name, StringMatchFlag_CaseInsensitive)) {
            first_slide = i;
            break;
         }
      }
   } else {
      // Nothing that can be shown - or nothing at all - which as a folder
      // gives the deck with no slides.
      deck = Deck_OpenFolder(path);
   }

   *first_slide_out = first_slide;
   return deck;
}

////////////////////////////////
//~ bgz: paths that stand for others (Deck_SetPathAlias)
//
// A few at most, at any time: a list, looked through. One that is cleared
// leaves its place, with an empty path, for the next.

typedef struct _Deck_Alias _Deck_Alias;
struct _Deck_Alias {
   _Deck_Alias* next;
   U8           path[1024];
   U64          path_size; // 0: not in use
   U8           stands_for[1024];
   U64          stands_for_size;
};

PRIVATE Arena*       _deck_alias_arena = 0;
PRIVATE _Deck_Alias* _deck_first_alias = 0;

void
Deck_ClearPathAlias(String8 path) {
   for (_Deck_Alias* alias = _deck_first_alias; alias != 0; alias = alias->next) {
      if (alias->path_size != 0 && Str8Match(Str8(alias->path, alias->path_size), path, StringMatchFlag_CaseInsensitive)) {
         alias->path_size = 0;
      }
   }
}

void
Deck_SetPathAlias(String8 path, String8 stands_for) {
   Deck_ClearPathAlias(path);
   if (path.size == 0 || path.size > sizeof(((_Deck_Alias*)0)->path) ||
       stands_for.size > sizeof(((_Deck_Alias*)0)->stands_for)) {
      return; // too long to keep: the path is only itself
   }

   _Deck_Alias* alias = 0;
   for (_Deck_Alias* a = _deck_first_alias; a != 0; a = a->next) {
      if (a->path_size == 0) {
         alias = a;
         break;
      }
   }
   if (alias == 0) {
      if (_deck_alias_arena == 0) {
         _deck_alias_arena = ArenaAlloc();
      }
      alias = PushArray(_deck_alias_arena, _Deck_Alias, 1);
      alias->next = _deck_first_alias;
      _deck_first_alias = alias;
   }
   MemoryCopy(alias->path, path.str, path.size);
   alias->path_size = path.size;
   MemoryCopy(alias->stands_for, stands_for.str, stands_for.size);
   alias->stands_for_size = stands_for.size;
}

// The file to read for a picture at path: path itself, or what it stands for.
PRIVATE String8
_Deck_FileFromPath(String8 path) {
   String8 result = path;
   for (_Deck_Alias* alias = _deck_first_alias; alias != 0; alias = alias->next) {
      if (alias->path_size != 0 && Str8Match(Str8(alias->path, alias->path_size), path, StringMatchFlag_CaseInsensitive)) {
         result = Str8(alias->stands_for, alias->stands_for_size);
         break;
      }
   }
   return result;
}

// The picture, loaded if it is not and can be: an image decoded, a video
// opened, paused at its start.
PRIVATE void
_Deck_LoadPicture(Deck_Picture* picture) {
   if (!Image_IsLoaded(&picture->image) && !picture->load_failed && picture->path.size != 0) {
      if (Video_NameIsVideo(picture->path)) {
         picture->video = Video_Open(_Deck_FileFromPath(picture->path));
         if (picture->video != 0) {
            picture->image.texture = Video_Texture(picture->video);
            picture->image.size = Video_Size(picture->video);
         }
      } else {
         picture->image = Image_LoadFromPath(picture->path);
      }
      picture->load_failed = !Image_IsLoaded(&picture->image);
   }
}

PRIVATE void
_Deck_ReleasePicture(Deck_Picture* picture) {
   if (picture->video != 0) {
      // The texture is the player's, and goes with it.
      Video_Close(picture->video);
      picture->video = 0;
      MemoryZeroStruct(&picture->image);
   } else {
      Image_Release(&picture->image);
   }
}

void
Deck_ReopenPicture(Deck_Picture* picture) {
   _Deck_ReleasePicture(picture);
   picture->load_failed = 0;
   _Deck_LoadPicture(picture);
}

U64
Deck_VideosFromSlide(Deck_Deck* deck, U64 slide_idx, Video_Video** videos) {
   U64 count = 0;
   if (slide_idx < deck->slide_count) {
      Deck_Slide* slide = &deck->slides[slide_idx];
      for (U64 k = 0; k < slide->picture_count && count < DECK_MAX_PICTURES; k += 1) {
         if (slide->pictures[k].video != 0) {
            videos[count] = slide->pictures[k].video;
            count += 1;
         }
      }
   }
   return count;
}

Video_Video*
Deck_VideoFromSlide(Deck_Deck* deck, U64 slide_idx) {
   Video_Video* videos[DECK_MAX_PICTURES] = {0};
   Video_Video* result = (Deck_VideosFromSlide(deck, slide_idx, videos) != 0) ? videos[0] : 0;
   return result;
}

B32
Deck_SlideHasVideo(Deck_Deck* deck, U64 slide_idx) {
   B32 result = 0;
   if (slide_idx < deck->slide_count) {
      Deck_Slide* slide = &deck->slides[slide_idx];
      for (U64 k = 0; k < slide->picture_count && !result; k += 1) {
         result = (!slide->pictures[k].load_failed && Video_NameIsVideo(slide->pictures[k].path));
      }
   }
   return result;
}

B32
Deck_SlideVideosAreOpen(Deck_Deck* deck, U64 slide_idx) {
   B32 result = 1;
   if (slide_idx < deck->slide_count) {
      Deck_Slide* slide = &deck->slides[slide_idx];
      for (U64 k = 0; k < slide->picture_count && result; k += 1) {
         Deck_Picture* picture = &slide->pictures[k];
         if (!picture->load_failed && Video_NameIsVideo(picture->path) && picture->video == 0) {
            result = 0;
         }
      }
   }
   return result;
}

void
Deck_UpdateVideos(Deck_Deck* deck) {
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      for (U64 k = 0; k < deck->slides[i].picture_count; k += 1) {
         if (deck->slides[i].pictures[k].video != 0) {
            Video_Update(deck->slides[i].pictures[k].video);
         }
      }
   }
   // A spare's video keeps its time too, for when its slide is back. A
   // spare whose slide has not come back by now is not kept any longer.
   U64 now_us = NowTimeUS();
   for (U64 i = 0; i < deck->spare_count; i += 1) {
      Deck_Picture* spare = &deck->spares[i];
      if (Image_IsLoaded(&spare->image) && now_us - spare->spared_us > (U64)(DECK_SPARE_SECONDS * 1000000.0)) {
         _Deck_ReleasePicture(spare);
      }
      if (spare->video != 0) {
         Video_Update(spare->video);
      }
   }
}

void
Deck_Close(Deck_Deck* deck) {
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      for (U64 k = 0; k < deck->slides[i].picture_count; k += 1) {
         _Deck_ReleasePicture(&deck->slides[i].pictures[k]);
      }
   }
   for (U64 i = 0; i < deck->spare_count; i += 1) {
      _Deck_ReleasePicture(&deck->spares[i]);
   }
   ArenaRelease(deck->arena);
}

void
Deck_LoadSlideOnly(Deck_Deck* deck, U64 slide_idx) {
   if (slide_idx < deck->slide_count) {
      Deck_Slide* slide = &deck->slides[slide_idx];
      for (U64 k = 0; k < slide->picture_count; k += 1) {
         _Deck_LoadPicture(&slide->pictures[k]);
      }
   }
}

void
Deck_LoadSlide(Deck_Deck* deck, U64 slide_idx) {
   if (slide_idx < deck->slide_count) {
      // Everything but this slide and its two neighbours goes. The neighbours
      // stay if they were shown, so going back a slide does not decode again.
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         B32 is_near = (i + 1 >= slide_idx && i <= slide_idx + 1);
         for (U64 k = 0; k < deck->slides[i].picture_count && !is_near; k += 1) {
            _Deck_ReleasePicture(&deck->slides[i].pictures[k]);
         }
      }
      Deck_LoadSlideOnly(deck, slide_idx);
   }
}

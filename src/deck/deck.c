////////////////////////////////
//~ bgz: deck
//
// See deck.h.

// File names in the order a person expects: letters without regard to case,
// and a run of digits as the number it spells, so "slide2" is before
// "slide10". Negative, zero or positive, as strcmp.
internal S32
_DECK_CompareNames(String8 a, String8 b) {
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
internal void
_DECK_SortNames(String8Array names) {
   for (U64 i = 1; i < names.count; i += 1) {
      String8 name = names.v[i];
      U64     slot = i;
      while (slot > 0 && _DECK_CompareNames(name, names.v[slot - 1]) < 0) {
         names.v[slot] = names.v[slot - 1];
         slot -= 1;
      }
      names.v[slot] = name;
   }
}

internal String8Array
DECK_PictureNamesFromFolder(Arena* arena, String8 folder) {
   String8List names = {0};
   Temp        scratch = ScratchBegin(&arena, 1);

   if (folder.size != 0) {
      // The folder's own images.
      FileIter* it = FileIterBegin(scratch.arena, folder, FileIterFlag_SkipFolders | FileIterFlag_SkipHiddenFiles);
      for (FileInfo info = {0}; FileIterNext(scratch.arena, it, &info);) {
         if ((IMG_NameIsImage(info.name) || VID_NameIsVideo(info.name))) {
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
            if ((IMG_NameIsImage(info.name) || VID_NameIsVideo(info.name))) {
               Str8ListPush(arena, &names, PushStr8F(arena, "%S/%S", node->string, info.name));
            }
         }
         FileIterEnd(it);
      }
   }

   String8Array result = Str8ArrayFromList(arena, &names);
   _DECK_SortNames(result);

   ScratchEnd(scratch);
   return result;
}

internal DECK_Deck*
DECK_OpenFolder(String8 folder) {
   Arena*     arena = ArenaAlloc();
   DECK_Deck* deck = PushArray(arena, DECK_Deck, 1);
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
         if (IMG_NameIsImage(info.name)) {
            Str8ListPush(scratch.arena, &names, info.name);
         }
      }
      FileIterEnd(it);

      String8Array sorted = Str8ArrayFromList(scratch.arena, &names);

      _DECK_SortNames(sorted);

      deck->slide_count = sorted.count;
      deck->slides = PushArray(arena, DECK_Slide, Max(sorted.count, 1));
      for (U64 i = 0; i < sorted.count; i += 1) {
         deck->slides[i].image_path = PushStr8F(arena, "%S\\%S", deck->folder, sorted.v[i]);
      }

      ScratchEnd(scratch);
   }

   return deck;
}

////////////////////////////////
//~ bgz: decks from text files

internal B32
_DECK_NameIsTextDeck(String8 name) {
   String8 extension = Str8SkipLastDot(name);
   B32     result = (Str8Match(extension, Str8Lit("md"), StringMatchFlag_CaseInsensitive) ||
                 Str8Match(extension, Str8Lit("txt"), StringMatchFlag_CaseInsensitive));
   return result;
}

// A slide being read: in a list, since how many there are is not known until
// the end of the file.
typedef struct _DECK_SlideNode _DECK_SlideNode;
struct _DECK_SlideNode {
   _DECK_SlideNode* next;
   DECK_Slide       slide;
};

// The string without the spaces and tabs at either end.
internal String8
_DECK_Trim(String8 string) {
   while (string.size > 0 && (string.str[0] == ' ' || string.str[0] == '\t')) {
      string.str += 1;
      string.size -= 1;
   }
   while (string.size > 0 && (string.str[string.size - 1] == ' ' || string.str[string.size - 1] == '\t')) {
      string.size -= 1;
   }
   return string;
}

internal DECK_Deck*
DECK_OpenFile(String8 file) {
   Temp scratch = ScratchBegin(0, 0);

   String8 text = DataFromFilePath(scratch.arena, file);

   // A byte order mark, which Notepad puts at the start of a UTF-8 file.
   if (text.size >= 3 && text.str[0] == 0xEF && text.str[1] == 0xBB && text.str[2] == 0xBF) {
      text = Str8Skip(text, 3);
   }

   // The \r of a Windows line ending is taken off each line as it is read.
   DECK_Deck* deck = DECK_OpenText(text, file);

   ScratchEnd(scratch);
   return deck;
}

internal String8
DECK_ThemeName(DECK_Theme theme) {
   String8 names[DECK_Theme_COUNT] = {Str8Lit("dark"), Str8Lit("light"), Str8Lit("blue"), Str8Lit("paper")};
   return names[(theme < DECK_Theme_COUNT) ? theme : 0];
}

internal String8
DECK_LayoutName(DECK_Layout layout) {
   String8 names[DECK_Layout_COUNT] = {Str8Lit("picture-right"), Str8Lit("picture-left"), Str8Lit("picture-behind"),
                                       Str8Lit("center")};
   return names[(layout < DECK_Layout_COUNT) ? layout : 0];
}

internal DECK_Deck*
DECK_OpenText(String8 text_to_copy, String8 file) {
   Arena*     arena = ArenaAlloc();
   DECK_Deck* deck = PushArray(arena, DECK_Deck, 1);
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

   _DECK_SlideNode* first_node = 0;
   _DECK_SlideNode* last_node = 0;
   U64              node_count = 0;

   // The slide lines are being added to. 0 at the start and after a ---: the
   // next line with anything on it starts a new slide.
   DECK_Slide* slide = 0;
   B32         gap_before = 0;

   // Where the last --- line began, for the slide it starts: that begins at
   // the rule, not at the first line after it with something on it.
   B32 has_rule_pos = 0;
   U64 rule_pos = 0;

   // An @layout line that came before its slide had anything on it - right
   // under a --- - waits here for the slide.
   B32         has_waiting_layout = 0;
   DECK_Layout waiting_layout = DECK_Layout_PictureRight;
   Rng1U64     waiting_layout_line = {0};

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
      line = _DECK_Trim(line);

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
         has_rule_pos = 1;
         rule_pos = line_start;
         continue;
      }

      //- bgz: @theme, @layout and @note: about the deck or the slide, not on it
      //
      // Only those three: any other line that starts with an @ is a paragraph.
      // A name that is not one of the names gives the first, as no line would.
      if (line.str[0] == '@') {
         U64 word_end = 0;
         while (word_end < line.size && line.str[word_end] != ' ' && line.str[word_end] != '\t') word_end += 1;
         String8 word = Str8Prefix(line, word_end);
         String8 name = _DECK_Trim(Str8Skip(line, word_end));
         Rng1U64 line_range = R1U64((U64)(line.str - text.str), (U64)(line.str - text.str) + line.size);

         if (Str8Match(word, Str8Lit("@theme"), StringMatchFlag_CaseInsensitive)) {
            if (!deck->has_theme_line) {
               deck->has_theme_line = 1;
               deck->theme_line = line_range;
               for (U64 i = 0; i < DECK_Theme_COUNT; i += 1) {
                  if (Str8Match(name, DECK_ThemeName((DECK_Theme)i), StringMatchFlag_CaseInsensitive)) {
                     deck->theme = (DECK_Theme)i;
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
         if (Str8Match(word, Str8Lit("@layout"), StringMatchFlag_CaseInsensitive)) {
            DECK_Layout layout = DECK_Layout_PictureRight;
            for (U64 i = 0; i < DECK_Layout_COUNT; i += 1) {
               if (Str8Match(name, DECK_LayoutName((DECK_Layout)i), StringMatchFlag_CaseInsensitive)) {
                  layout = (DECK_Layout)i;
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
            image_path = _DECK_Trim(Str8Substr(line, R1U64(path_start + 2, line.size - 1)));
         }
      }

      //- bgz: the slide it goes on
      if (slide == 0 || is_title) {
         _DECK_SlideNode* node = PushArray(scratch.arena, _DECK_SlideNode, 1);
         SLLQueuePush(first_node, last_node, node);
         node_count += 1;
         slide = &node->slide;
         slide->text_pos = (has_rule_pos && !is_title) ? rule_pos : line_start;
         gap_before = 0;

         if (has_waiting_layout) {
            slide->layout = waiting_layout;
            slide->has_layout_line = 1;
            slide->layout_line = waiting_layout_line;
         }
      }
      has_waiting_layout = 0;
      has_rule_pos = 0;

      //- bgz: add it
      //
      // Where a piece of the line is in the text: it is a piece of text, so
      // by where it starts. One with nothing in it is put at the line's end.
      U64 line_end_pos = (U64)(line.str - text.str) + line.size;
#define _DECK_RangeOf(piece) \
   (((piece).size != 0) ? R1U64((U64)((piece).str - text.str), (U64)((piece).str - text.str) + (piece).size) \
                        : R1U64(line_end_pos, line_end_pos))

      if (is_title) {
         slide->title = _DECK_Trim(Str8Skip(line, hash_count));
         slide->title_range = _DECK_RangeOf(slide->title);
      } else if (image_path.size != 0) {
         if (slide->image_path.size == 0) {
            slide->image_range = _DECK_RangeOf(image_path);
            // A path that says where it starts from is used as it is.
            B32 is_absolute = (image_path.str[0] == '/' || image_path.str[0] == '\\' ||
                               (image_path.size >= 2 && image_path.str[1] == ':'));
            if (is_absolute) {
               slide->image_path = image_path;
            } else {
               slide->image_path = PushStr8F(arena, "%S\\%S", deck->folder, image_path);
            }
         }
      } else {
         DECK_Item* item = PushArray(arena, DECK_Item, 1);
         SLLQueuePush(slide->first_item, slide->last_item, item);
         slide->item_count += 1;
         item->gap_before = gap_before;

         if (is_heading) {
            item->kind = DECK_ItemKind_Heading;
            item->text = _DECK_Trim(Str8Skip(line, hash_count));
         } else if (is_bullet) {
            item->kind = DECK_ItemKind_Bullet;
            item->text = _DECK_Trim(Str8Skip(line, 1));
            item->level = (U32)Min(indent / 2, DECK_MAX_BULLET_LEVEL);
         } else {
            item->kind = DECK_ItemKind_Text;
            item->text = line;
         }
         item->text_range = _DECK_RangeOf(item->text);
      }
#undef _DECK_RangeOf
      gap_before = 0;
   }

   deck->slide_count = node_count;
   deck->slides = PushArray(arena, DECK_Slide, Max(node_count, 1));
   U64 slide_idx = 0;
   for (_DECK_SlideNode* node = first_node; node != 0; node = node->next) {
      deck->slides[slide_idx] = node->slide;
      slide_idx += 1;
   }

   ScratchEnd(scratch);
   return deck;
}

internal U64
DECK_SlideFromTextPos(DECK_Deck* deck, U64 text_pos) {
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

internal void
DECK_TakeImages(DECK_Deck* dst, DECK_Deck* src) {
   for (U64 i = 0; i < dst->slide_count; i += 1) {
      DECK_Slide* dst_slide = &dst->slides[i];
      if (dst_slide->image_path.size == 0 || IMG_IsLoaded(&dst_slide->image)) {
         continue;
      }

      for (U64 j = 0; j < src->slide_count; j += 1) {
         DECK_Slide* src_slide = &src->slides[j];
         if (!Str8Match(src_slide->image_path, dst_slide->image_path, 0)) {
            continue;
         }
         if (IMG_IsLoaded(&src_slide->image)) {
            dst_slide->image = src_slide->image;
            dst_slide->video = src_slide->video; // a video goes on playing from where it is
            MemoryZeroStruct(&src_slide->image);
            src_slide->video = 0;
            break;
         }
         // A file that could not be loaded is not tried again either.
         if (src_slide->load_failed) {
            dst_slide->load_failed = 1;
         }
      }
   }
}

internal B32
DECK_SlideHasText(DECK_Slide* slide) {
   B32 result = (slide->title.size != 0 || slide->item_count != 0);
   return result;
}

////////////////////////////////
//~ bgz: decks of either kind

internal DECK_Deck*
DECK_OpenPath(String8 path, U64* first_slide_out) {
   DECK_Deck* deck = 0;
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
         deck = DECK_OpenFile(slides_file);
      } else {
         deck = DECK_OpenFolder(path);
      }
      ScratchEnd(scratch);
   } else if (_DECK_NameIsTextDeck(path)) {
      deck = DECK_OpenFile(path);
   } else if (IMG_NameIsImage(path)) {
      // A file with nothing before its name is in the folder the program was
      // started from.
      String8 folder = Str8ChopLastSlash(path);
      String8 name = Str8SkipLastSlash(path);
      if (folder.size == 0) {
         folder = Str8Lit(".");
      }

      deck = DECK_OpenFolder(folder);
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         if (Str8Match(Str8SkipLastSlash(deck->slides[i].image_path), name, StringMatchFlag_CaseInsensitive)) {
            first_slide = i;
            break;
         }
      }
   } else {
      // Nothing that can be shown - or nothing at all - which as a folder
      // gives the deck with no slides.
      deck = DECK_OpenFolder(path);
   }

   *first_slide_out = first_slide;
   return deck;
}

////////////////////////////////
//~ bgz: paths that stand for others (DECK_SetPathAlias)
//
// A few at most, at any time: a list, looked through. One that is cleared
// leaves its place, with an empty path, for the next.

typedef struct _DECK_Alias _DECK_Alias;
struct _DECK_Alias {
   _DECK_Alias* next;
   U8           path[1024];
   U64          path_size; // 0: not in use
   U8           stands_for[1024];
   U64          stands_for_size;
};

global Arena*       _deck_alias_arena = 0;
global _DECK_Alias* _deck_first_alias = 0;

internal void
DECK_ClearPathAlias(String8 path) {
   for (_DECK_Alias* alias = _deck_first_alias; alias != 0; alias = alias->next) {
      if (alias->path_size != 0 && Str8Match(Str8(alias->path, alias->path_size), path, StringMatchFlag_CaseInsensitive)) {
         alias->path_size = 0;
      }
   }
}

internal void
DECK_SetPathAlias(String8 path, String8 stands_for) {
   DECK_ClearPathAlias(path);
   if (path.size == 0 || path.size > sizeof(((_DECK_Alias*)0)->path) ||
       stands_for.size > sizeof(((_DECK_Alias*)0)->stands_for)) {
      return; // too long to keep: the path is only itself
   }

   _DECK_Alias* alias = 0;
   for (_DECK_Alias* a = _deck_first_alias; a != 0; a = a->next) {
      if (a->path_size == 0) {
         alias = a;
         break;
      }
   }
   if (alias == 0) {
      if (_deck_alias_arena == 0) {
         _deck_alias_arena = ArenaAlloc();
      }
      alias = PushArray(_deck_alias_arena, _DECK_Alias, 1);
      alias->next = _deck_first_alias;
      _deck_first_alias = alias;
   }
   MemoryCopy(alias->path, path.str, path.size);
   alias->path_size = path.size;
   MemoryCopy(alias->stands_for, stands_for.str, stands_for.size);
   alias->stands_for_size = stands_for.size;
}

// The file to read for a picture at path: path itself, or what it stands for.
internal String8
_DECK_FileFromPath(String8 path) {
   String8 result = path;
   for (_DECK_Alias* alias = _deck_first_alias; alias != 0; alias = alias->next) {
      if (alias->path_size != 0 && Str8Match(Str8(alias->path, alias->path_size), path, StringMatchFlag_CaseInsensitive)) {
         result = Str8(alias->stands_for, alias->stands_for_size);
         break;
      }
   }
   return result;
}

// The slide's picture, loaded if it is not and can be: an image decoded, a
// video opened, paused at its start.
internal void
_DECK_LoadSlideImage(DECK_Slide* slide) {
   if (!IMG_IsLoaded(&slide->image) && !slide->load_failed && slide->image_path.size != 0) {
      if (VID_NameIsVideo(slide->image_path)) {
         slide->video = VID_Open(_DECK_FileFromPath(slide->image_path));
         if (slide->video != 0) {
            slide->image.texture = VID_Texture(slide->video);
            slide->image.size = VID_Size(slide->video);
         }
      } else {
         slide->image = IMG_LoadFromPath(slide->image_path);
      }
      slide->load_failed = !IMG_IsLoaded(&slide->image);
   }
}

internal void
_DECK_ReleaseSlideImage(DECK_Slide* slide) {
   if (slide->video != 0) {
      // The texture is the player's, and goes with it.
      VID_Close(slide->video);
      slide->video = 0;
      MemoryZeroStruct(&slide->image);
   } else {
      IMG_Release(&slide->image);
   }
}

internal void
DECK_ReopenImage(DECK_Deck* deck, U64 slide_idx) {
   if (slide_idx < deck->slide_count) {
      DECK_Slide* slide = &deck->slides[slide_idx];
      _DECK_ReleaseSlideImage(slide);
      slide->load_failed = 0;
      _DECK_LoadSlideImage(slide);
   }
}

internal VID_Video*
DECK_VideoFromSlide(DECK_Deck* deck, U64 slide_idx) {
   VID_Video* result = (slide_idx < deck->slide_count) ? deck->slides[slide_idx].video : 0;
   return result;
}

internal B32
DECK_SlideHasVideo(DECK_Deck* deck, U64 slide_idx) {
   B32 result = 0;
   if (slide_idx < deck->slide_count) {
      DECK_Slide* slide = &deck->slides[slide_idx];
      result = (slide->image_path.size != 0 && !slide->load_failed && VID_NameIsVideo(slide->image_path));
   }
   return result;
}

internal void
DECK_UpdateVideos(DECK_Deck* deck) {
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      if (deck->slides[i].video != 0) {
         VID_Update(deck->slides[i].video);
      }
   }
}

internal void
DECK_Close(DECK_Deck* deck) {
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      _DECK_ReleaseSlideImage(&deck->slides[i]);
   }
   ArenaRelease(deck->arena);
}

internal IMG_Image*
DECK_ImageFromSlide(DECK_Deck* deck, U64 slide_idx) {
   IMG_Image* result = 0;

   if (slide_idx < deck->slide_count) {
      // Everything but this slide and its two neighbours goes. The neighbours
      // stay if they were shown, so going back a slide does not decode again.
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         B32 is_near = (i + 1 >= slide_idx && i <= slide_idx + 1);
         if (!is_near) {
            _DECK_ReleaseSlideImage(&deck->slides[i]);
         }
      }

      DECK_Slide* slide = &deck->slides[slide_idx];
      _DECK_LoadSlideImage(slide);
      if (IMG_IsLoaded(&slide->image)) {
         result = &slide->image;
      }
   }

   return result;
}

internal IMG_Image*
DECK_LoadImageOfSlide(DECK_Deck* deck, U64 slide_idx) {
   IMG_Image* result = 0;

   if (slide_idx < deck->slide_count) {
      DECK_Slide* slide = &deck->slides[slide_idx];
      _DECK_LoadSlideImage(slide);
      if (IMG_IsLoaded(&slide->image)) {
         result = &slide->image;
      }
   }

   return result;
}

internal IMG_Image*
DECK_LoadedImageFromSlide(DECK_Deck* deck, U64 slide_idx) {
   IMG_Image* result = 0;

   if (slide_idx < deck->slide_count && IMG_IsLoaded(&deck->slides[slide_idx].image)) {
      result = &deck->slides[slide_idx].image;
   }

   return result;
}

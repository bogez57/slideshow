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

      // Insertion sort: a folder of slides is small, and the file system
      // mostly hands them over in order already.
      for (U64 i = 1; i < sorted.count; i += 1) {
         String8 name = sorted.v[i];
         U64     slot = i;
         while (slot > 0 && _DECK_CompareNames(name, sorted.v[slot - 1]) < 0) {
            sorted.v[slot] = sorted.v[slot - 1];
            slot -= 1;
         }
         sorted.v[slot] = name;
      }

      deck->slide_count = sorted.count;
      deck->slides = PushArray(arena, DECK_Slide, Max(sorted.count, 1));
      for (U64 i = 0; i < sorted.count; i += 1) {
         deck->slides[i].path = PushStr8F(arena, "%S\\%S", deck->folder, sorted.v[i]);
      }

      ScratchEnd(scratch);
   }

   return deck;
}

internal DECK_Deck*
DECK_OpenPath(String8 path, U64* first_slide_out) {
   DECK_Deck* deck = 0;
   U64        first_slide = 0;

   FileProperties properties = PropertiesFromFilePath(path);
   B32            is_folder = !!(properties.flags & FilePropertyFlag_IsFolder);

   if (!is_folder && IMG_NameIsImage(path)) {
      // A file with nothing before its name is in the folder the program was
      // started from.
      String8 folder = Str8ChopLastSlash(path);
      String8 name = Str8SkipLastSlash(path);
      if (folder.size == 0) {
         folder = Str8Lit(".");
      }

      deck = DECK_OpenFolder(folder);
      for (U64 i = 0; i < deck->slide_count; i += 1) {
         if (Str8Match(Str8SkipLastSlash(deck->slides[i].path), name, StringMatchFlag_CaseInsensitive)) {
            first_slide = i;
            break;
         }
      }
   } else {
      // A folder - or something that is neither, which as a folder gives the
      // deck with no slides.
      deck = DECK_OpenFolder(path);
   }

   *first_slide_out = first_slide;
   return deck;
}

internal void
DECK_Close(DECK_Deck* deck) {
   for (U64 i = 0; i < deck->slide_count; i += 1) {
      IMG_Release(&deck->slides[i].image);
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
            IMG_Release(&deck->slides[i].image);
         }
      }

      DECK_Slide* slide = &deck->slides[slide_idx];
      if (!IMG_IsLoaded(&slide->image) && !slide->load_failed) {
         slide->image = IMG_LoadFromPath(slide->path);
         slide->load_failed = !IMG_IsLoaded(&slide->image);
      }
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

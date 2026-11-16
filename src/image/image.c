////////////////////////////////
//~ bgz: images
//
// See image.h.

// stb_image is held to its own warnings, not the app's. From memory only:
// files are read with bgz (DataFromFilePath), so its stdio half is left out.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_STDIO
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Weverything"
#endif
#include "../third_party/stb/stb_image.h" // relative: through -I it would find the copy in bgz's rad folder
#if defined(__clang__)
# pragma clang diagnostic pop
#endif

internal B32
IMG_NameIsImage(String8 name) {
   B32 result = 0;

   String8 extensions[] = {Str8Lit("png"), Str8Lit("jpg"), Str8Lit("jpeg"), Str8Lit("bmp"), Str8Lit("gif"),
                           Str8Lit("tga"), Str8Lit("psd"), Str8Lit("hdr"), Str8Lit("pic"), Str8Lit("pnm"),
                           Str8Lit("ppm"), Str8Lit("pgm")};

   // A name with no dot comes back whole, and matches nothing.
   String8 extension = Str8SkipLastDot(name);
   for (U64 i = 0; i < ArrayCount(extensions); i += 1) {
      if (Str8Match(extension, extensions[i], StringMatchFlag_CaseInsensitive)) {
         result = 1;
         break;
      }
   }

   return result;
}

internal IMG_Image
IMG_LoadFromPath(String8 path) {
   IMG_Image result = {0};
   Temp      scratch = ScratchBegin(0, 0);

   String8 data = DataFromFilePath(scratch.arena, path);

   if (data.size == 0) {
      printf("Could not read %.*s\n", Str8VArg(path));
   }
   else if (data.size > 0x7fffffff) {
      // stb_image takes the size as an int.
      printf("%.*s is too big to decode (2 GB or more)\n", Str8VArg(path));
   }
   else {
      int width = 0;
      int height = 0;
      int channels = 0;

      // Asked first, so an image too large for a texture is turned away
      // before it is decoded.
      if (!stbi_info_from_memory(data.str, (int)data.size, &width, &height, &channels)) {
         printf("Could not decode %.*s: %s\n", Str8VArg(path), stbi_failure_reason());
      }
      else if (width > IMG_MAX_DIM || height > IMG_MAX_DIM) {
         printf("%.*s is %d x %d - the most a texture can be is %d a side\n", Str8VArg(path), width, height, IMG_MAX_DIM);
      }
      else {
         // 4: always RGBA, whatever the file has.
         stbi_uc* pixels = stbi_load_from_memory(data.str, (int)data.size, &width, &height, &channels, 4);

         if (pixels == 0) {
            printf("Could not decode %.*s: %s\n", Str8VArg(path), stbi_failure_reason());
         }
         else {
            // Static: the renderer copies the pixels, so they are freed here.
            result.texture = Core_R_Tex2DAlloc(R_ResourceKind_Static, V2S32(width, height), R_Tex2DFormat_RGBA8, pixels);
            result.size = V2S32(width, height);
            stbi_image_free(pixels);
         }
      }
   }

   ScratchEnd(scratch);
   return result;
}

internal B32
IMG_IsLoaded(IMG_Image* image) {
   B32 result = !Core_R_HandleMatch(image->texture, Core_R_HandleZero());
   return result;
}

internal void
IMG_Release(IMG_Image* image) {
   if (IMG_IsLoaded(image)) {
      Core_R_Tex2DRelease(image->texture);
   }
   MemoryZeroStruct(image);
}

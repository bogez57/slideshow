////////////////////////////////
//~ bgz: images
//
// See image.h.

// stb_image's memory is an arena, not the C runtime's: while an image is
// being decoded (Image_LoadFromPath) everything stb_image asks for is pushed
// on this one, nothing it frees is given back, and the caller lets go of it
// all at once when the pixels have gone to the renderer.
PRIVATE Arena* _img_decode_arena = 0;

PRIVATE void*
_Image_DecodeRealloc(void* old, U64 old_size, U64 new_size) {
   void* result = ArenaPush(_img_decode_arena, new_size, 16, 0);
   if (old != 0) {
      MemoryCopy(result, old, Min(old_size, new_size));
   }
   return result;
}

#define STBI_MALLOC(size)                              ArenaPush(_img_decode_arena, (U64)(size), 16, 0)
#define STBI_REALLOC_SIZED(old, old_size, new_size)    _Image_DecodeRealloc((old), (U64)(old_size), (U64)(new_size))
#define STBI_FREE(memory)                              ((void)(memory))

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

B32
Image_NameIsImage(String8 name) {
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

Image_Image
Image_LoadFromPath(String8 path) {
   Image_Image result = {0};
   Temp      scratch = ScratchBegin(0, 0);

   String8 data = DataFromFilePath(scratch.arena, path);

   // What stb_image allocates below goes with the scratch.
   _img_decode_arena = scratch.arena;

   if (data.size == 0) {
      LogUserErrorF("Could not read %S\n", path);
   }
   else if (data.size > 0x7fffffff) {
      // stb_image takes the size as an int.
      LogUserErrorF("%S is too big to decode (2 GB or more)\n", path);
   }
   else {
      int width = 0;
      int height = 0;
      int channels = 0;

      // Asked first, so an image too large for a texture is turned away
      // before it is decoded.
      if (!stbi_info_from_memory(data.str, (int)data.size, &width, &height, &channels)) {
         LogUserErrorF("Could not decode %S: %s\n", path, stbi_failure_reason());
      }
      else if (width > IMG_MAX_DIM || height > IMG_MAX_DIM) {
         LogUserErrorF("%S is %d x %d - the most a texture can be is %d a side\n", path, width, height, IMG_MAX_DIM);
      }
      else {
         // 4: always RGBA, whatever the file has.
         stbi_uc* pixels = stbi_load_from_memory(data.str, (int)data.size, &width, &height, &channels, 4);

         if (pixels == 0) {
            LogUserErrorF("Could not decode %S: %s\n", path, stbi_failure_reason());
         }
         else {
            // Static: the renderer copies the pixels, so they can go with
            // the scratch.
            result.texture = Core_R_Tex2DAlloc(R_ResourceKind_Static, V2S32(width, height), R_Tex2DFormat_RGBA8, pixels);
            result.size = V2S32(width, height);
         }
      }
   }

   _img_decode_arena = 0;
   ScratchEnd(scratch);
   return result;
}

B32
Image_IsLoaded(Image_Image* image) {
   B32 result = !Core_R_HandleMatch(image->texture, Core_R_HandleZero());
   return result;
}

void
Image_Release(Image_Image* image) {
   if (Image_IsLoaded(image)) {
      Core_R_Tex2DRelease(image->texture);
   }
   MemoryZeroStruct(image);
}

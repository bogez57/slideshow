# image

An image file, decoded and handed to the renderer as a texture. Decoding is stb_image's (`src\third_party\stb`), compiled into this layer: bgz.dll has no image decoder. The formats it reads are in image.h.

## State

- `_img_decode_arena`: what stb_image allocates while one image is decoded, let go of all at once when the pixels have gone to the renderer.
- An `Image_Image` is the caller's: its texture and size.

## Functions

- `Image_NameIsImage`: whether a file name has an extension it decodes.
- `Image_LoadFromPath`: reads, decodes and makes the texture. Zero on failure, and why is logged.
- `Image_IsLoaded`, `Image_Release`.

## Used by

- deck: loads and releases the slides' pictures.
- view: draws a loaded picture.
- edit, open: tell image files from others by their names.

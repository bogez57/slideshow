////////////////////////////////
//~ bgz: images
//
// An image file, decoded and handed to the renderer as a texture. Decoding is
// stb_image's (src\third_party\stb), compiled into this layer: bgz.dll has no
// image decoder, and the app is where one belongs.
//
// Formats are stb_image's: png, jpg, bmp, gif (first frame), tga, psd, hdr,
// pic, pnm. A jpg's EXIF rotation is not read - a photo taken sideways shows
// sideways.

#ifndef IMAGE_H
#define IMAGE_H

// The longest side a texture can have (Direct3D 11's limit).
#define IMG_MAX_DIM 16384

typedef struct IMG_Image IMG_Image;
struct IMG_Image {
   R_Handle texture; // zero when there is no image
   Vec2S32  size;    // in pixels
};

// Whether a file name ends in an extension IMG_LoadFromPath can decode.
internal B32 IMG_NameIsImage(String8 name);

// Reads and decodes the file and makes a texture of it. On failure the image
// is zero, and why is printed.
internal IMG_Image IMG_LoadFromPath(String8 path);

internal B32  IMG_IsLoaded(IMG_Image* image);
internal void IMG_Release(IMG_Image* image);

#endif // IMAGE_H

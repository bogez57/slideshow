////////////////////////////////
//~ bgz: deck
//
// The slides, in the order they are shown. Today a deck is a folder: every
// image file directly in it is a slide, in order of file name - slide2 before
// slide10.
//
// A slide's image is loaded the first time it is asked for, not when the deck
// is opened, and only the slides around the one showing are kept: a folder of
// photos is far more than fits in video memory at once.

#ifndef DECK_H
#define DECK_H

typedef struct DECK_Slide DECK_Slide;
struct DECK_Slide {
   String8   path;
   IMG_Image image;      // zero until loaded
   B32       load_failed; // tried once and could not be: not tried again
};

typedef struct DECK_Deck DECK_Deck;
struct DECK_Deck {
   Arena*      arena; // the deck's own: everything it points to is on it
   String8     folder;
   DECK_Slide* slides;
   U64         slide_count;
};

// Never 0. A folder that is missing, or has no images in it, gives a deck
// with no slides.
internal DECK_Deck* DECK_OpenFolder(String8 folder);

// The image of a slide, loaded now if it is not yet. 0 if there is no such
// slide or its file could not be loaded. Slides more than one away from this
// one have their images released.
internal IMG_Image* DECK_ImageFromSlide(DECK_Deck* deck, U64 slide_idx);

#endif // DECK_H

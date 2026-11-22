# deck

The slides, in the order they are shown: opened from a folder of images, one image (its folder), or a text file that says what is on each slide. The text format is written out in full at the top of deck.h.

A slide's pictures - images or videos - are loaded the first time they are asked for, and only those of the slides around the one showing are kept.

## State

- A `Deck_Deck` is the caller's: `App_State`'s `deck`.
- `_deck_alias_arena`, `_deck_first_alias`: paths that stand for others for a while - a video on its way into the deck's folder plays from where it was dropped from (copy). For every deck, so a deck read again keeps them.

## Functions

- Opening: `Deck_OpenPath` (any kind), `Deck_OpenFolder`, `Deck_OpenFile`, `Deck_OpenText` (from the editor's text), `Deck_Close`.
- Reading a deck: `Deck_SlideFromTextPos`, `Deck_SlideHasText`, `Deck_PlaceName`, `Deck_ThemeName`, `Deck_LayoutName`, `Deck_NamedColorFromIdx`, `Deck_ColorFromString`.
- Pictures: `Deck_LoadSlide`, `Deck_LoadSlideOnly`, `Deck_TakeImages` (a deck read again keeps what the old one loaded), `Deck_ReopenPicture`, `Deck_PictureNamesFromFolder`, `Deck_SetPathAlias`, `Deck_ClearPathAlias`.
- Videos: `Deck_VideoFromSlide`, `Deck_VideosFromSlide`, `Deck_SlideHasVideo`, `Deck_SlideVideosAreOpen`, `Deck_UpdateVideos`.
- File names: `Deck_NameIsTextDeck`, `Deck_SortNames`.

## Used by

- frame.c: opens, reads again and closes decks, and loads the slide showing.
- main.c: an empty deck under the open screen, the first time.
- view: draws a slide from its fields.
- edit, place, look, caret: the slide the cursor is on, and where in the text its pieces are.
- slide_video, presenter: a slide's videos, and the presenter's next slide.
- copy: path aliases while a video is copied.
- open: which files are text decks, and their order.

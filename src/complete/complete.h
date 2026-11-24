#pragma once

////////////////////////////////
//~ bgz: complete
//
// The list under an @ line as it is typed in the deck's text: what can be
// written there, each with a few words on what it does. Up and Down move
// through it, Tab takes one - and Return, once some of the word is typed -
// and Esc puts it away. As the editor
// project's auto complete (src\auto_complete there), with the deck's format
// for its names.
//
//    @             the @ lines there are: @note, @box, @place, ...
//    @picture      not a line of the format: taking it opens Windows'
//                  open-file window, and the picture picked is put on the
//                  slide. @video file is the same, for a video
//    @place        what can come next: picture, text, title
//    @theme        the themes. @layout: the layouts. @video: how it plays
//    @color        the colours with names, for the block's words - and
//                  title, for the title's instead
//    @align        left, center, right - and title
//
// Where no list has what comes next, a hint is drawn after the cursor, dim,
// as if typed: the four numbers of @place, fewer as each is written; the
// number of @size; the words of @note; the rest of a #rrggbb.
//
// Whether the list is showing is worked out, never kept. It shows when all
// of these hold:
//
//    the keys in the text are typing's (vim's insert mode), and the text is
//    on the screen
//    nothing is selected, and the cursor's line starts with an @
//    the cursor is after a word of that line, or after a space, where
//    something of the list can come
//    something of the list starts with what is typed of the word
//    Esc was not pressed at this word since the text last changed
//
// So nothing opens or closes it in step with the text. The state is only
// what was chosen: which line of the list, and that it was put away.

// In App_State. Changed by ops only.
typedef struct Complete_State Complete_State;
struct Complete_State {
   // Up and Down. Means something only for the list it was picked in: this
   // word, of this text.
   U64 selected_idx;
   U64 selected_word_start;
   U64 selected_edit_count;

   // Esc. Away at this word until the text changes.
   B32 has_dismissed;
   U64 dismissed_word_start;
   U64 dismissed_edit_count;
};

typedef struct Complete_Item Complete_Item;
struct Complete_Item {
   String8 name;       // as written into the text
   String8 what;       // what it does, in a few words
   B32     takes_more; // something comes after it on the line: a space is written too
   // Taking it writes nothing: Windows' open-file window comes up, and the
   // picture or video picked there is put on the slide, in the line's place.
   B32 picks_file;
};

// The list as it is now. count 0: it is not showing.
typedef struct Complete_List Complete_List;
struct Complete_List {
   Complete_Item* v;
   U64            count;
   U64            selected_idx;

   // The word being typed is from word_start to cursor, in the text as it
   // was at edit_count. Taking an item writes it there.
   U64     word_start;
   U64     cursor;
   U64     edit_count;
   String8 prefix;
   U64     line_start; // the line's @
};

Complete_List Complete_ListFromState(const App_State* app_state, Arena* arena);

// Its keys, taken while it shows, before the edit screen's others. Pushes
// ops.
void Complete_Input(const App_State* app_state, StateOp_List* ops);

// The list, under the word in the text, which is drawn in box. In the edit
// screen's UI build, before anything it is to be over.
void Complete_Build(const App_State* app_state, Editor_Box box, Vec2F32 window_dim, F32 ui, F32 px);

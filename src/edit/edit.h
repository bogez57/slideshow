#pragma once

////////////////////////////////
//~ bgz: edit
//
// The edit screen. What it is and how it works is at the top of edit.c; here
// is what the rest of the program calls.

typedef struct App_State App_State;

// The edit screen's own, which is not app state, written with no state op:
// the preview's slide as it was last laid out, and its panel's lists.
typedef struct Edit_View Edit_View;
struct Edit_View {
   // Where the pieces of the slide in the preview are, as it was laid out
   // this frame in the UI build, and where in the text each is from - what
   // the boxes over it are built from. Good for the text as it was then:
   // hits_edit_count is the editor's edit_count they were made at.
   Arena*    hits_arena;
   View_Hit* hits;
   U64       hit_count;
   U64       hits_edit_count;

   // The runs of letters of the preview's slide, as laid out this frame
   // (View_HitList's runs), on hits_arena: what a click on a word is a click
   // on the letter of, and where the caret goes (caret\caret.c). Good for
   // the same text as hits.
   View_Run* runs;
   U64       run_count;

   // Where the preview's slide has its pictures, its title and its body
   // this frame (View_HitList's boxes), and its page: what the mouse moves
   // and resizes. Good for the same text as hits.
   View_Box boxes[VIEW_MAX_BOXES];
   U64      box_count;
   Rng2F32  page;

   // Where the Format panel's lines are scrolled to, in a window too short
   // for all of them.
   UI_ScrollRows format_scroll;

   // The list of pictures to pick from: the names of the files in the
   // deck's folder, looked up again every so often while the list is open,
   // and where the list is scrolled to.
   Arena*        pictures_arena;
   String8Array  pictures;
   U64           pictures_scanned_us; // 0: look now
   UI_ScrollRows pictures_scroll;

   // A text box just made by a click on the slide's open space
   // (Edit_AddTextBox), while nothing has been done to it: where its lines
   // are in the text, and the editor's edit_count once they were written.
   // Left like that - clicked away from, or Esc - it is taken out again, as
   // presentation programs do, so clicks that only miss things do not leave
   // boxes of "Text" all over the slide.
   B32     has_new_box;
   U64     new_box_edit_count;
   Rng1U64 new_box_range;

   // A piece picked whole: a picture clicked, or the title or a block of
   // words clicked in its box away from the words - where it is taken hold
   // of to be moved. Delete or Backspace then takes all of it out
   // (Edit_DeletePiece). Picked for as long as the text, the cursor and the
   // slide are as the click left them: typing, or the cursor going
   // anywhere, and it is not (Edit_PieceIsPicked).
   B32            has_picked;
   Deck_PlaceKind picked_kind;
   U64            picked_idx;
   U64            picked_slide_idx;
   U64            picked_edit_count;
   U64            picked_cursor;
   U64            picked_mark;
};
global Edit_View g_edit_view;

// Edit_Input: the keys, the editor's among them. Edit_Build builds the whole
// screen in FrameWork's UI build: the text (the editor draws it, and has its
// mouse), the Format panel, the list of pictures, the status line, and the
// preview. Not interactive: drawn, but taking no clicks or wheel, as when the
// open screen is over it. Neither changes app state: what a key or a click
// does is pushed as a state op (edit.c's own, and look's, caret's, place's).
void Edit_Input(const App_State* app_state, StateOp_List* ops, Vec2F32 window_dim);
void Edit_Build(const App_State* app_state, StateOp_List* ops, Vec2F32 window_dim, B32 interactive);

// Which of the preview's pieces (g_edit_view.hits) a point is on, or -1:
// where pieces are over each other, the one drawn last.
S64 Edit_PreviewHitAt(Vec2F32 pos);

// Whether the piece of the preview's slide of kind and idx is picked whole
// (g_edit_view.has_picked) and still is: Delete takes it out. The caret is
// not drawn in a block or a title picked so.
B32 Edit_PieceIsPicked(const App_State* app_state, Deck_PlaceKind kind, U64 idx);

// Writes a line that is about one piece of a slide - an @place, @size or
// @color line, for the idx-th picture or block of the body, or the title:
// over the one there is (old_line, with has_line), or as a new line next to
// the piece's own. Edit_TakeOutLine takes such a line out of the text, with
// its line ending. Both change the text: for ops only.
void Edit_PutLineFor(Editor_Editor* editor, Deck_Slide* slide, Deck_PlaceKind kind, U64 idx, B32 has_line, Rng1U64 old_line,
                     String8 line);
void Edit_TakeOutLine(Editor_Editor* editor, Rng1U64 line);

//- bgz: state ops, pushed with StateOp_<Name>(ops)
//
// The one other layers push. The rest of the edit screen's are edit.c's own.

// The preview: a press on the slide at pos, as a click - to the letter of the
// words there, or the picture's name (keeps_mark: selecting from where the
// cursor was). picks: the press was on the piece of kind and idx away from
// its words - a picture, or the box of the title or a block - so the piece
// is picked whole as well (g_edit_view.has_picked).
typedef struct Edit_PreviewClickOpParams Edit_PreviewClickOpParams;
struct Edit_PreviewClickOpParams {
   Vec2F32        pos;
   B32            keeps_mark;
   B32            picks;
   Deck_PlaceKind kind;
   U64            idx;
};
Edit_PreviewClickOpParams* StateOp_Edit_PreviewClick(StateOp_List* ops);
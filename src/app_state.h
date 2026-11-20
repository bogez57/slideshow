////////////////////////////////
//~ bgz: app state
//
// Everything the slideshow knows, in one struct. There is one, made in
// _EntryPoint (main.c).

#ifndef APP_STATE_H
#define APP_STATE_H

// A line of the open screen.
typedef enum OPEN_RowKind {
   OPEN_RowKind_Label,   // says what the rows under it are. Not to be clicked
   OPEN_RowKind_Recent,  // a deck opened lately
   OPEN_RowKind_Up,      // the folder this one is in
   OPEN_RowKind_Browse,  // Windows' own open-file window, starting in this folder
   OPEN_RowKind_New,     // a new text deck, in this folder
   OPEN_RowKind_Present, // this folder's pictures, as a deck
   OPEN_RowKind_Folder,
   OPEN_RowKind_Deck,    // a text deck in this folder
} OPEN_RowKind;

typedef struct OPEN_Row OPEN_Row;
struct OPEN_Row {
   OPEN_RowKind kind;
   String8      label; // as shown
   String8      path;  // what a click opens, or goes to
};

// A thing in the presenter's window that a click does something to.
typedef enum PRES_HitKind {
   PRES_HitKind_Null,
   PRES_HitKind_Slide, // go to slide_idx
   PRES_HitKind_Clock, // stop the clock, or let it run again
   PRES_HitKind_ClockReset, // back to 00:00
   PRES_HitKind_Video, // play the slide's video, or pause it
} PRES_HitKind;

typedef struct PRES_Hit {
   PRES_HitKind kind;
   Rng2F32      rect;
   U64          slide_idx;
} PRES_Hit;

#define PRES_MAX_HITS 96

typedef struct COPY_Job COPY_Job;

// A press on one of the preview's pictures, its title or its body, and what
// has come of it.
typedef enum EDIT_PlaceDragState {
   EDIT_PlaceDragState_None,
   EDIT_PlaceDragState_Pressed, // the button is down on it, and the mouse has not gone far: a click, so far
   EDIT_PlaceDragState_Dragging,
} EDIT_PlaceDragState;

typedef struct EDIT_PlaceDrag EDIT_PlaceDrag;
struct EDIT_PlaceDrag {
   EDIT_PlaceDragState state;
   DECK_PlaceKind      kind;
   U64                 idx;         // which of the slide's pictures, or of its body's blocks
   U32                 corner;      // 0: all of it is moved. 1 to 4: that corner is - top left, top right, bottom left, bottom right
   U64                 slide_idx;
   U64                 edit_count;  // the editor's when the button went down: a change to the text ends it
   Vec2F32             press_mouse;
   Rng2F32             start;       // where it was then, in parts of the page
   Rng2F32             now;         // where the mouse has it
   // With Shift down: what it has been lined up with, across and down, as
   // parts of the page - lines drawn over the slide while it is.
   B32                 has_guide_x;
   B32                 has_guide_y;
   F32                 guide_x;
   F32                 guide_y;
   // The other box each line is an edge or the middle of, outlined with it:
   // the body's box is larger than its words, and a line through empty space
   // says nothing until its box shows. None for the page's own lines.
   B32                 has_guide_box_x;
   B32                 has_guide_box_y;
   Rng2F32             guide_box_x;
   Rng2F32             guide_box_y;
   DECK_Place          before;     // the slide's place of that kind as the text has it, to go back to
};

typedef struct App_State App_State;
struct App_State {
   WM_Window    window;
   R_Handle     window_eqp;
   Arena*       frame_arena; // cleared at the end of every frame
   WM_EventList events;      // this frame's, on frame_arena
   B32          quit;

   // Text. A change in the window's size changes every size text is drawn
   // at, and the font cache is emptied of the old ones - see _FR_BeginText
   // (frame.c).
   TXT_Fonts fonts;
   Vec2F32   text_window_dim; // the window's size when the font cache was last emptied

   // The UI layer (src\ui): its box cache, hot and active boxes, and so on,
   // made once at the start. A UI tree is built inside FrameWork every frame,
   // between _FR_BeginUI and UI_EndBuild (frame.c).
   UI_State* ui_state;
   FNT_Tag   ui_icon_font;     // raddebugger's icon font, data\fonts\Icons.ttf. The zero tag without it: no icons
   U64       ui_last_frame_us; // when the last UI build began, for its animations. 0: none yet

   B32 show_info; // the slide's number and name, in the corner
   B32 show_notes; // the slide's notes and what comes next, along the bottom
   B32 show_help; // the list of keys

   // A slide's video (frame_video.c). video_auto_slide: the slide whose
   // video was started because the presentation came to it, plus one. 0:
   // none. video_key_us: when one of the video's keys was last pressed while
   // presenting - its strip of controls shows for a moment after.
   U64 video_auto_slide;
   U64 video_key_us;

   // Files on their way into the deck's folder (frame_copy.c).
   COPY_Job* first_copy;
   B32       quit_warned_copy; // told once that closing now loses one

   // The presenter's window (frame_presenter.c): a second window, for the
   // screen only whoever presents sees, with the slide, the one after it,
   // the notes and how long it has been. Opened and closed by the main loop
   // between frames, when a frame sets toggle_presenter.
   B32       toggle_presenter;
   B32       presenter_open;
   WM_Window presenter_window;
   R_Handle  presenter_window_eqp;
   U64       presenter_started_us; // what its clock counts from
   U64       presenter_paused_us; // when its clock was stopped. 0: it runs

   // What can be clicked in it, as PRES_Draw last laid it out. PRES_Input
   // asks these of a click, a frame later - the window is laid out as it is
   // drawn, and nothing moves between the two.
   PRES_Hit presenter_hits[PRES_MAX_HITS];
   U64      presenter_hit_count;
   // The slide a click in it asked for, plus one. 0: none. Gone to by the
   // presentation's own input, with its other ways of changing slide, so
   // that the change fades as theirs do.
   U64 presenter_goto;

   // What is logged (LogInfoF, LogUserErrorF) is gathered a frame at a time
   // and written to local\logs\slideshow_log.txt beside the program, and to
   // the console (FR_WriteLogs).
   Log* log;
   File log_file;
   U64  log_file_off; // how much has been written to it

   // Set by a frame that wants fullscreen turned on or off. The main loop
   // does it between frames - see _EntryPoint (main.c).
   B32 toggle_fullscreen;

   DECK_Deck* deck;
   U64        slide_idx;        // the slide showing
   U64        slide_changed_us; // when it became the one showing. 0: never changed
   F32        scroll_left_over; // mouse wheel movement not yet enough for a slide

   // A deck from a text file has an editor, which holds the text: the deck
   // is made from the editor's text, not the file's, and made again whenever
   // that changes (FR_SyncDeck). 0 for a folder of images.
   ED_Editor* editor;
   U64        parsed_edit_count; // the editor's edit_count the deck was made at
   U64        reload_checked_us; // when the file was last looked at for changes from outside

   // The watch on the folder the text deck's file is in: something written
   // there is when the file is looked at (FR_SyncDeck). 0 for a folder of
   // images, and when the folder could not be watched - then the file is
   // only looked at now and then.
   Arena*     watch_arena;
   FileWatch* deck_watch;

   // Which of the two screens is up: the edit screen (frame_edit.c), with the
   // text beside a preview, or the presentation (frame.c). Only a deck with
   // an editor has the edit screen.
   B32 is_editing;
   B32           show_format;   // edit screen: the list of what can be written is open
   UI_ScrollRows format_scroll; // where that list is scrolled to, in a window too short for all of it (EDIT_Build)

   // Edit screen: the list of pictures to pick from, open in place of the
   // format panel's lines. The names are of the files in the deck's folder,
   // looked up again every so often while the list is open.
   B32           show_pictures;
   Arena*        pictures_arena;
   String8Array  pictures;
   U64           pictures_scanned_us;
   UI_ScrollRows pictures_scroll; // where the list is scrolled to (EDIT_Build)

   // A line of text shown for a few seconds: something saved, something
   // refused. Always a string that lasts (a literal).
   String8 notice;
   U64     notice_us; // when it was set

   // Closing with unsaved changes asks once, with a notice. This is set when
   // it has: the next close quits.
   B32 quit_warned;

   // Edit screen: where the pieces of the slide in the preview were drawn
   // last frame, and where in the text each is from - what a click on the
   // preview is a click on. Good for the text as it was then:
   // preview_hits_edit_count is the editor's edit_count they were made at.
   Arena*    preview_hits_arena;
   VIEW_Hit* preview_hits;
   U64       preview_hit_count;
   U64       preview_hits_edit_count;

   // Edit screen: Esc has taken the box around the piece the cursor is in
   // off the preview. It is back when the cursor is anywhere but
   // preview_focus_hidden_cursor, or a piece is clicked.
   B32 preview_focus_hidden;
   U64 preview_focus_hidden_cursor;

   // Edit screen: the runs of letters of the preview's slide, as drawn last
   // frame (VIEW_HitList's runs), on preview_hits_arena: what a click on a
   // word is a click on the letter of, and where the caret goes
   // (frame_caret.c). Good for the same text as preview_hits.
   VIEW_Run* preview_runs;
   U64       preview_run_count;

   // Edit screen: how far down its words each block of the slide showing
   // shows, in page units (DECK_Box's scroll): by block for the ones with
   // an @place line, and one for the rest, which share the body's room.
   // caret_scroll_slide is the slide they are of: another starts from 0.
   // The caret_seen two are the cursor and the text the room last followed
   // the caret for.
   F32 caret_scroll[DECK_MAX_BOXES];
   F32 caret_body_scroll;
   U64 caret_scroll_slide;
   U64 caret_seen_cursor;
   U64 caret_seen_edit_count;

   // A scroll bar of the preview's slide with the mouse's button down on
   // it (frame_caret.c): the block whose it is, and how far under the top
   // of the bar's thumb the mouse took hold.
   B32 caret_bar_drag;
   U64 caret_bar_block;
   F32 caret_bar_grab;

   // Presenting: the boxes of the slide showing, as last drawn, for the
   // wheel over a block with more words than room (CARET_PresentWheel).
   VIEW_Box present_boxes[VIEW_MAX_BOXES];
   U64      present_box_count;
   Rng2F32  present_page;

   // Edit screen: where the preview's slide had its pictures, its title and
   // its body last frame (VIEW_HitList's boxes), and its page: what the mouse moves
   // and resizes. Good for the same text as preview_hits.
   VIEW_Box preview_boxes[VIEW_MAX_BOXES];
   U64      preview_box_count;
   Rng2F32  preview_page;

   // Edit screen: the picture, the title or the body being moved or resized with the
   // mouse (frame_edit.c). While it is, the slide is given the place the
   // mouse makes without its being in the text: the @place line is written
   // when the button comes up.
   EDIT_PlaceDrag place_drag;

   // The open screen (frame_open.c): over whichever screen is up, the decks
   // opened lately and what is in a folder, to pick a deck from.
   B32           show_open;
   Arena*        open_arena;      // open_folder and the rows
   String8       open_folder;     // whole, from the drive. Empty: the list of drives
   OPEN_Row*     open_rows;
   U64           open_row_count;
   UI_ScrollRows open_scroll;     // where the rows are scrolled to (OPEN_Build)
   U64           open_scanned_us; // when the folder was last read. 0: read it now

   // The decks opened lately, the latest first, as whole paths. Kept in a
   // file beside the program from one run to the next (FR_LoadRecents).
   Arena*       recents_arena;
   String8Array recents;

   // A change of slide fades the old one out as the new one comes in.
   B32 fade_active;
   U64 fade_from_idx; // the slide that was showing. Past the last: none, fade from the background
   U64 fade_start_us; // 0 until the frame that first draws the new slide

   // Playing: the deck moves on by itself, a slide every _FR_PLAY_SECONDS,
   // and from the last slide back to the first.
   B32 playing;
   U64 play_from_us; // when the slide showing began its turn
};

#endif // APP_STATE_H

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
   B32 show_help; // the list of keys

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
   B32 show_format; // edit screen: the list of what can be written is open

   // Edit screen: the list of pictures to pick from, open in place of the
   // format panel's lines. The names are of the files in the deck's folder,
   // looked up again every so often while the list is open.
   B32          show_pictures;
   Arena*       pictures_arena;
   String8Array pictures;
   U64          pictures_scanned_us;
   U64          pictures_first_row; // scrolled to

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

   // The open screen (frame_open.c): over whichever screen is up, the decks
   // opened lately and what is in a folder, to pick a deck from.
   B32          show_open;
   Arena*       open_arena;      // open_folder and the rows
   String8      open_folder;     // whole, from the drive. Empty: the list of drives
   OPEN_Row*    open_rows;
   U64          open_row_count;
   U64          open_first_row;  // scrolled to
   U64          open_scanned_us; // when the folder was last read. 0: read it now

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

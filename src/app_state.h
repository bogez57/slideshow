#pragma once

////////////////////////////////
//~ bgz: app state
//
// What the slideshow is doing, in one struct: the deck, the text, the slide
// showing, which screen is up. There is one, made in _EntryPoint (main.c). It
// is changed by state ops only (state_ops\state_ops.h): app_state.c's,
// declared at the end of this file, and the layers' own.
//
// What a screen keeps from frame to frame that is not this - where things
// were laid out, what the mouse holds, how far a list is scrolled - is the
// layer's own view (g_edit_view, g_caret_view, g_place_view, open.c's
// _open_view) or frame.c's _frame_state, written with no op.

typedef struct App_State App_State;
struct App_State {
   // The queue of state ops, run by _FR_UpdateAppState (frame.c).
   StateOp_List* op_list;

   //- bgz: made by main.c at the start
   WM_Window  window;
   R_Handle   window_eqp;
   Arena*     frame_arena; // cleared at the end of every frame
   Text_Fonts fonts;
   // The UI layer (src\ui): its box cache, hot and active boxes, and so on.
   // A UI tree is built inside FrameWork every frame, between
   // FR_BeginUIBuild and UI_EndBuild (frame.c).
   UI_State* ui_state;
   FNT_Tag   ui_icon_font; // raddebugger's icon font, data\fonts\Icons.ttf. The zero tag without it: no icons
   // What is logged (LogInfoF, LogUserErrorF) is gathered a frame at a time
   // and written to local\logs\slideshow_log.txt beside the program, and to
   // the console (FR_WriteLogs).
   Log* log;

   // This frame's events, on frame_arena. The list is main.c's, which fills
   // it before each frame; a key or a click that is read is taken off it.
   WM_EventList* events;

   B32 quit;

   B32 show_info; // the slide's number and name, in the corner
   B32 show_notes; // the slide's notes and what comes next, along the bottom
   B32 show_help; // the list of keys

   // A slide's video (slide_video\slide_video.c). video_auto_slide: the slide whose
   // video was started because the presentation came to it, plus one. 0:
   // none.
   U64 video_auto_slide;

   // Files on their way into the deck's folder (copy\copy.c).
   Copy_Job* first_copy;
   B32       quit_warned_copy; // told once that closing now loses one

   // The presenter's window (presenter\presenter.c): a second window, for the
   // screen only whoever presents sees, with the slide, the one after it,
   // the notes and how long it has been. Opened and closed by the main loop
   // between frames, when a frame sets toggle_presenter.
   B32       toggle_presenter;
   B32       presenter_open;
   WM_Window presenter_window;
   R_Handle  presenter_window_eqp;
   U64       presenter_started_us; // what its clock counts from
   U64       presenter_paused_us; // when its clock was stopped. 0: it runs

   // Its UI layer, a UI_State of its own: built in Presenter_Draw, after the
   // main window's frame, with its own events. Made the first time it opens.
   UI_State* presenter_ui_state;

   // Set by a frame that wants fullscreen turned on or off. The main loop
   // does it between frames - see _EntryPoint (main.c).
   B32 toggle_fullscreen;

   Deck_Deck* deck;
   U64        slide_idx;        // the slide showing
   U64        slide_changed_us; // when it became the one showing. 0: never changed

   // A deck from a text file has an editor, which holds the text: the deck
   // is made from the editor's text, not the file's, and made again whenever
   // that changes (FR_SyncDeck, in _FR_UpdateAppState). 0 for a folder of
   // images.
   Editor_Editor* editor;
   U64            parsed_edit_count; // the editor's edit_count the deck was made at

   // The mode the keys in the text are in, as vim has modes (vim\vim.h).
   Vim_State vim;

   // The list under an @ line as it is typed: what was chosen in it
   // (complete\complete.h).
   Complete_State complete;

   // The watch on the folder the text deck's file is in: something written
   // there is when the file is looked at (FR_SyncDeck). 0 for a folder of
   // images, and when the folder could not be watched - then the file is
   // only looked at now and then.
   Arena*     watch_arena;
   FileWatch* deck_watch;

   // Which of the two screens is up: the edit screen (edit\edit.c), with the
   // text beside a preview, or the presentation (frame.c). Only a deck with
   // an editor has the edit screen.
   B32 is_editing;
   B32 hide_text;   // edit screen: the left side is put away (Alt+[), and the preview has the window
   B32 show_format; // edit screen: the list of what can be written is open
   // Edit screen: the list of pictures to pick from, open in place of the
   // format panel's lines.
   B32 show_pictures;

   // A line of text shown for a few seconds: something saved, something
   // refused. Always a string that lasts (a literal).
   String8 notice;
   U64     notice_us; // when it was set

   // Closing with unsaved changes asks once, with a notice. This is set when
   // it has: the next close quits.
   B32 quit_warned;

   // The open screen (open\open.c): over whichever screen is up, the decks
   // opened lately and what is in a folder, to pick a deck from. Its rows and
   // where they are scrolled to are open.c's own.
   B32     show_open;

   // A deck that was to be opened while the one that is open had unsaved
   // changes: the open screen asks what to do with them - save, let go, or
   // not open it after all (open\open.c). pending_open_size 0: none.
   // pending_open_is_new: it is a new deck, still to be made.
   // open_discards: set by the op that answers, while it opens the deck, for
   // the changes to be let go.
   U8  pending_open[1024];
   U64 pending_open_size;
   B32 pending_open_is_new;
   B32 open_discards;
   Arena*  open_arena;  // open_folder
   String8 open_folder; // whole, from the drive. Empty: the list of drives

   // The decks opened lately, the latest first, as whole paths. Kept in a
   // file beside the program from one run to the next (FR_LoadRecents).
   Arena*       recents_arena;
   String8Array recents;

   // A change of slide fades the old one out as the new one comes in. The
   // fade is of the change made at slide_changed_us; how far through it is
   // is frame.c's (FR_IsFading).
   B32 fade_active;
   U64 fade_from_idx; // the slide that was showing. Past the last: none, fade from the background

   // Playing: the deck moves on by itself, a slide every FR_PLAY_SECONDS,
   // and from the last slide back to the first.
   B32 playing;
   U64 play_from_us; // when the slide showing began its turn
};
////////////////////////////////
//~ bgz: state ops (app_state.c)
//
// Pushed with StateOp_<Name>(ops). Only another op may call one's function.

// A line of text shown for a few seconds (FR_CurrentNotice). notice must
// last: a literal.
typedef struct App_NoticeOpParams App_NoticeOpParams;
struct App_NoticeOpParams {
   String8 notice;
};
App_NoticeOpParams* StateOp_App_Notice(StateOp_List* ops);

// Quits - unless there are unsaved changes, or a copy under way, that have
// not been warned about, which it warns about instead.
void StateOp_App_RequestQuit(StateOp_List* ops);

// Presenting: the slide's number, its notes, the list of keys.
void StateOp_App_ToggleInfo(StateOp_List* ops);
void StateOp_App_ToggleNotes(StateOp_List* ops);
void StateOp_App_ToggleHelp(StateOp_List* ops);

// Asks the main loop for fullscreen turned on or off, and for the
// presenter's window opened or closed: it does both between frames (main.c).
void StateOp_App_ToggleFullscreen(StateOp_List* ops);
void StateOp_App_TogglePresenter(StateOp_List* ops);

// Presenting: the deck moving on by itself, started and stopped.
void StateOp_App_TogglePlaying(StateOp_List* ops);

// From the presentation back to the edit screen, at the slide that was
// showing. Nothing for a deck with no text.
void StateOp_App_StartEditing(StateOp_List* ops);

// Opens the deck at path - a folder, an image or a text file - in place of
// the one that is open, and takes the open screen down (FR_OpenPath). path
// must last until the op has run: a copy on ops->arena does.
typedef struct App_OpenPathOpParams App_OpenPathOpParams;
struct App_OpenPathOpParams {
   String8 path;
};
App_OpenPathOpParams* StateOp_App_OpenPath(StateOp_List* ops);

// Presenting: shows the slide at slide_idx - the last one, if there are not
// that many - fading from the one that was showing.
typedef struct App_GoToSlideOpParams App_GoToSlideOpParams;
struct App_GoToSlideOpParams {
   U64 slide_idx;
};
App_GoToSlideOpParams* StateOp_App_GoToSlide(StateOp_List* ops);

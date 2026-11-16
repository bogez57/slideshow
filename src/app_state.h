////////////////////////////////
//~ bgz: app state
//
// Everything the slideshow knows, in one struct. There is one, made in
// _EntryPoint (main.c).

#ifndef APP_STATE_H
#define APP_STATE_H

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

   B32 show_info; // the slide's number and name, in the corner
   B32 show_help; // the list of keys

   // Set by a frame that wants fullscreen turned on or off. The main loop
   // does it between frames - see _EntryPoint (main.c).
   B32 toggle_fullscreen;

   DECK_Deck* deck;
   U64        slide_idx;        // the slide showing
   U64        slide_changed_us; // when it became the one showing. 0: never changed
   F32        scroll_left_over; // mouse wheel movement not yet enough for a slide

   // A deck from a text file is read again when the file is saved.
   U64 reload_checked_us; // when the file was last looked at

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

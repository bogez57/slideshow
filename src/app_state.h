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

   DECK_Deck* deck;
   U64        slide_idx; // the slide showing

   // A change of slide fades the old one out as the new one comes in.
   B32 fade_active;
   U64 fade_from_idx; // the slide that was showing
   U64 fade_start_us; // 0 until the frame that first draws the new slide
};

#endif // APP_STATE_H

////////////////////////////////
//~ bgz: open
//
// The open screen.
// What it is and how it works is at the top of open.c; here is what the
// rest of the program calls.

#ifndef OPEN_H
#define OPEN_H

//- bgz: the open screen (open\open.c)
//
// Input for its keys; OPEN_Build builds the panel, in FrameWork's UI build,
// and takes its clicks and wheel.
internal void OPEN_Show(App_State* app_state);
internal void OPEN_Input(App_State* app_state, Vec2F32 window_dim);
internal void OPEN_Build(App_State* app_state, Vec2F32 window_dim);

#endif // OPEN_H

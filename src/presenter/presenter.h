#pragma once

////////////////////////////////
//~ bgz: presenter
//
// The presenter's window.
// What it is and how it works is at the top of presenter.c; here is what the
// rest of the program calls.

typedef struct App_State App_State;

// Opens the window if it is closed and closes it if it is open. Between
// frames only (main.c).
void PRES_Toggle(App_State* app_state);

// Takes the window's events off this frame's: its close, and its keys, which
// become the main window's so that either window drives the slides. Its
// mouse is left for its own UI (PRES_Draw).
void PRES_Input(App_State* app_state);

// Builds the window's UI, with its own UI_State, and draws it. Inside its own
// Core_R_WindowBeginFrame and Core_R_WindowEndFrame, after FrameWork.
void PRES_Draw(App_State* app_state);

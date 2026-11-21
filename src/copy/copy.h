#pragma once

////////////////////////////////
//~ bgz: copy
//
// Files copied into the deck's folder, in the background. What it is and how
// it works is at the top of copy.c; here is what the rest of the program
// calls.

typedef struct App_State App_State;

// One file on its way, in app_state->first_copy's list. Its fields are
// copy.c's own.
typedef struct COPY_Job COPY_Job;

B32  COPY_IsCopyingTo(App_State* app_state, String8 dst);
void COPY_Start(App_State* app_state, String8 src, String8 dst);
B32  COPY_IsBusy(App_State* app_state);
void COPY_Update(App_State* app_state);
void COPY_CancelAll(App_State* app_state);

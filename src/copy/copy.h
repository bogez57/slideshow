////////////////////////////////
//~ bgz: copy
//
// Files copied into the deck's folder, in the background.
// What it is and how it works is at the top of copy.c; here is what the
// rest of the program calls.

#ifndef COPY_H
#define COPY_H

// Files copied into the deck's folder in the background: copy\copy.c.
internal B32  COPY_IsCopyingTo(App_State* app_state, String8 dst);
internal void COPY_Start(App_State* app_state, String8 src, String8 dst);
internal B32  COPY_IsBusy(App_State* app_state);
internal void COPY_Update(App_State* app_state);
internal void COPY_CancelAll(App_State* app_state);

#endif // COPY_H

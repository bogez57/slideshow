////////////////////////////////
//~ bgz: edit
//
// The edit screen.
// What it is and how it works is at the top of edit.c; here is what the
// rest of the program calls.

#ifndef EDIT_H
#define EDIT_H

//- bgz: the edit screen (edit\edit.c)
//
// EDIT_Input: the keys, the editor's among them. EDIT_Build builds the whole
// screen in FrameWork's UI build: the text (the editor draws it, and has its
// mouse), the Format panel, the list of pictures, the status line, and the
// preview. Not interactive: drawn, but taking no clicks or wheel, as when the
// open screen is over it.
internal void EDIT_Input(App_State* app_state, Vec2F32 window_dim);
internal void EDIT_Build(App_State* app_state, Vec2F32 window_dim, B32 interactive);

// The edit screen's preview: a press on the slide at pos, as a click - to
// the letter of the words there, or the picture's name (edit\edit.c).
internal void EDIT_PreviewClick(App_State* app_state, Vec2F32 pos, B32 keeps_mark);

#endif // EDIT_H

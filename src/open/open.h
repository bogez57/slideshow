#pragma once

////////////////////////////////
//~ bgz: open
//
// The open screen. What it is and how it works is at the top of open.c; here
// is what the rest of the program calls.

typedef struct App_State App_State;

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

// OPEN_Show puts the screen up. OPEN_Input is its keys; OPEN_Build builds the
// panel, in FrameWork's UI build, and takes its clicks and wheel.
void OPEN_Show(App_State* app_state);
void OPEN_Input(App_State* app_state, Vec2F32 window_dim);
void OPEN_Build(App_State* app_state, Vec2F32 window_dim);

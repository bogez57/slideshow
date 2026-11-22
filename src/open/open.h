#pragma once

////////////////////////////////
//~ bgz: open
//
// The open screen. What it is and how it works is at the top of open.c; here
// is what the rest of the program calls.

typedef struct App_State App_State;

// A line of the open screen.
typedef enum Open_RowKind {
   Open_RowKind_Label,   // says what the rows under it are. Not to be clicked
   Open_RowKind_Recent,  // a deck opened lately
   Open_RowKind_Up,      // the folder this one is in
   Open_RowKind_Browse,  // Windows' own open-file window, starting in this folder
   Open_RowKind_New,     // a new text deck, in this folder
   Open_RowKind_Present, // this folder's pictures, as a deck
   Open_RowKind_Folder,
   Open_RowKind_Deck,    // a text deck in this folder
} Open_RowKind;

typedef struct Open_Row Open_Row;
struct Open_Row {
   Open_RowKind kind;
   String8      label; // as shown
   String8      path;  // what a click opens, or goes to
};

// Open_Input is its keys; Open_Build builds the panel, in FrameWork's UI
// build, and takes its clicks and wheel. Both push state ops.
void Open_Input(const App_State* app_state, StateOp_List* ops);
void Open_Build(const App_State* app_state, Vec2F32 window_dim);

//- bgz: state ops, pushed with StateOp_<Name>(ops)

// Puts the screen up, in the folder of the deck that is open, and takes it
// down.
void StateOp_Open_Show(StateOp_List* ops);
void StateOp_Open_Close(StateOp_List* ops);

// Shows what is in folder: whole, from the drive, or empty for the list of
// drives. folder and path must last until the op has run: a copy on
// ops->arena does.
typedef struct Open_GoToFolderOpParams Open_GoToFolderOpParams;
struct Open_GoToFolderOpParams {
   String8 folder;
};
Open_GoToFolderOpParams* StateOp_Open_GoToFolder(StateOp_List* ops);

// Makes a new text deck at path and opens it.
typedef struct Open_NewDeckOpParams Open_NewDeckOpParams;
struct Open_NewDeckOpParams {
   String8 path;
};
Open_NewDeckOpParams* StateOp_Open_NewDeck(StateOp_List* ops);

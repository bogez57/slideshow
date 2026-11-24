#pragma once

////////////////////////////////
//~ bgz: vim
//
// The edit screen's keys in the text, as the editor project's vim has them
// (src\vim there): not all of vim, the keys that are used. The keys are
// listed at the top of vim.c.
//
// Three modes. Command: the keys move the cursor and act on the text, and
// nothing is typed. Insert: the editor's own keys and typing (editor.h).
// Visual: the keys move the cursor, and what is between it and where it was
// when the mode began is selected.

typedef enum Vim_Mode {
   Vim_Mode_Command, // 0: what a deck opens in
   Vim_Mode_Insert,
   Vim_Mode_Visual,
} Vim_Mode;

// In App_State. Changed by ops only.
typedef struct Vim_State Vim_State;
struct Vim_State {
   Vim_Mode mode;

   // Visual: whole lines are selected (V), not what is between the mark and
   // the cursor (v).
   B32 selects_lines;

   // The start of a command of more than one key, as typed so far: d, y, g.
   U8  pending[8];
   U64 pending_size;
};

// The mode's name, for the status line, and the tag of the colour it is
// shown in: one of the theme's (FR_BeginUIBuild, frame.c).
String8 Vim_ModeName(const App_State* app_state);
String8 Vim_ModeTag(const App_State* app_state);

// Esc has something of vim's to leave: insert or visual mode, or the start
// of a command. Then Esc is vim's (Vim_Escape) and nothing else's.
B32 Vim_WantsEsc(const App_State* app_state);

// For ops only.
//
// Vim_EditorOpened: a deck's text was opened. It starts in command mode.
// Vim_StartTyping: something was put in the text to be typed into or over -
// a click on the preview's slide, a line from the Format panel. Insert mode.
// Vim_AfterTextMouse: the mouse put the cursor in the text, or dragged it.
// A drag in command mode is visual mode, a click in visual mode leaves it.
void Vim_EditorOpened(App_State* app_state);
void Vim_StartTyping(App_State* app_state);
void Vim_AfterTextMouse(App_State* app_state);

////////////////////////////////
//~ bgz: state ops (vim.c)

// The keys and typing in the text: what is left of the frame's events after
// the edit screen's own keys. Vim's keys first, by the mode, and the rest
// is the editor's (Editor_Input).
typedef struct Vim_KeyInputOpParams Vim_KeyInputOpParams;
struct Vim_KeyInputOpParams {
   Editor_Box box;
};
Vim_KeyInputOpParams* StateOp_Vim_KeyInput(StateOp_List* ops);

// Esc: insert and visual mode back to command mode, and the start of a
// command dropped.
void StateOp_Vim_Escape(StateOp_List* ops);

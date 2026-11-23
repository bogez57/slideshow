#pragma once

////////////////////////////////
//~ bgz: editor
//
// A plain text editor for one file: the text, the cursor and selection, undo,
// and the box it is typed into and drawn in. What the text means is not its
// business - edit\edit.c hands the text to the deck layer.
//
// The text is UTF-8 with \n line endings and no tabs, whatever the file had:
// Editor_Open converts, and Editor_Save writes the file's own line endings back.
//
// Lines too long for the box are wrapped, at spaces where there are any. The
// font must be one whose characters are all the same width.
//
// Memory is two arenas of the editor's own, both released by Editor_Close. One
// has the editor and its text. The other has the undo history, whose copies
// of the text come and go in no order an arena can follow: the ones let go
// of are left where they are until they add up, and then the ones still
// wanted are copied to a fresh arena and the old one is cleared
// (_Editor_HistoryTidy in editor.c).

// How many changes can be undone. Each is a whole copy of the text, which for
// a deck is small.
#define ED_MAX_UNDO 256

typedef struct Editor_Snapshot Editor_Snapshot;
struct Editor_Snapshot {
   U8* text;
   U64 size;
   U64 cursor;
   U64 mark;
};

// What kind of change the last one was. Changes of the same kind that follow
// each other closely are undone together.
typedef enum Editor_EditKind {
   Editor_EditKind_None,
   Editor_EditKind_Type,
   Editor_EditKind_Delete,
   Editor_EditKind_Other, // never joined to the one before
} Editor_EditKind;

typedef struct Editor_Editor Editor_Editor;
struct Editor_Editor {
   Arena*    arena;         // the editor, its file's name and its text
   String8   file;
   B32       file_crlf;     // the file's line endings were \r\n
   DenseTime file_modified; // the file's time when read or written. 0: no file yet

   U8* text;
   U64 size;
   U64 cap;

   // The selection is between mark and cursor, either one first. Equal: none.
   U64 cursor;
   U64 mark;
   S64 want_column; // the column up and down try to keep. -1: the cursor's own

   U64 edit_count;  // goes up with every change to the text
   U64 saved_count; // edit_count when the text was last read or written

   Editor_Snapshot undo[ED_MAX_UNDO];
   U64         undo_count;
   Editor_Snapshot redo[ED_MAX_UNDO];
   U64         redo_count;
   Arena*      history_arena; // the snapshots' text - and that of ones let go of since the last tidy
   Arena*      history_spare; // empty: what the next tidy copies to
   Editor_EditKind last_edit_kind;
   U64         last_edit_us;

   F32 scroll_y;         // pixels the text is scrolled up by
   B32 scroll_to_cursor; // the cursor moved: bring it into view when next drawn
   U64 last_input_us;    // the cursor does not blink just after input
};

// One row of text as drawn: a line, or a part of one that is too long.
typedef struct Editor_Row Editor_Row;
struct Editor_Row {
   U64 start;
   U64 end;        // one past the last character drawn. Not the \n
   B32 line_start; // the row begins a line
   B32 line_end;   // the row ends a line: a \n or the end of the text follows
};

typedef struct Editor_RowArray Editor_RowArray;
struct Editor_RowArray {
   Editor_Row* v;
   U64     count;
};

// Where the text is drawn and how big. The same one is given to Editor_Input,
// the mouse's calls and Editor_Draw in a frame.
typedef struct Editor_Box Editor_Box;
struct Editor_Box {
   Rng2F32 rect;
   FNT_Tag font;
   F32     px;
};

// Never 0. A file that does not exist gives an empty editor, and Editor_Save
// makes the file.
Editor_Editor* Editor_Open(String8 file);
void       Editor_Close(Editor_Editor* editor);

// The text as it is now. Good until the next change.
String8 Editor_Text(Editor_Editor* editor);

B32 Editor_HasUnsavedChanges(Editor_Editor* editor);
B32 Editor_Save(Editor_Editor* editor);

// Whether the file has been written by something else since it was read or
// saved here.
B32 Editor_FileHasChanged(Editor_Editor* editor);

// Reads the file again, dropping the text here and its undo history.
void Editor_Reload(Editor_Editor* editor);

// Puts string where the selection is, or at the cursor if there is none.
void Editor_Insert(Editor_Editor* editor, String8 string, Editor_EditKind kind);

// Puts string on a line of its own: straight in if the cursor's line is
// empty, else on a new line under it. The part of string from select_start to
// select_end is left selected, to be typed over - both the same for a plain
// cursor there.
void Editor_InsertLine(Editor_Editor* editor, String8 string, U64 select_start, U64 select_end);

// Puts string in place of the text from start to end, and the cursor after it.
void Editor_ReplaceRange(Editor_Editor* editor, U64 start, U64 end, String8 string);

// Puts mark before and after the selection, which stays selected - or takes
// them away, if they are there already. With no selection, writes
// placeholder between two marks and selects it, to be typed over.
void Editor_WrapSelection(Editor_Editor* editor, String8 mark, String8 placeholder);

void Editor_SetCursor(Editor_Editor* editor, U64 pos);

// The changes made since the undo history had undo_count_before steps are
// undone as one: for an op that changes the text in several places at once.
// Called with editor->undo_count before the first of them.
void Editor_JoinUndoSince(Editor_Editor* editor, U64 undo_count_before);

// Keys and typing. Events it uses are eaten, so what the caller wants for
// itself (its own shortcuts) it takes first. arena is for the frame: the row
// layout and the clipboard's text go on it.
void Editor_Input(Editor_Editor* editor, Arena* arena, WM_EventList* events, Editor_Box box);

// The mouse, as the caller's box over the text has it (the edit screen's UI
// box): a press at pos puts the cursor at the nearest gap between letters
// there - selecting from where it was, with is_selecting; a drag, while the
// button that went down in the text stays down, over the box or not, takes
// the cursor along and leaves the mark; the wheel scrolls notches down.
void Editor_Press(Editor_Editor* editor, Arena* arena, Editor_Box box, Vec2F32 pos, B32 is_selecting);
void Editor_Drag(Editor_Editor* editor, Arena* arena, Editor_Box box, Vec2F32 pos);
void Editor_Wheel(Editor_Editor* editor, Editor_Box box, F32 notches);

void Editor_Draw(Editor_Editor* editor, Arena* arena, Editor_Box box);

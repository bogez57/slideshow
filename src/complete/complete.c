////////////////////////////////
//~ bgz: complete
//
// What the list has, by what the line has before the word being typed. The
// names are the deck's format's (deck.h), and what is said of each is kept
// short: it is read at a glance, while typing.

PRIVATE Complete_Item _complete_lines[] = {
    {Str8LitComp("@picture"), Str8LitComp("pick a picture file to put on this slide"), 0, 1},
    {Str8LitComp("@note"), Str8LitComp("a note to yourself, not on the slide"), 1},
    {Str8LitComp("@box"), Str8LitComp("another block of words, to place on its own"), 0},
    {Str8LitComp("@place"), Str8LitComp("where a picture, the text or the title goes"), 1},
    {Str8LitComp("@size"), Str8LitComp("how large the words are"), 1},
    {Str8LitComp("@color"), Str8LitComp("the colour of the words"), 1},
    {Str8LitComp("@layout"), Str8LitComp("how this slide is laid out"), 1},
    {Str8LitComp("@theme"), Str8LitComp("the whole deck's colours"), 1},
    {Str8LitComp("@video"), Str8LitComp("how this slide's video plays"), 1},
};

PRIVATE Complete_Item _complete_place[] = {
    {Str8LitComp("picture"), Str8LitComp("then left top width height, 0 to 1"), 1},
    {Str8LitComp("text"), Str8LitComp("then left top width height, 0 to 1"), 1},
    {Str8LitComp("title"), Str8LitComp("then left top width height, 0 to 1"), 1},
};

PRIVATE Complete_Item _complete_size[] = {
    {Str8LitComp("title"), Str8LitComp("then a number: 1.5 is half as large again"), 1},
    {Str8LitComp("text"), Str8LitComp("then a number: 0.8 is a little smaller"), 1},
};

PRIVATE Complete_Item _complete_color[] = {
    {Str8LitComp("title"), Str8LitComp("then a colour's name, or #rrggbb"), 1},
    {Str8LitComp("text"), Str8LitComp("then a colour's name, or #rrggbb"), 1},
};

PRIVATE Complete_Item _complete_video[] = {
    {Str8LitComp("file"), Str8LitComp("pick a video file to put on this slide"), 0, 1},
    {Str8LitComp("loop"), Str8LitComp("starts over at its end"), 0},
    {Str8LitComp("mute"), Str8LitComp("no sound"), 0},
    {Str8LitComp("manual"), Str8LitComp("waits for K to start"), 0},
};

PRIVATE Complete_Item _complete_layouts[] = {
    {Str8LitComp("picture-right"), Str8LitComp("words left, picture right: as with no line"), 0},
    {Str8LitComp("picture-left"), Str8LitComp("picture left, words right"), 0},
    {Str8LitComp("picture-behind"), Str8LitComp("the picture fills the slide, behind the words"), 0},
    {Str8LitComp("center"), Str8LitComp("every line in the middle"), 0},
};

PRIVATE Complete_Item _complete_themes[] = {
    {Str8LitComp("dark"), Str8LitComp("light words on dark: as with no line"), 0},
    {Str8LitComp("light"), Str8LitComp("dark words on white"), 0},
    {Str8LitComp("blue"), Str8LitComp("light words on blue"), 0},
    {Str8LitComp("paper"), Str8LitComp("dark words on cream"), 0},
};

// How many of the list are shown at once: the rest are scrolled to.
#define _COMPLETE_ROWS_SHOWN 10

// The most words an @ line has before the one the list is for.
#define _COMPLETE_MAX_WORDS 4

Complete_List
Complete_ListFromState(const App_State* app_state, Arena* arena) {
   Complete_List  result = {0};
   Editor_Editor* editor = app_state->editor;

   B32 can_show = (editor != 0 && app_state->is_editing && !app_state->hide_text && !app_state->show_open &&
                   app_state->vim.mode == Vim_Mode_Insert && editor->cursor == editor->mark);
   if (!can_show) {
      return result;
   }

   //- bgz: the line up to the cursor: its words, and the one being typed
   U64 cursor = editor->cursor;
   U64 first = Editor_LineStart(editor, cursor);
   while (first < cursor && editor->text[first] == ' ') first += 1;
   if (first >= cursor || editor->text[first] != '@') {
      return result;
   }

   String8 words[_COMPLETE_MAX_WORDS];
   U64     word_count = 0;
   U64     word_start = first;
   for (U64 at = first; at < cursor; at += 1) {
      if (editor->text[at] == ' ') {
         if (at > word_start) {
            if (word_count == _COMPLETE_MAX_WORDS) {
               return result;
            }
            words[word_count] = Str8(editor->text + word_start, at - word_start);
            word_count += 1;
         }
         word_start = at + 1;
      }
   }
   String8 prefix = Str8(editor->text + word_start, cursor - word_start);

   //- bgz: what can come there
   Complete_Item* from = 0;
   U64            from_count = 0;
   Complete_Item* colors = 0;
#define _Complete_From(items_) (from = (items_), from_count = ArrayCount(items_))
#define _Complete_WordIs(idx_, name_) Str8Match(words[idx_], Str8Lit(name_), StringMatchFlag_CaseInsensitive)
   if (word_count == 0) {
      _Complete_From(_complete_lines);
   } else if (word_count == 1 && _Complete_WordIs(0, "@place")) {
      _Complete_From(_complete_place);
   } else if (word_count == 1 && _Complete_WordIs(0, "@size")) {
      _Complete_From(_complete_size);
   } else if (word_count == 1 && _Complete_WordIs(0, "@color")) {
      _Complete_From(_complete_color);
   } else if (word_count == 1 && _Complete_WordIs(0, "@layout")) {
      _Complete_From(_complete_layouts);
   } else if (word_count == 1 && _Complete_WordIs(0, "@theme")) {
      _Complete_From(_complete_themes);
   } else if (word_count >= 1 && word_count <= 2 && _Complete_WordIs(0, "@video")) {
      _Complete_From(_complete_video);
   } else if (word_count == 2 && _Complete_WordIs(0, "@color")) {
      // The colours with names, as the deck layer has them.
      colors = PushArray(arena, Complete_Item, DECK_NAMED_COLOR_COUNT);
      for (U64 i = 0; i < DECK_NAMED_COLOR_COUNT; i += 1) {
         colors[i].name = Deck_NamedColorFromIdx(i).name;
      }
      from = colors;
      from_count = DECK_NAMED_COLOR_COUNT;
   }
#undef _Complete_From
#undef _Complete_WordIs
   if (from == 0) {
      return result;
   }

   //- bgz: those that start with what is typed
   const Complete_State* state = &app_state->complete;
   B32 is_dismissed = (state->has_dismissed && state->dismissed_word_start == word_start &&
                       state->dismissed_edit_count == editor->edit_count);
   if (is_dismissed) {
      return result;
   }

   result.v = PushArray(arena, Complete_Item, from_count);
   for (U64 i = 0; i < from_count; i += 1) {
      String8 name = from[i].name;
      // Not what the line has already: the second of @video loop mute.
      B32 is_written = 0;
      for (U64 word_idx = 1; word_idx < word_count; word_idx += 1) {
         is_written |= (from == _complete_video && Str8Match(words[word_idx], name, StringMatchFlag_CaseInsensitive));
      }
      // A file is picked in the line's place: not once the line has more.
      if (is_written || (from[i].picks_file && word_count > 1)) {
         continue;
      }
      if (name.size >= prefix.size && Str8Match(Str8Prefix(name, prefix.size), prefix, StringMatchFlag_CaseInsensitive)) {
         result.v[result.count] = from[i];
         result.count += 1;
      }
   }

   // Nothing left to say when the one thing it could be is written out.
   if (result.count == 1 && !result.v[0].takes_more && !result.v[0].picks_file && result.v[0].name.size == prefix.size) {
      result.count = 0;
   }

   result.word_start = word_start;
   result.line_start = first;
   result.cursor = cursor;
   result.edit_count = editor->edit_count;
   result.prefix = prefix;
   if (state->selected_word_start == word_start && state->selected_edit_count == editor->edit_count &&
       state->selected_idx < result.count) {
      result.selected_idx = state->selected_idx;
   }
   return result;
}

////////////////////////////////
//~ bgz: state ops

typedef struct Complete_SelectOpParams Complete_SelectOpParams;
struct Complete_SelectOpParams {
   U64 selected_idx;
   U64 word_start;
   U64 edit_count;
};
void
Complete_Select(App_State* app_state, StateOp_List* ops, Complete_SelectOpParams* params) {
   UnusedVariable(ops);
   app_state->complete.selected_idx = params->selected_idx;
   app_state->complete.selected_word_start = params->word_start;
   app_state->complete.selected_edit_count = params->edit_count;
}
STATE_OP(Complete_Select)

typedef struct Complete_DismissOpParams Complete_DismissOpParams;
struct Complete_DismissOpParams {
   U64 word_start;
   U64 edit_count;
};
void
Complete_Dismiss(App_State* app_state, StateOp_List* ops, Complete_DismissOpParams* params) {
   UnusedVariable(ops);
   app_state->complete.has_dismissed = 1;
   app_state->complete.dismissed_word_start = params->word_start;
   app_state->complete.dismissed_edit_count = params->edit_count;
}
STATE_OP(Complete_Dismiss)

// Writes text over the word, from word_start to cursor. Only onto the text
// the list was made from: a key typed in the same frame, run before this,
// has moved the word, and then this does nothing.
typedef struct Complete_AcceptOpParams Complete_AcceptOpParams;
struct Complete_AcceptOpParams {
   U64     word_start;
   U64     cursor;
   U64     edit_count;
   String8 text; // on ops->arena
};
void
Complete_Accept(App_State* app_state, StateOp_List* ops, Complete_AcceptOpParams* params) {
   UnusedVariable(ops);
   Editor_Editor* editor = app_state->editor;
   if (editor != 0 && editor->edit_count == params->edit_count && params->cursor <= editor->size) {
      // Its own step to undo: the whole name, not part of a run of typing.
      Editor_ReplaceRange(editor, params->word_start, params->cursor, params->text);
      editor->last_input_us = NowTimeUS();
   }
}
STATE_OP(Complete_Accept)

////////////////////////////////
//~ bgz: input

void
Complete_Input(const App_State* app_state, StateOp_List* ops) {
   Temp          scratch = ScratchBegin(0, 0);
   Complete_List list = Complete_ListFromState(app_state, scratch.arena);
   if (list.count != 0) {
      WM_EventList* events = app_state->events;
      WM_Window     window = app_state->window;

      //- bgz: Up and Down, around at either end
      S64 delta = 0;
      while (Core_WM_KeyPress(events, window, 0, WM_Key_Down)) delta += 1;
      while (Core_WM_KeyPress(events, window, 0, WM_Key_Up)) delta -= 1;
      if (delta != 0) {
         S64 count = (S64)list.count;
         S64 idx = (((S64)list.selected_idx + delta) % count + count) % count;
         Complete_SelectOpParams* select = StateOp_Complete_Select(ops);
         select->selected_idx = (U64)idx;
         select->word_start = list.word_start;
         select->edit_count = list.edit_count;
         list.selected_idx = (U64)idx;
      }

      //- bgz: Tab and Return take the one selected - unless it is written
      // out already, and then the key is the editor's, as with no list
      Complete_Item* item = &list.v[list.selected_idx];
      B32            is_written = (!item->takes_more && !item->picks_file && Str8Match(item->name, list.prefix, 0));
      // With nothing of the word typed yet, Tab only: Return there ends a
      // line that may well be done (@video loop).
      B32 takes = 0;
      if (!is_written) {
         takes = Core_WM_KeyPress(events, window, 0, WM_Key_Tab) ||
                 (list.prefix.size != 0 && Core_WM_KeyPress(events, window, 0, WM_Key_Return));
      }
      if (takes && item->picks_file) {
         // bgz's shell: the system's window, which does not return until it
         // is answered. It lists every file: a picture or a video is what
         // can be taken. Cancelled, the line is left as typed.
         B32     is_video = Str8Match(list.v[list.selected_idx].name, Str8Lit("file"), 0);
         String8 folder = (app_state->deck != 0) ? FR_WholePath(scratch.arena, app_state->deck->folder) : Str8Zero();
         String8 picked = ShPickFile(ops->arena, is_video ? Str8Lit("Pick a video") : Str8Lit("Pick a picture"), folder, Str8Zero());
         if (picked.size != 0 && (Image_NameIsImage(picked) || Video_NameIsVideo(picked))) {
            // What was typed goes, and the file is put on the slide as one
            // dropped on the window is: copied into the deck's folder if it
            // is not there, and its line written.
            Complete_AcceptOpParams* accept = StateOp_Complete_Accept(ops);
            // The whole line, with the \n before it: the picture's line is
            // written under the line above, or under the slide's other
            // pictures, and no empty line is left where this one was.
            U64 from = Editor_LineStart(app_state->editor, list.line_start);
            accept->word_start = (from > 0) ? from - 1 : from;
            accept->cursor = list.cursor;
            accept->edit_count = list.edit_count;
            accept->text = Str8Zero();
            StateOp_Edit_DropPicture(ops)->path = picked;
         } else if (picked.size != 0) {
            StateOp_App_Notice(ops)->notice = Str8Lit("That is not a picture or a video.");
         }
      } else if (takes) {
         Complete_AcceptOpParams* accept = StateOp_Complete_Accept(ops);
         accept->word_start = list.word_start;
         accept->cursor = list.cursor;
         accept->edit_count = list.edit_count;
         accept->text = PushStr8F(ops->arena, "%S%s", item->name, item->takes_more ? " " : "");
      }

      //- bgz: Esc puts it away, and does nothing else this press
      if (Core_WM_KeyPress(events, window, 0, WM_Key_Esc)) {
         Complete_DismissOpParams* dismiss = StateOp_Complete_Dismiss(ops);
         dismiss->word_start = list.word_start;
         dismiss->edit_count = list.edit_count;
      }
   }
   ScratchEnd(scratch);
}

////////////////////////////////
//~ bgz: the list, in the UI layer
//
// Under the word, or over it when there is no room under: a panel with a
// row for each item, the name and then what it does, dimmer. The one
// selected is on a wash. It takes no clicks: the keys are Complete_Input's.

void
Complete_Build(const App_State* app_state, Editor_Box box, Vec2F32 window_dim, F32 ui, F32 px) {
   Complete_List list = Complete_ListFromState(app_state, app_state->frame_arena);
   if (list.count == 0) {
      return;
   }
   Rng2F32 word = Editor_RectFromPos(app_state->editor, app_state->frame_arena, box, list.word_start);
   if (word.max.y <= word.min.y) {
      // The word is scrolled out of the box.
      return;
   }

   FNT_Tag name_font = app_state->fonts.mono;
   FNT_Tag what_font = app_state->fonts.regular;
   F32     line_h = Max(Text_LineHeight(name_font, px), Text_LineHeight(what_font, px));
   F32     row_h = (F32)(S32)(line_h * 1.45f);
   F32     pad = (F32)(S32)(10.0f * ui);

   // Ten rows at most, scrolled to keep the one selected in view.
   U64 shown = Min(list.count, _COMPLETE_ROWS_SHOWN);
   U64 first = (list.selected_idx >= shown) ? list.selected_idx - shown + 1 : 0;

   F32 name_w = 0;
   F32 what_w = 0;
   for (U64 i = 0; i < list.count; i += 1) {
      name_w = Max(name_w, Text_Width(name_font, px, list.v[i].name));
      what_w = Max(what_w, Text_Width(what_font, px, list.v[i].what));
   }
   F32 width = (F32)(S32)(pad + name_w + (what_w > 0 ? pad * 2.0f + what_w : 0) + pad);
   F32 height = row_h * (F32)shown + pad * 0.5f;

   F32 x = Clamp(0, word.min.x - pad, Max(window_dim.x - width, 0));
   F32 y = word.max.y + 2.0f * ui;
   if (y + height > window_dim.y && word.min.y - 2.0f * ui - height >= 0) {
      y = word.min.y - 2.0f * ui - height;
   }
   x = (F32)(S32)x;
   y = (F32)(S32)y;

   Vec4F32 name_color = UI_ThemeColor(Str8Lit("syntax"), UI_ColorSlot_Text);
   Vec4F32 what_color = UI_ThemeColor(Str8Lit("dim"), UI_ColorSlot_Text);

   // What is built first is drawn over what is built after: the words, the
   // wash under the one selected, then the panel under all of it.
   F32 row_y = y + (F32)(S32)(pad * 0.25f);
   for (U64 i = first; i < first + shown; i += 1) {
      Complete_Item* item = &list.v[i];
      F32            text_y = (F32)(S32)(row_y + (row_h - line_h) * 0.5f);
      FR_BuildWords(name_font, px, V2F32(x + pad, text_y), name_color, item->name, "###complete name %llu", (unsigned long long)i);
      if (item->what.size != 0) {
         FR_BuildWords(what_font, px, V2F32(x + pad + name_w + pad * 2.0f, text_y), what_color, item->what, "###complete what %llu",
                       (unsigned long long)i);
      }
      if (i == list.selected_idx) {
         UI_SetNextBackgroundColor(V4F32(0.22f, 0.32f, 0.55f, 1.0f));
         FR_BuildRectBox(UI_BoxFlag_DrawBackground, R2F32P(x, row_y, x + width, row_y + row_h), "###complete selected");
      }
      row_y += row_h;
   }

   UI_SetNextBackgroundColor(V4F32(0.12f, 0.125f, 0.145f, 1.0f));
   UI_Box* panel = FR_BuildRectBox(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder, R2F32P(x, y, x + width, y + height), "###complete panel");
   panel->border_color = V4F32(1, 1, 1, 0.16f);
}

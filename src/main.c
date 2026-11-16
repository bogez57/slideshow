////////////////////////////////
//~ bgz: slideshow
//
// The program's one unit: bgz's headers, then each layer's header, then each
// layer's .c, in dependency order. bgz itself is not compiled here - it is
// bgz.dll (src\bgz\build.bat), linked through bin\bgz.lib.
//
//    bin\slideshow.exe            opens the deck that was open last. The
//                                 first time, the open screen, to pick one
//    bin\slideshow.exe <folder>   shows the images in that folder - or its
//                                 slides.md, if it has one
//    bin\slideshow.exe <image>    shows the images in that image's folder,
//                                 starting at it
//    bin\slideshow.exe <file>     opens a text file that describes slides (.md
//                                 or .txt - the format is in deck\deck.h) for
//                                 editing, beside a preview. The file need
//                                 not exist yet
//
// Nothing has to be given: Ctrl+O is the open screen (frame_open.c), where a
// deck is picked or a new one started.
//
// Keys are in frame.c (presenting), frame_edit.c and editor\editor.c (editing).

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h> // printf: the log, to the console as well as its file (FR_WriteLogs)

// bgz lib
#include "bgz_inc.h"

// The UI library - the editor project's own (src\editor\ui there), copied in
// as app code and built on bgz's draw and font cache. Its file-private
// functions are marked PRIVATE, which the editor defines in its state_ops.h;
// the slideshow has no state ops, so it is defined here, ahead of the UI.
#define PRIVATE static
#include "ui/ui_inc.h"

////////////////////////////////
//~ bgz: headers / public types

#include "image/image.h"
#include "text/text.h"
#include "deck/deck.h"
#include "view/view.h"
#include "editor/editor.h"
#include "app_state.h"
#include "frame.h"

////////////////////////////////
//~ bgz: implementations

// The UI times its passes with raddebugger's ProfScope. With no profiler
// built in - the slideshow has none - that is DeferLoop((0), ProfEnd()), and
// the (0) is an unused value. The editor builds with Spall, so it never sees
// this. Only that warning, and only for the UI.
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunused-value"
#endif
#include "ui/ui_inc.c"
#if defined(__clang__)
# pragma clang diagnostic pop
#endif
#include "image/image.c"
#include "text/text.c"
#include "deck/deck.c"
#include "view/view.c"
#include "editor/editor.c"
#include "frame_edit.c"
#include "frame_open.c"
#include "frame.c"

////////////////////////////////
//~ bgz: one frame
//
// The main loop's frame, also run by Windows while the window is being
// resized. A drag of the window's edge runs Windows' own message loop, which
// does not return to ours until the drag ends - so without this the window
// shows a stale, stretched frame the whole time. raddebugger's window manager
// calls update() on every WM_SIZE and WM_PAINT instead, and update() calls
// frame() - which bgz.dll hands on to _Main_RepaintFrame (see
// CoreMainThreadEntry in src\bgz\bgz\base\base_entry_point.h).

// What _Main_RepaintFrame draws: it is called by bgz with no arguments. 0
// until the window is ready to be drawn into.
global App_State* g_repaint_app_state = 0;

// Set while a frame is being drawn, so a repaint asked for from inside one
// does nothing - see _Main_RepaintFrame.
global B32 g_frame_running = 0;

internal void
_Main_RunFrame(App_State* app_state) {
   g_frame_running = 1;

   Core_R_BeginFrame();
   Core_R_WindowBeginFrame(app_state->window, app_state->window_eqp);

   FrameWork(app_state);

   Core_R_WindowEndFrame(app_state->window, app_state->window_eqp);
   Core_R_EndFrame();

   g_frame_running = 0;
}

// A frame with no input, at the window's new size. Called from inside
// Core_WM_GetEvents, part way through gathering this frame's events onto the
// frame arena, so:
//
//  - Everything it puts on the frame arena is given back before it returns
//    (a temp, not ArenaClear), leaving the events gathered so far intact.
//  - It sees no events. app_state->events is still the last frame's list at
//    this point, which lived on the frame arena the main loop has since
//    cleared.
//
// Returns what raddebugger's frame() returns - whether to quit - which only
// raddebugger's own main loops read. Ours is below, so: never.
internal B32
_Main_RepaintFrame(void) {
   App_State* app_state = g_repaint_app_state;

   if (app_state != 0 && !g_frame_running) {
      // The swap chain takes no frame it was not waited for.
      Core_R_WaitForFrame();

      Temp         temp = TempBegin(app_state->frame_arena);
      WM_EventList events = app_state->events;

      MemoryZeroStruct(&app_state->events);
      _Main_RunFrame(app_state);

      app_state->events = events;
      TempEnd(temp);
   }

   return 0;
}

////////////////////////////////
//~ bgz: entry point

internal void
_EntryPoint(CmdLine* cmdline) {
   SetThreadName(Str8Lit("MainThread"));

   App_State app_state = {0};
   app_state.frame_arena = ArenaAlloc();

   // The log, before anything that can have something to say. What is
   // logged before the first frame is written just ahead of it, and after
   // that each frame writes its own (FrameWork).
   app_state.log = LogAlloc();
   LogSelect(app_state.log);
   LogScopeBegin();
   FR_OpenLogFile(&app_state);

   app_state.fonts = TXT_OpenFonts();

   // The UI layer's state, selected for the rest of the run: there is only
   // the one. The icon font is found beside the program, not from the
   // folder it was started in.
   app_state.ui_state = UI_StateAlloc();
   UI_SelectState(app_state.ui_state);
   {
      Temp    scratch = ScratchBegin(0, 0);
      String8 icon_font_path = PushStr8F(scratch.arena, "%S\\..\\data\\fonts\\Icons.ttf", GetProcessInfo()->binary_path);
      app_state.ui_icon_font = FilePathExists(icon_font_path) ? Core_FNT_TagFromPath(icon_font_path) : Core_FNT_TagZero();
      ScratchEnd(scratch);
   }

   // What to show: the first thing on the command line that is not an
   // option - a folder, an image or a text deck. With nothing there, the
   // deck that was open last. And with no such deck - the first run - an
   // empty one, under the open screen.
   FR_LoadRecents(&app_state);
   if (cmdline->inputs.first != 0) {
      FR_OpenPath(&app_state, cmdline->inputs.first->string);
   } else if (app_state.recents.count != 0) {
      Temp    scratch = ScratchBegin(0, 0);
      String8 last = PushStr8Copy(scratch.arena, app_state.recents.v[0]); // opening it makes the list again
      FR_OpenPath(&app_state, last);
      ScratchEnd(scratch);
   } else {
      app_state.deck = DECK_OpenFolder(Str8Zero());
      OPEN_Show(&app_state);
   }

   // UseDefaultPosition: where Windows puts a new window - only the rect's
   // size is used.
   app_state.window = Core_WM_WindowOpen(R2F32P(0, 0, 1280, 720), WM_WindowFlag_UseDefaultPosition, Str8Lit("Slideshow"));
   app_state.window_eqp = Core_R_WindowEquip(app_state.window);

   // The first frame, drawn and presented while the window is still hidden,
   // so it shows with its contents rather than blank. Without this, nothing
   // draws until the loop handles the WM_PAINT that showing it sends.
   {
      Temp temp = TempBegin(app_state.frame_arena);
      FR_WriteLogs(&app_state, LogScopeEnd(app_state.frame_arena));
      TempEnd(temp);
   }

   g_repaint_app_state = &app_state;
   _Main_RunFrame(&app_state);

   Core_WM_WindowFirstPaint(app_state.window);
   Core_WM_WindowFocus(app_state.window);

   while (!app_state.quit) {
      Core_R_WaitForFrame(); // vsync

      app_state.events = Core_WM_GetEvents(app_state.frame_arena, 0);
      _Main_RunFrame(&app_state);

      // Between frames, not from inside one: the change of size makes Windows
      // ask for a repaint before the call returns, and _Main_RepaintFrame
      // does nothing while a frame is running - the window would show the
      // frame drawn for its old size, stretched, until the next one.
      if (app_state.toggle_fullscreen) {
         app_state.toggle_fullscreen = 0;
         Core_WM_WindowSetFullscreen(app_state.window, !Core_WM_WindowIsFullscreen(app_state.window));
      }

      ArenaClear(app_state.frame_arena);
   }

   g_repaint_app_state = 0;

   if (app_state.deck_watch != 0) {
      FileWatchClose(app_state.deck_watch);
   }
   FileClose(app_state.log_file);

   Core_R_Shutdown();
}

int main(int argc, char** argv) {
   UnusedVariable(argc);
   UnusedVariable(argv);

   CoreMainThreadEntry(_EntryPoint, _Main_RepaintFrame);
   return 0;
}

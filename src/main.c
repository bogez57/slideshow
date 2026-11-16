////////////////////////////////
//~ bgz: slideshow
//
// The program's one unit: bgz's headers, then each layer's header, then each
// layer's .c, in dependency order. bgz itself is not compiled here - it is
// bgz.dll (src\bgz\build.bat), linked through bin\bgz.lib.
//
//    bin\slideshow.exe            shows the images in data\slides
//    bin\slideshow.exe <folder>   shows the images in that folder
//
// Keys are in frame.c.

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

// The folder shown when none is given, relative to where the program is
// started from.
#define DEFAULT_SLIDES_FOLDER "data\\slides"

// bgz lib
#include "bgz_inc.h"

////////////////////////////////
//~ bgz: headers / public types

#include "image/image.h"
#include "deck/deck.h"
#include "app_state.h"

////////////////////////////////
//~ bgz: implementations

#include "image/image.c"
#include "deck/deck.c"
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

   // The folder to show: the first thing on the command line that is not an
   // option, or the default.
   String8 folder = Str8Lit(DEFAULT_SLIDES_FOLDER);
   if (cmdline->inputs.first != 0) {
      folder = cmdline->inputs.first->string;
   }
   app_state.deck = DECK_OpenFolder(folder);
   printf("%llu slides in %.*s\n", (unsigned long long)app_state.deck->slide_count, Str8VArg(app_state.deck->folder));

   // UseDefaultPosition: where Windows puts a new window - only the rect's
   // size is used.
   app_state.window = Core_WM_WindowOpen(R2F32P(0, 0, 1280, 720), WM_WindowFlag_UseDefaultPosition, Str8Lit("Slideshow"));
   app_state.window_eqp = Core_R_WindowEquip(app_state.window);

   // The first frame, drawn and presented while the window is still hidden,
   // so it shows with its contents rather than blank. Without this, nothing
   // draws until the loop handles the WM_PAINT that showing it sends.
   g_repaint_app_state = &app_state;
   _Main_RunFrame(&app_state);

   Core_WM_WindowFirstPaint(app_state.window);
   Core_WM_WindowFocus(app_state.window);

   while (!app_state.quit) {
      Core_R_WaitForFrame(); // vsync

      app_state.events = Core_WM_GetEvents(app_state.frame_arena, 0);
      _Main_RunFrame(&app_state);

      ArenaClear(app_state.frame_arena);
   }

   g_repaint_app_state = 0;

   Core_R_Shutdown();
}

int main(int argc, char** argv) {
   UnusedVariable(argc);
   UnusedVariable(argv);

   CoreMainThreadEntry(_EntryPoint, _Main_RepaintFrame);
   return 0;
}

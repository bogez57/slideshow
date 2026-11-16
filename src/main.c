////////////////////////////////
//~ bgz: slideshow
//
// The program's one unit: bgz's headers, then each layer's header, then each
// layer's .c, in dependency order. bgz itself is not compiled here - it is
// bgz.dll (src\bgz\build.bat), linked through bin\bgz.lib.

#define _CRT_SECURE_NO_WARNINGS

// bgz lib
#include "bgz_inc.h"

////////////////////////////////
//~ bgz: app state

typedef struct App_State App_State;
struct App_State {
   WM_Window    window;
   R_Handle     window_eqp;
   Arena*       frame_arena; // cleared at the end of every frame
   WM_EventList events;      // this frame's, on frame_arena
   B32          quit;
};

// What _Main_RepaintFrame draws: it is called by bgz with no arguments. 0
// until the window is ready to be drawn into.
global App_State* g_repaint_app_state = 0;

// Set while a frame is being drawn, so a repaint asked for from inside one
// does nothing - see _Main_RepaintFrame.
global B32 g_frame_running = 0;

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

internal void
_Main_RunFrame(App_State* app_state) {
   g_frame_running = 1;

   for (WM_Event* event = app_state->events.first; event != 0; event = event->next) {
      if (event->kind == WM_EventKind_WindowClose) {
         app_state->quit = 1;
      }
   }

   Core_R_BeginFrame();
   Core_R_WindowBeginFrame(app_state->window, app_state->window_eqp);
   Core_D_BeginFrame();

   // Nothing to show yet: the window filled with one color.
   D_Bucket* bucket = Core_D_BucketMake();
   Core_D_BucketScope(bucket) {
      Rng2F32 client_rect = Core_WM_ClientRectFromWindow(app_state->window);
      Vec2F32 client_dim = Dim2F32(client_rect);
      Core_D_Rect(R2F32P(0, 0, client_dim.x, client_dim.y), R2F32P(0, 0, 1, 1), V4F32(0.10f, 0.10f, 0.12f, 1.0f));
   }
   Core_D_SubmitBucket(app_state->window, app_state->window_eqp, bucket);

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
   UnusedVariable(cmdline);
   SetThreadName(Str8Lit("MainThread"));

   App_State app_state = {0};
   app_state.frame_arena = ArenaAlloc();

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

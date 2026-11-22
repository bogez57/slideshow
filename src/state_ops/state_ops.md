# state_ops

State ops: the one way app state is changed. A layer pushes an op onto a queue during the frame, and `_FR_UpdateAppState` (frame.c) runs the queue in order, which is where `App_State` is written. It is the editor project's state ops (src\editor\state_ops there) without the metaprogram: each op is written by hand.

An op is a function `<Name>(App_State*, StateOp_List*, <Name>OpParams*)` with `STATE_OP(<Name>)` under it, or `<Name>(App_State*, StateOp_List*)` with `STATE_OP_NO_PARAMS(<Name>)`. The macro makes its pusher, `StateOp_<Name>(ops)`, which queues the op and returns its params, zeroed, to fill in: `StateOp_App_GoToSlide(ops)->slide_idx = 3;`. Code that is not an op pushes it and never calls it. An op that is running may call another op's function directly when it needs that done before it carries on: a push would only run after it returns.

An op other layers push has its params struct and its pusher declared in its layer's header. One only its own `.c` pushes is declared nowhere else.

The frame runs the queue twice, as the editor's does: at its start, for what the UI build pushed last frame, and after the input, so a key is drawn the frame it is pressed. A click on a UI box is drawn the frame after.

`_FR_UpdateAppState` is the only function handed an `App_State` it can write: `FrameWork` gives everything else a `const App_State*`, so a write outside an op does not compile. After the ops it also does what follows from them: makes the deck again from the text if that changed (`FR_SyncDeck`), finds the slide the cursor is on while editing, and once a frame runs what goes on by itself (`Copy_Update`, `SlideVideo_Update`).

What is not app state is written with no op, by its own layer: frame.c's `_frame_state` (the fade's clock, the wheel's left-over, the log's file), and a layer's view - `g_edit_view` (the preview as laid out, the panel's lists), `g_caret_view` (scrolls, a scroll bar held), `g_place_view` (a drag), open.c's `_open_view` (its rows) - as the editor project's frame state and views are. A view's value that a slide is drawn with is put on the slide only while it is drawn (`Caret_BeforeDraw`, `Place_PutDrag`).

Where the ops are, each layer's `.md` naming its own: app_state.c (the notice, quitting, the overlays, playing, opening a deck, going to a slide), open, presenter, slide_video, edit (the text, the cursor and the edit screen's panels), caret, place and look.

A function that changes the text or app state and is not an op is an op's helper, called from ops only: `FR_OpenPath`, `FR_SyncDeck`, `Caret_Click`, `Edit_PutLineFor`, `Edit_TakeOutLine`, `Copy_Start`, the editor layer's `Editor_` functions. The const pointer does not reach through `editor` and `deck`, so that much is by rule, not by the compiler.

Not ops: main.c makes the `App_State` at the start, and its loop opens and closes the presenter's window (`Presenter_Toggle`) and turns fullscreen on and off between frames, from flags ops set. A slide's pictures are loaded when it is first drawn (`Deck_LoadSlide`).
## State

- The queue, a `StateOp_List`: made in `_EntryPoint` (main.c), and `App_State`'s `op_list` points at it. Its arena holds the ops and their params until they have run.

## Functions

- `StateOp_Init`: zeroes a `StateOp_List` and gives it its arena.
- `StateOp_PushRaw`: queues a function with zeroed params of a size. The pushers `STATE_OP` makes call it.
- `StateOp_ProcessList`: runs the queue in push order, including ops pushed while it runs, then empties it and its arena.
- `STATE_OP`, `STATE_OP_NO_PARAMS`: under an op's function, make its pusher.
- `PRIVATE`: `static`, for a layer's own functions and variables, which start with `_`.

## Used by

- main.c: `StateOp_Init` at startup.
- frame.c: `_FR_UpdateAppState` calls `StateOp_ProcessList`.
- app_state.c: its ops. Every layer that shows a notice, quits or asks for fullscreen pushes them.

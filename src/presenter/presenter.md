# presenter

The presenter's window: a second window for whoever presents, with the slide showing, the next one, the notes, a clock and every slide small, while the main window shows only the slide. V, while presenting, opens and closes it. Its keys are handed to the main window; its mouse is its own. Its picture is at the top of presenter.c.

## State

In `App_State`:

- `toggle_presenter`: a frame asked for the window to be opened or closed (`App_TogglePresenter`), which the main loop does between frames.
- `presenter_open`, `presenter_window`, `presenter_window_eqp`: the window.
- `presenter_started_us`, `presenter_paused_us`: its clock, changed by the ops below.
- `presenter_ui_state`, `presenter_ui_last_frame_us`: its own UI state.

## Functions and ops

- `Presenter_Toggle`: opens or closes the window. Between frames only, from the main loop: not an op.
- `Presenter_Input`: takes the window's events off the frame's.
- `Presenter_Draw`: builds and draws the window, in its own frame after the main window's. Its clicks push ops, run at the start of the next frame: `App_GoToSlide` for a slide, slide_video's `SlideVideo_ToggleSlide` for the video's button, and its own for the clock.
- Ops: `Presenter_ToggleClock`, `Presenter_ResetClock`.

## Used by

- main.c: toggles it between frames, draws it, and closes it at the end.
- frame.c: its input, while presenting.

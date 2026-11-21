# copy

Files copied into the deck's folder in the background. A video dropped on the edit screen goes in the deck's folder, and can be very large, so it is not waited for: the slide names the file at once, plays it from where it was dropped from until the copy is whole, and is moved over to the copy when the video is not playing. The steps are at the top of copy.c.

## State

- `App_State`'s `first_copy`: the copies under way, a `COPY_Job` each (its fields are copy.c's), each with a thread.
- `quit_warned_copy`: closing while one is under way has been warned about once.

## Functions

- `COPY_Start`: starts copying a file into the deck's folder.
- `COPY_IsCopyingTo`, `COPY_IsBusy`.
- `COPY_Update`: once a frame, moves slides over to copies that are whole, and says what failed.
- `COPY_CancelAll`: at the end of the run.

## Used by

- edit: a dropped video.
- frame.c: the update, and the warning when closing.
- main.c: cancels what is left when the program ends.

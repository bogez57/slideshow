# complete

The list under an @ line as it is typed in the deck's text: the @ lines there are, and after one, what can come next - picture, text or title, the themes, the layouts, the colours, the sides of `@align` - each with a few words on what it does. `@picture`, and `file` after `@video`, write nothing: taking one opens Windows' open-file window, and the file picked is put on the slide as one dropped on the window is. Where no list has what comes next - the numbers of `@place`, the number of `@size`, the words of `@note` - a hint is drawn after the cursor, dim. Up and Down move through it, Tab or Return takes one, Esc puts it away. As the editor project's auto complete, with the deck's format for its names. When it shows is at the top of complete.h; what it lists is at the top of complete.c.

## State

In `App_State`, changed by ops only:

- `complete` (`Complete_State`, complete.h): which line of the list is selected, and that Esc put the list away - each for one word of the text as it was then. Whether the list shows is worked out from the text and the cursor, never kept.

## Functions and ops

- `Complete_ListFromState`: the list as it is now. Nothing in it: it is not showing.
- `Complete_Input`: its keys, taken while it shows. Pushes ops.
- `Complete_Build`: the list, under the word, or the hint after the cursor, in the edit screen's UI build.
- Ops, its own: `Complete_Select`, `Complete_Dismiss`, `Complete_Accept` (writes the name over the word, as one step to undo).

## Used by

- edit: its keys before the screen's own, and its list built over the text.

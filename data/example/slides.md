# Slides from a text file

---

# How it works

This deck is data\example\slides.md: bin\slideshow.exe data\example opens it

- The text is on the left, and this preview follows the cursor
- A line starting with # and a space begins a new slide and is its title
- A line of --- begins a slide with no title
- A line starting with - and a space is a bullet
  - Two more spaces in front make it an inner bullet
    - And two more, one further in
- Anything else is a paragraph
The Format panel under the text lists all of this. Click a line there to write it.

# Long lines wrap

A paragraph that is too long for the page is broken at spaces into as many lines as it needs, and the lines stay inside the margins whatever size the window is, because the page is laid out again every frame.

- Bullets wrap as well, and the lines after the first start under the text of the first, not under the dot
- Click in the words in the preview and type: they wrap at the edge of their box, and stay the size they are

## A heading inside a slide

Two # make a heading, which belongs to the slide it is on.

# Bold and code

- Two stars on each side make **bold words**
- Backticks make `code`, as in `@theme light`
- Select words and press Ctrl+B or Ctrl+E, or click the line in Format
- A lone ** or ` with no partner is drawn as it is

# A video

![](clip.mp4)

- A video in the picture line plays when the slide is presented
- K pauses it, J and L step back and on 5 seconds
- M turns sound off and on
- Drop a video on the window to add one

# A video behind the words

@layout picture-behind
@video loop mute
![](clip.mp4)

- A line of @video says how the slide's video plays
- loop starts it over at its end, mute keeps it silent
- manual has it wait for K when the slide comes up

# A picture beside the text

![](hills.png)

- ![](path) puts a picture on the slide
- The path is found from the text file's folder
- With text, the picture has the right half
- A slide can have several: they share the room

# Several pictures

![](hills.png)
![](bars.jpg)
![](clip.mp4)

- Each picture line is one more picture or video
- They share the half beside the words
- Drop a file on the window to add one

# Moved with the mouse

@place text 0.060 0.300 0.440 0.560
![](circles.png)
@place picture 0.560 0.300 0.380 0.375

- Drag a picture, the title or the words in the preview to move them
- Drag a handle to resize them: a corner for both ways, a side for one
- A line of @place is written for it: right-click to take it away

# Blocks of words

- The lines of a slide are one block
- A line of @box starts another

@box
@place text 0.560 0.244 0.380 0.300
## On its own
- Each block is dragged and sized by itself
- Shift lines it up with the others

# Sizes and colours
@size title 1.2
@color title orange

- Ctrl+] makes the words the cursor is in larger, and Ctrl+[ smaller
- The Format panel has the colours to click

@box
@size text 0.8
@color text green
- A block of words has a size and a colour of its own
- A line of @size or @color is written for it

# Layouts

@layout picture-left
![](circles.png)

- A line of @layout says how the slide it is on is laid out
- picture-left puts the picture on this side
- picture-right is what a slide gets with no such line

# A picture behind the words

@layout picture-behind
![](hills.png)

- picture-behind fills the slide with the picture
- The words go over it, on a wash of the background

# Everything in the middle

@layout center

center puts every line in the middle of the slide

- Bullets lose their dots
- Good for a quote, or a few words

# Themes

- One line of @theme anywhere in the file colours the whole deck
- dark is what a deck gets with no such line
- Try writing @theme light, @theme blue or @theme paper on a line of its own

# A picture under a title

![](bars.jpg)

---

![](circles.png)

# A lot on one slide

@place text 0.070 0.246 0.400 0.680
- One
- Two
- Three
- Four
- Five
- Six
- Seven
- Eight

@box
@place text 0.530 0.246 0.400 0.680
- Nine
- Ten
- Eleven
- Twelve
- Thirteen
- Fourteen
- Fifteen
- Sixteen

# Notes to yourself

@note This line is a note: it is not on the slide.
@note While presenting, N shows a slide's notes and the title of the next slide.

- A line of @note is for whoever presents
- It never shows on the slide
- Press N while presenting to see the notes

# Keys

- F5 presents, and F5 or Esc comes back to the text
- Ctrl+S saves
- While presenting, H shows every key and N the notes

# The end

# Slides from a text file

---

# How it works

This deck is the file data\example\slides.md. Open it with:

bin\slideshow.exe data\example

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
- A slide with more on it than fits is drawn smaller until it does

## A heading inside a slide

Two # make a heading, which belongs to the slide it is on.

# A picture beside the text

![](hills.png)

- ![](path) puts a picture on the slide
- The path is found from the text file's folder
- With text, the picture has the right half
- A slide has one picture: the first

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

- One
- Two
- Three
- Four
- Five
- Six
- Seven
- Eight
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

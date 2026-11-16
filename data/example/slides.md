# Slides from a text file

---

# How it works

This deck is the file data\example\slides.md. Open it with:

bin\slideshow.exe data\example

- A line starting with #  begins a new slide and is its title
- A line of --- begins a slide with no title
- A line starting with -  is a bullet
  - Two more spaces in front make it an inner bullet
    - And two more, one further in
- Anything else is a paragraph

Save the file while the program is running and the slides change as you write.

# Long lines wrap

A paragraph that is too long for the page is broken at spaces into as many lines as it needs, and the lines stay inside the margins whatever size the window is, because the page is laid out again every frame.

- Bullets wrap as well, and the lines after the first start under the text of the first, not under the dot
- A slide with more on it than fits is drawn smaller until it does

## A heading inside a slide

Two # make a heading, which belongs to the slide it is on.

# A picture beside the text

![](../slides/slide1.png)

- ![](path) puts a picture on the slide
- The path is found from the text file's folder
- With text, the picture has the right half
- A slide has one picture: the first

# A picture under a title

![](../slides/slide2.jpg)

---

![](../slides/slide3.png)

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

# Keys

- H or F1 shows every key
- I shows the slide number
- P plays, F is fullscreen

# The end

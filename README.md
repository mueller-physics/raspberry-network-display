---
layout: default
---

# Raspberry Pi Network Display Code

The 'Raspberry Network Display' is a small collection of software to display images
on the HDMI port on a Raspberry Pi (or basically, any Linux machine with a compatible frame-buffer interface),
and quickly (20ms) change between them via network commands (in UDP packages).

The software was developed to control spatial light modulators for microscopes, but is general enough to
probably work in other context. It is of typical 'side-project' quality, i.e., it was written because and
while it was needed for this specific purpose.

It consists of two parts:

## Display code (in 'display'):

* Load images into memory
  * Either uploaded via TCP or read from PNGs
  * Multiple images groups in 'banks' (e.g. 4 banks with 10 images each)
* Switch between images via UDP packages
  * Either by providing a number
  * Or by providing the filename
* Fixed pattern (stripes, checkerboard) also available
* At startup, a test image is loaded
* At compile time, the resolution is set (see 'global.h'). Code will fail if images are not in that resolution.

This part of the software is written in C, and supposed to run on e.g. a Raspberry Pi (hence the name). It
should however be compatible with any linux system offering frame-buffer access.

It should compile almost everywhere by using the included Makefile and gcc. It requires the "wiring" library
for an optional feature, that is sending a UDP package each time a TTL pulse is received on the Pi's GPIO pins.


## Control code (in 'control'):

This is a plugin to [ImageJ](https://imagej.nih.gov/ij/) / [Fiji](https://fiji.sc/), which currently allows to:

* Issue "switch image" commands to the display
* Upload a stack to a memory bank on the display
* Switch to predefined pattern (stripes, checkerboard)

## Raspberry config (in 'raspberry-config'):

Some (commented) example configs to

* auto-start the software on Raspberry boot up
* configure a very specific modeline on the HDMI output (needed for the display in use)


## License and lodepng

Everything is under GPLv3 or later, expect for stuff in 'external', which is

* [lodepng](http://lodev.org/lodepng/), under zLib license (compatible with GPL).
* The [resolution test chart](https://en.wikipedia.org/wiki/File:EIA_Resolution_Chart_1956.svg), under public domain.





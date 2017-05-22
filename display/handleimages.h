/*
This file is part of Raspberry Network Display (RND).

RND is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RND is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RND.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef INC_READIMAGES
#define INC_READIMAGES

#include "global.h"

static const int maxReadImages = 250;


// allocate and initialize the image buffer
bbpnd_imageBuffer * bbpnd_initImageBuffer(int w, int h, int size) ;

// function to fill all memory banks from images read from disk
// it will check in folder 'dirname' for files names
// img_mb##_nr##_w####_h####.png
// e.g. img_mb02_nr10_w1024_h0768.png
// to load image 10 for memory bank 2 on a 1024x768 sized display
int bbpnd_readImages(const char * dirname, bbpnd_imageBuffer **buf);

// find an image in the buffer by its name
// returns either the index or -1 if not found
int bbpnd_findByName( const char * fname, const bbpnd_imageBuffer *);

// convert an 32bit png (with alpha) to 3x8bit color
void bbpnd_convert32to24( unsigned char * in, unsigned char * out, int w, int h);

// connect the framebuffer to an imageContent
unsigned char * bbpnd_connectFramebuffer(bbpnd_imageContent *fb);

// initialize a pattern set
void bbpnd_initPattern(bbpnd_imageBuffer * buf,
    int px, int len, int invert );


// fill with a pattern
void bbpnd_initHorz( bbpnd_imageContent , 
    int px, int len, int pha, 
    int invert );

// fill with a pattern
void bbpnd_initVert( bbpnd_imageContent , 
    int px, int len, int pha, 
    int invert );

// fill with a 45deg. pattern
void bbpnd_init45( bbpnd_imageContent img , 
    int px, int len, int pha , 
    int invert , int dir);

// fill with a chessboard pattern
void bbpnd_initChessboard( 
    bbpnd_imageContent img,
    int len, int xs, int ys ) ;


#endif

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



/* This should be the main file needed to be edited when
 * changing between different displays, i.e., setting different
 * resolutions */

#ifndef INC_BBPND_DEFINES
#define INC_BBPND_DEFINES

// this sets width and height of the framebuffer
// all images have to be in this size
#define FB_IMAGE_W 1280
#define FB_IMAGE_H 1024

// this sets how many memory banks there are, and
// how many image slots per memory bank. Keep in mind
// all of these have to fit the raspberry's memory in 24bit per pxl
#define BBPND_MAXIMUM_MEMORY_BANK 5
#define BBPND_IMAGES_PER_STACK 15

#include <pthread.h>

// structure to hold the content of a png
typedef struct {
    unsigned char * px;	     // the pixel data, 3x8 bit RGB
    unsigned int width;	     // width of image
    unsigned int height;     // height of image
    pthread_mutex_t mutex; // mutex for image access 
    char fname[255];	     // name of the image
} bbpnd_imageContent ; 

// strture to hold the image buffer in memory
typedef struct {
    unsigned int count;
    unsigned int width;	    // width of image
    unsigned int height;    // height of image
    bbpnd_imageContent * img;
    char bufname[255];

} bbpnd_imageBuffer;

#endif

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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/kd.h>

#include "../external/lodepng.h"
#include "handleimages.h"

/**
 * Allocate the image buffer
 * */
bbpnd_imageBuffer *  bbpnd_initImageBuffer(int w, int h, int size) {

    bbpnd_imageBuffer * buf = calloc( 1, sizeof( bbpnd_imageBuffer ) );

    // store the size
    buf->count  = size;
    buf->width  = w;
    buf->height = h;

    size_t bytes = w * h * 3;

    // allocate space to store at most maxImages (see .h) images
    buf->img = 
	(bbpnd_imageContent*) calloc( size , sizeof(bbpnd_imageContent));
    
    // allocate and initialize all images
    int i=0;
    for( i=0; i<size; i++) {
	buf->img[i].px = malloc( bytes );
	buf->img[i].width  = w;
	buf->img[i].height = h;
	pthread_mutex_init(&(buf->img[i].mutex), NULL);
	//bzero( buf->img[i].fname, 256);
    }

    // TODO: check allocate for fails 

    return buf;
}

/**
 * Free the image buffer
 * */
void bbpnd_freeImageBuffer( bbpnd_imageBuffer * buf ) {

    // free the images
    int i;
    for (i=0; i<buf->count;i++) {
	free( buf->img[i].px );
	pthread_mutex_destroy(&(buf->img[i].mutex));
    }

    // free the buffer
    free( buf );



}


// ===================================================================

/**
 * Read all PNGs in the given folder into data
 * */
int bbpnd_readImages(const char * dirname, bbpnd_imageBuffer * buf) {

    // data structures
    struct dirent *dp;
    DIR *dfd;


    // open the directory
    if ( (dfd = opendir(dirname)) == NULL ) {
	perror("# Opening directory for reading images failed");
	exit(-1);
    }

    // loop through all the files
    char cur_fname[2048];
    int imageCount = 1;
    
    bbpnd_imageContent * pngs = buf->img;

    while (((dp = readdir(dfd)) != NULL)&&( imageCount +1 < buf->count )) {
	sprintf( cur_fname, "%s/%s" , dirname, dp->d_name);

	// check file ending
	int isPNG = 0;
	char * point;
	if((point = strrchr(cur_fname,'.')) != NULL ) 
	    if(strcmp(point,".png") == 0)
		isPNG=1;

	// read the file if .png
	if (isPNG) {
	    printf("# Reading PNG [%03d] :: %s \n" ,
		imageCount, cur_fname);

	    // call into lodepng
	    unsigned char * tmpImgData;
	    unsigned int in_width, in_height;
	    unsigned error = lodepng_decode32_file(
	       &tmpImgData , &in_width, &in_height, cur_fname);

	    // store the filename (w/o path), max 255 chars
	    snprintf(pngs[imageCount].fname, 
			255, "%s", dp->d_name); 


	    // for the moment: Fail completely if any single
	    // image fails loading
	    if(error) {
		fprintf(stderr, "error %u: %s\n", 
		    error, lodepng_error_text(error));
		exit(-3);
	    }

	    if (in_width!=buf->width) {
		fprintf(stderr,
		    "Image width mismatch, should be %d, is %d\n",
		    buf->width, in_width);
		exit(-3);
	    }

	    if (in_height!=buf->height) {
		fprintf(stderr,
		    "Image height mismatch, should be %d, is %d\n",
		    buf->height, in_height);
		exit(-3);
	    }

	    // convert the PNG
	    bbpnd_convert32to24( tmpImgData, 
		pngs[imageCount].px,
		pngs[imageCount].width, 
		pngs[imageCount].height);
	    free( tmpImgData );

	    // increase image counter
	    imageCount++;
	}
	// ignore file if !.png
	else {
	    printf("# Ignoring file     :: %s \n", cur_fname);
	}
	
    
    }

    // return our collected images
    printf("# Read %d images successfully\n", imageCount);
    return imageCount;

}

// ===================================================================

// convert an 32bit png (with alpha) to 3x8bit color
void bbpnd_convert32to24( unsigned char * in, unsigned char * out, int w, int h) {

    int i;

    for (i=0; i<w*h;i++) {
	out[3*i+0] = in[4*i+0];
	out[3*i+1] = in[4*i+1];
	out[3*i+2] = in[4*i+2];
    }

}

// ===================================================================

// return the index of the (first) image with filename fname 
int bbpnd_findByName( const char * fname, 
    const bbpnd_imageBuffer * buf) {

    int i=0;
    for (i=0; i<buf->count; i++) {
	if ( strncmp( fname , buf->img[i].fname, 255) == 0 )
	    return i;
    }
    return -1;

}


// ===================================================================



// init a vertical pattern
void bbpnd_initVert( bbpnd_imageContent img , 
    int px, int len, int pha , 
    int invert ) {

    int y;

    for (y=0; y< img.height; y++) {

	if ( (((y+pha)%len)<px) ^ (invert!=0) )
	    memset( &img.px[y*img.width*3], 255, img.width*3);
	else 
	    memset( &img.px[y*img.width*3], 0, img.width*3);
    } 
}

// init a horz. pattern
void bbpnd_initHorz( bbpnd_imageContent img , 
    int px, int len, int pha , 
    int invert ) {

    int x,y;

    char line[ 3 * img.width ];

    for (x=0; x< img.width; x++) {
	if ( (((x+pha)%len)<px) ^ (invert!=0) )
	    memset( &line[x*3], 255, 3);
	else 
	    memset( &line[x*3], 0, 3);
    }

    for (y=0; y< img.height; y++) {
	memcpy( &img.px[ 3 * y * img.width] , line, 3 * img.width );
    }
 
}


// init a 45 deg. pattern
void bbpnd_init45( bbpnd_imageContent img , 
    int px, int len, int pha , 
    int invert , int dir) {

    int x,y;

    char line[ 3 * img.width ];

    for (x=0; x< img.width; x++) {
	if ( (((x+pha)%len)<px) ^ (invert!=0) )
	    memset( &line[x*3], 255, 3);
	else 
	    memset( &line[x*3], 0, 3);
    }

    for (y=1; y< img.height-1; y++) {
	memcpy( &img.px[ 3 * ( y * img.width + (y%len)*dir ) ] ,
	    line, 3 * img.width );
    }
 
}

// ===================================================================

unsigned char * bbpnd_connectFramebuffer(bbpnd_imageContent * framebuf) {

// store information about the frambuffer
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

// open the framebuffer
int fbfd =0, tty=0;
fbfd = open("/dev/fb0", O_RDWR);

if (fbfd == -1) {
    perror("Error: cannot open framebuffer device");
    exit(-1);
}
printf("# The framebuffer device was opened successfully.\n");
 
// set the tty to graphics mode 
tty = open("/dev/tty1", O_RDWR);
if(ioctl(tty, KDSETMODE, KD_GRAPHICS) == -1) {
    printf("# Failed to set graphics mode on tty1\n");
    printf("# --> condire chaning tty permissions\n");
}

 
// Get fixed screen information
if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
 perror("Error reading fixed information");
 exit(-1);
}
  
// Get variable screen information
if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
  perror("Error reading variable information");
  exit(-1);
}
   

// check our framebuffer size
if (vinfo.xres != FB_IMAGE_W) {
    fprintf(stderr, 
	"Size mismatch: FB width should be %d, is %d",
	FB_IMAGE_W, vinfo.xres);
    exit(-2);
    }

if (vinfo.yres != FB_IMAGE_H) {
    fprintf(stderr, 
	"Size mismatch: FB width should be %d, is %d",
	FB_IMAGE_H, vinfo.yres);
    exit(-2);
    }

if (vinfo.bits_per_pixel != 24 ) {
    perror("BitsPerPixel is not 24, use /boot/config to fix");
    exit(-2);
}
printf("# FB: %dx%d, %dbpp Ok!\n", 
	vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);



// calculate the size of the screen
 const int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

// Map the device to memory
unsigned char * fbp = (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
if (fbp == (unsigned char *)-1) {
  perror("Error: failed to map framebuffer device to memory");
  exit(-1);
}


// write output
framebuf->width   = vinfo.xres;
framebuf->height  = vinfo.yres;
framebuf->px = fbp ;
sprintf(framebuf->fname , "[FRAMEBUFFER]"); 


printf("# The framebuffer device was mapped to memory successfully.\n");

return fbp;


}



// ===================================================================

// initialize a full SIM line pattern set
void bbpnd_initPattern(bbpnd_imageBuffer * buf,
    int px, int len, int invert ) {

    int pha[3]; pha[0]=0; pha[1]=len/3; pha[2]=len*2/3;
    int i;

    for (i=1;i<=3;i++)
	bbpnd_initHorz( buf->img[i  ], px, len, pha[i-1], invert);
    for (i=1;i<=3;i++)
	bbpnd_initVert( buf->img[i+3], px, len, pha[i-1], invert);
    for (i=1;i<=3;i++)
	bbpnd_init45(   buf->img[i+6], 
	    px*3/2, len*3/2, pha[i-1]*3/2, invert, 1);
    for (i=1;i<=3;i++)
	bbpnd_init45(   buf->img[i+9], 
	    px*3/2, len*3/2, pha[i-1]*3/2, invert,-1);

    sprintf(buf->bufname, "%dpx%s", len, (invert)?("Inv"):(""));

}


// ===================================================================

void bbpnd_initChessboard( 
    bbpnd_imageContent img,
    int len, int xs, int ys ) {

    int x,y;
    char lineEven[ 3 * img.width ];
    char lineOdd[  3 * img.width ];

    for (x=0; x< img.width; x++) {
	if (((x+xs)%(len*2))<len) { 
	    memset( &lineEven[x*3], 255, 3);
	    memset( &lineOdd[ x*3],   0, 3);
	}
	else { 
	    memset( &lineEven[x*3],   0, 3);
	    memset( &lineOdd[ x*3], 255, 3);
	}
    }

    for (y=0; y< img.height; y++) {
	if (((y+ys)%(len*2))<len)
	    memcpy( &img.px[ 3 * y * img.width] , 
		lineEven, 3 * img.width );
	else
	    memcpy( &img.px[ 3 * y * img.width] , 
		lineOdd, 3 * img.width );

    }

}




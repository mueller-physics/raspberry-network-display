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

/**
 * This is a small programm meant to control the
 * raspberry's framebuffer to output images to DVI / HDMI.
 *
 * It is either controlled via command line or by sending
 * UDP pacakges in a simple format (it should run on a small
 * local network on the experiment, not open to the internet)
 *
 * Its purpose is to control the Holoeye SLM.
 *
 * */

#include <stdio.h>
#include <string.h> // for memcpy

#include <pthread.h> // for the server threads
#include <assert.h>

#include "global.h"

#include "handleimages.h"
#include "network-udp.h"
#include "network-tcp.h"
#include "misc.h"

#include "../external/lodepng.h"

int main( int argc, char ** argv) {
    
    // allocate the image buffers
    bbpnd_imageBuffer * imgBuffers[BBPND_MAXIMUM_MEMORY_BANK];    
    unsigned char * testImgData =0 ;

    {
	int i;
	for (i=0;i<BBPND_MAXIMUM_MEMORY_BANK;i++) {
	    printf("# Init buffer : %d \n",i);
	    imgBuffers[i] = bbpnd_initImageBuffer( 
		FB_IMAGE_W, FB_IMAGE_H, BBPND_IMAGES_PER_STACK);
	}
    }
 
    bbpnd_imageBuffer * images = imgBuffers[0];


    // set the output line buffered
    setlinebuf( stdout);
    setlinebuf( stderr);



    // initalize the image buffers
    {
	    double initPtime = getTime();
	    bbpnd_initPattern( imgBuffers[0] , 2 , 6, 0);
	    bbpnd_initPattern( imgBuffers[1] , 2 , 6, 1);
	    bbpnd_initPattern( imgBuffers[2] , 3 , 9, 0);
	    bbpnd_initPattern( imgBuffers[3] , 3 , 9, 1);
	    initPtime = getTime() - initPtime;
	    printf("# Init Pattern %7.3f ms / 48 pattern : %7.3f ms/pattern \n", initPtime, initPtime/48 );
    }
    
    // TODO / NOTE: change these lines to automatically load PNG images at programm start
    int nrImages = bbpnd_readImages( "./images", imgBuffers);
    printf("# Number of images : %d \n", nrImages);

    // connect to the framebuffer
    bbpnd_imageContent fb;
    if ( argc < 2 || strncmp( argv[1],"nofb",4 ) != 0 ) {
        bbpnd_connectFramebuffer( &fb );
    } else {
	// if no framebuffer, just init to any unused buffer
	printf("# Not opening framebuffer (as requested)\n");
	fb.width = FB_IMAGE_W; fb.height = FB_IMAGE_H;
	fb.px = malloc( FB_IMAGE_W * FB_IMAGE_H * 3);
	pthread_mutex_init(&(fb.mutex), NULL);
    }
    
    // fork a server thread
    pthread_t server_thread;
    bbpnd_serverParameters server_param;

    server_param.portNr = 32321;
    server_param.buf    = imgBuffers;
    server_param.maxMemoryBanks = BBPND_MAXIMUM_MEMORY_BANK;
    server_param.fb = &fb;


    int rc = pthread_create(
	    &server_thread, NULL, 
	    &bbpnd_incomingImageServerPThread, 
	    &server_param);

    if (0 != rc) 
	perror("! Could not launch TCP server thread\n");




    // open the UDP port
    int udpSocket  = bbpnd_initUdp( 32320 );

    bbpnd_udpContent cliMsg;
    ipaddr	     cliAddr;

    // load start up image
    {
	unsigned int x,y;
	char * startName = malloc(200*sizeof(char));
	sprintf(startName, "StartImage_%04dx%04d.png",FB_IMAGE_W,FB_IMAGE_H);
 
	unsigned error = lodepng_decode32_file(
	       &testImgData , &x, &y, startName);
	if ((error>=0)&&(x==FB_IMAGE_W)&&(y==FB_IMAGE_H)) {
		bbpnd_convert32to24( testImgData, testImgData, FB_IMAGE_W, FB_IMAGE_H);
		memcpy( fb.px, testImgData, FB_IMAGE_W*FB_IMAGE_H*3);
		printf("# Displaying startup image : %s \n", startName);
	} else {
		printf("# FAILED to load test image");
		memset( fb.px, 25, FB_IMAGE_W*FB_IMAGE_H*3);
		testImgData = 0;
	}

    }
    printf("# -----------------------------------\n");

    // listen for package
    for ( ;; ) {

	// wait for a package, start the clock if recived
	int r = bbpnd_listenUdp( udpSocket , &cliMsg, &cliAddr);
	double start = getTime();

	// check if the message is valid
	if (r<0) continue;   

	// prepare our response
	printf(" -> MESSAGE  in: ");
	bbpnd_printUdp( &cliMsg );
	bbpnd_udpContent pingReply;
	pingReply.serialNr = cliMsg.serialNr;
	bzero( pingReply.attr , 256 );

	// check what we want to do...
	int respond = 0;
	
	// reply to ping
	if (bbpnd_cmdIsPing( &cliMsg )) {
	    sprintf( pingReply.cmd  , "PONG");
	    bzero( pingReply.attr , 256) ;
	    respond = 1;
	}


	// TODO: reimplement this to load images directly from disk
	/*
	// switch the displayed image by name
	if (bbpnd_cmdIsLoadName( & cliMsg) ) {

	    // see if we have that image
	    int r = bbpnd_findByName( cliMsg.attr , images );
	    if (r>=0) {
		memcpy( fb.px, images->img[r].px, FB_IMAGE_W*FB_IMAGE_H*3);
		sprintf( pingReply.cmd , "OKLDFN");
		sprintf( pingReply.attr , "%s" , images->img[r].fname) ;
	    } else {
		sprintf( pingReply.cmd  , "ERR") ;
		sprintf( pingReply.attr , "File not found.") ;
	    }
	    
	    respond = 1;
	} 
	*/
	
	
	// switch the displayed image by number
	if (bbpnd_cmdIsLoadNumber( & cliMsg) ) {

	    // see if we have that image
	    int r = *cliMsg.pad;
	    if ((r>=0)&&(r<images->count)) {
		memcpy( fb.px, images->img[r].px, FB_IMAGE_W*FB_IMAGE_H*3);
		sprintf( pingReply.cmd , "OKLDNR");
		sprintf( pingReply.attr , "%s" , images->img[r].fname) ;
		sprintf( pingReply.attr , "loaded #%d", r) ;
	    } else {
		sprintf( pingReply.cmd  , "ERR") ;
		sprintf( pingReply.attr , "Number out of range") ;
	    }
	    
	    respond = 1;
	} 

	// switch the displayed to a fixed value
	if (bbpnd_cmdIsSetConstant( & cliMsg) ) {

	    // see if we have that image
	    int r = *cliMsg.pad;
	    if ((r>=0)||(r<=255)) {
		memset( fb.px, r, FB_IMAGE_W*FB_IMAGE_H*3);
		sprintf( pingReply.cmd , "OKSETC");
		sprintf( pingReply.attr , "set all px to %d" , r) ;
	    } else {
		sprintf( pingReply.cmd  , "ERR") ;
		sprintf( pingReply.attr , "px out of range") ;
	    }
	    
	    respond = 1;
	} 
	
	// select a different memory bank
	if (bbpnd_cmdIsSelectMemoryBank( & cliMsg) ) {

	    // see if we have that image
	    int r = *cliMsg.pad;
	    if ((r>=0)||(r<=BBPND_MAXIMUM_MEMORY_BANK)) {
		images = imgBuffers[r];
		sprintf( pingReply.cmd , "OKSELMB");
		snprintf( pingReply.attr , 255, "sel. mem. #%d, %s" , 
			r, imgBuffers[r]->bufname) ;
	    } else {
		sprintf( pingReply.cmd  , "ERR") ;
		sprintf( pingReply.attr , "mem idx out of range") ;
	    }
	    
	    respond = 1;
	}

	// switch the displayed to the test image value
	if (bbpnd_cmdIsSetTestImage( & cliMsg) ) {
	    if (testImgData != 0) {
		memcpy( fb.px, testImgData, FB_IMAGE_W*FB_IMAGE_H*3);
		sprintf( pingReply.cmd , "OKTEST");
		sprintf( pingReply.attr , "test image loaded") ;
	    } else {
		sprintf( pingReply.cmd  , "ERR") ;
		sprintf( pingReply.attr , "no test image found") ;
	    }
	    
	    respond = 1;
	} 
	
	// switch the displayed image to a line pattern with parameters
	if (bbpnd_cmdIsSetLinePattern( & cliMsg) ) {
	    // check all the parameters
	    int angle  = cliMsg.pad[0];
	    int px     = cliMsg.pad[1];
	    int len    = cliMsg.pad[2];
	    int pha    = cliMsg.pad[3];

	    if ((  ( angle == 0 ) || ( angle == 90 ) || 
		  ( angle == 45) || ( angle == 135 ) ) &&  	
		( px < len ) && (px > 0) && (len>0 ) )
		{

		sprintf( pingReply.cmd , "OKPTLI");
		sprintf( pingReply.attr ,
		    "line-pttrn: a:%d , px: %d, l: %d, pha: %d, i: %d", 
		    angle, px, len, pha, 0) ;

		// horz. pattern
		if ( angle == 0 )
		    bbpnd_initVert( fb, px, len, pha , 0 );
		if ( angle == 90 )
		    bbpnd_initHorz( fb, px, len, pha , 0 );
		if ( angle == 45 )
		    bbpnd_init45(   fb, px, len, pha , 0 , 1);
		if ( angle == 135 )
		    bbpnd_init45(   fb, px, len, pha , 0 ,-1);


	    } else {
		sprintf( pingReply.cmd  , "ERR") ;
		sprintf( pingReply.attr , 
		    "a:%d , p: %d, l: %d, i: %d", angle, px, len, 0) ;
	    }
	    
	    respond = 1;
	} 

	// switch the displayed image to a chessboard pattern with parameters
	if (bbpnd_cmdIsSetChessboardPattern( & cliMsg) ) {
	    // check all the parameters
	    int len	= cliMsg.pad[0];
	    int xs	= cliMsg.pad[1];
	    int ys	= cliMsg.pad[2];


	    if ((xs>=0)&&(xs<2*len)&&(ys>=0)&&(ys<2*len)&&(len>0)) {
		sprintf( pingReply.cmd , "OKPTCB");
		sprintf( pingReply.attr ,
		    "cb-pttrn: l:%d , xs: %d, ys: %d", 
		    len, xs, ys) ;
		
		bbpnd_initChessboard( fb , len, xs, ys ); 
	    }
	    else {
		sprintf( pingReply.cmd , "ERR");
		sprintf( pingReply.attr ,
		    "cb-pttrn: l:%d , xs: %d, ys: %d", 
		    len, xs, ys) ;
	    }
	    
	    respond = 1;
	} 

	// send our response
	double stop = getTime();
	if ( respond ) {
	    pingReply.timing = stop - start;
	    bbpnd_sendUdp( udpSocket , &pingReply, &cliAddr );
	    printf(" -> MESSAGE out: ");
	    bbpnd_printUdp( &pingReply);
	}
	printf("# We took %8.2f ms \n", stop-start);

    }

    //
    rc = pthread_join( server_thread , NULL);
    assert( 0 == rc);

}

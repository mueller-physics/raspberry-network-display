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

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>


#include "network-tcp.h"

void * bbpnd_incomingImageServerPThread(void * _inParam) {

    // cast input pointer
    bbpnd_serverParameters * param = (bbpnd_serverParameters *) _inParam;

    // allocate recv buffer
    size_t inputLen = param->buf[0]->width * param->buf[0]->height *3 ;
    unsigned char * inputBuf = malloc( inputLen +32);

    // socket variables
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    clilen = sizeof(cli_addr);
    int sockfd, newsockfd;

    // create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // set up our listening TCP socket
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(param->portNr);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
	    sizeof(serv_addr)) < 0) {
        perror("ERROR on binding TCP socket");
	return NULL;
    }

    // start to listen on our socket
    if (listen(sockfd,10)!=0) {
	perror("ERROR listeing on TCP socket");
	return NULL;
    }


    // loop accepting connections
    bbpnd_tcpCommand todo;

    printf("# TCP thread: Listening on port %d\n", param->portNr);

    for ( ;; ) {

	// accept connection
	newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
   
	// set a timeout 
	struct timeval tv;
	tv.tv_sec  = 1;
	tv.tv_usec = 0;
	setsockopt(newsockfd, SOL_SOCKET, 
	    SO_RCVTIMEO,&tv,sizeof(struct timeval));


	// read what to do
	int r1=-1, r2=-1;
	r1 = recv( newsockfd, 
	    &todo, sizeof(bbpnd_tcpCommand), MSG_WAITALL); 
		
	// TODO:
	// check the command that was send ... :)

	printf("# TCP incoming connection \n");

	// if the message is complete, lets see what to do
	if ( r1 == sizeof(bbpnd_tcpCommand) ) {

	    printf("# TCP Command recvd.: '%s' \n", todo.cmd);
	    int cmdOK=0;

	    // store an image	   
	    if (cmdIsStoreImage( todo )) { 
		cmdOK=1;

		// recieve our image into the buffer
		// call recieve multiple times, so a short timeout
		// can be maintained while reading a large image
		for (r2 =0;r2<inputLen;) {
		    //printf("#DBG :: Reading input %d \n", r2);
		    r1 = recv( newsockfd, inputBuf+r2, inputLen-r2+1, 0); 
		    if (r1<=0) break;
		    r2+=r1;
		}
	
		printf(" %s // %d // %d \n", 
		    todo.cmd,  todo.len1, todo.len2 );
	    
		// check if the the image was recieved completely
		if (r2 >= inputLen) {
		    // TODO: Lock the mutex ....
		    int pos1 = todo.len1;
		    int pos2 = todo.len2;
		    if ((pos2>=0)&&(pos2 < param->maxMemoryBanks )) {
		    if ((pos1>=0)&&(pos1 < param->buf[pos2]->count)) {
			memcpy( 
				param->buf[pos2]->img[pos1].px , 
				inputBuf , inputLen );
			printf("# Stored new image via TCP at buffer %d, bank %d \n",pos1, pos2);
			}
		      else {
			printf("# Wrong position for image : %d,%d \n", pos1, pos2);
		      }
		    }
		    else {
			printf("# Wrong position for image : %d,%d \n", pos1, pos2);
		    }
		
		} 
		else {
		    printf("# Image not received completely via TCP :: %d of %ld \n", r2, inputLen);
		}

	    } 


	    // display an image
	    if (cmdIsRecvAndShowImage( todo )) { 
		cmdOK=1;

		// recieve our image into the buffer
		// call recieve multiple times, so a short timeout
		// can be maintained while reading a large image
		for (r2 =0;r2<inputLen;) {
		    //printf("#DBG :: Reading input %d \n", r2);
		    r1 = recv( newsockfd, inputBuf+r2, inputLen-r2+1, 0); 
		    if (r1<=0) break;
		    r2+=r1;
		}
		
		// check if the the image was recieved completely
		if (r2 >= inputLen) {
		    printf("# %s --> directly displaying received image \n", todo.cmd );
		    memcpy( param->fb->px, inputBuf, FB_IMAGE_W*FB_IMAGE_H*3);
		} else {
		    printf("# Image not received completely via TCP :: %d of %ld \n", r2, inputLen);
		} 
		 
	    }

	




	    // send the full framebuffer
	    if ( cmdIsGetFB( todo )) {
		cmdOK=1;	    

		printf("# Sending out framebuffer content via TCP\n");
		memcpy( inputBuf, param->fb->px , FB_IMAGE_W*FB_IMAGE_H*3);

		//printf("# input len :: %d ", inputLen);
		
		for (r2 =0;r2<inputLen;) {
		    //printf("#DBG :: Reading input %d \n", r2);
		    r1 = send( newsockfd, inputBuf+r2, inputLen-r2, MSG_NOSIGNAL); 
		    //printf("# DBG send %d %d \n",r1,r2);
		    if (r1<0) break;
		    r2+=r1;
		}
	
	    }
	
	    // send some of the framebuffer
	    if ( cmdIsGetCropFB( todo )) {
		cmdOK=1;	    

		printf("# Sending out framebuffer content via TCP\n");
		int i;
    
		for (i=0; i<256;i++)
		memcpy( inputBuf+i*256*3, 
		    param->fb->px+(FB_IMAGE_W/2-128 + FB_IMAGE_W*(i+(FB_IMAGE_H/2)-128))*3  , 256*3);

		//printf("# input len :: %d ", inputLen);
		
		for (r2 =0;r2<256*256*3;) {
		    //printf("#DBG :: Reading input %d \n", r2);
		    r1 = send( newsockfd, inputBuf+r2, inputLen-r2, MSG_NOSIGNAL); 
		    //printf("# DBG send %d %d \n",r1,r2);
		    if (r1<0) break;
		    r2+=r1;
		}
	
	    }
	    
	    // output some
	    if (!cmdOK) {
		    printf("# Init command not received completely via TCP\n");
	    }
	}
	

	// close our listening socket and loop	
	shutdown( newsockfd, SHUT_RDWR );
	close( newsockfd );
	
    }

    // TODO: in fact, these parts will never be reached...

    // free input buffer
    free ( inputBuf );
    return NULL;
}



// return if the command is "store an image"
int cmdIsStoreImage( bbpnd_tcpCommand todo ) {
    return ( strcmp( todo.cmd , "STORIMG")==0 );
}

// return if the command is "receive and show an image"
int cmdIsRecvAndShowImage( bbpnd_tcpCommand todo ) {
    return ( strcmp( todo.cmd , "RECVSHOW")==0 );
}

// return if the command is "get the full framebuffer"
int cmdIsGetFB( bbpnd_tcpCommand todo ) {
    return ( strcmp( todo.cmd , "GETFFB")==0 );
}

// return if the command is "get a part of the framebuffer"
int cmdIsGetCropFB( bbpnd_tcpCommand todo ) {
    return ( strcmp( todo.cmd , "GETCFB")==0 );
}

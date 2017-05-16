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

#ifndef INC_BBPND_TCPSERVER
#define INC_BBPND_TCPSERVER

#include "global.h"

// struct holding all server paramters, to pass to
// the tcp server thread
typedef struct {
   
    int portNr;
    int maxMemoryBanks;
    bbpnd_imageBuffer ** buf; 
    bbpnd_imageContent * fb;

} bbpnd_serverParameters ;

// container for the upd content
typedef struct {
    char     cmd[8];
    uint64_t serialNr;
    double   timing;
    int	     len1;
    int	     len2;
} bbpnd_tcpCommand;


// TCP image server, to run in its own pthread
// 'param' should point to a valid bbpnd_serverParameters structure
void * bbpnd_incomingImageServerPThread(void * param);


// return if the command is "store an image"
int cmdIsStoreImage( bbpnd_tcpCommand todo ) ;
// return if the command is "store an image"
int cmdIsRecvAndShowImage( bbpnd_tcpCommand todo ) ;
// return if the command is "get the framebuffer"
int cmdIsGetFB( bbpnd_tcpCommand todo ) ;
// return if the command is "get the framebuffer"
int cmdIsGetCropFB( bbpnd_tcpCommand todo ) ;


#endif

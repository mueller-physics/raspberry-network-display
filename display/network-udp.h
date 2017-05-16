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

#ifndef INC_UDPNETWORK
#define INC_UDPNETWORK

#include <stdint.h>
#include <netinet/in.h>

typedef struct sockaddr_in ipaddr;

// container for the upd content
typedef struct {
    char     cmd[8];
    uint64_t serialNr;
    double   timing;
    int      pad[10];
    char     attr[256];
} bbpnd_udpContent;


// container for sending socket destination
typedef struct  {
    struct sockaddr_in servaddr;
    int socketFd;
} bbpnd_udpSocket ;


// open the upd port for listening
int bbpnd_initUdp(int portNr);

// open UPD port for sending
int bbpnd_initSendUdp(bbpnd_udpSocket * socket, char * ipaddr, int port);

// set socket timeout
void bbpnd_setTimeout( int socketFd, double seconds );

// wait for and return an UDP package
int bbpnd_listenUdp(int sockedFd, bbpnd_udpContent * message, ipaddr * cli) ; 

// send an Udp package back to the client
void bbpnd_sendUdp( int sockedFd, bbpnd_udpContent * message, ipaddr * cliaddr);

// send an Udp package back to the client
void bbpnd_newSendUdp( bbpnd_udpSocket socket, bbpnd_udpContent * message);

// print out the content of a UDP package (for debug)
void bbpnd_printUdp( bbpnd_udpContent * message);

// return if the command is "load a file"
int bbpnd_cmdIsLoadName(bbpnd_udpContent  * message);

// return if the command is "ping"
int bbpnd_cmdIsPing(bbpnd_udpContent * message);

// return if the command is "load a pattern"
int bbpnd_cmdIsLoadPattern(bbpnd_udpContent * message);

// return if the command is "load a number"
int bbpnd_cmdIsLoadNumber(bbpnd_udpContent * message);

// return if the command is "set to a constant value"
int bbpnd_cmdIsSetConstant(bbpnd_udpContent * message);

// return if the command is "set to a constant value"
int bbpnd_cmdIsSetTestImage(bbpnd_udpContent * message);

// return if the command is "switch memory bank"
int bbpnd_cmdIsSelectMemoryBank(bbpnd_udpContent * message );

// return if the command is "set to pattern"
int bbpnd_cmdIsSetLinePattern(bbpnd_udpContent * message );

// return if the command is "set to pattern"
int bbpnd_cmdIsSetChessboardPattern(bbpnd_udpContent * message );


#endif

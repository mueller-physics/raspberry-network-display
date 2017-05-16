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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "network-udp.h"

// Initialize a UDP listeing socket
int bbpnd_initUdp(int portNr) {

   // bind 
   int ourSockedFd = socket(AF_INET,SOCK_DGRAM,0);

   struct sockaddr_in ourServAddr;
   bzero(&ourServAddr,sizeof(ourServAddr));
   ourServAddr.sin_family = AF_INET;
   ourServAddr.sin_addr.s_addr=htonl(INADDR_ANY);
   ourServAddr.sin_port=htons(portNr);
   bind(ourSockedFd,
	(struct sockaddr *)&ourServAddr,sizeof(ourServAddr));

   // TODO: check if the bind worked at all...
   return ourSockedFd;
}

// Initialize a UDP send socket
int bbpnd_initSendUdp(bbpnd_udpSocket * sock, char * ipaddr, int port) {
    
    bzero(sock,sizeof(bbpnd_udpSocket));
    
    sock->socketFd=socket(AF_INET,SOCK_DGRAM,0);
    sock->servaddr.sin_family = AF_INET;
    sock->servaddr.sin_addr.s_addr=inet_addr(ipaddr);
    sock->servaddr.sin_port=htons(port);
    
    // TODO: return error handling instead of socketFd
    return sock->socketFd;
}

// Set a socket timeout TODO: not sure this always works
void bbpnd_setTimeout( int socketFd, double seconds ) {

    struct timeval timeouttime;
    timeouttime.tv_sec  = (int)seconds;
    timeouttime.tv_usec = ((int)(seconds*1000000))%1000000;

    setsockopt( socketFd, SOL_SOCKET, SO_RCVTIMEO,
	(void *)&timeouttime, sizeof(struct timeval));
}



// Listen for a UDP packet on socket
int bbpnd_listenUdp(int socketFd, 
    bbpnd_udpContent * message, ipaddr * cliaddr) {

    // message and length of address struct
    char mesg[1000];
    socklen_t len = sizeof(ipaddr);
    
    // recieve the message
    int n = recvfrom(socketFd,
	mesg,1000,
	0,(struct sockaddr *)cliaddr,&len);

    // For debug
    //bbpnd_printUdp( (bbpnd_udpContent *) mesg );
 
    // message to short
    if (n<64) return -1;
    // message to long
    if (n>(256+64)) return -2; 

    // copy the content to the bbpnd_udpContent struct
    memcpy( message  , mesg , n );

    // return
    return n; 

}

// Send a UDP packet
void bbpnd_sendUdp( int sockedFd, bbpnd_udpContent * message, ipaddr * cliaddr) {

    socklen_t len = sizeof(ipaddr);
    sendto( sockedFd, message, sizeof(bbpnd_udpContent), 0,
	    (struct sockaddr *) cliaddr, len); 

}


// Send a UDP packet (new)
void bbpnd_newSendUdp( bbpnd_udpSocket sock, bbpnd_udpContent * message) {

    socklen_t len = sizeof(ipaddr);
    sendto( sock.socketFd, message, sizeof(bbpnd_udpContent), 0,
	    (struct sockaddr *) &sock.servaddr, len); 

}




void bbpnd_printUdp( bbpnd_udpContent * message ) {
    char cmd[9];
    snprintf( cmd, 9, "%s", message->cmd);
    printf(" CMD : %s  SERIAL : %ld  TIME : %lf  ATTR : %s \n",
	cmd, (long)message->serialNr, message->timing, message->attr);

}




int bbpnd_cmdIsLoadName(bbpnd_udpContent * message ) {
    int r1 = (strcmp("LDNAME", message->cmd)==0);
    int r2 = (strcmp("LDFILE", message->cmd)==0);
    return (r1 || r2);
}
int bbpnd_cmdIsLoadPattern(bbpnd_udpContent * message ) {
    return (strcmp("LDPTRN", message->cmd)==0);
}

int bbpnd_cmdIsLoadNumber(bbpnd_udpContent * message ) {
    return (strcmp("LDNMBR", message->cmd)==0);
}

int bbpnd_cmdIsPing(bbpnd_udpContent * message ) {
    return (strcmp("PING", message->cmd)==0);
}

int bbpnd_cmdIsSetConstant(bbpnd_udpContent * message ) {
    return (strcmp("SETCNST", message->cmd)==0);
}

int bbpnd_cmdIsSetTestImage(bbpnd_udpContent * message ) {
    return (strcmp("SETTEST", message->cmd)==0);
}

int bbpnd_cmdIsSelectMemoryBank(bbpnd_udpContent * message ) {
    return (strcmp("SELMB", message->cmd)==0);
}

int bbpnd_cmdIsSetLinePattern(bbpnd_udpContent * message ) {
    return (strcmp("PTRNLI", message->cmd)==0);
}
int bbpnd_cmdIsSetChessboardPattern(bbpnd_udpContent * message ) {
    return (strcmp("PTRNCB", message->cmd)==0);
}



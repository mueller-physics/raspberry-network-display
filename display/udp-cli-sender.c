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

/* Sample UDP client */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "network-udp.h"

int main(int argc, char**argv)
{
   int sockfd,n;
   struct sockaddr_in servaddr ;//,cliaddr;

   bbpnd_udpContent message; 

   bzero( &message, sizeof(message));

   if (!(( argc==3 ) || (argc==4) || (argc == 7)))
   {
      printf("usage:  udpcli <IP address> <cmd> [<message> / <p0> <p1> <p2> <p3> ] \n");
      exit(1);
   }

   // setup the socket and address
   sockfd=socket(AF_INET,SOCK_DGRAM,0);
   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(argv[1]);
   servaddr.sin_port=htons(32320);

   // compose the message
   snprintf( message.cmd, 8, "%s", argv[2]);
   message.serialNr = 123;
   message.timing = 0.0;


    if (argc ==3 ) {
	bzero( message.attr, 256);
    }
    if (argc == 4) {
	snprintf( message.attr, 256, "%s", argv[3]);
    }

    if (argc == 7 ) {
	message.pad[0]= atoi( argv[3] );
	message.pad[1]= atoi( argv[4] );
	message.pad[2]= atoi( argv[5] );
	message.pad[3]= atoi( argv[6] );
    }
    

      bbpnd_printUdp( &message );
    int i=0;
    for (i=0;i<8;i++) printf(" %d ",message.pad[i]);
    printf("\n");


      sendto(sockfd,&message,sizeof(message),0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
      n=recvfrom(sockfd,&message,sizeof(message),0,NULL,NULL);
      if (n>0)
	bbpnd_printUdp( &message ); 

   return 0;
}


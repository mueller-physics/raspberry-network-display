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
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <wiringPi.h>

#include "network.h"


/* *** Global variables *** */

// Mutex / Wait condition for the TTL input
pthread_mutex_t inputWaitMutex;
pthread_cond_t	inputWaitCond;

// these are initial values, they can
// be overwritten via UDP 
pthread_mutex_t paramMutex;	// since UDP- and main-thread need access
int led_cycles = 3;			// number of LED on/off cycles
int led_timeOff = 50;		// off-time (ms, switching image)
int led_timeOn  = 150;		// on-time (ms, LED enable)


// udp sockets for send / recv
bbpnd_udpSocket udpSendSocket;
int udpRecvSocket;


/** This ISR wakes up the display sequence each time an
 *  TTL edge is detected on the input */
void pin0isr() {

    // check if the mutex is held. If so, the
    // display sequence is running, which means the timing is off
    if ( pthread_mutex_trylock( &inputWaitMutex ) != 0 ) {
	printf("Received trigger, but display sequence still running\n");
	return;
    }

    // re-release the mutex, wake up main thread
    pthread_mutex_unlock( &inputWaitMutex );
    pthread_cond_signal( &inputWaitCond );
    printf("Received trigger\n");
    delay(2);
}

/** This listen on UDP and switches to other
 *  timing parameters */
void * udpListenerThread(void * none) {

    bbpnd_udpContent message;
    ipaddr inIp;

    printf("Listening for UDP packages...\n");

    for (;;) {

	// wait for incoming UDP message
	bzero( &message, sizeof(message));
	int r = bbpnd_listenUdp( udpRecvSocket, &message, &inIp );
	if (r<0) continue;
	printf("-> UDP RECV:");
	bbpnd_printUdp( &message);

	// kind of a PING, blink the LED
	if  (strcmp("SLMBLNK", message.cmd)==0) {
	    digitalWrite(3, 0);
	    delay(100);
	    digitalWrite(3, 1);
	    delay(100);
	    digitalWrite(3, 0);
	    delay(100);
	    digitalWrite(3, 1);
	    snprintf(message.cmd,8,"OKBLNK");
	    bbpnd_sendUdp( udpRecvSocket , &message, &inIp );
	    continue;
	}

	// see if message is for us
	if  (strcmp("SLMTIME", message.cmd)!=0) {
	    //printf("SLMTIME <-> %s", message.cmd);
	    continue;
	}

	// extract and validate timing
	int ton    = message.pad[0]; 
	int toff   = message.pad[1]; 
	int cycles = message.pad[2]; 

	if ( (cycles >0) && (cycles < 24) &&
	     (ton > 0)   && ( ton < 1000)  &&
	     (toff> 0)   && ( toff< 1000) ) {
	
	    // copy to global
	    pthread_mutex_lock( &paramMutex );
	    led_cycles = cycles;
	    led_timeOn = ton;
	    led_timeOff= toff;
	    pthread_mutex_unlock( &paramMutex );
	   
	    // confirm by output, udp and and blink 
	    printf("Set LED times: %d ms on, %d ms off, %d cycles \n",
		ton, toff,cycles );
	    
	    snprintf(message.cmd,8,"OKSLMT");
	    snprintf(message.attr, 256, "on/off/cycle: %d, %d, %d",
		ton, toff, cycles); 

	    bbpnd_sendUdp( udpRecvSocket , &message, &inIp );

	    digitalWrite(3, 0);
	    delay(100);
	    digitalWrite(3, 1);
	
	} 
	else {
	    printf("Out of bounds: %d ms on, %d ms off, %d cycles \n",
		ton, toff, cycles );
	}
	    
	

    }

} 



/** The main thread */
int main(int argc, char**argv)
{
    int i,n, dbg=1, count=0;

    // set the output line buffered
    setlinebuf( stdout);
    setlinebuf( stderr);
    
    // check command line argument
    if (argc != 2) {
	printf("Please provide IP address, e.g. \n slm-simulator 127.0.0.1\n");
	return 1;
    }

    // setup the network sockets, sending timeout, message
    bbpnd_initSendUdp( &udpSendSocket, argv[1], 32320 );
    udpRecvSocket = bbpnd_initUdp( 32322 );

    bbpnd_setTimeout( udpSendSocket.socketFd, 0.25 );

    bbpnd_udpContent message; 
    bzero( &message, sizeof(message));

    // setup wiringPi
    if (wiringPiSetup () == -1) {
	printf("Error setting up WiringPi\n");
	return 1;
    }

    // setting pull-up on pin 0
    pinMode(0, INPUT);
    pullUpDnControl(0, PUD_UP);

    // setting pins 2,3 to output
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);

    // set up wait condition, to be released in ISR
    pthread_mutex_init( &inputWaitMutex, NULL);
    pthread_cond_init(  &inputWaitCond,  NULL);
    pthread_mutex_lock( &inputWaitMutex );
    
    // register ISR for input pin
    wiringPiISR( 0 , INT_EDGE_FALLING, &pin0isr );

    // set PIN 3 high
    digitalWrite(3,1);


    // start UDP listener
    pthread_t server_thread;
    int rc = pthread_create(
	    &server_thread, NULL, 
	    &udpListenerThread, 
	    NULL);
    
    if (0 != rc) 
	perror("! Could not launch UDP server thread\n");

   
    // endless loop 
    for (;;) {

	// wait for signal from isr 
	pthread_cond_wait( &inputWaitCond, &inputWaitMutex);

	// Compose pattern switch UDP packet (TODO: that could be more elegant here!)
	if (dbg) printf("Switching pattern\n");
		
	snprintf( message.cmd, 8, "%s", "PTRNLI");
	message.serialNr = 123;
	message.timing = 0.0;
	message.pad[0] = 45*((count/3)%4);
	int deg45 = (count/3)%2;
	message.pad[1] = (deg45)?(5):(3) ;
	message.pad[2] = (deg45)?(12):(9);
	message.pad[3] = (deg45)?((count%3)*4):((count%3)*3);

	bbpnd_printUdp( &message ); 

	// send / recv UDP packets
	bbpnd_newSendUdp( udpSendSocket, &message );	
	n=recvfrom(udpSendSocket.socketFd,&message,sizeof(message),0,NULL,NULL);
	
	if (n>0)
	    bbpnd_printUdp( &message ); 
	else
	    printf("UDP timeout reached!\n"); 
    
	bzero( &message, sizeof(message));

	// copy timing parameters to local variables, locked via mutex
	pthread_mutex_lock( &paramMutex );
	int cycles = led_cycles;
	int tOn    = led_timeOn;
	int tOff   = led_timeOff;
	pthread_mutex_unlock( &paramMutex );

	// cycle LED on/off	
	for (i=0;i<cycles;i++) {
	    digitalWrite(2,1);
	    delay( tOn);
	    digitalWrite(2,0);
	    delay( tOff );
	}

	// increase pattern counter
	count++;
    }

   return 0;
}



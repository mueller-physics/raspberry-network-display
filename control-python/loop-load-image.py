#!/usr/bin/python3
import socket
import time
from struct import pack

# setup UDP socket
conn = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )


# we just loop over 15 entries, setting a new image every n'th sec
index = 0

while (True): 
    index = (index+1)%9
    
    # pack a data packet to send
    sendbytes = pack('< 8s Q d 1i 36x 256s', bytes("LDNMBR","ascii"), 0, 0, index, bytes("","ascii"))

    # send the UDP package
    conn.sendto( sendbytes, ("192.168.42.2", 32320)  )

    # wait two seconds
    time.sleep(0.5)



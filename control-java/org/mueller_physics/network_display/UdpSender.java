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

package org.mueller_physics.network_display;

import java.io.*;
import java.net.*;
import java.util.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class UdpSender {
    public static void main(String[] args) throws IOException {

        if (args.length != 2) {
             System.out.println("Usage: java UdpSender <hostname> <filename>");
             return;
        }

	if (args[1].length()>255) {
	    System.out.println(" Filename longer than 255 chars");
	    return;
	}


	// get a datagram socket
        DatagramSocket socket = new DatagramSocket();

        // send request
        byte[] buf = new byte[255+24];
        InetAddress address = InetAddress.getByName(args[0]);

	String cmd = "LDFILE";

	ByteBuffer bb = ByteBuffer.wrap( buf );
	bb.order( ByteOrder.LITTLE_ENDIAN );

	System.arraycopy( cmd.getBytes(), 0, buf, 0, cmd.length());
	bb.putLong(8,123);
	bb.putDouble(16,234);
	
	System.arraycopy( args[1].getBytes(), 0, buf, 32, args[1].length());

    
        DatagramPacket packet = new DatagramPacket(buf, buf.length, address, 32320);
        socket.send(packet);
    
        // get response
        packet = new DatagramPacket(buf, buf.length);

	socket.setSoTimeout(500);
	try {
	    socket.receive(packet);
	    // decode and display response
	    String  retState = BytesString.getString(buf, 0, 8);
	    Long    retSb    = bb.getLong(8);
	    Double  retMs    = bb.getDouble(16);
	    String  retAttr  = BytesString.getString(buf,24,255);
	    System.out.println("state: " + retState + " (took: "+retMs+" ms ) :: "+retAttr);
	}
	catch (SocketTimeoutException e) {
	    // timeout exception.
	    System.out.println("ERR: got no reply, timeout reached" + e);
	}

        socket.close();
    }
}





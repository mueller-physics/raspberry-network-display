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

import ij.ImagePlus;
import ij.plugin.PlugIn;
import ij.process.ImageProcessor;
import ij.process.ByteProcessor;
import ij.process.ColorProcessor;

import ij.ImageStack;
import ij.process.StackProcessor;

import ij.IJ;
import ij.Prefs;

import ij.gui.GenericDialog;


import java.net.Socket;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.SocketTimeoutException;
import java.io.OutputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;

import javax.swing.*;
import java.awt.GridLayout;
import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.BorderLayout;

import java.text.DecimalFormat;

// Small plugin to send images over the network
public class Send_Raspberry implements PlugIn {

	
	private static final int displWidth  = 1280;
	private static final int displHeight = 1024;

	private String [] MemBanks = { "O", "I", "II", "III", "IV" };

	private final static int maxStackLength = 12;

	private final static String prefKeyHostname="org.mueller-physics.network-display.hostname";

	public static void main( String [] args ) {
	    Send_Raspberry cl = new Send_Raspberry();
	    cl.run("control");
	}



	/**
	 * Run method to be called by ImageJ.
	 *
	 * @param arg Accepts "upload" to display the stack uploader,
	 *	      "control" to fork a control window thread.
	 * */
	@Override
	public void run(String arg) {

		if (arg.equals("upload"))
			sendStack();

		if (arg.equals("control"))
			new Thread(new ControlPanel()).start();

		if (arg.equals("send-and-display")) {
		    ImagePlus aip = ij.WindowManager.getCurrentImage();
		    if ((aip == null) || (aip.getProcessor() == null)) {
			IJ.showMessage("no image selected / open");
			return;
		    }
		    // try to send the image...
		    try {
			displayImage( null, aip.getProcessor());
		    }  catch (Exception e) {
			IJ.log("Problem sending image: "+e);
		    }
		    IJ.showStatus("Image sent.");
		}

		if (arg.equals("set-hostname")) {
		    GenericDialog gd = new GenericDialog("Network display");
		    String hostname = Prefs.get(prefKeyHostname, "Name or IP");
		    gd.addStringField("Hostname", hostname);
		    gd.showDialog();
		    if (gd.wasCanceled()) return;
		    hostname = gd.getNextString();
		    Prefs.set(prefKeyHostname, hostname);
		    //IJ.log("Raspberry hostname set to: "+hostname);
		}

	}


	/** internal function to send a stack */
	void sendStack() {
		// get the active image plus instance
		ImagePlus aip = ij.WindowManager.getCurrentImage();
		if (aip == null) {
			IJ.showMessage("No active image stack selected");
			return;
		}
		int numImages = aip.getStack().getSize();
		if (numImages > maxStackLength ) {
			IJ.showMessage("Please provide a stack with less than "+maxStackLength+" images");
			return;
		}

		// check the image format
		//if ( aip.getBitDepth() != 8 ) {
		//		IJ.showMessage("Please use an 8bit grayscale stack");
		//		return ;
		//	}

		// get our presets
		String hostname = Prefs.get(prefKeyHostname, "127.0.0.1");

		// Display the parameter dialog
 		GenericDialog gd = new GenericDialog("Transfer to Raspberry");
 		gd.addStringField("Hostname", hostname);
		gd.addCheckbox("Save hostname?",false);
		gd.addChoice("Memory bank", MemBanks, "I");
		gd.showDialog();
      		if (gd.wasCanceled()) return;

		hostname = gd.getNextString();
		boolean storeConf = gd.getNextBoolean();

		int nrMemory = gd.getNextChoiceIndex();

		if (storeConf)
			Prefs.set(prefKeyHostname, hostname);

		ImageStack out;
		
		// see if we have to resize the stack
		if (  (aip.getStack().getWidth() != displWidth ) 
		   || (aip.getStack().getHeight()!= displHeight ) ) {
		   	StackProcessor sp = new StackProcessor( aip.getStack() );
		   	out = sp.resize(displWidth,displHeight,true);
		   	ImagePlus newStack = new ImagePlus ("Resized stack to display", out );
		   	newStack.show();
		   } 
		else {
			out = aip.getStack();
		}

		Timing sendTimer = new Timing();
		sendTimer.start();
		
		// send the stack
		try {
			sendStack( hostname, out, nrMemory );
		} catch (Exception e) {
			IJ.showMessage("Network connection problem: "+e);	
		}

		sendTimer.stop();
		double rate  = (displWidth*displHeight*3*numImages) / (1024.*1024.);
		       rate /= sendTimer.getMS() / 1000.;
		DecimalFormat df = new DecimalFormat("##.###");
		       
		IJ.showMessage("Stack with "+numImages+" images transmitted ["
			+sendTimer+" / "+df.format(rate)+" MB/s]");
	
	}

	/**
	 * Send an 8bit grayscale stack to the network display.
	 * The image stack is only send and stored, not automatically displayed.
	 *
	 * @param host The hostname of the remote display
	 * @param is   The image stack to send
	 * @param nrMemory The storage position on the remote display, 0..4.
	 */
	static public void sendStack( String host, ImageStack is , int nrMemory ) 
		throws java.net.UnknownHostException, java.io.IOException {
		if (  (is.getWidth() != displWidth ) || (is.getHeight()!= displHeight ) )
		   throw new RuntimeException("Stack is not sized "+displWidth+"x"+displHeight);

		//if (is.getBitDepth() != 8)
		//   throw new RuntimeException("Stack is not 8 bit grayscale");
		if (is.getSize() > maxStackLength)
		   throw new RuntimeException("Stack is larger than "+maxStackLength+" images");
		
		// Send the images
		for (int i=1; i<=is.getSize(); i++) {
			IJ.showProgress(i, is.getSize());
			sendImage( host, is.getProcessor(i),i , nrMemory );
		}
	}

	

	/**
	 *  Send a 8bit grayscale processor to the network display.
	 *
	 *  @param host The hostname of the remote display
	 *  @param ip   The ImageProcessor to send
	 *  @param storPos The storage position (0..11) in the memory bank
	 *  @param nrMemory Number of the memory bank to store to
	 */
	static public void sendImage( String host , ImageProcessor ip, int storPos , int nrMemory )
		throws java.net.UnknownHostException, java.io.IOException {

		// generate raw data
		final int w =  ip.getWidth() ;
		final int h = ip.getHeight() ;
		final int len   = w * h * 3;
		final byte [] px = new byte[ len ];

		for (int y=0;y<h;y++)
		for (int x=0;x<w; x++) {
			px[(x + y*w)*3+0] = (byte)ip.getPixel(x,y); 
			px[(x + y*w)*3+1] = (byte)ip.getPixel(x,y); 
			px[(x + y*w)*3+2] = (byte)ip.getPixel(x,y); 
		}
		
		// connect a Socket
		Socket comm = new Socket( host, 32321 );
		OutputStream commStr = comm.getOutputStream();

		// build the send command
		ByteBuffer command = ByteBuffer.allocate(32);
		command.order(java.nio.ByteOrder.LITTLE_ENDIAN);
		
		String commandString = "STORIMG";

		command.put( commandString.getBytes("US-ASCII"),0,7);
		command.putInt( 24, storPos );
		command.putInt( 28, nrMemory );

		// send the command, then the image
		commStr.write( command.array() );
		commStr.flush();
		commStr.write( px );

		commStr.flush();
		comm.shutdownInput();
		comm.shutdownOutput();
		
	}


	/**
	 *  Sends and automatically displays an ImageProcessor
	 *
	 *  @param host The hostname of the remote display,
	 *		set null to use default
	 *  @param ip   The ImageProcessor to send
	 */
	static public void displayImage( String host , ImageProcessor ip )
		throws java.net.UnknownHostException, java.io.IOException {

		// get hostname
		if ( host == null)	
			host = Prefs.get(prefKeyHostname, "invalid.tld");
		if ( host.equals("invalid"))
		    throw new java.net.UnknownHostException("No hostname set");

		// set size and resize if needed
		final int w=displWidth, h=displHeight, len   = w * h * 3;
		final byte [] px = new byte[ len ];
		if (( ip.getWidth() != w ) || ( ip.getHeight() != h))
		    ip = ip.resize( w, h);


		// convert to color, copy the color values into output bytes
		ColorProcessor cp = ip.convertToColorProcessor();
		int [] rgb = new int[3];
		for (int y=0;y<h;y++)
		for (int x=0;x<w; x++) {
		    cp.getPixel(x,y,rgb);
		    for (int i=0;i<3;i++)
			px[(x + y*w)*3+i] = (byte)rgb[i]; 
		}
		
		// connect a Socket
		Socket comm = new Socket( host, 32321 );
		OutputStream commStr = comm.getOutputStream();

		// build the send command
		ByteBuffer command = ByteBuffer.allocate(32);
		command.order(java.nio.ByteOrder.LITTLE_ENDIAN);
		
		String commandString = "RECVSHOW";
		command.put( commandString.getBytes("US-ASCII"),0,8);

		// send the command, then the image
		commStr.write( command.array() );
		commStr.flush();
		commStr.write( px );
	
		// clean up
		commStr.flush();
		comm.shutdownInput();
		comm.shutdownOutput();
	}





	/**
	 *  Return the current content of the remote framebuffer. This allows
	 *  to easily grep the image that is currently displayed.
	 *
	 *  @param host The hostname of the remote display
	 *  @param full If set, the full framebuffer is copied, otherwise only
	 *		a 256x256 center crop.
	 *  @return A ByteProcessor with the framebuffer content.
	 */
	static public ByteProcessor getFramebuffer( String host, boolean full )
		throws java.net.UnknownHostException, java.io.IOException {

		// generate raw data
		final byte [] px = new byte[ displWidth*displHeight*3 ];
		ByteProcessor bs;
		if (full) 
		    bs = new ByteProcessor( displWidth, displHeight);
		else
		    bs = new ByteProcessor( 256, 256);

		// connect a Socket
		Socket comm = new Socket( host, 32321 );
		comm.setReceiveBufferSize( displWidth*displHeight );
		
		OutputStream commStrOut = comm.getOutputStream();
		InputStream  commStrIn  = comm.getInputStream();

		// build the send command
		ByteBuffer command = ByteBuffer.allocate(32);
		command.order(java.nio.ByteOrder.LITTLE_ENDIAN);
		
		String commandString = (full)?("GETFFB"):("GETCFB");

		command.put( commandString.getBytes("US-ASCII"),
			0, commandString.length());

		// send the command, then get the image
		commStrOut.write( command.array() );
		commStrOut.flush();

		if (full)
		for (int i=0;i<px.length; )
			i+=commStrIn.read( px, i , px.length-i );
		else
		for (int i=0;i<256*256*3; )
			i+=commStrIn.read( px, i , 256*256*3-i );
			

		comm.shutdownInput();
		comm.shutdownOutput();
	
		// copy image into the Byte Processor
		if (full)
		    for (int y=0;y<displHeight;y++)
		    for (int x=0;x<displWidth;x++) {
		        bs.set(x,y,px[(x + y*displWidth)*3]); 
		    }
		else
		    for (int y=0;y<256;y++)
		    for (int x=0;x<256;x++) {
		        bs.set(x,y,px[(x + y*256)*3]); 
		    }
		
		return bs;

	}

	/**
	 * Send a datagram switching to a SIM pattern.
	 *
	 * @param hostname The hostname of the remote display
	 * @param angle The angle of the pattern, may be 0,45,90,135
	 * @param px    Number of 'on' pixel in each pattern repetition
	 * @param len   Length of the pattern, in pixel
	 * @param pha   Phase offset, in pixel
	 */
	static public void setSimPattern(String hostname, int angle, int px, int len, int pha ) 
		throws java.net.UnknownHostException, java.io.IOException  { 
		sendUDP( hostname, "PTRNLI", angle, px, len, pha);
		//IJ.log("sim pattern: "+angle+" "+px+" "+len+" "+pha);
	}


	/**
	 * Send a datagram switching to a Checkerboard Pattern.
	 *
	 * @param hostname The hostname of the remote display
	 * @param len  The width and height of each checker, in pixel
	 * @param xo   The offset in x-direction, in pixel
	 * @param yo   The offset in y-direction, in pixel
	 */
	static public void setCBPattern(String hostname, int len, int xo, int yo ) 
		throws java.net.UnknownHostException, java.io.IOException  { 
		sendUDP( hostname, "PTRNCB", len, xo, yo, 0);
	}


	/**
	 * Set a checkerboard to one of 8 predefined position.
	 *
	 * @param hostname The hostname of the remote display
	 * @param len      The width and height of each checker, in pixel
	 * @param nr       The number of the pattern
	 *
	 */
	static public void setStdCBPattern( String hostname, int len, int nr) 
		throws java.net.UnknownHostException, java.io.IOException  {

		if ( ( nr>=0 ) && ( nr < 4 ) )
			setCBPattern( hostname, len, (len/2)*nr,0 );
		if ( ( nr>=4 ) && ( nr < 8 ) )
			setCBPattern( hostname, len, (len/2)*((nr+1)%4),len/2 );

	}


	/**
	 * Set one of the 12 standard sim pattern angle/phase combiantions
	 */
	static public void setStdSimPattern( String hostname, int len, int pos) 
		throws java.net.UnknownHostException, java.io.IOException  
		{

		int third = len/3;

		// angle == 0
		if ((pos>=0)&&(pos<3))
			setSimPattern( hostname, 0, third, len, third*pos);

		// angle == 90
		if ((pos>=3)&&(pos<6))
			setSimPattern( hostname, 90, third, len, third*(pos-3));

		// angle == 45
		if ((pos>=6)&&(pos<9))
			setSimPattern( hostname, 45, 
				third*3/2, len*3/2, third*3*(pos-6)/2);

		// angle == 135	
		if ((pos>=9)&&(pos<12))
			setSimPattern( hostname, 135, 
				third*3/2, len*3/2, third*3*(pos-9)/2);
		
	}
	

	/**
	 * TODO: Update comment. Test for background substraction
	 * */
	static public void setSparseSimPattern( String hostname, 
	    int lenIn, int posIn, int sparseness ) 
		throws java.net.UnknownHostException, java.io.IOException  
		{

		int third = lenIn/3;

		int len = lenIn * sparseness;
		
		int pos = posIn % 12;
		int off = (posIn/12)*lenIn;


		// angle == 0
		if ((pos>=0)&&(pos<3))
			setSimPattern( hostname, 0, third, len, third*pos +off);

		// angle == 90
		if ((pos>=3)&&(pos<6))
			setSimPattern( hostname, 90, third, len, third*(pos-3) +off);

		// angle == 45
		if ((pos>=6)&&(pos<9))
			setSimPattern( hostname, 45, 
				third*3/2, len*3/2, (third*(pos-6)+off)*3/2);

		// angle == 135	
		if ((pos>=9)&&(pos<12))
			setSimPattern( hostname, 135, 
				third*3/2, len*3/2, (third*(pos-9)+off)*3/2);
		
	}







	/**
	 * Send a datagram switching to an image by number.
	 */
	static public void switchImage(String hostname, int imgNr ) 
		throws java.net.UnknownHostException, java.io.IOException  { 
		sendUDP( hostname, "LDNMBR", imgNr);
	}
	/**
	 * Send a datagram switching to a different memory bank.
	 */
	static public void switchMemoryBank(String hostname, int imgNr ) 
		throws java.net.UnknownHostException, java.io.IOException  { 
		sendUDP( hostname, "SELMB", imgNr);
	}

	/**
	 * Send a datagram to load the test image
	 * */
	static public void setTestImage(String hostname) 
		throws java.net.UnknownHostException, java.io.IOException  { 
		//IJ.log(" Sending Test image");
		sendUDP( hostname, "SETTEST", 0);
	}

	/**
	 * Send a datagram to set a constant gray value
	 * */
	static public void setConstantGray(String hostname, int gray) 
		throws java.net.UnknownHostException, java.io.IOException  { 
		//IJ.log(" Sending constant "+gray);
		sendUDP( hostname, "SETCNST", gray);
	}



	static void sendUDP(String hostname, String command, int a0) 
		throws java.net.UnknownHostException, java.io.IOException  {
			sendUDP( hostname, command, a0, 0,0,0);
		}




	/** Send a datagram to standard port 32320 */
	static void sendUDP(String hostname, String command, int a0, int a1, int a2, int a3 ) 
		throws java.net.UnknownHostException, java.io.IOException  { 
	    sendUDP( hostname, command, a0, a1, a2, a3, 32320 );
	}

	/** Send a datagram */
	static void sendUDP(String hostname, String command, int a0, int a1, int a2, int a3, int port ) 
		throws java.net.UnknownHostException, java.io.IOException  { 
		// get a datagram socket
       		DatagramSocket socket = new DatagramSocket();

		//IJ.log("Switch :"+hostname+" Nr "+imgNr);

        	// send request
        	ByteBuffer bb = ByteBuffer.allocate( 65 );

		bb.order( java.nio.ByteOrder.LITTLE_ENDIAN );
		byte [] cmdBytes = command.getBytes("US-ASCII");
		bb.put( cmdBytes, 0, cmdBytes.length);
		//bb.put( "(none)".getBytes("US-ASCII"),32,5);
		bb.putInt( 24, a0 );
		bb.putInt( 28, a1 );
		bb.putInt( 32, a2 );
		bb.putInt( 36, a3 );

		String foo="";
		for (int i=0; i<bb.array().length; i++) {
		    foo+=" "+bb.array()[i];
		}
		//IJ.log("Sending UPD message: "+foo);
	 
        	DatagramPacket sendpacket = 
        		new DatagramPacket(
        			bb.array(), bb.array().length, 
        			java.net.InetAddress.getByName(hostname), port);
		socket.send(sendpacket);
		
        	// get response
        	byte [] buf = new byte[64+256];
        	DatagramPacket packet = new DatagramPacket(buf, buf.length);
	
		socket.setSoTimeout(500);
		try {
	    		socket.receive(packet);
	    		// decode and display response
	    		ByteBuffer bbr = ByteBuffer.wrap(buf);
	    		bbr.order( java.nio.ByteOrder.LITTLE_ENDIAN );
	    		String  retState = BytesString.getString(buf, 0, 8);
	    		Long    retSb    = bbr.getLong(8);
	    		Double  retMs    = bbr.getDouble(16);
	    		String  retAttr  = BytesString.getString(buf,64,255);
	    		IJ.showStatus(retState + " (took: "+retMs+" ms ) "+retAttr);
		}
		catch (SocketTimeoutException e) {
			// timeout exception.
			IJ.showStatus("ERR: got no reply, timeout reached" );
		}

        	socket.close();
        	
    	}



	// small subclass to fork a control panel for UDP
	class ControlPanel implements Runnable, ActionListener {

		JButton [] nums;
		JButton [] mems;
		JButton ping,test,white,black,getffb,getcfb,
			sendCBpat, sendSIMpat, testCB, testSIM, 
			test2xSparseSIM, test3xSparseSIM, 
			sendSimSimTime, sendSlmBlink;
		JTextField host;

		JSpinner angSpin, phaSpin, patSpin, lenSpin, 
			 cbLenSpin, xoSpin, yoSpin;
		    
		JSpinner tOnSpin, tOffSpin, cycleSpin;
		
		// initialize the GUI
		{
			// tabs
			JTabbedPane tabbedPane = new JTabbedPane();

			// --------------------------------------------------------
			//
			// Striped Pattern tab

			JPanel patternContent = new JPanel(new GridLayout(5,2));

			angSpin = new JSpinner(new SpinnerNumberModel(0,0,135,45));
			phaSpin = new JSpinner(new SpinnerNumberModel(0,0,24,1));
			patSpin = new JSpinner(new SpinnerNumberModel(3,0,24,1));
			lenSpin = new JSpinner(new SpinnerNumberModel(9,0,24,1));
			sendSIMpat = new JButton("Send");
			sendSIMpat.addActionListener(this);

			patternContent.add(new JLabel("Angle"));
			patternContent.add( angSpin );
			patternContent.add(new JLabel("Phase"));
			patternContent.add( phaSpin );
			patternContent.add(new JLabel("Pattern on"));
			patternContent.add( patSpin );
			patternContent.add(new JLabel("Pattern len"));
			patternContent.add( lenSpin );
			patternContent.add(new JLabel(">"));
			patternContent.add(sendSIMpat);

			tabbedPane.addTab("Stripes", patternContent);

			// --------------------------------------------------------
			//
			// Checkerboard Pattern tab
			
			JPanel cbPatternContent = new JPanel(new GridLayout(4,2));

			xoSpin = new JSpinner(new SpinnerNumberModel(0,0,192,1));
			yoSpin = new JSpinner(new SpinnerNumberModel(0,0,192,1));
			cbLenSpin = new JSpinner(new SpinnerNumberModel(24,0,192,2));
			sendCBpat = new JButton("Send");
			sendCBpat.addActionListener(this);

			cbPatternContent.add(new JLabel("Patten len"));
			cbPatternContent.add( cbLenSpin );
			cbPatternContent.add(new JLabel("X offset"));
			cbPatternContent.add( xoSpin );
			cbPatternContent.add(new JLabel("Y offset"));
			cbPatternContent.add( yoSpin );
			cbPatternContent.add(new JLabel(">"));
			cbPatternContent.add(sendCBpat);
			
			tabbedPane.addTab("CB", cbPatternContent);

			// --------------------------------------------------------
			//
			// Memory tab
			
			// number buttons
			nums = new JButton[13];
			JPanel numsContent = new JPanel( new GridLayout(4,3) );
			for (int i=1;i<nums.length;i++) {
				nums[i] = new JButton(""+i);
				nums[i].addActionListener(this);
				numsContent.add(nums[i]);
				
			}
			
			// Memory buttons 
			mems = new JButton[ MemBanks.length ];
			JPanel memsContent = new JPanel( new GridLayout(  mems.length ,1) );
			for (int i=0;i<mems.length;i++) {
				mems[i] = new JButton( MemBanks[i] );
				mems[i].addActionListener(this);
				memsContent.add(mems[i]);
			}

			JPanel memRow     = new JPanel(new FlowLayout());
			memRow.add(  numsContent  );
			memRow.add(  memsContent );
			tabbedPane.addTab("Memory", memRow);



			// --------------------------------------------------------
			//
			// SLM Simulator tab

			/*
			JPanel simSimContent = new JPanel(new GridLayout(5,2));

			tOnSpin   = new JSpinner(new SpinnerNumberModel(200,0,1000,5));
			tOffSpin  = new JSpinner(new SpinnerNumberModel(100,0,500,5));
			cycleSpin = new JSpinner(new SpinnerNumberModel(4,1,16,1));
			sendSimSimTime = new JButton("Send");
			sendSimSimTime.addActionListener(this);
			sendSlmBlink = new JButton("Blink/Ping");
			sendSlmBlink.addActionListener(this);

			slmSimContent.add(new JLabel("tOn (ms)"));
			slmSimContent.add( tOnSpin );
			slmSimContent.add(new JLabel("tOff (ms)"));
			slmSimContent.add( tOffSpin );
			slmSimContent.add(new JLabel("Cycles"));
			slmSimContent.add( cycleSpin );
			slmSimContent.add(new JLabel(">"));
			slmSimContent.add( sendSimSimTime );
			slmSimContent.add(new JLabel(">"));
			slmSimContent.add( sendSlmBlink );

			
			tabbedPane.addTab("SlmSim", simSimContent);
			*/
						
			// --------------------------------------------------------
			//
			// Low static low
			
			// hostname and ping button
			JPanel lowButtons = new JPanel(new GridLayout(2,3));
			JPanel lowRow     = new JPanel(new GridLayout(2,1));
			
			host = new JTextField(Prefs.get(prefKeyHostname, "127.0.0.1"));
			ping = new JButton("Ping");
			test = new JButton("Test");
			getffb= new JButton("Get full");
			getcfb= new JButton("Get 256");
			black= new JButton("Black");
			white= new JButton("White");
			
			ping.addActionListener( this );
			test.addActionListener( this );
			white.addActionListener( this );
			black.addActionListener( this );
			getffb.addActionListener( this );
			getcfb.addActionListener( this );

			lowButtons.add( black );
			lowButtons.add( white );
			lowButtons.add( test );
			
			lowButtons.add( ping );
			lowButtons.add( getffb );
			lowButtons.add( getcfb );
		
			lowRow.add( host );
			lowRow.add( lowButtons );
	

			// create the main window
			JFrame frame = new JFrame("Raspberry control");
			frame.getContentPane().add( tabbedPane , BorderLayout.CENTER);	
			frame.getContentPane().add( lowRow , BorderLayout.PAGE_END);
			frame.pack();
			frame.setVisible(true);			
		}

		// this gets called on thread creation
		@Override
		public void run() {

			
		}
		// callback for our buttons
  		@Override
    		public void actionPerformed(ActionEvent e) {

		    Object who = e.getSource();
		    String hostname = host.getText();

		    try {
			// one of the numbers ...
			for (int i=1;i<nums.length;i++)
			if ( who == nums[i] ) 
			    switchImage( hostname, i);
			// one of the memory banks ...
			for (int i=0;i<mems.length;i++)
			if ( who == mems[i] ) 
			    switchMemoryBank( hostname, i);
			
			// or the test image
			if ( who == test )
			   setTestImage( hostname );
			
			// or a ping
			if ( who == ping )
			   sendUDP( hostname , "PING", 0);
			
			// or a fully black image
			if ( who == black )
			   setConstantGray( hostname , 0);
			
			// or a fully white image
			if ( who == white )
			   setConstantGray( hostname , 255);
		
			// or display the full image
			if ( who == getffb ) {
			    ImageProcessor ip = getFramebuffer( hostname , true );
			    ImagePlus displ01 = new ImagePlus("Current Display", ip);
			    displ01.show();
			}
			// or display 256^2 of the image
			if ( who == getcfb ) {
			    ImageProcessor ip = getFramebuffer( hostname , false );
			    ImagePlus displ01 = new ImagePlus("Current Display", ip);
			    displ01.show();
			}

			// set a sim pattern
			if ( who == sendSIMpat ) {
				int angle = (Integer)angSpin.getValue(); 
				int len   = (Integer)lenSpin.getValue(); 
				int pha   = (Integer)phaSpin.getValue();
				int pat   = (Integer)patSpin.getValue();

				setSimPattern( hostname, angle, pat, len, pha);
				
			}
	    		
	    		// set a cb pattern
			if ( who == sendCBpat ) {
				int len   = (Integer)cbLenSpin.getValue(); 
				int xo    = (Integer)xoSpin.getValue();
				int yo    = (Integer)yoSpin.getValue();

				setCBPattern( hostname, len, xo, yo);
				
			}

			// test the cb pattern
			if ( who == testCB ) {
				int len   = (Integer)cbLenSpin.getValue(); 
				ImageStack is = new ImageStack(256,256);
				for (int i=0;i<8;i++) {
					IJ.showProgress(i, 8);
					setStdCBPattern( hostname, len, i);
					is.addSlice(getFramebuffer( hostname , false )); 
				}

				ImagePlus displ01 = new ImagePlus("CB Test",is);
				displ01.show();

				
			}
	    
			// test the SIM pattern
			if ( who == testSIM ) {
				int len   = (Integer)lenSpin.getValue(); 
				ImageStack is = new ImageStack(256,256);
				for (int i=0;i<12;i++) {
					IJ.showProgress(i, 12);
					setStdSimPattern( hostname, len, i);
					is.addSlice(getFramebuffer( hostname , false )); 
				}

				ImagePlus displ01 = new ImagePlus("SIM Test",is);
				displ01.show();

				
			}
	    
			// test 2x sparse SIM pattern
			if ( who == test2xSparseSIM ) {
				int len   = (Integer)lenSpin.getValue(); 
				ImageStack is = new ImageStack(256,256);
				for (int i=0;i<24;i++) {
					IJ.showProgress(i, 24);
					setSparseSimPattern( hostname, len, i, 2);
					is.addSlice(getFramebuffer( hostname , false )); 
				}

				ImagePlus displ01 = new ImagePlus("sparse 2x SIM Test",is);
				displ01.show();

				
			}
			// test 3x sparse SIM pattern
			if ( who == test3xSparseSIM ) {
				int len   = (Integer)lenSpin.getValue(); 
				ImageStack is = new ImageStack(256,256);
				for (int i=0;i<36;i++) {
					IJ.showProgress(i, 36);
					setSparseSimPattern( hostname, len, i, 3);
					is.addSlice(getFramebuffer( hostname , false )); 
				}

				ImagePlus displ01 = new ImagePlus("sparse 3x SIM Test",is);
				displ01.show();

				
			}
			
			// update SIM Simulator timing
			/*
			if ( who == sendSimSimTime ) {
				int tOn    = (Integer)tOnSpin.getValue(); 
				int tOff   = (Integer)tOffSpin.getValue(); 
				int cycles = (Integer)cycleSpin.getValue(); 

				sendUDP( hostname, "SLMTIME", tOn, tOff, cycles, 0, 32322);
				
			}
			*/
			
			// send a blink / ping
			if ( who == sendSlmBlink )
			   sendUDP( hostname , "SLMBLNK", 0,0,0,0,32322);




		    } catch (java.net.UnknownHostException ex) {
			IJ.showMessage("Network failed: "+ex);
		    } catch (java.io.IOException ex2) {
			IJ.showMessage("IO Exception "+ex2);
		    }
	
    		}
		
	}

	/**
	 * Provides a timer
	 */
        class Timing {
                long start, stop, runtime, outtime;
                Timing() { start =  System.currentTimeMillis(); }
                public void start() { start = System.currentTimeMillis(); };
                public void stop() {
                    stop = System.currentTimeMillis();
                    runtime += stop-start;
                    outtime=runtime;
                    runtime=0;
                    }
                public void hold(){
                    stop = System.currentTimeMillis();
                    runtime += stop-start;
                    outtime  = runtime;
                    start =stop;
                }
                @Override public String toString(){ return("ms: "+(outtime));}
                public double getMS() { return outtime ; }
        }

}

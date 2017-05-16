Configuration specific to the Raspberry Pi.

The 'config.txt' file is commented and sets up
the HDMI output for the Holoeye SLM. 

For other devices, this might not be needed, but the Holoeye
requires a very specific modeline setup... it communicates correct
EDID data (with that peculiar 58Hz refresh modeline), 
but the Raspberry does not interpret that correctly.

You will also need to edit e.g. '/etc/rc.local' or
write an init script if you want the framebuffer
software to start together with the raspberry.

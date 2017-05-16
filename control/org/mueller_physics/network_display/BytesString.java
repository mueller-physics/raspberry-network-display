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

public class BytesString {
    /** Converts null-terminated byte array to a string */
    static String getString( byte [] in, int off, int maxLen ) {

	int pos = 0;
	for (int i=off; i<maxLen+off; i++) {
	    pos = i;
	    if (in[i] ==0) break;
	}
	
	String ret = new String( in , off, pos-off );
	return ret;

    }

}

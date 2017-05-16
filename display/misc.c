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

#include "misc.h"
#include <sys/time.h>

double getTime() {

    struct timeval  tv;
    gettimeofday(&tv, 0);

    double time_in_mill = 
         (tv.tv_sec) * 1000. + (tv.tv_usec) / 1000. ; 
    return time_in_mill;
}

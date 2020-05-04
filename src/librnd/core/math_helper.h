/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <math.h>

/* ---------------------------------------------------------------------------
 * some math constants
 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781 /* 1/sqrt(2) */
#endif
#define RND_M180                 (M_PI/180.0)
#define RND_RAD_TO_DEG           (180.0/M_PI)
#define RND_TAN_22_5_DEGREE_2    0.207106781  /* tan(22.5)/2 */
#define RND_COS_22_5_DEGREE      0.923879533  /* cos(22.5) */
#define RND_TAN_30_DEGREE        0.577350269  /* tan(30) */
#define RND_TAN_60_DEGREE        1.732050808  /* tan(60) */
#define RND_LN_2_OVER_2          0.346573590
#define RND_TO_RADIANS(degrees)  (RND_M180 * (degrees))
#define RND_SQRT2                1.41421356237309504880 /* sqrt(2) */

#define RND_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define RND_ABS(a)               (((a) < 0) ? -(a) : (a))
#define RND_SQUARE(x)            ((float) (x) * (float) (x))

/* ---------------------------------------------------------------------------
 * misc macros, some might already be defined by <limits.h>
 */
#define RND_MIN(a,b)  ((a) < (b) ? (a) : (b))
#define RND_MAX(a,b)  ((a) > (b) ? (a) : (b))
#define RND_SGN(a)    ((a) >0 ? 1 : ((a) == 0 ? 0 : -1))

#ifndef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#endif
#ifndef SGN
#define SGN(a)  ((a) >0 ? 1 : ((a) == 0 ? 0 : -1))
#endif
#define RND_SGNZ(a)                  ((a) >=0 ? 1 : -1)
#define RND_MAKE_MIN(a,b)            if ((b) < (a)) (a) = (b)
#define RND_MAKE_MAX(a,b)            if ((b) > (a)) (a) = (b)


#define RND_SWAP_SIGN_X(x)  (x)
#define RND_SWAP_SIGN_Y(y)  (-(y))

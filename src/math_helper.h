/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include <math.h>

/* ---------------------------------------------------------------------------
 * some math constants
 */
#ifndef	M_PI
#define	M_PI			3.14159265358979323846
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 		0.707106781	/* 1/sqrt(2) */
#endif
#define	PCB_M180			(M_PI/180.0)
#define PCB_RAD_TO_DEG		(180.0/M_PI)
#define	PCB_TAN_22_5_DEGREE_2	0.207106781	/* 0.5*tan(22.5) */
#define PCB_COS_22_5_DEGREE		0.923879533	/* cos(22.5) */
#define	PCB_TAN_30_DEGREE		0.577350269	/* tan(30) */
#define	PCB_TAN_60_DEGREE		1.732050808	/* tan(60) */
#define PCB_LN_2_OVER_2		0.346573590
#define PCB_TO_RADIANS(degrees)	(PCB_M180 * (degrees))

#define PCB_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define PCB_ABS(a)	   (((a) < 0) ? -(a) : (a))

/* ---------------------------------------------------------------------------
 * misc macros, some might already be defined by <limits.h>
 */
#ifndef	MIN
#define	MIN(a,b)		((a) < (b) ? (a) : (b))
#define	MAX(a,b)		((a) > (b) ? (a) : (b))
#endif
#ifndef SGN
#define SGN(a)			((a) >0 ? 1 : ((a) == 0 ? 0 : -1))
#endif
#define PCB_SGNZ(a)                 ((a) >=0 ? 1 : -1)
#define PCB_MAKE_MIN(a,b)            if ((b) < (a)) (a) = (b)
#define PCB_MAKE_MAX(a,b)            if ((b) > (a)) (a) = (b)


#define	PCB_SWAP_SIGN_X(x)		(x)
#define	PCB_SWAP_SIGN_Y(y)		(-(y))

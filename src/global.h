/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* definition of types */
#ifndef	PCB_GLOBAL_H
#define	PCB_GLOBAL_H

#include "config.h"

#include "const.h"
#include "macro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#include "global_typedefs.h"
#include "global_objs.h"
#include "attrib.h"
#include "rats_patch.h"
#include "list_common.h"
#include "list_line.h"
#include "list_arc.h"
#include "list_text.h"
#include "list_poly.h"
#include "list_pad.h"
#include "list_pin.h"
#include "list_rat.h"
#include "vtonpoint.h"
#include "hid.h"
#include "polyarea.h"
#include "vtroutestyle.h"

/* Needs to be gone: */
#include "board.h"

#define PCB_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define PCB_ABS(a)	   (((a) < 0) ? -(a) : (a))

/* ---------------------------------------------------------------------------
 * the basic object types supported by PCB
 */

#include "global_element.h"

/* ----------------------------------------------------------------------
 * pointer to low-level copy, move and rotate functions
 */
typedef struct {
	void *(*Line) (LayerTypePtr, LineTypePtr);
	void *(*Text) (LayerTypePtr, TextTypePtr);
	void *(*Polygon) (LayerTypePtr, PolygonTypePtr);
	void *(*Via) (PinTypePtr);
	void *(*Element) (ElementTypePtr);
	void *(*ElementName) (ElementTypePtr);
	void *(*Pin) (ElementTypePtr, PinTypePtr);
	void *(*Pad) (ElementTypePtr, PadTypePtr);
	void *(*LinePoint) (LayerTypePtr, LineTypePtr, PointTypePtr);
	void *(*Point) (LayerTypePtr, PolygonTypePtr, PointTypePtr);
	void *(*Arc) (LayerTypePtr, ArcTypePtr);
	void *(*Rat) (RatTypePtr);
} ObjectFunctionType, *ObjectFunctionTypePtr;

/* ---------------------------------------------------------------------------
 * structure used by device drivers
 */

typedef struct {								/* holds a connection */
	Coord X, Y;										/* coordinate of connection */
	long int type;								/* type of object in ptr1 - 3 */
	void *ptr1, *ptr2;						/* the object of the connection */
	pcb_cardinal_t group;								/* the layer group of the connection */
	LibraryMenuType *menu;				/* the netmenu this *SHOULD* belong too */
} ConnectionType, *ConnectionTypePtr;

struct net_st {								/* holds a net of connections */
	pcb_cardinal_t ConnectionN,					/* the number of connections contained */
	  ConnectionMax;							/* max connections from malloc */
	ConnectionTypePtr Connection;
	RouteStyleTypePtr Style;
};

typedef struct {								/* holds a list of nets */
	pcb_cardinal_t NetN,								/* the number of subnets contained */
	  NetMax;											/* max subnets from malloc */
	NetTypePtr Net;
} NetListType, *NetListTypePtr;

typedef struct {								/* holds a list of net lists */
	pcb_cardinal_t NetListN,						/* the number of net lists contained */
	  NetListMax;									/* max net lists from malloc */
	NetListTypePtr NetList;
} NetListListType, *NetListListTypePtr;

typedef struct {								/* holds a generic list of pointers */
	pcb_cardinal_t PtrN,								/* the number of pointers contained */
	  PtrMax;											/* max subnets from malloc */
	void **Ptr;
} PointerListType, *PointerListTypePtr;

typedef struct {
	pcb_cardinal_t BoxN,								/* the number of boxes contained */
	  BoxMax;											/* max boxes from malloc */
	BoxTypePtr Box;

} BoxListType, *BoxListTypePtr;

/* ---------------------------------------------------------------------------
 * Macros called by various action routines to show usage or to report
 * a syntax error and fail
 */
#define AUSAGE(x) Message (PCB_MSG_INFO, "Usage:\n%s\n", (x##_syntax))
#define AFAIL(x) { Message (PCB_MSG_ERROR, "Syntax error.  Usage:\n%s\n", (x##_syntax)); return 1; }

/* Make sure to catch usage of non-portable functions in debug mode */
#ifndef NDEBUG
#	undef strdup
#	undef strndup
#	undef snprintf
#	undef round
#	define strdup      never_use_strdup__use_pcb_strdup
#	define strndup     never_use_strndup__use_pcb_strndup
#	define snprintf    never_use_snprintf__use_pcb_snprintf
#	define round       never_use_round__use_pcb_round
#endif

#endif /* PCB_GLOBAL_H  */

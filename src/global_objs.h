/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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

#ifndef GLOBAL_OBJS_H
#define GLOBAL_OBJS_H
#include <genlist/gendlist.h>
#include "config.h"
#include "attrib.h"
#include "flag.h"
#include "globalconst.h"
#include "global_typedefs.h"
#include "polyarea.h"

struct pcb_box_s {								/* a bounding box */
	Coord X1, Y1;									/* upper left */
	Coord X2, Y2;									/* and lower right corner */
};


/* ---------------------------------------------------------------------------
 * Do not change the following definitions even if they're not very
 * nice.  It allows us to have functions act on these "base types" and
 * not need to know what kind of actual object they're working on.
 */

/* Any object that uses the "object flags" defined in const.h, or
   exists as an object on the pcb, MUST be defined using this as the
   first fields, either directly or through ANYLINEFIELDS.  */
#define ANYOBJECTFIELDS			\
	BoxType		BoundingBox;	\
	long int	ID;		\
	FlagType	Flags; \
	AttributeListType Attributes

	/*  struct LibraryEntryType *net */

/* Lines, pads, and rats all use this so they can be cross-cast.  */
#define	ANYLINEFIELDS			\
	ANYOBJECTFIELDS;		\
	Coord		Thickness,      \
                        Clearance;      \
	PointType	Point1,		\
			Point2

/* All on-pcb objects (elements, lines, pads, vias, rats, etc) are
   based on this. */
typedef struct {
	ANYOBJECTFIELDS;
} AnyObjectType, *AnyObjectTypePtr;

struct pcb_point_s {   /* a line/polygon point */
	Coord X, Y, X2, Y2;   /* so Point type can be cast as BoxType */
	long int ID;
};

/* Lines, rats, pads, etc.  */
typedef struct {
	ANYLINEFIELDS;
} AnyLineObjectType, *AnyLineObjectTypePtr;

struct polygon_st {							/* holds information about a polygon */
	ANYOBJECTFIELDS;
	pcb_cardinal_t PointN,							/* number of points in polygon */
	  PointMax;										/* max number from malloc() */
	POLYAREA *Clipped;						/* the clipped region of this polygon */
	PLINE *NoHoles;								/* the polygon broken into hole-less regions */
	int NoHolesValid;							/* Is the NoHoles polygon up to date? */
	PointTypePtr Points;					/* data */
	pcb_cardinal_t *HoleIndex;					/* Index of hole data within the Points array */
	pcb_cardinal_t HoleIndexN;					/* number of holes in polygon */
	pcb_cardinal_t HoleIndexMax;				/* max number from malloc() */
	gdl_elem_t link;              /* a text is in a list of a layer */
};

struct pcb_rat_line_s {          /* a rat-line */
	ANYLINEFIELDS;
	pcb_cardinal_t group1, group2; /* the layer group each point is on */
	gdl_elem_t link;               /* an arc is in a list on a design */
};

struct pad_st {									/* a SMD pad */
	ANYLINEFIELDS;
	Coord Mask;
	char *Name, *Number;					/* 'Line' */
	void *Element;
	void *Spare;
	gdl_elem_t link;              /* a pad is in a list (element) */
};

struct pin_st {
	ANYOBJECTFIELDS;
	Coord Thickness, Clearance, Mask, DrillingHole;
	Coord X, Y;										/* center and diameter */
	char *Name, *Number;
	void *Element;
	void *Spare;
	gdl_elem_t link;              /* a pin is in a list (element) */
};

/* This is the extents of a Pin or Via, depending on whether it's a
   hole or not.  */
#define PIN_SIZE(pinptr) (TEST_FLAG(PCB_FLAG_HOLE, (pinptr)) \
			  ? (pinptr)->DrillingHole \
			  : (pinptr)->Thickness)

/* ---------------------------------------------------------------------------
 * symbol and font related stuff
 */
typedef struct symbol_st {								/* a single symbol */
	LineTypePtr Line;
	pcb_bool Valid;
	pcb_cardinal_t LineN,								/* number of lines */
	  LineMax;
	Coord Width, Height,					/* size of cell */
	  Delta;											/* distance to next symbol */
} SymbolType, *SymbolTypePtr;

struct pcb_font_s {        /* complete set of symbols */
	Coord MaxHeight,         /* maximum cell width and height */
	  MaxWidth;
	BoxType DefaultSymbol;   /* the default symbol is a filled box */
	SymbolType Symbol[MAX_FONTPOSITION + 1];
	pcb_bool Valid;
};

/* TODO: this could be replaced with pcb_obj_t */
typedef struct onpoint_st {
	int type;
	union {
		void *any;
		LineType *line;
		ArcType *arc;
	} obj;
} OnpointType;

#endif

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

/* global source constants */

#ifndef	PCB_CONST_H
#define	PCB_CONST_H

#include <limits.h>
#include <math.h>

/* ---------------------------------------------------------------------------
 * the layer-numbers of the two additional special layers
 * 'component' and 'solder'. The offset of MAX_LAYER is not added
 */
#define	SOLDER_LAYER		0
#define	COMPONENT_LAYER		1

/* ---------------------------------------------------------------------------
 * some math constants
 */
#ifndef	M_PI
#define	M_PI			3.14159265358979323846
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 		0.707106781	/* 1/sqrt(2) */
#endif
#define	M180			(M_PI/180.0)
#define RAD_TO_DEG		(180.0/M_PI)
#define	TAN_22_5_DEGREE_2	0.207106781	/* 0.5*tan(22.5) */
#define COS_22_5_DEGREE		0.923879533	/* cos(22.5) */
#define	TAN_30_DEGREE		0.577350269	/* tan(30) */
#define	TAN_60_DEGREE		1.732050808	/* tan(60) */
#define LN_2_OVER_2		0.346573590

/* PCB/physical unit conversions */
#define COORD_TO_MIL(n)	((n) / 25400.0)
#define MIL_TO_COORD(n)	((n) * 25400.0)
#define COORD_TO_MM(n)	((n) / 1000000.0)
#define MM_TO_COORD(n)	((n) * 1000000.0)
#define COORD_TO_INCH(n)	(COORD_TO_MIL(n) / 1000.0)
#define INCH_TO_COORD(n)	(MIL_TO_COORD(n) * 1000.0)

/* These need to be carefully written to avoid overflows, and return
   a Coord type.  */
#define SCALE_TEXT(COORD,TEXTSCALE) ((Coord)((COORD) * ((double)(TEXTSCALE) / 100.0)))
#define UNSCALE_TEXT(COORD,TEXTSCALE) ((Coord)((COORD) * (100.0 / (double)(TEXTSCALE))))

/* ---------------------------------------------------------------------------
 * modes
 */
typedef enum {
	PCB_MODE_NO              = 0,   /* no mode selected */
	PCB_MODE_VIA             = 1,   /* draw vias */
	PCB_MODE_LINE            = 2,   /* draw lines */
	PCB_MODE_RECTANGLE       = 3,   /* create rectangles */
	PCB_MODE_POLYGON         = 4,   /* draw filled polygons */
	PCB_MODE_PASTE_BUFFER    = 5,   /* paste objects from buffer */
	PCB_MODE_TEXT            = 6,   /* create text objects */
	PCB_MODE_ROTATE          = 102, /* rotate objects */
	PCB_MODE_REMOVE          = 103, /* remove objects */
	PCB_MODE_MOVE            = 104, /* move objects */
	PCB_MODE_COPY            = 105, /* copy objects */
	PCB_MODE_INSERT_POINT    = 106, /* insert point into line/polygon */
	PCB_MODE_RUBBERBAND_MOVE = 107, /* move objects and attached lines */
	PCB_MODE_THERMAL         = 108, /* toggle thermal layer flag */
	PCB_MODE_ARC             = 109, /* draw arcs */
	PCB_MODE_ARROW           = 110, /* selection with arrow mode */
	PCB_MODE_PAN             = 0,   /* same as no mode */
	PCB_MODE_LOCK            = 111, /* lock/unlock objects */
	PCB_MODE_POLYGON_HOLE    = 112  /* cut holes in filled polygons */
} pcb_mode_t;

/* ---------------------------------------------------------------------------
 * object flags
 */

/* %start-doc pcbfile ~objectflags
@node Object Flags
@section Object Flags

Note that object flags can be given numerically (like @code{0x0147})
or symbolically (like @code{"found,showname,square"}.  Some numeric
values are reused for different object types.  The table below lists
the numeric value followed by the symbolic name.

@table @code
@item 0x0001 pin
If set, this object is a pin.  This flag is for internal use only.
@item 0x0002 via
Likewise, for vias.
@item 0x0004 found
If set, this object has been found by @code{FindConnection()}.
@item 0x0008 hole
For pins and vias, this flag means that the pin or via is a hole
without a copper annulus.
@item 0x0010 rat
If set for a line, indicates that this line is a rat line instead of a
copper trace.
@item 0x0010 pininpoly
For pins and pads, this flag is used internally to indicate that the
pin or pad overlaps a polygon on some layer.
@item 0x0010 clearpoly
For polygons, this flag means that pins and vias will normally clear
these polygons (thus, thermals are required for electrical
connection).  When clear, polygons will solidly connect to pins and
vias.
@item 0x0010 hidename
For elements, when set the name of the element is hidden.
@item 0x0020 showname
For elements, when set the names of pins are shown.
@item 0x0020 clearline
For lines and arcs, the line/arc will clear polygons instead of
connecting to them.
@item 0x0020 fullpoly
For polygons, the full polygon is drawn (i.e. all parts instead of only the biggest one).
@item 0x0040 selected
Set when the object is selected.
@item 0x0080 onsolder
For elements and pads, indicates that they are on the solder side.
@item 0x0080 auto
For lines and vias, indicates that these were created by the
autorouter.
@item 0x0100 square
For pins and pads, indicates a square (vs round) pin/pad.
@item 0x0200 rubberend
For lines, used internally for rubber band moves.
@item 0x0200 warn
For pins, vias, and pads, set to indicate a warning.
@item 0x0400 usetherm
Obsolete, indicates that pins/vias should be drawn with thermal
fingers.
@item 0x0400
Obsolete, old files used this to indicate lines drawn on silk.
@item 0x0800 octagon
Draw pins and vias as octagons.
@item 0x1000 drc
Set for objects that fail DRC.
@item 0x2000 lock
Set for locked objects.
@item 0x4000 edge2
For pads, indicates that the second point is closer to the edge.  For
pins, indicates that the pin is closer to a horizontal edge and thus
pinout text should be vertical.
@item 0x8000 marker
Marker used internally to avoid revisiting an object.
@item 0x10000 nopaste
For pads, set to prevent a solderpaste stencil opening for the
pad.  Primarily used for pads used as fiducials.
@end table
%end-doc */
typedef enum {
	NOFLAG           = 0x00000,
	PINFLAG          = 0x00001, /* is a pin */
	VIAFLAG          = 0x00002, /* is a via */
	FOUNDFLAG        = 0x00004, /* used by 'FindConnection()' */
	HOLEFLAG         = 0x00008, /* pin or via is only a hole */
	NOPASTEFLAG      = 0x00008, /* pad should not receive solderpaste.  This is to support fiducials */
	RATFLAG          = 0x00010, /* indicates line is a rat line */
	PININPOLYFLAG    = 0x00010, /* pin found inside poly - same as rat line since not used on lines */
	CLEARPOLYFLAG    = 0x00010, /* pins/vias clear these polygons */
	HIDENAMEFLAG     = 0x00010, /* hide the element name */
	DISPLAYNAMEFLAG  = 0x00020, /* display the names of pins/pads of an element */
	CLEARLINEFLAG    = 0x00020, /* line doesn't touch polygons */
	FULLPOLYFLAG     = 0x00020, /* full polygon is drawn (i.e. all parts instead of only the biggest one) */
	SELECTEDFLAG     = 0x00040, /* object has been selected */
	ONSOLDERFLAG     = 0x00080, /* element is on bottom side */
	AUTOFLAG         = 0x00080, /* line/via created by auto-router */
	SQUAREFLAG       = 0x00100, /* pin is square, not round */
	RUBBERENDFLAG    = 0x00200, /* indicates one end already rubber banding same as warn flag since pins/pads won't use it */
	WARNFLAG         = 0x00200, /* Warning for pin/via/pad */
	USETHERMALFLAG   = 0x00400, /* draw pin, via with thermal fingers */
	ONSILKFLAG       = 0x00400, /* old files use this to indicate silk */
	OCTAGONFLAG      = 0x00800, /* draw pin/via as octagon instead of round */
	DRCFLAG          = 0x01000, /* flag like FOUND flag for DRC checking */
	LOCKFLAG         = 0x02000, /* object locked in place */
	EDGE2FLAG        = 0x04000, /* Padr.Point2 is closer to outside edge also pinout text for pins is vertical */
	VISITFLAG        = 0x08000, /* marker to avoid re-visiting an object */
	NONETLISTFLAG    = 0x10000, /* element is not on the netlist and should not interfere with the netlist */
	MINCUTFLAG       = 0x20000, /* used by the mincut short find code */
	ONPOINTFLAG      = 0x40000, /*!< crosshair is on line point or arc point */
/*	NOCOPY_FLAGS     = (FOUNDFLAG | CONNECTEDFLAG | ONPOINTFLAG)*/
} pcb_flag_t;

/* ---------------------------------------------------------------------------
 * object types (bitfield)
 */
typedef enum {
	PCB_TYPE_NONE          = 0x00000, /* special: no object */
	PCB_TYPE_VIA           = 0x00001,
	PCB_TYPE_ELEMENT       = 0x00002,
	PCB_TYPE_LINE          = 0x00004,
	PCB_TYPE_POLYGON       = 0x00008,
	PCB_TYPE_TEXT          = 0x00010,
	PCB_TYPE_RATLINE       = 0x00020,

	PCB_TYPE_PIN           = 0x00100, /* objects that are part */
	PCB_TYPE_PAD           = 0x00200, /* 'pin' of SMD element */
	PCB_TYPE_ELEMENT_NAME  = 0x00400, /* of others */
	PCB_TYPE_POLYGON_POINT = 0x00800,
	PCB_TYPE_LINE_POINT    = 0x01000,
	PCB_TYPE_ELEMENT_LINE  = 0x02000,
	PCB_TYPE_ARC           = 0x04000,
	PCB_TYPE_ELEMENT_ARC   = 0x08000,

	PCB_TYPE_LOCKED        = 0x10000, /* used to tell search to include locked items. */
	PCB_TYPE_NET           = 0x20000, /* used to select whole net. */

	/* groups/properties */
	PCB_TYPEMASK_PIN       = (PCB_TYPE_VIA | PCB_TYPE_PIN),
	PCB_TYPEMASK_LOCK      = (PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLYGON | PCB_TYPE_ELEMENT | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_LOCKED),

	PCB_TYPEMASK_ALL       = (~0)   /* all bits set */
} pcb_obj_type_t;

#endif

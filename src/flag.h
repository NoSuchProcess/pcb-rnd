/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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

#ifndef PCB_FLAG_H
#define PCB_FLAG_H

#include "globalconst.h"

/* Nobody should know about the internals of this except the macros in
   macros.h that access it.  This structure must be simple-assignable
   for now.  */
typedef struct pcb_unknown_flag_s pcb_unknown_flag_t;
struct pcb_unknown_flag_s {
	char *str;
	pcb_unknown_flag_t *next;
};

typedef struct {
	unsigned long f;							/* generic flags */
	unsigned char t[(MAX_LAYER + 1) / 2];	/* thermals */
	unsigned char q;							/* square geometry flag */
	unsigned char int_conn_grp;
	pcb_unknown_flag_t *unknowns;
} pcb_flag_t;

extern pcb_flag_t no_flags;

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
%end-doc */
typedef enum {
	PCB_FLAG_NO           = 0x00000,
	PCB_FLAG_PIN          = 0x00001, /*!< If set, this object is a pin.  This flag is for internal use only. */
	PCB_FLAG_VIA          = 0x00002, /*!< Likewise, for vias. */
	PCB_FLAG_FOUND        = 0x00004, /*!< If set, this object has been found by @code{FindConnection()}. */
	PCB_FLAG_HOLE         = 0x00008, /*!< For pins and vias, this flag means that the pin or via is a hole without a copper annulus. */
	PCB_FLAG_NOPASTE      = 0x00008, /*!< Pad should not receive solderpaste.  This is to support fiducials */
	PCB_FLAG_RAT          = 0x00010, /*!< If set for a line, indicates that this line is a rat line instead of a copper trace. */
	PCB_FLAG_PININPOLY    = 0x00010, /*!< For pins and pads, this flag is used internally to indicate that the pin or pad overlaps a polygon on some layer.*/
	PCB_FLAG_CLEARPOLY    = 0x00010, /*!< For polygons, this flag means that pins and vias will normally clear these polygons (thus, thermals are required for electrical connection).  When clear, polygons will solidly connect to pins and vias. */
	PCB_FLAG_HIDENAME     = 0x00010, /*!< For elements, when set the name of the element is hidden. */
	PCB_FLAG_DISPLAYNAME  = 0x00020, /*!< For elements, when set the names of pins are shown. */
	PCB_FLAG_CLEARLINE    = 0x00020, /*!< For lines and arcs, the line/arc will clear polygons instead of connecting to them. */
	PCB_FLAG_FULLPOLY     = 0x00020, /*!< For polygons, the full polygon is drawn (i.e. all parts instead of only the biggest one). */
	PCB_FLAG_SELECTED     = 0x00040, /*!< Set when the object is selected. */
	PCB_FLAG_ONSOLDER     = 0x00080, /*!< For elements and pads, indicates that they are on the solder side. */
	PCB_FLAG_AUTO         = 0x00080, /*!< For lines and vias, indicates that these were created by the autorouter. */
	PCB_FLAG_SQUARE       = 0x00100, /*!< For pins and pads, indicates a square (vs round) pin/pad. */
	PCB_FLAG_RUBBEREND    = 0x00200, /*!< For lines, used internally for rubber band moves: indicates one end already rubber banding. */
	PCB_FLAG_WARN         = 0x00200, /*!< For pins, vias, and pads, set to indicate a warning. */
	PCB_FLAG_USETHERMAL   = 0x00400, /*!< Obsolete, indicates that pins/vias should be drawn with thermal fingers. */
	PCB_FLAG_ONSILK       = 0x00400, /*!< Obsolete, old files used this to indicate lines drawn on silk. (Used by io_pcb for compatibility.) */
	PCB_FLAG_OCTAGON      = 0x00800, /*!< Draw pins and vias as octagons. */
	PCB_FLAG_DRC          = 0x01000, /*!< Set for objects that fail DRC: flag like FOUND flag for DRC checking. */
	PCB_FLAG_LOCK         = 0x02000, /*!< Set for locked objects. */
	PCB_FLAG_EDGE2        = 0x04000, /*!< For pads, indicates that the second point is closer to the edge.  For pins, indicates that the pin is closer to a horizontal edge and thus pinout text should be vertical. (Padr.Point2 is closer to outside edge also pinout text for pins is vertical) */
	PCB_FLAG_VISIT        = 0x08000, /*!< marker to avoid re-visiting an object */
	PCB_FLAG_NONETLIST    = 0x10000, /* element is not on the netlist and should not interfere with the netlist */
	PCB_FLAG_MINCUT       = 0x20000, /* used by the mincut short find code */
	PCB_FLAG_ONPOINT      = 0x40000  /*!< crosshair is on line point or arc point */
/*	PCB_FLAG_NOCOPY     = (PCB_FLAG_FOUND | CONNECTEDFLAG | PCB_FLAG_ONPOINT)*/
} pcb_flag_values_t;

/* ---------------------------------------------------------------------------
 * PCB flags - kept only for file format compatibility reasons; these bits
   should be a mirror of booleans from the conf.
 */
/* %start-doc pcbfile ~pcbflags
@node PCBFlags
@section PCBFlags
@table @code
@item 0x00001
Pinout displays pin numbers instead of pin names.
@item 0x00002
Use local reference for moves, by setting the mark at the beginning of
each move.
@item 0x00004
When set, only polygons and their clearances are drawn, to see if
polygons have isolated regions.
@item 0x00008
Display DRC region on crosshair.
@item 0x00010
Do all move, mirror, rotate with rubberband connections.
@item 0x00020
Display descriptions of elements, instead of refdes.
@item 0x00040
Display names of elements, instead of refdes.
@item 0x00080
Auto-DRC flag.  When set, PCB doesn't let you place copper that
violates DRC.
@item 0x00100
Enable 'all-direction' lines.
@item 0x00200
Switch starting angle after each click.
@item 0x00400
Force unique names on board.
@item 0x00800
New lines/arc clear polygons.
@item 0x01000
pcb_crosshair snaps to pins and pads.
@item 0x02000
Show the solder mask layer.
@item 0x04000
Draw with thin lines.
@item 0x08000
Move items orthogonally.
@item 0x10000
Draw autoroute paths real-time.
@item 0x20000
New polygons are full ones.
@item 0x40000
Names are locked, the mouse cannot select them.
@item 0x80000
Everything but names are locked, the mouse cannot select anything else.
@item 0x100000
New polygons are full polygons.
@item 0x200000
When set, element names are not drawn.
+@item 0x800000
+snap to certain off-grid points.
+@item 0x1000000
+highlight lines and arcs when the crosshair is on one of their endpoints.
@end table
%end-doc */

#define PCB_FLAGS               0x01ffffff	/* all used flags */

#define SHOWNUMBERFLAG          0x00000001
#define LOCALREFFLAG            0x00000002
#define CHECKPLANESFLAG         0x00000004
#define SHOWPCB_FLAG_DRC             0x00000008
#define RUBBERBANDFLAG          0x00000010
#define	DESCRIPTIONFLAG         0x00000020
#define	NAMEONPCBFLAG           0x00000040
#define AUTOPCB_FLAG_DRC             0x00000080
#define	ALLDIRECTIONFLAG        0x00000100
#define SWAPSTARTDIRFLAG        0x00000200
#define UNIQUENAMEFLAG          0x00000400
#define CLEARNEWFLAG            0x00000800
#define SNAPPCB_FLAG_PIN             0x00001000
#define SHOWMASKFLAG            0x00002000
#define THINDRAWFLAG            0x00004000
#define ORTHOMOVEFLAG           0x00008000
#define LIVEROUTEFLAG           0x00010000
#define THINDRAWPOLYFLAG        0x00020000
#define LOCKNAMESFLAG           0x00040000
#define ONLYNAMESFLAG           0x00080000
#define NEWPCB_FLAG_FULLPOLY         0x00100000
#define HIDENAMESFLAG           0x00200000
#define ENABLEPCB_FLAG_MINCUT        0x00400000



/* For passing modified flags to other functions. */
pcb_flag_t pcb_flag_make(unsigned int);
pcb_flag_t pcb_flag_old(unsigned int);
pcb_flag_t pcb_flag_add(pcb_flag_t, unsigned int);
pcb_flag_t pcb_flag_mask(pcb_flag_t, unsigned int);
void pcb_flag_erase(pcb_flag_t *f);
#define		pcb_no_flags() pcb_flag_make(0)

/* ---------------------------------------------------------------------------
 * some routines for flag setting, clearing, changing and testing
 */
#define PCB_FLAG_SET(F,P)       ((P)->Flags.f |= (F))
#define PCB_FLAG_CLEAR(F,P)     ((P)->Flags.f &= (~(F)))
#define PCB_FLAG_TEST(F,P)      ((P)->Flags.f & (F) ? 1 : 0)
#define PCB_FLAG_TOGGLE(F,P)    ((P)->Flags.f ^= (F))
#define PCB_FLAG_ASSIGN(F,V,P)  ((P)->Flags.f = ((P)->Flags.f & (~(F))) | ((V) ? (F) : 0))
#define PCB_FLAGS_TEST(F,P)     (((P)->Flags.f & (F)) == (F) ? 1 : 0)

typedef enum {
	PCB_CHGFLG_CLEAR,
	PCB_CHGFLG_SET,
	PCB_CHGFLG_TOGGLE
} pcb_change_flag_t;

#define PCB_FLAG_CHANGE(how, F, P) \
do { \
	switch(how) { \
		case PCB_CHGFLG_CLEAR:  PCB_FLAG_CLEAR(F, P); break; \
		case PCB_CHGFLG_SET:    PCB_FLAG_SET(F, P); break; \
		case PCB_CHGFLG_TOGGLE: PCB_FLAG_TOGGLE(F, P); break; \
	} \
} while(0)

#define PCB_FLAG_EQ(F1,F2)	(memcmp (&F1, &F2, sizeof(pcb_flag_t)) == 0)

#define PCB_FLAG_THERM(L)		(0xf << (4 *((L) % 2)))

#define PCB_FLAG_THERM_TEST(L,P)		((P)->Flags.t[(L)/2] & PCB_FLAG_THERM(L) ? 1 : 0)
#define PCB_FLAG_THERM_GET(L,P)		(((P)->Flags.t[(L)/2] >> (4 * ((L) % 2))) & 0xf)
#define PCB_FLAG_THERM_CLEAR(L,P)	(P)->Flags.t[(L)/2] &= ~PCB_FLAG_THERM(L)
#define PCB_FLAG_THERM_ASSIGN(L,V,P)	(P)->Flags.t[(L)/2] = ((P)->Flags.t[(L)/2] & ~PCB_FLAG_THERM(L)) | ((V)  << (4 * ((L) % 2)))


#define PCB_FLAG_SQUARE_GET(P)		((P)->Flags.q)
#define PCB_FLAG_SQUARE_CLEAR(P)		(P)->Flags.q = 0
#define PCB_FLAG_SQUARE_ASSIGN(V,P)	(P)->Flags.q = V


#define PCB_FLAG_INTCONN_GET(P)		((P)->Flags.int_conn_grp)

extern int pcb_mem_any_set(unsigned char *, int);
#define PCB_FLAG_THERM_TEST_ANY(P)	pcb_mem_any_set((P)->Flags.t, sizeof((P)->Flags.t))

#endif

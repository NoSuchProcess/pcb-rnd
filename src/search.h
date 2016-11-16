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

/* prototypes for search routines */

#ifndef	PCB_SEARCH_H
#define	PCB_SEARCH_H

#include "obj_common.h"
#include "rats.h"
#include "misc_util.h"

int pcb_lines_intersect(pcb_coord_t ax1, pcb_coord_t ay1, pcb_coord_t ax2, pcb_coord_t ay2, pcb_coord_t bx1, pcb_coord_t by1, pcb_coord_t bx2, pcb_coord_t by2);

#define SLOP 5
/* ---------------------------------------------------------------------------
 * some useful macros
 */
/* ---------------------------------------------------------------------------
 * some define to check for 'type' in box
 */
#define	PCB_POINT_IN_BOX(x,y,b)	\
	(PCB_IS_BOX_NEGATIVE(b) ?  \
	((x) >= (b)->X2 && (x) <= (b)->X1 && (y) >= (b)->Y2 && (y) <= (b)->Y1) \
	: \
	((x) >= (b)->X1 && (x) <= (b)->X2 && (y) >= (b)->Y1 && (y) <= (b)->Y2))

#define	PCB_VIA_OR_PIN_IN_BOX(v,b) \
	PCB_POINT_IN_BOX((v)->X,(v)->Y,(b))

#define	PCB_LINE_IN_BOX(l,b)	\
	(PCB_POINT_IN_BOX((l)->Point1.X,(l)->Point1.Y,(b)) &&	\
	PCB_POINT_IN_BOX((l)->Point2.X,(l)->Point2.Y,(b)))

#define	PCB_PAD_IN_BOX(p,b)	PCB_LINE_IN_BOX((pcb_line_t *)(p),(b))

#define	PCB_BOX_IN_BOX(b1,b)	\
	((b1)->X1 >= (b)->X1 && (b1)->X2 <= (b)->X2 &&	\
	((b1)->Y1 >= (b)->Y1 && (b1)->Y2 <= (b)->Y2))

#define	PCB_TEXT_IN_BOX(t,b)	\
	(PCB_BOX_IN_BOX(&((t)->BoundingBox), (b)))

#define	PCB_POLYGON_IN_BOX(p,b)	\
	(PCB_BOX_IN_BOX(&((p)->BoundingBox), (b)))

#define	PCB_ELEMENT_IN_BOX(e,b)	\
	(PCB_BOX_IN_BOX(&((e)->BoundingBox), (b)))

#define PCB_ARC_IN_BOX(a,b)		\
	(PCB_BOX_IN_BOX(&((a)->BoundingBox), (b)))

/* == the same but accept if any part of the object touches the box == */
#define PCB_POINT_IN_CIRCLE(x, y, cx, cy, r) \
	(pcb_distance2(x, y, cx, cy) <= (double)(r) * (double)(r))

#define PCB_CIRCLE_TOUCHES_BOX(cx, cy, r, b) \
	(    PCB_POINT_IN_BOX((cx)-(r),(cy),(b)) || PCB_POINT_IN_BOX((cx)+(r),(cy),(b)) || PCB_POINT_IN_BOX((cx),(cy)-(r),(b)) || PCB_POINT_IN_BOX((cx),(cy)+(r),(b)) \
	  || PCB_POINT_IN_CIRCLE((b)->X1, (b)->Y1, (cx), (cy), (r)) || PCB_POINT_IN_CIRCLE((b)->X2, (b)->Y1, (cx), (cy), (r)) || PCB_POINT_IN_CIRCLE((b)->X1, (b)->Y2, (cx), (cy), (r)) || PCB_POINT_IN_CIRCLE((b)->X2, (b)->Y2, (cx), (cy), (r)))

#define	PCB_VIA_OR_PIN_TOUCHES_BOX(v,b) \
	PCB_CIRCLE_TOUCHES_BOX((v)->X,(v)->Y,((v)->Thickness + (v)->DrillingHole/2),(b))

#define	PCB_LINE_TOUCHES_BOX(l,b)	\
	(    pcb_lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X1, (b)->Y1, (b)->X2, (b)->Y1) \
	  || pcb_lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X1, (b)->Y1, (b)->X1, (b)->Y2) \
	  || pcb_lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X2, (b)->Y2, (b)->X1, (b)->Y2) \
	  || pcb_lines_intersect((l)->Point1.X,(l)->Point1.Y,(l)->Point2.X,(l)->Point2.Y, (b)->X2, (b)->Y2, (b)->X2, (b)->Y1) \
	  || PCB_LINE_IN_BOX((l), (b)))

#define	PCB_PAD_TOUCHES_BOX(p,b)	PCB_LINE_TOUCHES_BOX((pcb_line_t *)(p),(b))

/* a corner of either box is within the other, or edges cross */
#define	PCB_BOX_TOUCHES_BOX(b1,b2)	\
	(   PCB_POINT_IN_BOX((b1)->X1,(b1)->Y1,b2) || PCB_POINT_IN_BOX((b1)->X1,(b1)->Y2,b2) || PCB_POINT_IN_BOX((b1)->X2,(b1)->Y1,b2) || PCB_POINT_IN_BOX((b1)->X2,(b1)->Y2,b2)  \
	 || PCB_POINT_IN_BOX((b2)->X1,(b2)->Y1,b1) || PCB_POINT_IN_BOX((b2)->X1,(b2)->Y2,b1) || PCB_POINT_IN_BOX((b2)->X2,(b2)->Y1,b1) || PCB_POINT_IN_BOX((b2)->X2,(b2)->Y2,b1)  \
	 || pcb_lines_intersect((b1)->X1,(b1)->Y1, (b1)->X2,(b1)->Y1, (b2)->X1,(b2)->Y1, (b2)->X1,(b2)->Y2) \
	 || pcb_lines_intersect((b2)->X1,(b2)->Y1, (b2)->X2,(b2)->Y1, (b1)->X1,(b1)->Y1, (b1)->X1,(b1)->Y2))

#define	PCB_TEXT_TOUCHES_BOX(t,b)	\
	(PCB_BOX_TOUCHES_BOX(&((t)->BoundingBox), (b)))

#define	PCB_POLYGON_TOUCHES_BOX(p,b)	\
	(PCB_BOX_TOUCHES_BOX(&((p)->BoundingBox), (b)))

#define	PCB_ELEMENT_TOUCHES_BOX(e,b)	\
	(PCB_BOX_TOUCHES_BOX(&((e)->BoundingBox), (b)))

#define PCB_ARC_TOUCHES_BOX(a,b)		\
	(PCB_BOX_TOUCHES_BOX(&((a)->BoundingBox), (b)))


/* == the combination of *_IN_* and *_TOUCHES_*: use IN for positive boxes == */
#define PCB_IS_BOX_NEGATIVE(b) (((b)->X2 < (b)->X1) || ((b)->Y2 < (b)->Y1))

#define	PCB_BOX_NEAR_BOX(b1,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_BOX_TOUCHES_BOX(b1,b) : PCB_BOX_IN_BOX(b1,b))

#define	PCB_VIA_OR_PIN_NEAR_BOX(v,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_VIA_OR_PIN_TOUCHES_BOX(v,b) : PCB_VIA_OR_PIN_IN_BOX(v,b))

#define	PCB_LINE_NEAR_BOX(l,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_LINE_TOUCHES_BOX(l,b) : PCB_LINE_IN_BOX(l,b))

#define	PCB_PAD_NEAR_BOX(p,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_PAD_TOUCHES_BOX(p,b) : PCB_PAD_IN_BOX(p,b))

#define	PCB_TEXT_NEAR_BOX(t,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_TEXT_TOUCHES_BOX(t,b) : PCB_TEXT_IN_BOX(t,b))

#define	PCB_POLYGON_NEAR_BOX(p,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_POLYGON_TOUCHES_BOX(p,b) : PCB_POLYGON_IN_BOX(p,b))

#define	PCB_ELEMENT_NEAR_BOX(e,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_ELEMENT_TOUCHES_BOX(e,b) : PCB_ELEMENT_IN_BOX(e,b))

#define PCB_ARC_NEAR_BOX(a,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_ARC_TOUCHES_BOX(a,b) : PCB_ARC_IN_BOX(a,b))



/* ---------------------------------------------------------------------------
 * prototypes
 */
pcb_bool pcb_is_point_on_line(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_line_t *);
pcb_bool pcb_is_point_in_pin(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_pin_t *);
pcb_bool pcb_is_point_on_arc(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_arc_t *);
pcb_bool pcb_is_point_on_line_end(pcb_coord_t, pcb_coord_t, pcb_rat_t *);
pcb_bool pcb_is_line_in_rectangle(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_line_t *);
pcb_bool pcb_is_line_in_quadrangle(pcb_point_t p[4], pcb_line_t *Line);
pcb_bool pcb_is_arc_in_rectangle(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_arc_t *);
pcb_bool pcb_is_point_in_pad(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_pad_t *);
pcb_bool pcb_is_point_in_box(pcb_coord_t, pcb_coord_t, pcb_box_t *, pcb_coord_t);

int pcb_search_obj_by_location(unsigned, void **, void **, void **, pcb_coord_t, pcb_coord_t, pcb_coord_t);
int pcb_search_screen(pcb_coord_t, pcb_coord_t, int, void **, void **, void **);
int pcb_search_grid_slop(pcb_coord_t, pcb_coord_t, int, void **, void **, void **);
int pcb_search_obj_by_id(pcb_data_t *, void **, void **, void **, int, int);
pcb_element_t *pcb_search_elem_by_name(pcb_data_t *, const char *);

#endif

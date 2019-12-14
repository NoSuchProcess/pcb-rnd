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

/* Search object by location routines */

#ifndef PCB_SEARCH_H
#define PCB_SEARCH_H

#include "global_typedefs.h"
#include "layer.h"

int pcb_lines_intersect(pcb_coord_t ax1, pcb_coord_t ay1, pcb_coord_t ax2, pcb_coord_t ay2, pcb_coord_t bx1, pcb_coord_t by1, pcb_coord_t bx2, pcb_coord_t by2);
pcb_bool pcb_arc_in_box(pcb_arc_t *arc, pcb_box_t *b);

#define PCB_SLOP 5

/* ---------------------------------------------------------------------------
 * some define to check for 'type' in box
 */
#define	PCB_POINT_IN_BOX(x,y,b) \
	(PCB_IS_BOX_NEGATIVE(b) ?  \
	((x) >= (b)->X2 && (x) <= (b)->X1 && (y) >= (b)->Y2 && (y) <= (b)->Y1) \
	: \
	((x) >= (b)->X1 && (x) <= (b)->X2 && (y) >= (b)->Y1 && (y) <= (b)->Y2))

#define	PCB_VIA_OR_PIN_IN_BOX(v,b) \
	( \
	PCB_POINT_IN_BOX((v)->X-(v)->Thickness/2,(v)->Y-(v)->Thickness/2,(b)) && \
	PCB_POINT_IN_BOX((v)->X+(v)->Thickness/2,(v)->Y+(v)->Thickness/2,(b)) \
	)

#define	PCB_LINE_IN_BOX(l,b) \
	(PCB_POINT_IN_BOX((l)->Point1.X,(l)->Point1.Y,(b)) &&	\
	PCB_POINT_IN_BOX((l)->Point2.X,(l)->Point2.Y,(b)))

#define	PCB_PAD_IN_BOX(p,b) PCB_LINE_IN_BOX((pcb_line_t *)(p),(b))

#define	PCB_BOX_IN_BOX(b1,b) \
	((b1)->X1 >= (b)->X1 && (b1)->X2 <= (b)->X2 &&	\
	((b1)->Y1 >= (b)->Y1 && (b1)->Y2 <= (b)->Y2))

#define	PCB_TEXT_IN_BOX(t,b) \
	(PCB_BOX_IN_BOX(&((t)->BoundingBox), (b)))

#define	PCB_POLYGON_IN_BOX(p,b) \
	(PCB_BOX_IN_BOX(&((p)->BoundingBox), (b)))

#define	PCB_SUBC_IN_BOX(s,b) \
	(PCB_BOX_IN_BOX(&((s)->BoundingBox), (b)))

#define	PCB_ELEMENT_IN_BOX(e,b) \
	(PCB_BOX_IN_BOX(&((e)->BoundingBox), (b)))

/* the bounding box is much larger than the minimum, use it to decide it
   it is worth doing the expensive precise calculations */
#define PCB_ARC_IN_BOX(a,b) \
	((PCB_BOX_TOUCHES_BOX(&((a)->BoundingBox), (b))) && (pcb_arc_in_box(a,b)))

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

#define	PCB_XYLINE_ISECTS_BOX(x1,y1,x2,y2,b) \
	(    pcb_lines_intersect(x1,y1,x2,y2, (b)->X1, (b)->Y1, (b)->X2, (b)->Y1) \
	  || pcb_lines_intersect(x1,y1,x2,y2, (b)->X1, (b)->Y1, (b)->X1, (b)->Y2) \
	  || pcb_lines_intersect(x1,y1,x2,y2, (b)->X2, (b)->Y2, (b)->X1, (b)->Y2) \
	  || pcb_lines_intersect(x1,y1,x2,y2, (b)->X2, (b)->Y2, (b)->X2, (b)->Y1) \
	)

#define	PCB_PAD_TOUCHES_BOX(p,b)	PCB_LINE_TOUCHES_BOX((pcb_line_t *)(p),(b))

/* a corner of either box is within the other, or edges cross */
#define	PCB_BOX_TOUCHES_BOX(b1,b2) \
	(   PCB_POINT_IN_BOX((b1)->X1,(b1)->Y1,b2) || PCB_POINT_IN_BOX((b1)->X1,(b1)->Y2,b2) || PCB_POINT_IN_BOX((b1)->X2,(b1)->Y1,b2) || PCB_POINT_IN_BOX((b1)->X2,(b1)->Y2,b2)  \
	 || PCB_POINT_IN_BOX((b2)->X1,(b2)->Y1,b1) || PCB_POINT_IN_BOX((b2)->X1,(b2)->Y2,b1) || PCB_POINT_IN_BOX((b2)->X2,(b2)->Y1,b1) || PCB_POINT_IN_BOX((b2)->X2,(b2)->Y2,b1)  \
	 || pcb_lines_intersect((b1)->X1,(b1)->Y1, (b1)->X2,(b1)->Y1, (b2)->X1,(b2)->Y1, (b2)->X1,(b2)->Y2) \
	 || pcb_lines_intersect((b2)->X1,(b2)->Y1, (b2)->X2,(b2)->Y1, (b1)->X1,(b1)->Y1, (b1)->X1,(b1)->Y2))

#define	PCB_TEXT_TOUCHES_BOX(t,b) \
	(PCB_BOX_TOUCHES_BOX(&((t)->BoundingBox), (b)))

#define	PCB_POLYGON_TOUCHES_BOX(p,b) \
	(pcb_poly_is_rect_in_p((b)->X2, (b)->Y2, (b)->X1, (b)->Y1, (p)))

#define	PCB_SUBC_TOUCHES_BOX(s,b) \
	(PCB_BOX_TOUCHES_BOX(&((s)->BoundingBox), (b)))

#define	PCB_ELEMENT_TOUCHES_BOX(e,b) \
	(PCB_BOX_TOUCHES_BOX(&((e)->BoundingBox), (b)))

#define PCB_ARC_TOUCHES_BOX(a,b) \
	(pcb_is_arc_in_rectangle((b)->X2, (b)->Y2, (b)->X1, (b)->Y1, (a)))


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

#define	PCB_SUBC_NEAR_BOX(s,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_SUBC_TOUCHES_BOX(s,b) : PCB_SUBC_IN_BOX(s,b))

#define	PCB_ELEMENT_NEAR_BOX(e,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_ELEMENT_TOUCHES_BOX(e,b) : PCB_ELEMENT_IN_BOX(e,b))

#define PCB_ARC_NEAR_BOX(a,b) \
	(PCB_IS_BOX_NEGATIVE(b) ? PCB_ARC_TOUCHES_BOX(a,b) : PCB_ARC_IN_BOX(a,b))

pcb_bool pcb_is_point_on_line(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius, pcb_line_t *Line);
pcb_bool pcb_is_point_on_thinline( pcb_coord_t X, pcb_coord_t Y, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2,pcb_coord_t Y2 );
pcb_bool pcb_is_point_on_line_end(pcb_coord_t X, pcb_coord_t Y, pcb_rat_t *Line);
pcb_bool pcb_is_point_on_arc(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius, pcb_arc_t *Arc);
pcb_bool pcb_is_line_in_rectangle(pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_line_t *Line);
pcb_bool pcb_is_line_in_quadrangle(pcb_point_t p[4], pcb_line_t *Line);
pcb_bool pcb_is_arc_in_rectangle(pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_arc_t *Arc);
pcb_bool pcb_is_point_in_line(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius, pcb_any_line_t *Pad);
pcb_bool pcb_is_point_in_box(pcb_coord_t X, pcb_coord_t Y, pcb_box_t *box, pcb_coord_t Radius);

/* Return the distance^2 between a line-center and a point */
double pcb_point_line_dist2(pcb_coord_t X, pcb_coord_t Y, pcb_line_t *Line);

/* Return the first line object that has its centerline crossing the point;
   if ang is not NULL, only return lines that are pointing in the right
   angle (also accept 180 degree rotation)
   if no_subc_part is true, ignore lines that are part of subcircuits;
   if no_term is true, ignore lines that are terminals */
pcb_line_t *pcb_line_center_cross_point(pcb_layer_t *layer, pcb_coord_t x, pcb_coord_t y, pcb_angle_t *ang, pcb_bool no_subc_part, pcb_bool no_term);

int pcb_search_screen(pcb_coord_t X, pcb_coord_t Y, int Type, void **Result1, void **Result2, void **Result3);
int pcb_search_grid_slop(pcb_coord_t X, pcb_coord_t Y, int Type, void **Result1, void **Result2, void **Result3);
int pcb_search_obj_by_location(unsigned long Type, void **Result1, void **Result2, void **Result3, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius);
int pcb_search_obj_by_id(pcb_data_t *Base, void **Result1, void **Result2, void **Result3, int ID, int type);
int pcb_search_obj_by_id_(pcb_data_t *Base, void **Result1, void **Result2, void **Result3, int ID, int type); /* no-hace version */

/* Same as pcb_search_obj_by_id, but search in buffers too */
int pcb_search_obj_by_id_buf2(pcb_data_t *Base, void **Result1, void **Result2, void **Result3, int ID, int type);

#endif


/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, box.h, was written and is
 *  Copyright (c) 2001 C. Scott Ananian.
 *
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *
 *  Old contact info:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */


/* random box-related utilities. */

#ifndef PCB_BOX_H
#define PCB_BOX_H

#include <assert.h>
#include "math_helper.h"
#include "global_typedefs.h"
#include "config.h"
#include "obj_common.h"

struct pcb_box_list_s {
	pcb_cardinal_t BoxN,								/* the number of boxes contained */
	  BoxMax;											/* max boxes from malloc */
	pcb_box_t *Box;
};

#include "misc_util.h"

typedef enum {
	PCB_NORTH = 0, PCB_EAST = 1, PCB_SOUTH = 2, PCB_WEST = 3,
	PCB_NE = 4, PCB_SE = 5, PCB_SW = 6, PCB_NW = 7,
	PCB_ANY_DIR = 8
} pcb_direction_t;

/* rotates box 90-degrees cw */
/* that's a strange rotation! */
#define PCB_BOX_ROTATE_CW(box) { pcb_coord_t t;\
    t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
    (box).X2 = -(box).Y1; (box).Y1 = t;\
}
#define PCB_BOX_ROTATE_TO_NORTH(box, dir) do { pcb_coord_t t;\
  switch(dir) {\
  case PCB_EAST: \
   t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
   (box).X2 = (box).Y2; (box).Y2 = -t; break;\
  case PCB_SOUTH: \
   t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
   t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
  case PCB_WEST: \
   t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
   (box).X2 = -(box).Y1; (box).Y1 = t; break;\
  case PCB_NORTH: break;\
  default: assert(0);\
  }\
  } while (0)
#define PCB_BOX_ROTATE_FROM_NORTH(box, dir) do { pcb_coord_t t;\
  switch(dir) {\
  case PCB_WEST: \
   t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
   (box).X2 = (box).Y2; (box).Y2 = -t; break;\
  case PCB_SOUTH: \
   t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
   t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
  case PCB_EAST: \
   t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
   (box).X2 = -(box).Y1; (box).Y1 = t; break;\
  case PCB_NORTH: break;\
  default: assert(0);\
  }\
  } while (0)

/* to avoid overflow, we calculate centers this way */
#define PCB_BOX_CENTER_X(b) ((b).X1 + ((b).X2 - (b).X1)/2)
#define PCB_BOX_CENTER_Y(b) ((b).Y1 + ((b).Y2 - (b).Y1)/2)

#define	PCB_MOVE_POINT(xs,ys,deltax,deltay) \
	do { \
		((xs) += (deltax)); \
		((ys) += (deltay)); \
	} while(0)

#define	PCB_BOX_MOVE_LOWLEVEL(b,dx,dy)		\
	do {									\
		PCB_MOVE_POINT((b)->X1,(b)->Y1,(dx),(dy)); \
		PCB_MOVE_POINT((b)->X2,(b)->Y2,(dx),(dy)); \
	} while(0)


typedef struct pcb_cheap_point_s {
	pcb_coord_t X, Y;
} pcb_cheap_point_t;


/* note that boxes are closed on top and left and open on bottom and right. */
/* this means that top-left corner is in box, *but bottom-right corner is
 * not*.  */
PCB_INLINE pcb_bool pcb_point_in_box(const pcb_box_t * box, pcb_coord_t X, pcb_coord_t Y) 
{
	return (X >= box->X1) && (Y >= box->Y1) && (X < box->X2) && (Y < box->Y2);
}

PCB_INLINE pcb_bool pcb_point_in_closed_box(const pcb_box_t * box, pcb_coord_t X, pcb_coord_t Y)
{
	return (X >= box->X1) && (Y >= box->Y1) && (X <= box->X2) && (Y <= box->Y2);
}

PCB_INLINE pcb_bool pcb_box_is_good(const pcb_box_t * b)
{
	return (b->X1 < b->X2) && (b->Y1 < b->Y2);
}

PCB_INLINE pcb_bool pcb_box_intersect(const pcb_box_t * a, const pcb_box_t * b)
{
	return (a->X1 < b->X2) && (b->X1 < a->X2) && (a->Y1 < b->Y2) && (b->Y1 < a->Y2);
}

PCB_INLINE pcb_cheap_point_t pcb_closest_pcb_point_in_box(const pcb_cheap_point_t * from, const pcb_box_t * box)
{
	pcb_cheap_point_t r;
	assert(box->X1 < box->X2 && box->Y1 < box->Y2);
	r.X = (from->X < box->X1) ? box->X1 : (from->X > box->X2 - 1) ? box->X2 - 1 : from->X;
	r.Y = (from->Y < box->Y1) ? box->Y1 : (from->Y > box->Y2 - 1) ? box->Y2 - 1 : from->Y;
	assert(pcb_point_in_box(box, r.X, r.Y));
	return r;
}

PCB_INLINE pcb_bool pcb_box_in_box(const pcb_box_t * outer, const pcb_box_t * inner)
{
	return (outer->X1 <= inner->X1) && (inner->X2 <= outer->X2) && (outer->Y1 <= inner->Y1) && (inner->Y2 <= outer->Y2);
}

PCB_INLINE pcb_box_t pcb_clip_box(const pcb_box_t * box, const pcb_box_t * clipbox)
{
	pcb_box_t r;
	assert(pcb_box_intersect(box, clipbox));
	r.X1 = MAX(box->X1, clipbox->X1);
	r.X2 = MIN(box->X2, clipbox->X2);
	r.Y1 = MAX(box->Y1, clipbox->Y1);
	r.Y2 = MIN(box->Y2, clipbox->Y2);
	assert(pcb_box_in_box(clipbox, &r));
	return r;
}

PCB_INLINE pcb_box_t pcb_shrink_box(const pcb_box_t * box, pcb_coord_t amount)
{
	pcb_box_t r = *box;
	r.X1 += amount;
	r.Y1 += amount;
	r.X2 -= amount;
	r.Y2 -= amount;
	return r;
}

PCB_INLINE pcb_box_t pcb_bloat_box(const pcb_box_t * box, pcb_coord_t amount)
{
	return pcb_shrink_box(box, -amount);
}

/* construct a minimum box that touches the input box at the center */
PCB_INLINE pcb_box_t pcb_box_center(const pcb_box_t * box)
{
	pcb_box_t r;
	r.X1 = box->X1 + (box->X2 - box->X1) / 2;
	r.X2 = r.X1 + 1;
	r.Y1 = box->Y1 + (box->Y2 - box->Y1) / 2;
	r.Y2 = r.Y1 + 1;
	return r;
}

/* construct a minimum box that touches the input box at the corner */
PCB_INLINE pcb_box_t pcb_box_corner(const pcb_box_t * box)
{
	pcb_box_t r;
	r.X1 = box->X1;
	r.X2 = r.X1 + 1;
	r.Y1 = box->Y1;
	r.Y2 = r.Y1 + 1;
	return r;
}

/* construct a box that holds a single point */
PCB_INLINE pcb_box_t pcb_point_box(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_box_t r;
	r.X1 = X;
	r.X2 = X + 1;
	r.Y1 = Y;
	r.Y2 = Y + 1;
	return r;
}

/* close a bounding box by pushing its upper right corner */
PCB_INLINE void pcb_close_box(pcb_box_t * r)
{
	r->X2++;
	r->Y2++;
}

/* return the square of the minimum distance from a point to some point
 * inside a box.  The box is half-closed!  That is, the top-left corner
 * is considered in the box, but the bottom-right corner is not. */
PCB_INLINE double pcb_dist2_to_box(const pcb_cheap_point_t * p, const pcb_box_t * b)
{
	pcb_cheap_point_t r = pcb_closest_pcb_point_in_box(p, b);
	return pcb_distance(r.X, r.Y, p->X, p->Y);
}


/* Modify dst to include src */
PCB_INLINE void pcb_box_bump_box(pcb_box_t *dst, const pcb_box_t *src)
{
	if (src->X1 < dst->X1) dst->X1 = src->X1;
	if (src->Y1 < dst->Y1) dst->Y1 = src->Y1;
	if (src->X2 > dst->X2) dst->X2 = src->X2;
	if (src->Y2 > dst->Y2) dst->Y2 = src->Y2;
}

/* Modify dst to include src */
PCB_INLINE void pcb_box_bump_point(pcb_box_t *dst, pcb_coord_t x, pcb_coord_t y)
{
	if (x < dst->X1) dst->X1 = x;
	if (y < dst->Y1) dst->Y1 = y;
	if (x > dst->X2) dst->X2 = x;
	if (y > dst->Y2) dst->Y2 = y;
}

/* sets the bounding box of a point */
void pcb_set_point_bounding_box(pcb_point_t *Pnt);

/* rotates a box in 90 degree steps */
void pcb_box_rotate90(pcb_box_t *Box, pcb_coord_t X, pcb_coord_t Y, unsigned Number);

/* Enlarge a box by adding current width,height multiplied by xfactor,yfactor */
void pcb_box_enlarge(pcb_box_t *box, double xfactor, double yfactor);

#endif /* __BOX_H_INCLUDED__ */

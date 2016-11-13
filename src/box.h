/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, box.h, was written and is
 *  Copyright (c) 2001 C. Scott Ananian.
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
#include "macro.h"
#include "move.h"
#include "obj_common.h"

struct pcb_box_list_s {
	pcb_cardinal_t BoxN,								/* the number of boxes contained */
	  BoxMax;											/* max boxes from malloc */
	pcb_box_t *Box;
};

#include "misc_util.h"

typedef enum {
	NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3, NE = 4, SE = 5, SW = 6, NW = 7, ALL = 8
} pcb_direction_t;

/* rotates box 90-degrees cw */
/* that's a strange rotation! */
#define PCB_BOX_ROTATE_CW(box) { pcb_coord_t t;\
    t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
    (box).X2 = -(box).Y1; (box).Y1 = t;\
}
#define PCB_BOX_ROTATE_TO_NORTH(box, dir) do { pcb_coord_t t;\
  switch(dir) {\
  case EAST: \
   t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
   (box).X2 = (box).Y2; (box).Y2 = -t; break;\
  case SOUTH: \
   t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
   t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
  case WEST: \
   t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
   (box).X2 = -(box).Y1; (box).Y1 = t; break;\
  case NORTH: break;\
  default: assert(0);\
  }\
  } while (0)
#define PCB_BOX_ROTATE_FROM_NORTH(box, dir) do { pcb_coord_t t;\
  switch(dir) {\
  case WEST: \
   t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
   (box).X2 = (box).Y2; (box).Y2 = -t; break;\
  case SOUTH: \
   t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
   t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
  case EAST: \
   t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
   (box).X2 = -(box).Y1; (box).Y1 = t; break;\
  case NORTH: break;\
  default: assert(0);\
  }\
  } while (0)

/* to avoid overflow, we calculate centers this way */
#define PCB_BOX_CENTER_X(b) ((b).X1 + ((b).X2 - (b).X1)/2)
#define PCB_BOX_CENTER_Y(b) ((b).Y1 + ((b).Y2 - (b).Y1)/2)
/* some useful box utilities. */

#define	PCB_BOX_MOVE_LOWLEVEL(b,dx,dy)		\
	{									\
		MOVE((b)->X1,(b)->Y1,(dx),(dy))	\
		MOVE((b)->X2,(b)->Y2,(dx),(dy))	\
	}


typedef struct pcb_cheap_point_s {
	pcb_coord_t X, Y;
} pcb_cheap_point_t;


/* note that boxes are closed on top and left and open on bottom and right. */
/* this means that top-left corner is in box, *but bottom-right corner is
 * not*.  */
static inline PCB_FUNC_UNUSED pcb_bool pcb_point_in_box(const pcb_box_t * box, pcb_coord_t X, pcb_coord_t Y) 
{
	return (X >= box->X1) && (Y >= box->Y1) && (X < box->X2) && (Y < box->Y2);
}

static inline PCB_FUNC_UNUSED pcb_bool pcb_point_in_closed_box(const pcb_box_t * box, pcb_coord_t X, pcb_coord_t Y)
{
	return (X >= box->X1) && (Y >= box->Y1) && (X <= box->X2) && (Y <= box->Y2);
}

static inline PCB_FUNC_UNUSED pcb_bool pcb_box_is_good(const pcb_box_t * b)
{
	return (b->X1 < b->X2) && (b->Y1 < b->Y2);
}

static inline PCB_FUNC_UNUSED pcb_bool pcb_box_intersect(const pcb_box_t * a, const pcb_box_t * b)
{
	return (a->X1 < b->X2) && (b->X1 < a->X2) && (a->Y1 < b->Y2) && (b->Y1 < a->Y2);
}

static inline PCB_FUNC_UNUSED pcb_cheap_point_t pcb_closest_pcb_point_in_box(const pcb_cheap_point_t * from, const pcb_box_t * box)
{
	pcb_cheap_point_t r;
	assert(box->X1 < box->X2 && box->Y1 < box->Y2);
	r.X = (from->X < box->X1) ? box->X1 : (from->X > box->X2 - 1) ? box->X2 - 1 : from->X;
	r.Y = (from->Y < box->Y1) ? box->Y1 : (from->Y > box->Y2 - 1) ? box->Y2 - 1 : from->Y;
	assert(pcb_point_in_box(box, r.X, r.Y));
	return r;
}

static inline PCB_FUNC_UNUSED pcb_bool pcb_box_in_box(const pcb_box_t * outer, const pcb_box_t * inner)
{
	return (outer->X1 <= inner->X1) && (inner->X2 <= outer->X2) && (outer->Y1 <= inner->Y1) && (inner->Y2 <= outer->Y2);
}

static inline PCB_FUNC_UNUSED pcb_box_t pcb_clip_box(const pcb_box_t * box, const pcb_box_t * clipbox)
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

static inline PCB_FUNC_UNUSED pcb_box_t pcb_shrink_box(const pcb_box_t * box, pcb_coord_t amount)
{
	pcb_box_t r = *box;
	r.X1 += amount;
	r.Y1 += amount;
	r.X2 -= amount;
	r.Y2 -= amount;
	return r;
}

static inline PCB_FUNC_UNUSED pcb_box_t pcb_bloat_box(const pcb_box_t * box, pcb_coord_t amount)
{
	return pcb_shrink_box(box, -amount);
}

/* construct a minimum box that touches the input box at the center */
static inline PCB_FUNC_UNUSED pcb_box_t pcb_box_center(const pcb_box_t * box)
{
	pcb_box_t r;
	r.X1 = box->X1 + (box->X2 - box->X1) / 2;
	r.X2 = r.X1 + 1;
	r.Y1 = box->Y1 + (box->Y2 - box->Y1) / 2;
	r.Y2 = r.Y1 + 1;
	return r;
}

/* construct a minimum box that touches the input box at the corner */
static inline PCB_FUNC_UNUSED pcb_box_t pcb_box_corner(const pcb_box_t * box)
{
	pcb_box_t r;
	r.X1 = box->X1;
	r.X2 = r.X1 + 1;
	r.Y1 = box->Y1;
	r.Y2 = r.Y1 + 1;
	return r;
}

/* construct a box that holds a single point */
static inline PCB_FUNC_UNUSED pcb_box_t pcb_point_box(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_box_t r;
	r.X1 = X;
	r.X2 = X + 1;
	r.Y1 = Y;
	r.Y2 = Y + 1;
	return r;
}

/* close a bounding box by pushing its upper right corner */
static inline PCB_FUNC_UNUSED void pcb_close_box(pcb_box_t * r)
{
	r->X2++;
	r->Y2++;
}

/* return the square of the minimum distance from a point to some point
 * inside a box.  The box is half-closed!  That is, the top-left corner
 * is considered in the box, but the bottom-right corner is not. */
static inline PCB_FUNC_UNUSED double pcb_dist2_to_box(const pcb_cheap_point_t * p, const pcb_box_t * b)
{
	pcb_cheap_point_t r = pcb_closest_pcb_point_in_box(p, b);
	return Distance(r.X, r.Y, p->X, p->Y);
}

pcb_box_t *GetBoxMemory(pcb_box_list_t *);
void FreeBoxListMemory(pcb_box_list_t *);
void SetPointBoundingBox(pcb_point_t *Pnt);

#endif /* __BOX_H_INCLUDED__ */

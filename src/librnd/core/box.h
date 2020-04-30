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
 *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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


#ifndef RND_BOX_H
#define RND_BOX_H

#include <assert.h>
#include <librnd/config.h>
#include <librnd/core/math_helper.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/pcb_bool.h>

struct rnd_rnd_box_list_s {
	rnd_cardinal_t BoxN;   /* the number of boxes contained */
	rnd_cardinal_t BoxMax; /* max boxes from malloc */
	rnd_rnd_box_t *Box;
};

#include <librnd/core/misc_util.h>

typedef enum {
	RND_NORTH = 0, RND_EAST = 1, RND_SOUTH = 2, RND_WEST = 3,
	RND_NE = 4, RND_SE = 5, RND_SW = 6, RND_NW = 7, RND_ANY_DIR = 8
} rnd_direction_t;

/* rotates box 90-degrees cw */
/* that's a strange rotation! */
#define RND_BOX_ROTATE_CW(box) \
do { rnd_coord_t t;\
	t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
	(box).X2 = -(box).Y1; (box).Y1 = t;\
} while(0)

#define RND_BOX_ROTATE_TO_NORTH(box, dir) \
do { rnd_coord_t t;\
	switch(dir) {\
	case RND_EAST: \
		t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
		(box).X2 = (box).Y2; (box).Y2 = -t; break;\
	case RND_SOUTH: \
		t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
		t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
	case RND_WEST: \
		t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
		(box).X2 = -(box).Y1; (box).Y1 = t; break;\
	case RND_NORTH: break;\
	default: assert(0);\
	}\
} while (0)

#define RND_BOX_ROTATE_FROM_NORTH(box, dir) \
do { rnd_coord_t t;\
	switch(dir) {\
	case RND_WEST: \
		t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
		(box).X2 = (box).Y2; (box).Y2 = -t; break;\
	case RND_SOUTH: \
		t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
		t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
	case RND_EAST: \
		t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
		(box).X2 = -(box).Y1; (box).Y1 = t; break;\
	case RND_NORTH: break;\
	default: assert(0);\
	}\
} while (0)

/* to avoid overflow, we calculate centers this way */
#define RND_BOX_CENTER_X(b) ((b).X1 + ((b).X2 - (b).X1)/2)
#define RND_BOX_CENTER_Y(b) ((b).Y1 + ((b).Y2 - (b).Y1)/2)

#define	RND_MOVE_POINT(xs,ys,deltax,deltay) \
	do { \
		((xs) += (deltax)); \
		((ys) += (deltay)); \
	} while(0)

#define	RND_BOX_MOVE_LOWLEVEL(b,dx,dy) \
	do { \
		RND_MOVE_POINT((b)->X1,(b)->Y1,(dx),(dy)); \
		RND_MOVE_POINT((b)->X2,(b)->Y2,(dx),(dy)); \
	} while(0)


typedef struct rnd_cheap_point_s {
	rnd_coord_t X, Y;
} rnd_cheap_point_t;


/* note that boxes are closed on top and left and open on bottom and right.
   this means that top-left corner is in box, *but bottom-right corner is
   not*.  */
RND_INLINE rnd_bool rnd_point_in_box(const rnd_rnd_box_t * box, rnd_coord_t X, rnd_coord_t Y)
{
	return (X >= box->X1) && (Y >= box->Y1) && (X < box->X2) && (Y < box->Y2);
}

RND_INLINE rnd_bool rnd_point_in_closed_box(const rnd_rnd_box_t * box, rnd_coord_t X, rnd_coord_t Y)
{
	return (X >= box->X1) && (Y >= box->Y1) && (X <= box->X2) && (Y <= box->Y2);
}

RND_INLINE rnd_bool rnd_box_is_good(const rnd_rnd_box_t * b)
{
	return (b->X1 < b->X2) && (b->Y1 < b->Y2);
}

RND_INLINE rnd_bool rnd_box_intersect(const rnd_rnd_box_t * a, const rnd_rnd_box_t * b)
{
	return (a->X1 < b->X2) && (b->X1 < a->X2) && (a->Y1 < b->Y2) && (b->Y1 < a->Y2);
}

RND_INLINE rnd_cheap_point_t rnd_closest_cheap_point_in_box(const rnd_cheap_point_t * from, const rnd_rnd_box_t * box)
{
	rnd_cheap_point_t r;
	assert(box->X1 < box->X2 && box->Y1 < box->Y2);
	r.X = (from->X < box->X1) ? box->X1 : (from->X > box->X2 - 1) ? box->X2 - 1 : from->X;
	r.Y = (from->Y < box->Y1) ? box->Y1 : (from->Y > box->Y2 - 1) ? box->Y2 - 1 : from->Y;
	assert(rnd_point_in_box(box, r.X, r.Y));
	return r;
}

RND_INLINE rnd_bool rnd_box_in_box(const rnd_rnd_box_t * outer, const rnd_rnd_box_t * inner)
{
	return (outer->X1 <= inner->X1) && (inner->X2 <= outer->X2) && (outer->Y1 <= inner->Y1) && (inner->Y2 <= outer->Y2);
}

RND_INLINE rnd_rnd_box_t rnd_clip_box(const rnd_rnd_box_t * box, const rnd_rnd_box_t * clipbox)
{
	rnd_rnd_box_t r;
	assert(rnd_box_intersect(box, clipbox));
	r.X1 = MAX(box->X1, clipbox->X1);
	r.X2 = MIN(box->X2, clipbox->X2);
	r.Y1 = MAX(box->Y1, clipbox->Y1);
	r.Y2 = MIN(box->Y2, clipbox->Y2);
	assert(rnd_box_in_box(clipbox, &r));
	return r;
}

RND_INLINE rnd_rnd_box_t rnd_shrink_box(const rnd_rnd_box_t * box, rnd_coord_t amount)
{
	rnd_rnd_box_t r = *box;
	r.X1 += amount;
	r.Y1 += amount;
	r.X2 -= amount;
	r.Y2 -= amount;
	return r;
}

RND_INLINE rnd_rnd_box_t rnd_bloat_box(const rnd_rnd_box_t * box, rnd_coord_t amount)
{
	return rnd_shrink_box(box, -amount);
}

/* construct a minimum box that touches the input box at the center */
RND_INLINE rnd_rnd_box_t rnd_box_center(const rnd_rnd_box_t * box)
{
	rnd_rnd_box_t r;
	r.X1 = box->X1 + (box->X2 - box->X1) / 2;
	r.X2 = r.X1 + 1;
	r.Y1 = box->Y1 + (box->Y2 - box->Y1) / 2;
	r.Y2 = r.Y1 + 1;
	return r;
}

/* construct a minimum box that touches the input box at the corner */
RND_INLINE rnd_rnd_box_t rnd_box_corner(const rnd_rnd_box_t * box)
{
	rnd_rnd_box_t r;
	r.X1 = box->X1;
	r.X2 = r.X1 + 1;
	r.Y1 = box->Y1;
	r.Y2 = r.Y1 + 1;
	return r;
}

/* construct a box that holds a single point */
RND_INLINE rnd_rnd_box_t rnd_point_box(rnd_coord_t X, rnd_coord_t Y)
{
	rnd_rnd_box_t r;
	r.X1 = X;
	r.X2 = X + 1;
	r.Y1 = Y;
	r.Y2 = Y + 1;
	return r;
}

/* close a bounding box by pushing its upper right corner */
RND_INLINE void rnd_close_box(rnd_rnd_box_t * r)
{
	r->X2++;
	r->Y2++;
}

/* return the square of the minimum distance from a point to some point
   inside a box.  The box is half-closed!  That is, the top-left corner
   is considered in the box, but the bottom-right corner is not. */
RND_INLINE double rnd_dist2_to_box(const rnd_cheap_point_t * p, const rnd_rnd_box_t * b)
{
	rnd_cheap_point_t r = rnd_closest_cheap_point_in_box(p, b);
	return rnd_distance(r.X, r.Y, p->X, p->Y);
}


/* Modify dst to include src */
RND_INLINE void rnd_box_bump_box(rnd_rnd_box_t *dst, const rnd_rnd_box_t *src)
{
	if (src->X1 < dst->X1) dst->X1 = src->X1;
	if (src->Y1 < dst->Y1) dst->Y1 = src->Y1;
	if (src->X2 > dst->X2) dst->X2 = src->X2;
	if (src->Y2 > dst->Y2) dst->Y2 = src->Y2;
}

/* Modify dst to include src */
RND_INLINE void rnd_box_bump_point(rnd_rnd_box_t *dst, rnd_coord_t x, rnd_coord_t y)
{
	if (x < dst->X1) dst->X1 = x;
	if (y < dst->Y1) dst->Y1 = y;
	if (x > dst->X2) dst->X2 = x;
	if (y > dst->Y2) dst->Y2 = y;
}

/* rotates a box in 90 degree steps */
void rnd_box_rotate90(rnd_rnd_box_t *Box, rnd_coord_t X, rnd_coord_t Y, unsigned Number);

/* Enlarge a box by adding current width,height multiplied by xfactor,yfactor */
void rnd_box_enlarge(rnd_rnd_box_t *box, double xfactor, double yfactor);

#endif

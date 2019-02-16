/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 */

typedef struct {
	pcb_any_obj_t *o1, *o2;
	pcb_coord_t o1x, o1y, o2x, o2y;
	pcb_layergrp_id_t o1g, o2g;
	double dist2;
} pcb_subnet_dist_t;

static pcb_subnet_dist_t sdist_invalid = { NULL, NULL, 0, 0, 0, 0, -1, -1, HUGE_VAL };

#define is_line_manhattan(l) (((l)->Point1.X == (l)->Point2.X) || ((l)->Point1.Y == (l)->Point2.Y))

#define dist_(o1_, o1x_, o1y_, o2_, o2x_, o2y_) \
do { \
	double __dx__, __dy__; \
	curr.o1 = (pcb_any_obj_t *)o1_; \
	curr.o1x= o1x_; \
	curr.o1y= o1y_; \
	curr.o2 = (pcb_any_obj_t *)o2_; \
	curr.o2x= o2x_; \
	curr.o2y= o2y_; \
	__dx__ = o1x_ - o2x_; \
	__dy__ = o1y_ - o2y_; \
	curr.dist2 = __dx__ * __dx__ + __dy__ * __dy__; \
} while(0)


#define dist2(o1, o1x, o1y, o2, o2x, o2y) \
do { \
	dist_(o1, o1x, o1y, o2, o2x, o2y); \
	if (curr.dist2 < best.dist2) \
		best = curr; \
} while(0)

#define dist1(o1, o1x, o1y, o2, o2x, o2y) \
do { \
	dist_(o1, o1x, o1y, o2, o2x, o2y); \
	best = curr; \
} while(0)

static pcb_subnet_dist_t pcb_dist_arc_arc(pcb_arc_t *o1, pcb_arc_t *o2, pcb_rat_accuracy_t acc)
{
	pcb_subnet_dist_t best, curr;
	pcb_coord_t o1x1, o1y1, o1x2, o1y2, o2x1, o2y1, o2x2, o2y2;

	if (acc & PCB_RATACC_ONLY_MANHATTAN)
		return sdist_invalid;

	pcb_arc_get_end(o1, 0, &o1x1, &o1y1);
	pcb_arc_get_end(o1, 1, &o1x2, &o1y2);
	pcb_arc_get_end(o2, 0, &o2x1, &o2y1);
	pcb_arc_get_end(o2, 1, &o2x2, &o2y2);

	dist1(o1, o1x1, o1y1, o2, o2x1, o2y1);
	dist2(o1, o1x1, o1y1, o2, o2x2, o2y2);
	dist2(o1, o1x2, o1y2, o2, o2x1, o2y1);
	dist2(o1, o1x2, o1y2, o2, o2x2, o2y2);

	return best;
}

static pcb_subnet_dist_t pcb_dist_line_line(pcb_line_t *o1, pcb_line_t *o2, pcb_rat_accuracy_t acc)
{
	pcb_subnet_dist_t best, curr;

	if ((acc & PCB_RATACC_ONLY_MANHATTAN) && ((!is_line_manhattan(o1) || !is_line_manhattan(o2))))
		return sdist_invalid;

	dist1(o1, o1->Point1.X, o1->Point1.Y, o2, o2->Point1.X, o2->Point1.Y);
	dist2(o1, o1->Point1.X, o1->Point1.Y, o2, o2->Point2.X, o2->Point2.Y);
	dist2(o1, o1->Point2.X, o1->Point2.Y, o2, o2->Point1.X, o2->Point1.Y);
	dist2(o1, o1->Point2.X, o1->Point2.Y, o2, o2->Point2.X, o2->Point2.Y);

	return best;
}

static pcb_subnet_dist_t pcb_dist_line_arc(pcb_line_t *o1, pcb_arc_t *o2, pcb_rat_accuracy_t acc)
{
	pcb_subnet_dist_t best, curr;
	pcb_coord_t o2x1, o2y1, o2x2, o2y2;

	if (acc & PCB_RATACC_ONLY_MANHATTAN)
		return sdist_invalid;
	if ((acc & PCB_RATACC_ONLY_MANHATTAN) && (!is_line_manhattan(o1)))
		return sdist_invalid;

	pcb_arc_get_end(o2, 0, &o2x1, &o2y1);
	pcb_arc_get_end(o2, 1, &o2x2, &o2y2);

	dist1(o1, o1->Point1.X, o1->Point1.Y, o2, o2x1, o2y1);
	dist2(o1, o1->Point1.X, o1->Point1.Y, o2, o2x2, o2y2);
	dist2(o1, o1->Point2.X, o1->Point2.Y, o2, o2x1, o2y1);
	dist2(o1, o1->Point2.X, o1->Point2.Y, o2, o2x2, o2y2);

	return best;
}

static pcb_subnet_dist_t pcb_dist_poly_arc(pcb_poly_t *o1, pcb_arc_t *o2, pcb_rat_accuracy_t acc)
{
	if (acc & PCB_RATACC_ONLY_MANHATTAN)
		return sdist_invalid;
	return sdist_invalid;
}

static pcb_subnet_dist_t pcb_dist_poly_line(pcb_poly_t *o1, pcb_line_t *o2, pcb_rat_accuracy_t acc)
{
	if (acc & PCB_RATACC_ONLY_MANHATTAN)
		return sdist_invalid;

	return sdist_invalid;
}

static pcb_subnet_dist_t pcb_dist_poly_poly(pcb_poly_t *o1, pcb_poly_t *o2, pcb_rat_accuracy_t acc)
{
	if (acc & PCB_RATACC_ONLY_MANHATTAN)
		return sdist_invalid;

	return sdist_invalid;
}


static pcb_subnet_dist_t pcb_dist_pstk_arc(pcb_pstk_t *o1, pcb_arc_t *o2, pcb_rat_accuracy_t acc)
{
	if (acc & PCB_RATACC_ONLY_MANHATTAN)
		return sdist_invalid;

	return sdist_invalid;
}

static pcb_subnet_dist_t pcb_dist_pstk_line(pcb_pstk_t *o1, pcb_line_t *o2, pcb_rat_accuracy_t acc)
{
	if ((acc & PCB_RATACC_ONLY_MANHATTAN) &&(!is_line_manhattan(o2)))
		return sdist_invalid;
	return sdist_invalid;
}

static pcb_subnet_dist_t pcb_dist_pstk_poly(pcb_pstk_t *o1, pcb_poly_t *o2, pcb_rat_accuracy_t acc)
{
	if (acc & PCB_RATACC_ONLY_MANHATTAN)
		return sdist_invalid;

	return sdist_invalid;
}

static pcb_subnet_dist_t pcb_dist_pstk_pstk(pcb_pstk_t *o1, pcb_pstk_t *o2, pcb_rat_accuracy_t acc)
{
	return sdist_invalid;
}


static pcb_subnet_dist_t pcb_obj_obj_distance(pcb_any_obj_t *o1, pcb_any_obj_t *o2, pcb_rat_accuracy_t acc)
{
	switch(o1->type) {
		case PCB_OBJ_ARC:
			switch(o2->type) {
				case PCB_OBJ_ARC:  return pcb_dist_arc_arc((pcb_arc_t *)o2, (pcb_arc_t *)o1, acc);
				case PCB_OBJ_LINE: return pcb_dist_line_arc((pcb_line_t *)o2, (pcb_arc_t *)o1, acc);
				case PCB_OBJ_POLY: return pcb_dist_poly_arc((pcb_poly_t *)o2, (pcb_arc_t *)o1, acc);
				case PCB_OBJ_PSTK: return pcb_dist_pstk_arc((pcb_pstk_t *)o2, (pcb_arc_t *)o1, acc);

				case PCB_OBJ_RAT: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
				case PCB_OBJ_VOID: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM: case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP: goto wrongtype;
			}
			break;
		case PCB_OBJ_LINE:
			switch(o2->type) {
				case PCB_OBJ_ARC:  return pcb_dist_line_arc((pcb_line_t *)o1, (pcb_arc_t *)o2, acc);
				case PCB_OBJ_LINE: return pcb_dist_line_line((pcb_line_t *)o1, (pcb_line_t *)o2, acc);
				case PCB_OBJ_POLY: return pcb_dist_poly_line((pcb_poly_t *)o2, (pcb_line_t *)o1, acc);
				case PCB_OBJ_PSTK: return pcb_dist_pstk_arc((pcb_pstk_t *)o2, (pcb_arc_t *)o1, acc);

				case PCB_OBJ_RAT: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
				case PCB_OBJ_VOID: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM: case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP: goto wrongtype;
			}
			break;
		case PCB_OBJ_POLY:
			switch(o2->type) {
				case PCB_OBJ_ARC:  return pcb_dist_poly_arc((pcb_poly_t *)o1, (pcb_arc_t *)o2, acc);
				case PCB_OBJ_LINE: return pcb_dist_poly_line((pcb_poly_t *)o1, (pcb_line_t *)o2, acc);
				case PCB_OBJ_POLY: return pcb_dist_poly_poly((pcb_poly_t *)o1, (pcb_poly_t *)o2, acc);
				case PCB_OBJ_PSTK: return pcb_dist_pstk_poly((pcb_pstk_t *)o2, (pcb_poly_t *)o1, acc);

				case PCB_OBJ_RAT: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
				case PCB_OBJ_VOID: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM: case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP: goto wrongtype;
			}
			break;

		case PCB_OBJ_PSTK:
			switch(o2->type) {
				case PCB_OBJ_ARC:  return pcb_dist_pstk_arc((pcb_pstk_t *)o1, (pcb_arc_t *)o2, acc);
				case PCB_OBJ_LINE: return pcb_dist_pstk_line((pcb_pstk_t *)o1, (pcb_line_t *)o2, acc);
				case PCB_OBJ_POLY: return pcb_dist_pstk_poly((pcb_pstk_t *)o1, (pcb_poly_t *)o2, acc);
				case PCB_OBJ_PSTK: return pcb_dist_pstk_pstk((pcb_pstk_t *)o1, (pcb_pstk_t *)o2, acc);

				case PCB_OBJ_RAT: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
				case PCB_OBJ_VOID: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM: case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP: goto wrongtype;
			}
			break;

		case PCB_OBJ_RAT: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
		case PCB_OBJ_VOID: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM: case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP: goto wrongtype;
	}

	wrongtype:;
	return sdist_invalid;
}

static pcb_subnet_dist_t pcb_subnet_dist(vtp0_t *objs1, vtp0_t *objs2, pcb_rat_accuracy_t acc)
{
	int i1, i2;
	pcb_subnet_dist_t best, curr;

	memset(&best, 0, sizeof(best));
	best.dist2 = HUGE_VAL;

	for(i1 = 0; i1 < vtp0_len(objs1); i1++) {
		for(i2 = 0; i2 < vtp0_len(objs2); i2++) {
			pcb_any_obj_t *o1 = objs1->array[i1], *o2 = objs2->array[i2];

			curr = pcb_obj_obj_distance(o1, o2, acc);

#if 0
			pcb_any_obj_t *o_in_poly = NULL, *poly
			if (curr.dist2 == 0.0) {
				/* if object needs to connect to sorrunding polygon, use a special rat
				   to indicate "in-place" connection */
				if ((o1->type == PCB_OBJ_POLY) && (o2->type != PCB_OBJ_POLY) {
					o_in_poly = o2;
					poly = o1;
				}
				else if ((o2->type == PCB_OBJ_POLY) && (o1->type != PCB_OBJ_POLY)) {
					o_in_poly = o1;
					poly = o2;
				}
				if (o_in_poly && pcb_poly_is_point_in_p_ignore_holes(cirr->o1x, curr->o1y, poly)
					goto check_if_better;
			}
#endif

			if (curr.dist2 < best.dist2)
				best = curr;
/*			check_if_better:;*/
		}
	}

TODO("Set groups");
	return best;
}

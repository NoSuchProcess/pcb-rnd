/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/*
 * This moduel, autoplace.c, was written by and is
 * Copyright (c) 2001 C. Scott Ananian
 */

/* functions used to autoplace subcircuits.
 */

#include "config.h"

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include "board.h"
#include "autoplace.h"
#include "box.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "layer.h"
#include "intersect.h"
#include "rtree.h"
#include "macro.h"
#include "move.h"
#include "netlist2.h"
#include "remove.h"
#include "rotate.h"
#include "obj_rat.h"
#include "obj_term.h"
#include "obj_pstk_inlines.h"
#include "board.h"
#include "data_it.h"
#include <genvector/vtp0.h>

#define EXPANDRECTXY(r1, x1, y1, x2, y2) { \
  r1->X1=MIN(r1->X1, x1); r1->Y1=MIN(r1->Y1, y1); \
  r1->X2=MAX(r1->X2, x2); r1->Y2=MAX(r1->Y2, y2); \
}
#define EXPANDRECT(r1, r2) EXPANDRECTXY(r1, r2->X1, r2->Y1, r2->X2, r2->Y2)

const struct {
	double via_cost;
	double congestion_penalty;		/* penalty length / unit area */
	double overlap_penalty_min;		/* penalty length / unit area at start */
	double overlap_penalty_max;		/* penalty length / unit area at end */
	double out_of_bounds_penalty;	/* assessed for each component oob */
	double overall_area_penalty;	/* penalty length / unit area */
	double matching_neighbor_bonus;	/* length bonus per same-type neigh. */
	double aligned_neighbor_bonus;	/* length bonus per aligned neigh. */
	double oriented_neighbor_bonus;	/* length bonus per same-rot neigh. */
#if 0
	double pin_alignment_bonus;		/* length bonus per exact alignment */
	double bound_alignment_bonus;	/* length bonus per exact alignment */
#endif
	double m;											/* annealing stage cutoff constant */
	double gamma;									/* annealing schedule constant */
	int good_ratio;								/* ratio of moves to good moves for halting */
	pcb_bool fast;										/* ignore SMD/pin conflicts */
	pcb_coord_t large_grid_size;				/* snap perturbations to this grid when T is high */
	pcb_coord_t small_grid_size;				/* snap to this grid when T is small. */
}
/* wire cost is manhattan distance (in mils), thus 1 inch = 1000 */ CostParameter =
{
	3e3,													/* via cost */
		2e-2,												/* congestion penalty */
		1e-2,												/* initial overlap penalty */
		1e2,												/* final overlap penalty */
		1e3,												/* out of bounds penalty */
		1e0,												/* penalty for total area used */
		1e0,												/* subtract 1000 from cost for every same-type neighbor */
		1e0,												/* subtract 1000 from cost for every aligned neighbor */
		1e0,												/* subtract 1000 from cost for every same-rotation neighbor */
		20,													/* move on when each module has been profitably moved 20 times */
		0.75,												/* annealing schedule constant: 0.85 */
		40,													/* halt when there are 60 times as many moves as good moves */
		pcb_false,											/* don't ignore SMD/pin conflicts */
		PCB_MIL_TO_COORD(100),					/* coarse grid is 100 mils */
		PCB_MIL_TO_COORD(10),						/* fine grid is 10 mils */
};

enum ewhich { SHIFT, ROTATE, EXCHANGE };

typedef struct {
	pcb_any_obj_t *comp;
	enum ewhich which;
	pcb_coord_t DX, DY;									/* for shift */
	unsigned rotate;							/* for rotate/flip */
	pcb_any_obj_t *other;					/* for exchange */
} PerturbationType;

TODO("cleanup: remove this and use genvect")
#define STEP_POINT 100

/* get next slot for a box, allocates memory if necessary */
static pcb_box_t *pcb_box_new(pcb_box_list_t *Boxes)
{
	pcb_box_t *box = Boxes->Box;

	/* realloc new memory if necessary and clear it */
	if (Boxes->BoxN >= Boxes->BoxMax) {
		Boxes->BoxMax = STEP_POINT + (2 * Boxes->BoxMax);
		box = (pcb_box_t *) realloc(box, Boxes->BoxMax * sizeof(pcb_box_t));
		Boxes->Box = box;
		memset(box + Boxes->BoxN, 0, (Boxes->BoxMax - Boxes->BoxN) * sizeof(pcb_box_t));
	}
	return (box + Boxes->BoxN++);
}

/* frees memory used by a box list */
static void pcb_box_free(pcb_box_list_t *Boxlist)
{
	if (Boxlist) {
		free(Boxlist->Box);
		memset(Boxlist, 0, sizeof(pcb_box_list_t));
	}
}

/* Return a terminal's preferred layer group ID or -1 on error */
static pcb_layergrp_id_t obj_layergrp(const pcb_any_obj_t *obj)
{
	pcb_layergrp_id_t SLayer = -1;
	pcb_layer_t *layer;

	switch (obj->type) {
		case PCB_OBJ_PSTK:
			pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &SLayer, 1);
			return SLayer;  /* any layer will do */
			break;

		/* terminals on layer */
		case PCB_OBJ_LINE:
		case PCB_OBJ_ARC:
		case PCB_OBJ_TEXT:
		case PCB_OBJ_POLY:
			layer = pcb_layer_get_real(obj->parent.layer);
			if (layer != NULL)
				return layer->meta.real.grp;
			return SLayer;  /* any layer will do */
		default:
			pcb_message(PCB_MSG_ERROR, "Odd terminal type encountered in obj_layergrp()\n");
	}
	return -1;
}

/* ---------------------------------------------------------------------------
 * Create a list of selected subcircuits
 */
static vtp0_t collectSelectedSubcircuits()
{
	vtp0_t list;

	vtp0_init(&list);
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc)) {
			pcb_subc_t **epp = (pcb_subc_t **)vtp0_alloc_append(&list, 1);
			*epp = subc;
		}
	}
	PCB_END_LOOP;
	return list;
}

#if 0														/* only for debugging box lists */
/* makes a line on the solder layer surrounding all boxes in blist */
static void showboxes(pcb_box_list_t *blist)
{
	pcb_cardinal_t i;
	pcb_layer_t *SLayer = &(PCB->Data->Layer[pcb_solder_silk_layer]);
	for (i = 0; i < blist->BoxN; i++) {
		pcb_line_new(SLayer, blist->Box[i].X1, blist->Box[i].Y1, blist->Box[i].X2, blist->Box[i].Y1, 1, 1, 0);
		pcb_line_new(SLayer, blist->Box[i].X1, blist->Box[i].Y2, blist->Box[i].X2, blist->Box[i].Y2, 1, 1, 0);
		pcb_line_new(SLayer, blist->Box[i].X1, blist->Box[i].Y1, blist->Box[i].X1, blist->Box[i].Y2, 1, 1, 0);
		pcb_line_new(SLayer, blist->Box[i].X2, blist->Box[i].Y1, blist->Box[i].X2, blist->Box[i].Y2, 1, 1, 0);
	}
}
#endif

/* ---------------------------------------------------------------------------
 * Helper function to compute "closest neighbor" for a box in a rtree.
 * The closest neighbor on a certain side is the closest one in a trapezoid
 * emanating from that side.
 */
/*------ r_find_neighbor ------*/
struct r_neighbor_info {
	const pcb_box_t *neighbor;
	pcb_box_t trap;
	pcb_direction_t search_dir;
};
#define ROTATEBOX(box) { pcb_coord_t t;\
    t = (box).X1; (box).X1 = - (box).Y1; (box).Y1 = t;\
    t = (box).X2; (box).X2 = - (box).Y2; (box).Y2 = t;\
    t = (box).X1; (box).X1 =   (box).X2; (box).X2 = t;\
}
/* helper methods for __r_find_neighbor */
static pcb_r_dir_t __r_find_neighbor_reg_in_sea(const pcb_box_t * region, void *cl)
{
	struct r_neighbor_info *ni = (struct r_neighbor_info *) cl;
	pcb_box_t query = *region;
	PCB_BOX_ROTATE_TO_NORTH(query, ni->search_dir);
	/*  ______________ __ trap.y1     __
	 *  \            /               |__| query rect.
	 *   \__________/  __ trap.y2
	 *   |          |
	 *   trap.x1    trap.x2   sides at 45-degree angle
	 */
	if ((query.Y2 > ni->trap.Y1) && (query.Y1 < ni->trap.Y2) && (query.X2 + ni->trap.Y2 > ni->trap.X1 + query.Y1) && (query.X1 + query.Y1 < ni->trap.X2 + ni->trap.Y2))
		return PCB_R_DIR_FOUND_CONTINUE;
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t __r_find_neighbor_rect_in_reg(const pcb_box_t * box, void *cl)
{
	struct r_neighbor_info *ni = (struct r_neighbor_info *) cl;
	pcb_box_t query = *box;
	int r;
	PCB_BOX_ROTATE_TO_NORTH(query, ni->search_dir);
	/*  ______________ __ trap.y1     __
	 *  \            /               |__| query rect.
	 *   \__________/  __ trap.y2
	 *   |          |
	 *   trap.x1    trap.x2   sides at 45-degree angle
	 */
	r = (query.Y2 > ni->trap.Y1) && (query.Y1 < ni->trap.Y2) &&
		(query.X2 + ni->trap.Y2 > ni->trap.X1 + query.Y1) && (query.X1 + query.Y1 < ni->trap.X2 + ni->trap.Y2);
	r = r && (query.Y2 <= ni->trap.Y2);
	if (r) {
		ni->trap.Y1 = query.Y2;
		ni->neighbor = box;
	}
	return r ? PCB_R_DIR_FOUND_CONTINUE : PCB_R_DIR_NOT_FOUND;
}

/* main r_find_neighbor routine.  Returns NULL if no neighbor in the
 * requested direction. */
static const pcb_box_t *r_find_neighbor(pcb_rtree_t * rtree, const pcb_box_t * box, pcb_direction_t search_direction)
{
	struct r_neighbor_info ni;
	pcb_box_t bbox;

	ni.neighbor = NULL;
	ni.trap = *box;
	ni.search_dir = search_direction;

	bbox.X1 = bbox.Y1 = 0;
	bbox.X2 = PCB->MaxWidth;
	bbox.Y2 = PCB->MaxHeight;
	/* rotate so that we can use the 'north' case for everything */
	PCB_BOX_ROTATE_TO_NORTH(bbox, search_direction);
	PCB_BOX_ROTATE_TO_NORTH(ni.trap, search_direction);
	/* shift Y's such that trap contains full bounds of trapezoid */
	ni.trap.Y2 = ni.trap.Y1;
	ni.trap.Y1 = bbox.Y1;
	/* do the search! */
	pcb_r_search(rtree, NULL, __r_find_neighbor_reg_in_sea, __r_find_neighbor_rect_in_reg, &ni, NULL);
	return ni.neighbor;
}

static int pstk_ispad(pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	return !PCB_PSTK_PROTO_CUTS(proto);
}

/* ---------------------------------------------------------------------------
 * Compute cost function.
 *  note that area overlap cost is correct for SMD devices: SMD devices on
 *  opposite sides of the board don't overlap.
 *
 * Algorithms follow those described in sections 4.1 of
 *  "Placement and Routing of Electronic Modules" edited by Michael Pecht
 *  Marcel Dekker, Inc. 1993.  ISBN: 0-8247-8916-4 TK7868.P7.P57 1993
 */
static double ComputeCost(double T0, double T)
{
	double W = 0;									/* wire cost */
	double delta1 = 0;						/* wire congestion penalty function */
	double delta2 = 0;						/* module overlap penalty function */
	double delta3 = 0;						/* out of bounds penalty */
	double delta4 = 0;						/* alignment bonus */
	double delta5 = 0;						/* total area penalty */
	pcb_cardinal_t i;
	pcb_coord_t minx, maxx, miny, maxy;
	pcb_bool allpads, allsameside;
	pcb_cardinal_t thegroup;
	pcb_box_list_t bounds = { 0, 0, NULL };	/* save bounding rectangles here */
	pcb_box_list_t solderside = { 0, 0, NULL };	/* solder side component bounds */
	pcb_box_list_t componentside = { 0, 0, NULL };	/* component side bounds */


	{
		htsp_entry_t *e;

		/* wire length term.  approximated by half-perimeter of minimum
		 * rectangle enclosing the net.  Note that we penalize vias in
		 * all-SMD nets by making the rectangle a cube and weighting
		 * the "layer height" of the net. */

		for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
			pcb_net_t *net = e->value;
			pcb_net_term_t *t;
			pcb_any_obj_t *obj;
			if (pcb_termlist_length(&net->conns) < 2)
				continue; /* no cost to go nowhere */

			t = pcb_termlist_first(&net->conns);
			obj = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
			pcb_obj_center(obj, &maxx, &maxy);
			minx = maxx;
			miny = maxy;
			thegroup = obj_layergrp(obj);
			allpads = pstk_ispad((pcb_pstk_t *)obj);
			allsameside = pcb_true;

			for(t = pcb_termlist_next(t); t != NULL; t = pcb_termlist_next(t)) {
				pcb_coord_t X, Y;
				obj = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
				pcb_obj_center(obj, &X, &Y);
				PCB_MAKE_MIN(minx, X);
				PCB_MAKE_MAX(maxx, X);
				PCB_MAKE_MIN(miny, Y);
				PCB_MAKE_MAX(maxy, Y);
				if (!pstk_ispad((pcb_pstk_t *)obj))
					allpads = pcb_false;
				if (obj_layergrp(obj) != thegroup)
					allsameside = pcb_false;
			}
			/* okay, add half-perimeter to cost! */
			W += PCB_COORD_TO_MIL(maxx - minx) + PCB_COORD_TO_MIL(maxy - miny) + ((allpads && !allsameside) ? CostParameter.via_cost : 0);
		}
	}

	/* now compute penalty function Wc which is proportional to
	 * amount of overlap and congestion. */
	/* delta1 is congestion penalty function */
	delta1 = CostParameter.congestion_penalty * sqrt(fabs(pcb_intersect_box_box(&bounds)));
#if 0
	printf("Wire Congestion Area: %f\n", pcb_intersect_box_box(&bounds));
#endif
	/* free bounding rectangles */
	pcb_box_free(&bounds);
	/* now collect module areas (bounding rect of pins/pads) */
	/* two lists for solder side / component side. */
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_box_list_t *thisside, *otherside;
		pcb_box_t *box, *lastbox = NULL;
		pcb_coord_t clearance;
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		int onbtm = 0;

		pcb_subc_get_side(subc, &onbtm);
TODO("subc: this ignores the possibility of other-side pads; need to do this on a per object basis")
		if (onbtm) {
			thisside = &solderside;
			otherside = &componentside;
		}
		else {
			thisside = &componentside;
			otherside = &solderside;
		}
		box = pcb_box_new(thisside);
		/* initialize box so that it will take the dimensions of the first pin/pad */
		box->X1 = box->Y1 = PCB_MAX_COORD;
		box->X2 = box->Y2 = -PCB_MAX_COORD;
		for(o = pcb_data_first(&it, subc->data, PCB_TERM_OBJ_TYPES); o != NULL; o = pcb_data_next(&it)) {
			if (o->term == NULL)
				continue; /* we are interested in terminals only */

TODO("subc: look up clearance")
			clearance = 0;
			EXPANDRECTXY(box,
				o->BoundingBox.X1 - clearance, o->BoundingBox.Y1 - clearance,
				o->BoundingBox.X2 + clearance, o->BoundingBox.Y2 + clearance);

			/* add a box for each thru-hole pin to the "opposite side":
			 * surface mount components can't sit on top of pins */
			if ((!CostParameter.fast) && (o->type == PCB_OBJ_PSTK)) {
				pcb_box_t box2;
				box2.X1 = o->BoundingBox.X1 - clearance;
				box2.Y1 = o->BoundingBox.Y1 - clearance;
				box2.X2 = o->BoundingBox.X2 + clearance;
				box2.Y2 = o->BoundingBox.Y2 + clearance;
				/* speed hack! coalesce with last box if we can */
				if (lastbox != NULL &&
						((lastbox->X1 == box2.X1 &&
							lastbox->X2 == box2.X2 &&
							MIN(labs(lastbox->Y1 - box2.Y2), labs(box2.Y1 - lastbox->Y2)) < clearance) ||
							(lastbox->Y1 == box2.Y1 && lastbox->Y2 == box2.Y2 && MIN(labs(lastbox->X1 - box2.X2), labs(box2.X1 - lastbox->X2)) < clearance))) {
					EXPANDRECT(lastbox, box);
					otherside->BoxN--;
				}
				else
					lastbox = box;
			}
		}
	}
	PCB_END_LOOP;

	/* compute intersection area of module areas box list */
	delta2 = sqrt(fabs(pcb_intersect_box_box(&solderside) + pcb_intersect_box_box(&componentside))) * (CostParameter.overlap_penalty_min + (1 - (T / T0)) * CostParameter.overlap_penalty_max);
#if 0
	printf("Module Overlap Area (solder): %f\n", pcb_intersect_box_box(&solderside));
	printf("Module Overlap Area (component): %f\n", pcb_intersect_box_box(&componentside));
#endif
	pcb_box_free(&solderside);
	pcb_box_free(&componentside);
	/* reward pin/pad x/y alignment */
	/* score higher if pins/pads belong to same *type* of component */
	/* XXX: subkey should be *distance* from thing aligned with, so that
	 * aligning to something far away isn't profitable */
	{
		/* create r tree */
		vtp0_t seboxes, ceboxes;
		struct ebox {
			pcb_box_t box;
TODO("subc: when elements are removed, turn this into pcb_subc_t * and remove the fields below")
			pcb_any_obj_t *comp;
			const char *refdes;
			int rot90;
			pcb_box_t *vbox;
		};
		pcb_direction_t dir[4] = { PCB_NORTH, PCB_EAST, PCB_SOUTH, PCB_WEST };
		struct ebox **boxpp, *boxp;
		pcb_rtree_t *rt_s, *rt_c;
		int factor;

		vtp0_init(&seboxes);
		vtp0_init(&ceboxes);

		PCB_SUBC_LOOP(PCB->Data);
		{
			int onbtm = 0;
			double rot = 0;

			pcb_subc_get_side(subc, &onbtm);
			boxpp = (struct ebox **) vtp0_alloc_append(onbtm ? &seboxes : &ceboxes, 1);
			*boxpp = (struct ebox *) malloc(sizeof(**boxpp));
			if (*boxpp == NULL) {
				fprintf(stderr, "malloc() failed in ComputeCost\n");
				exit(1);
			}

			pcb_subc_get_rotation(subc, &rot);
			(*boxpp)->box = subc->BoundingBox;
			(*boxpp)->comp = (pcb_any_obj_t *)subc;
			(*boxpp)->refdes = subc->refdes;
			(*boxpp)->rot90 = pcb_round(rot / 90.0);
			(*boxpp)->vbox = &subc->BoundingBox;
		}
		PCB_END_LOOP;

		rt_s = pcb_r_create_tree();
		pcb_r_insert_array(rt_s, (const pcb_box_t **) seboxes.array, vtp0_len(&seboxes));
		rt_c = pcb_r_create_tree();
		pcb_r_insert_array(rt_c, (const pcb_box_t **) ceboxes.array, vtp0_len(&ceboxes));
		vtp0_uninit(&seboxes);
		vtp0_uninit(&ceboxes);
		/* now, for each subcircuit, find its neighbor on all four sides */
		delta4 = 0;
		for (i = 0; i < 4; i++) {
			PCB_SUBC_LOOP(PCB->Data);
			{
				int onbtm = 0, rot90;
				double rot = 0;

				pcb_subc_get_side(subc, &onbtm);
				pcb_subc_get_rotation(subc, &rot);
				rot90 = pcb_round(rot / 90.0);

				boxp = (struct ebox *)r_find_neighbor(onbtm ? rt_s : rt_c, &subc->BoundingBox, dir[i]);
				/* score bounding box alignments */
				if (!boxp)
					continue;
				factor = 1;
				if (subc->refdes && boxp->refdes && 0 == PCB_NSTRCMP(subc->refdes, boxp->refdes)) {
					delta4 += CostParameter.matching_neighbor_bonus;
					factor++;
				}
				if (rot90 == boxp->rot90)
					delta4 += factor * CostParameter.oriented_neighbor_bonus;
				if (subc->BoundingBox.X1 == boxp->vbox->X1 ||
						subc->BoundingBox.X1 == boxp->vbox->X2 ||
						subc->BoundingBox.X2 == boxp->vbox->X1 ||
						subc->BoundingBox.X2 == boxp->vbox->X2 ||
						subc->BoundingBox.Y1 == boxp->vbox->Y1 ||
						subc->BoundingBox.Y1 == boxp->vbox->Y2 ||
						subc->BoundingBox.Y2 == boxp->vbox->Y1 ||
						subc->BoundingBox.Y2 == boxp->vbox->Y2)
					delta4 += factor * CostParameter.aligned_neighbor_bonus;
			}
			PCB_END_LOOP;
		}
		/* free k-d tree memory */
		pcb_r_free_tree_data(rt_s, free);
		pcb_r_free_tree_data(rt_c, free);
		pcb_r_destroy_tree(&rt_s);
		pcb_r_destroy_tree(&rt_c);
	}
	/* penalize total area used by this layout */
	{
		pcb_coord_t minX = PCB_MAX_COORD, minY = PCB_MAX_COORD;
		pcb_coord_t maxX = -PCB_MAX_COORD, maxY = -PCB_MAX_COORD;
		PCB_SUBC_LOOP(PCB->Data);
		{
			PCB_MAKE_MIN(minX, subc->BoundingBox.X1);
			PCB_MAKE_MIN(minY, subc->BoundingBox.Y1);
			PCB_MAKE_MAX(maxX, subc->BoundingBox.X2);
			PCB_MAKE_MAX(maxY, subc->BoundingBox.Y2);
		}
		PCB_END_LOOP;
		if (minX < maxX && minY < maxY)
			delta5 = CostParameter.overall_area_penalty * sqrt(PCB_COORD_TO_MIL(maxX - minX) * PCB_COORD_TO_MIL(maxY - minY));
	}
	if (T == 5) {
		T = W + delta1 + delta2 + delta3 - delta4 + delta5;
		printf("cost components are %.3f %.3f %.3f %.3f %.3f %.3f\n",
			W / T, delta1 / T, delta2 / T, delta3 / T, -delta4 / T, delta5 / T);
	}
	/* done! */
	return W + (delta1 + delta2 + delta3 - delta4 + delta5);
}

static pcb_bool is_smd(const pcb_any_obj_t *obj)
{
	return padstacklist_length(&(((pcb_subc_t *)obj)->data->padstack)) != 0;
}

static pcb_bool on_bottom(const pcb_any_obj_t *obj)
{
	int onbtm = 0;
	pcb_subc_get_side((pcb_subc_t *)obj, &onbtm);
	return onbtm;
}

/* ---------------------------------------------------------------------------
 * Perturb:
 *  1) flip SMD from solder side to component side or vice-versa.
 *  2) rotate component 90, 180, or 270 degrees.
 *  3) shift component random + or - amount in random direction.
 *     (magnitude of shift decreases over time)
 *  -- Only perturb selected subcircuits (need count/list of selected?) --
 */
PerturbationType createPerturbation(vtp0_t *selected, double T)
{
	PerturbationType pt = { 0 };
	/* pick subcircuit to perturb */
	pt.comp = (pcb_any_obj_t *) selected->array[pcb_rand() % vtp0_len(selected)];
	/* exchange, flip/rotate or shift? */
	switch (pcb_rand() % ((vtp0_len(selected) > 1) ? 3 : 2)) {
	case 0:
		{														/* shift! */
			pcb_coord_t grid;
			double scaleX = PCB_CLAMP(sqrt(T), PCB_MIL_TO_COORD(2.5), PCB->MaxWidth / 3);
			double scaleY = PCB_CLAMP(sqrt(T), PCB_MIL_TO_COORD(2.5), PCB->MaxHeight / 3);
			pt.which = SHIFT;
			pt.DX = scaleX * 2 * ((((double) pcb_rand()) / RAND_MAX) - 0.5);
			pt.DY = scaleY * 2 * ((((double) pcb_rand()) / RAND_MAX) - 0.5);
			/* snap to grid. different grids for "high" and "low" T */
			grid = (T > PCB_MIL_TO_COORD(10)) ? CostParameter.large_grid_size : CostParameter.small_grid_size;
			/* (round away from zero) */
			pt.DX = ((pt.DX / grid) + SGN(pt.DX)) * grid;
			pt.DY = ((pt.DY / grid) + SGN(pt.DY)) * grid;
			/* limit DX/DY so we don't fall off board */
			{
				pcb_subc_t *s = (pcb_subc_t *)pt.comp;
				pt.DX = MAX(pt.DX, -s->BoundingBox.X1);
				pt.DX = MIN(pt.DX, PCB->MaxWidth - s->BoundingBox.X2);
				pt.DY = MAX(pt.DY, -s->BoundingBox.Y1);
				pt.DY = MIN(pt.DY, PCB->MaxHeight - s->BoundingBox.Y2);
			}
			/* all done but the movin' */
			break;
		}
	case 1:
		{														/* flip/rotate! */
			/* only flip if it's an SMD component */
			pcb_bool isSMD = is_smd(pt.comp);

			pt.which = ROTATE;
			pt.rotate = isSMD ? (pcb_rand() & 3) : (1 + (pcb_rand() % 3));
			/* 0 - flip; 1-3, rotate. */
			break;
		}
	case 2:
		{														/* exchange! */
			pt.which = EXCHANGE;
			pt.other = (pcb_any_obj_t *)selected->array[pcb_rand() % (vtp0_len(selected) - 1)];
			if (pt.other == pt.comp)
				pt.other = (pcb_any_obj_t *)selected->array[vtp0_len(selected) - 1];
			/* don't allow exchanging a solderside-side SMD component
			 * with a non-SMD component. */
			if ((!is_smd(pt.comp) && on_bottom(pt.other)) ||
			    (!is_smd(pt.other) && on_bottom(pt.comp)))
				return createPerturbation(selected, T);
			break;
		}
	default:
		assert(0);
	}
	return pt;
}

void doPerturb(vtp0_t *selected, PerturbationType *pt, pcb_bool undo)
{
	pcb_box_t *bb;
	pcb_coord_t bbcx, bbcy;
	pcb_subc_t *subc = (pcb_subc_t *)pt->comp;
	/* compute center of element bounding box */

	bb = &subc->BoundingBox;
	bbcx = (bb->X1 + bb->X2) / 2;
	bbcy = (bb->Y1 + bb->Y2) / 2;
	/* do exchange, shift or flip/rotate */
	switch (pt->which) {
	case SHIFT:
		{
			pcb_coord_t DX = pt->DX, DY = pt->DY;
			if (undo) {
				DX = -DX;
				DY = -DY;
			}
			pcb_subc_move(subc, DX, DY, 1);
			return;
		}
	case ROTATE:
		{
			unsigned b = pt->rotate;
			if (undo)
				b = (4 - b) & 3;
			/* 0 - flip; 1-3, rotate. */
			if (b) {
				pcb_subc_rotate90(subc, bbcx, bbcy, b);
			}
			else {
				pcb_cardinal_t n;
				pcb_coord_t y = bb->Y1;
				pcb_subc_change_side(subc, (bb->Y1+bb->Y2)/2);
				/* mirroring moves the subc.  move it back. */
				pcb_subc_move(subc, 0, y - subc->BoundingBox.Y1, 1);
				for(n = 0; n < vtp0_len(selected); n++)
					if (selected->array[n] == pt->comp)
						selected->array[n] = subc;
				pt->comp = (pcb_any_obj_t *)subc;
			}
			return;
		}
	case EXCHANGE:
		{
			/* first exchange positions */
			pcb_coord_t x1 = bb->X1;
			pcb_coord_t y1 = bb->Y1;
			pcb_coord_t x2 = pt->other->BoundingBox.X1;
			pcb_coord_t y2 = pt->other->BoundingBox.Y1;

			pcb_subc_move(subc, x2 - x1, y2 - y1, 1);
			pcb_subc_move((pcb_subc_t *)pt->other, x1 - x2, y1 - y2, 1);

			/* then flip both subcircuits if they are on opposite sides */
			if (on_bottom(pt->comp) != on_bottom(pt->other)) {
				PerturbationType mypt;
				mypt.comp = pt->comp;
				mypt.which = ROTATE;
				mypt.rotate = 0;				/* flip */
				doPerturb(selected, &mypt, undo);
				pt->comp = mypt.comp;
				mypt.comp = pt->other;
				doPerturb(selected, &mypt, undo);
				pt->other = mypt.comp;
			}
			/* done */
			return;
		}
	default:
		assert(0);
	}
}

/* ---------------------------------------------------------------------------
 * Auto-place selected components.
 */
pcb_bool AutoPlaceSelected(void)
{
	vtp0_t Selected;
	PerturbationType pt;
	double C00, C0, T0;
	pcb_bool changed = pcb_false;

	vtp0_init(&Selected);

	Selected = collectSelectedSubcircuits();
	if (vtp0_len(&Selected) == 0) {
		pcb_message(PCB_MSG_ERROR, _("No subcircuits selected to autoplace.\n"));
		goto done;
	}

	/* simulated annealing */
	{															/* compute T0 by doing a random series of moves. */
		const int TRIALS = 10;
		const double Tx = PCB_MIL_TO_COORD(300), P = 0.95;
		double Cs = 0.0;
		int i;
		C00 = C0 = ComputeCost(Tx, Tx);
		for (i = 0; i < TRIALS; i++) {
			pt = createPerturbation(&Selected, PCB_INCH_TO_COORD(1));
			doPerturb(&Selected, &pt, pcb_false);
			Cs += fabs(ComputeCost(Tx, Tx) - C0);
			doPerturb(&Selected, &pt, pcb_true);
		}
		T0 = -(Cs / TRIALS) / log(P);
		printf("Initial T: %f\n", T0);
	}
	/* now anneal in earnest */
	{
		double T = T0;
		long steps = 0;
		int good_moves = 0, moves = 0;
		const int good_move_cutoff = CostParameter.m * vtp0_len(&Selected);
		const int move_cutoff = 2 * good_move_cutoff;
		printf("Starting cost is %.0f\n", ComputeCost(T0, 5));
		C0 = ComputeCost(T0, T);
		while (1) {
			double Cprime;
			pt = createPerturbation(&Selected, T);
			doPerturb(&Selected, &pt, pcb_false);
			Cprime = ComputeCost(T0, T);
			if (Cprime < C0) {				/* good move! */
				C0 = Cprime;
				good_moves++;
				steps++;
			}
			else if ((pcb_rand() / (double) RAND_MAX) < exp(MIN(MAX(-20, (C0 - Cprime) / T), 20))) {
				/* not good but keep it anyway */
				C0 = Cprime;
				steps++;
			}
			else
				doPerturb(&Selected, &pt, pcb_true);		/* undo last change */
			moves++;
			/* are we at the end of a stage? */
			if (good_moves >= good_move_cutoff || moves >= move_cutoff) {
				printf("END OF STAGE: COST %.0f\t" "GOOD_MOVES %d\tMOVES %d\t" "T: %.1f\n", C0, good_moves, moves, T);
				pcb_draw();
				if (pcb_hid_progress(C00-T, C00, "Optimizing the placement..."))
					break;
				/* is this the end? */
				if (T < 5 || good_moves < moves / CostParameter.good_ratio)
					break;
				/* nope, adjust T and continue */
				moves = good_moves = 0;
				T *= CostParameter.gamma;
				/* cost is T dependent, so recompute */
				C0 = ComputeCost(T0, T);
			}
		}
		changed = (steps > 0);
	}
done:
	pcb_hid_progress(0, 0, NULL);
	if (changed) {
		pcb_rats_destroy(pcb_false);
		pcb_net_add_all_rats(PCB, PCB_RATACC_PRECISE);
		pcb_redraw();
	}
	vtp0_uninit(&Selected);
	return changed;
}

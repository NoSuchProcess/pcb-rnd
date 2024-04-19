/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design - Rubber Band Stretch Router
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust in 2024)
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

#include <math.h>
#include <genht/hash.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/math_helper.h>
#include <librnd/hid/hid_inlines.h>
#include <libgrbs/route.h>
#include <libgrbs/debug.h>
#include <libgrbs/geo.h>
#include "obj_pstk_inlines.h"
#include "layer_ui.h"
#include "draw.h"
#include "draw_wireframe.h"

static const char rbs_routing_map_cookie[] = "rbs_routing map.c";

RND_INLINE void make_point_from_pstk_shape(rbsr_map_t *rbs, pcb_pstk_t *ps, pcb_pstk_shape_t *shp, pcb_layer_t *ly)
{
	pcb_board_t *pcb = rbs->pcb;
	rnd_coord_t clr = obj_pstk_get_clearance(pcb, ps, ly);
	rnd_coord_t copdia, x, y, xc, yc;
	grbs_line_t *line;
	grbs_point_t *pt, *pt2, *prevpt = NULL, *firstpt = NULL;
	grbs_2net_t *tn;
	unsigned int tries = 0, n;


	retry:;
	switch(shp->shape) {
		case PCB_PSSH_LINE:
			copdia = shp->data.line.thickness;
			TODO("rather do a 4-corner solution here");
			if (shp->data.line.square)
				copdia *= sqrt(2);
			x = ps->x + shp->data.line.x1;
			y = ps->y + shp->data.line.y1;
			pt = grbs_point_new(&rbs->grbs, RBSR_R2G(x), RBSR_R2G(y), RBSR_R2G(copdia/2.0), RBSR_R2G(clr));
			x = ps->x + shp->data.line.x2;
			y = ps->y + shp->data.line.y2;
			pt2 = grbs_point_new(&rbs->grbs, RBSR_R2G(x), RBSR_R2G(y), RBSR_R2G(copdia/2.0), RBSR_R2G(clr));
			tn = grbs_2net_new(&rbs->grbs, copdia, clr);
			line = grbs_line_realize(&rbs->grbs, tn, pt, pt2);
			line->immutable = 1;

			/* add the center point for incident lines */
			xc = ps->x + rnd_round((shp->data.line.x1 + shp->data.line.x2)/2);
			yc = ps->y + rnd_round((shp->data.line.y1 + shp->data.line.y2)/2);
			pt = grbs_point_new(&rbs->grbs, RBSR_R2G(xc), RBSR_R2G(yc), RBSR_R2G(copdia/2.0), RBSR_R2G(clr));
			htpp_set(&rbs->term4incident, ps, pt);

			break;

		case PCB_PSSH_CIRC:
			copdia = shp->data.circ.dia;
			x = ps->x + shp->data.circ.x;
			y = ps->y + shp->data.circ.y;
			pt = grbs_point_new(&rbs->grbs, RBSR_R2G(x), RBSR_R2G(y), RBSR_R2G(copdia/2.0), RBSR_R2G(clr));
			htpp_set(&rbs->term4incident, ps, pt);
			break;
			
		case PCB_PSSH_HSHADOW:
			copdia = 0;
			shp = pcb_pstk_shape_mech_at(pcb, ps, ly);
			tries++;
			if (tries < 2)
				goto retry;

			rnd_message(RND_MSG_ERROR, "rbs_routing: failed to figure padstack shape around %mm;%mm in make_point_from_pstk_shape(): infinite recursion\n", ps->x, ps->y);
			break;

		case PCB_PSSH_POLY:
			copdia = 0;
			xc = yc = 0;
			for(n = 0; n < shp->data.poly.len; n++) {
				xc += shp->data.poly.x[n];
				yc += shp->data.poly.y[n];
				x = ps->x + shp->data.poly.x[n];
				y = ps->y + shp->data.poly.y[n];
				pt = grbs_point_new(&rbs->grbs, RBSR_R2G(x), RBSR_R2G(y), 0, RBSR_R2G(clr));
				if (firstpt == NULL)
					firstpt = pt;
				if (prevpt != NULL) {
					tn = grbs_2net_new(&rbs->grbs, copdia, clr);
					line = grbs_line_realize(&rbs->grbs, tn, prevpt, pt);
					line->immutable = 1;
				}
				prevpt = pt;
			}
			if ((firstpt != NULL) && (firstpt != pt)) {
				tn = grbs_2net_new(&rbs->grbs, copdia, clr);
				line = grbs_line_realize(&rbs->grbs, tn, pt, firstpt);
				line->immutable = 1;
			}

			/* add the center point for incident lines */
			xc = ps->x + rnd_round((double)xc/(double)shp->data.poly.len);
			yc = ps->y + rnd_round((double)yc/(double)shp->data.poly.len);
			pt = grbs_point_new(&rbs->grbs, RBSR_R2G(xc), RBSR_R2G(yc), RBSR_R2G(copdia/2.0), RBSR_R2G(clr));
			htpp_set(&rbs->term4incident, ps, pt);

			break;
	}
}

RND_INLINE int map_pstks(rbsr_map_t *rbs)
{
	pcb_board_t *pcb = rbs->pcb;
	pcb_layer_t *ly = pcb_get_layer(pcb->Data, rbs->lid);
	pcb_pstk_t *ps;

	assert(ly != NULL);

	/* add all padstacks as points */
	for(ps = padstacklist_first(&pcb->Data->padstack); ps != NULL; ps = ps->link.next) {
		pcb_pstk_shape_t *shp;
		shp = pcb_pstk_shape_at(pcb, ps, ly); /* prefer copper shape */
		if (shp == NULL)
			shp = pcb_pstk_shape_mech_at(pcb, ps, ly);
		if (shp != NULL)
			make_point_from_pstk_shape(rbs, ps, shp, ly);
	}
	

	TODO("also add all non-padstack terminals");
	return 0;
}

/* When a new currarc is created, this function looks back if there is a
   pending prevline from prevarc and finalizes it. Also updates prevarc. */
static int bind_prev_line(rbsr_map_t *rbs, grbs_arc_t **prevarc, grbs_line_t **prevline, grbs_arc_t *currarc)
{
	if (*prevline != NULL) {
		assert(*prevarc != NULL); assert(currarc != NULL);

		grbs_line_attach(&rbs->grbs, *prevline, *prevarc, 1);
		grbs_line_attach(&rbs->grbs, *prevline, currarc, 2);
		grbs_line_bbox(*prevline);
		grbs_line_reg(&rbs->grbs, *prevline);
	}
	*prevarc = currarc;
	return 0;
}


static int map_2nets_incident(rbsr_map_t *rbs, grbs_2net_t *tn, pcb_2netmap_obj_t *obj, grbs_arc_t **prevarc, grbs_line_t **prevline)
{
	grbs_arc_t *a;
	grbs_point_t *pt;

	if (obj->orig == NULL)
		return -2;

	pt = htpp_get(&rbs->term4incident, obj->orig);
	if (pt == NULL)
		return -4;

	a = grbs_arc_new(&rbs->grbs, pt, 0, 0, 0, 0);
	gdl_append(&tn->arcs, a, link_2net);
	a->user_data = obj;
	a->in_use = 1;

	bind_prev_line(rbs, prevarc, prevline, a);

	return 0;
}

/* How far the target point could be, +- manhattan distance */
#define FIND_PT_DELTA   2
#define FIND_PT_DELTA2  RBSR_R2G(FIND_PT_DELTA * FIND_PT_DELTA)

RND_INLINE grbs_point_t *rbsr_find_point_(rbsr_map_t *rbs, rnd_coord_t cx_, rnd_coord_t cy_, double bestd2, double delta)
{
	double cx = RBSR_R2G(cx_), cy = RBSR_R2G(cy_);
	grbs_point_t *pt, *best = NULL;
	grbs_rtree_it_t it;
	grbs_rtree_box_t bbox;

	bbox.x1 = cx - delta;
	bbox.y1 = cy - delta;
	bbox.x2 = cx + delta;
	bbox.y2 = cy + delta;

	for(pt = grbs_rtree_first(&it, &rbs->grbs.point_tree, &bbox); pt != NULL; pt = grbs_rtree_next(&it)) {
		double dx = cx - pt->x, dy = cy - pt->y, d2 = dx*dx + dy*dy;
		if (d2 < bestd2) {
			bestd2 = d2;
			best = pt;
		}
	}

	return best;
}

grbs_point_t *rbsr_find_point_by_center(rbsr_map_t *rbs, rnd_coord_t cx, rnd_coord_t cy)
{
	return rbsr_find_point_(rbs, cx, cy, FIND_PT_DELTA2+1, FIND_PT_DELTA);
}

grbs_point_t *rbsr_find_point(rbsr_map_t *rbs, rnd_coord_t cx, rnd_coord_t cy)
{
	return rbsr_find_point_(rbs, cx, cy, RND_COORD_MAX, FIND_PT_DELTA);
}

grbs_point_t *rbsr_find_point_thick(rbsr_map_t *rbs, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t delta)
{
	return rbsr_find_point_(rbs, cx, cy, RND_COORD_MAX, delta);
}

static int map_2nets_intermediate(rbsr_map_t *rbs, grbs_2net_t *tn, pcb_2netmap_obj_t *prev, pcb_2netmap_obj_t *obj, grbs_arc_t **prevarc, grbs_line_t **prevline)
{
	grbs_arc_t *a;
	grbs_point_t *pt;
	pcb_arc_t *arc;
	double r, da, sa;

	switch(obj->o.any.type) {
		case PCB_OBJ_LINE:
			if ((prev != NULL) && (prev->o.any.type == PCB_OBJ_LINE))
				return -8;
			*prevline = grbs_line_new(&rbs->grbs);
			(*prevline)->user_data = obj;
			if (obj->orig != NULL)
				htpp_set(&rbs->robj2grbs, obj->orig, *prevline);
			break;
		case PCB_OBJ_ARC:
			if ((prev != NULL) && (prev->o.any.type == PCB_OBJ_ARC))
				return -16;

			arc = (pcb_arc_t *)obj->orig;
			if (arc == NULL)
				return -32;

			pt = rbsr_find_point_by_center(rbs, arc->X, arc->Y);
			if (pt == NULL)
				return -64;

			sa = -(arc->StartAngle - 180.0) / RND_RAD_TO_DEG;
			da = -(arc->Delta / RND_RAD_TO_DEG);
			r = RBSR_R2G(arc->Height);

			/* the pcb object's arc may not be oriented to suit the sequential
			   order; this is detected by prev line's endpoint matching not the
			   starting endpoint (sa) of the arc but the ending endpoint (sa+da);
			   in this case swap arc orientation */
			if (*prevline != NULL) {
				double e2x, e2y, l1x, l1y, l2x, l2y, ea;
				pcb_2netmap_obj_t *lineo = (*prevline)->user_data;
				pcb_line_t *line = (pcb_line_t *)lineo->orig;

				assert(line->type == PCB_OBJ_LINE);

				ea = sa+da;
				l1x = RBSR_R2G(line->Point1.X);
				l1y = RBSR_R2G(line->Point1.Y);
				l2x = RBSR_R2G(line->Point2.X);
				l2y = RBSR_R2G(line->Point2.Y);
				e2x = pt->x + cos(ea) * r;
				e2y = pt->y + sin(ea) * r;

				/* if either endpoint of the line is too close to the sa+da end of
				   the arc we need to swap angles */
				if ((crdeq(l1x, e2x) && crdeq(l1y, e2y)) || (crdeq(l2x, e2x) && crdeq(l2y, e2y))) {
					sa = ea;
					da = -da;
				}
			}

			a = grbs_arc_new(&rbs->grbs, pt, 0, r, sa, da);
			assert(a != NULL);
			gdl_append(&tn->arcs, a, link_2net);
			a->user_data = obj;
			a->in_use = 1;
			grbs_arc_bbox(a);
			grbs_arc_reg(&rbs->grbs, a);

			htpp_set(&rbs->robj2grbs, arc, a);
			bind_prev_line(rbs, prevarc, prevline, a);
			break;
		default:
			return -128;
	}
	return 0;
}

/* finalize a two-net */
RND_INLINE int tn_postproc(rbsr_map_t *rbs, grbs_2net_t *tn)
{
	grbs_arc_t *last, *first;

	first = gdl_first(&tn->arcs);
	if (first == NULL)
		return -256;
	grbs_inc_ang_update(&rbs->grbs, first);

	last = gdl_last(&tn->arcs);
	if ((first != last) && (last->r == 0))
		grbs_inc_ang_update(&rbs->grbs, last);

	return 0;
}

static int cmp_arc_ang(const void *A, const void *B)
{
	const grbs_arc_t * const *a = A;
	const grbs_arc_t * const *b = B;

	return (fabs((*a)->da) > fabs((*b)->da)) ? -1 : +1;
}

/* all arcs are in seg[0]; sort them into segments and insert sentinels */
RND_INLINE int map_2nets_postproc_point(rbsr_map_t *rbs, grbs_point_t *pt, vtp0_t *tmp)
{
	grbs_arc_t *a, *snt;
	long n;
	int segid, res = 0;

	tmp->used = 0;

	while((a = gdl_first(&pt->arcs[0])) != NULL) {
		gdl_remove(&pt->arcs[0], a, link_point);
		vtp0_append(tmp, a);
	}

	qsort(tmp->array, tmp->used, sizeof(void *), cmp_arc_ang);

	for(n = 0; n < tmp->used; n++) {
		a = tmp->array[n];
		segid = grbs_get_seg_idx(&rbs->grbs, pt, a->sa + a->da/2.0, 0);
		if (segid < 0) segid = grbs_get_seg_idx(&rbs->grbs, pt, a->sa, 0);
		if (segid < 0) segid = grbs_get_seg_idx(&rbs->grbs, pt, a->sa + a->da, 0);
		if (segid < 0) {
			/* need to create a new seg; we are inserting the largest arc first so
			   it is safe to create the sentinel */

			snt = grbs_new_sentinel(&rbs->grbs, pt, a->sa, a->da, &segid);
			if (snt == NULL) {
				rnd_message(RND_MSG_ERROR, "map_2nets_postproc_point(): failed to create sentinel, probably ran out of segments\n");
				res = 1;
				continue;
			}

			/* normalize sentinel angles */
			if (snt->da < 0) { /* swap endpoints so da is always positive */
				snt->sa = snt->sa + snt->da;
				snt->da = -snt->da;
			}
			if (snt->sa < 0)
				snt->sa += 2.0*GRBS_PI;
			else if (snt->sa > 2.0*GRBS_PI)
				snt->sa -= 2.0*GRBS_PI;
		}
		gdl_append(&pt->arcs[segid], a, link_point);
	}

	return res;
}

RND_INLINE int map_2nets_postproc_points(rbsr_map_t *rbs)
{
	grbs_point_t *pt;
	grbs_rtree_it_t it;
	vtp0_t tmp = {0};
	int res = 0;

	for(pt = grbs_rtree_all_first(&it, &rbs->grbs.point_tree); pt != NULL; pt = grbs_rtree_all_next(&it))
		res |= map_2nets_postproc_point(rbs, pt, &tmp);

	vtp0_uninit(&tmp);
	return res;
}

RND_INLINE int map_2nets(rbsr_map_t *rbs)
{
	pcb_2netmap_oseg_t *seg;
	int res = 0;

	for(seg = rbs->twonets.osegs; seg != NULL; seg = seg->next) {
		long n;
		grbs_2net_t *tn;
		grbs_arc_t *prevarc = NULL;
		grbs_line_t *prevline = NULL;
		double rcop, rclr, copper, clearance;

		if (seg->objs.used <= 1)
			continue;

		rnd_trace("net %p\n", seg->net);

		/* figure copper and clearance size for this two-net */
		copper = clearance = 0;
		for(n = 1; n < seg->objs.used-1; n++) {
			pcb_2netmap_obj_t *obj = seg->objs.array[n];
			if ((obj->o.any.type == PCB_OBJ_LINE) && (obj->orig != NULL)) {
				pcb_line_t *line = (pcb_line_t *)obj->orig;
				rcop = RBSR_R2G(line->Thickness/2);
				rclr = RBSR_R2G(line->Clearance/2);
				if (copper == 0) {
					copper = rcop;
					clearance = rclr;
				}
				else {
					if ((rcop != copper) || (rclr != clearance))
						rnd_message(RND_MSG_ERROR, "rbs_routing: two-net with variable thickness or clearance\n");
				}
			}
			else if ((obj->o.any.type == PCB_OBJ_ARC) && (obj->orig != NULL)) {
				pcb_arc_t *arc = (pcb_arc_t *)obj->orig;
				rcop = RBSR_R2G(arc->Thickness/2);
				rclr = RBSR_R2G(arc->Clearance/2);
				if (copper == 0) {
					copper = rcop;
					clearance = rclr;
				}
				else {
					if ((rcop != copper) || (rclr != clearance))
						rnd_message(RND_MSG_ERROR, "rbs_routing: two-net with variable thickness or clearance\n");
				}
			}
		}

		/* map arcs and lines into a sequence */
		tn = grbs_2net_new(&rbs->grbs, copper, clearance);
		res |= map_2nets_incident(rbs, tn, seg->objs.array[0], &prevarc, &prevline);
		for(n = 1; n < seg->objs.used-1; n++) {
			pcb_2netmap_obj_t *obj = seg->objs.array[n];
			char typec = '?';
			if (obj->o.any.type == PCB_OBJ_LINE) typec = 'L';
			else if (obj->o.any.type == PCB_OBJ_ARC) typec = 'A';
			rnd_trace(" obj=%c:%p orig=%p %ld\n", typec, obj, obj->orig, obj->orig == NULL ? 0 : obj->orig->ID);
			res |= map_2nets_intermediate(rbs, tn, seg->objs.array[n-1], seg->objs.array[n], &prevarc, &prevline);
		}
		res |= map_2nets_incident(rbs, tn, seg->objs.array[seg->objs.used-1], &prevarc, &prevline);
		res |= tn_postproc(rbs, tn);

		rnd_trace(" res=%d\n", res);
	}

	res |= map_2nets_postproc_points(rbs);

	return res;
}

grbs_rtree_dir_t draw_point(void *cl, void *obj, const grbs_rtree_box_t *box)
{
	grbs_point_t *pt = obj;
/*	pcb_draw_info_t *info = cl;*/
	rnd_coord_t x = RBSR_G2R(pt->x), y = RBSR_G2R(pt->y);

	rnd_hid_set_line_width(pcb_draw_out.fgGC, RBSR_G2R(pt->copper*2.0));
	rnd_render->draw_line(pcb_draw_out.fgGC, x, y, x, y);

	rnd_hid_set_line_width(pcb_draw_out.fgGC, 1);
	pcb_draw_wireframe_line(pcb_draw_out.fgGC, x, y, x, y, RBSR_G2R(pt->copper*2.0+pt->clearance*2.0), 0);

	return rnd_RTREE_DIR_FOUND_CONT;
}

grbs_rtree_dir_t draw_line(void *cl, void *obj, const grbs_rtree_box_t *box)
{
	grbs_line_t *line = obj;
/*	pcb_draw_info_t *info = cl;*/
	rnd_coord_t x1 = RBSR_G2R(line->x1), y1 = RBSR_G2R(line->y1);
	rnd_coord_t x2 = RBSR_G2R(line->x2), y2 = RBSR_G2R(line->y2);
	grbs_2net_t *tn = NULL;
	double copper = 1, clearance = 1;

	if (line->a1 != NULL) tn = grbs_arc_parent_2net(line->a1);
	if (line->a2 != NULL) tn = grbs_arc_parent_2net(line->a2);
	if (tn != NULL) {
		copper = tn->copper;
		clearance = tn->clearance;
	}

	if (!line->RBSR_WIREFRAME_FLAG) {
		rnd_hid_set_line_width(pcb_draw_out.fgGC, RBSR_G2R(copper*2));
		rnd_render->draw_line(pcb_draw_out.fgGC, x1, y1, x2, y2);
	}

	rnd_hid_set_line_width(pcb_draw_out.fgGC, 1);

	if (line->RBSR_WIREFRAME_FLAG)
		pcb_draw_wireframe_line(pcb_draw_out.fgGC, x1, y1, x2, y2, RBSR_G2R(copper*2), -1);
	else
		pcb_draw_wireframe_line(pcb_draw_out.fgGC, x1, y1, x2, y2, RBSR_G2R(copper*2+clearance*2), -1);

	return rnd_RTREE_DIR_FOUND_CONT;
}

grbs_rtree_dir_t draw_arc(void *cl, void *obj, const grbs_rtree_box_t *box)
{
	grbs_arc_t *arc = obj;
	pcb_arc_t tmparc;
/*	pcb_draw_info_t *info = cl;*/
	rnd_coord_t cx = RBSR_G2R(arc->parent_pt->x), cy = RBSR_G2R(arc->parent_pt->y);
	rnd_coord_t r = RBSR_G2R(arc->r);
	grbs_2net_t *tn = grbs_arc_parent_2net(arc);
	double copper = 1, clearance = 1, sa, da;

	if (tn != NULL) {
		copper = tn->copper;
		clearance = tn->clearance;
	}

	sa = 180.0 - (arc->sa * RND_RAD_TO_DEG);
	da = - (arc->da * RND_RAD_TO_DEG);

	if (!arc->RBSR_WIREFRAME_FLAG) {
		rnd_hid_set_line_width(pcb_draw_out.fgGC, RBSR_G2R(copper*2));
		rnd_render->draw_arc(pcb_draw_out.fgGC, cx, cy, r, r, sa, da);
	}

	rnd_hid_set_line_width(pcb_draw_out.fgGC, 1);

	tmparc.X = cx; tmparc.Y = cy;
	tmparc.Width = tmparc.Height = r;
	tmparc.StartAngle = sa; tmparc.Delta = da;

	if (arc->RBSR_WIREFRAME_FLAG)
		pcb_draw_wireframe_arc_(pcb_draw_out.fgGC, &tmparc, RBSR_G2R(copper*2), 0);
	else
		pcb_draw_wireframe_arc_(pcb_draw_out.fgGC, &tmparc, RBSR_G2R(copper*2+clearance*2), 0);

	return rnd_RTREE_DIR_FOUND_CONT;
}

static void rbsr_plugin_draw(pcb_draw_info_t *info, const pcb_layer_t *Layer)
{
	rbsr_map_t *rbs = Layer->plugin_draw_data;
	grbs_rtree_box_t gbox;

	gbox.x1 = RBSR_R2G(info->drawn_area->X1);
	gbox.y1 = RBSR_R2G(info->drawn_area->Y1);
	gbox.x2 = RBSR_R2G(info->drawn_area->X2);
	gbox.y2 = RBSR_R2G(info->drawn_area->Y2);

	rnd_render->set_color(pcb_draw_out.fgGC, &Layer->meta.real.color);

	grbs_rtree_search_any(&rbs->grbs.point_tree, &gbox, NULL, draw_point, info, NULL);
	grbs_rtree_search_any(&rbs->grbs.line_tree, &gbox, NULL, draw_line, info, NULL);
	grbs_rtree_search_any(&rbs->grbs.arc_tree, &gbox, NULL, draw_arc, info, NULL);
}

static void setup_ui_layer(rbsr_map_t *rbs)
{
	pcb_layer_t *ly = pcb_get_layer(rbs->pcb->Data, rbs->lid);
	rbs->ui_layer = pcb_uilayer_alloc(rbs->pcb, rbs_routing_map_cookie, "rbs_routing", &ly->meta.real.color);
	rbs->ui_layer->plugin_draw = rbsr_plugin_draw;
	rbs->ui_layer->plugin_draw_data = rbs;
}


int rbsr_map_pcb(rbsr_map_t *dst, pcb_board_t *pcb, rnd_layer_id_t lid)
{
	int res;

	dst->twonets.find_floating = 1;
	if (pcb_map_2nets_init(&dst->twonets, pcb) != 0) {
		rnd_msg_error("rbs_routing: failed to map 2-nets\n");
		return -1;
	}

	if (dst->twonets.junc_at != NULL) {
		rnd_msg_error("rbs_routing: failed to map 2-nets: there's a junction at object #%ld\n", dst->twonets.junc_at->ID);
		pcb_map_2nets_uninit(&dst->twonets);
		return -1;
	}

	htpp_init(&dst->term4incident, ptrhash, ptrkeyeq);
	htpp_init(&dst->robj2grbs, ptrhash, ptrkeyeq);
	grbs_init(&dst->grbs);
	dst->pcb = pcb;
	dst->lid = lid;

	res = map_pstks(dst);
	res |= map_2nets(dst);

	setup_ui_layer(dst);

	return res;
}


void rbsr_map_uninit(rbsr_map_t *dst)
{
	htpp_uninit(&dst->term4incident);
	htpp_uninit(&dst->robj2grbs);
	pcb_map_2nets_uninit(&dst->twonets);
	dst->pcb = NULL;
	dst->lid = -1;
	TODO("free the locally allocated map");
}


void grbs_draw_wires(grbs_t *grbs, FILE *f);
void grbs_dump_wires(grbs_t *grbs, FILE *f);
void grbs_draw_2net(grbs_t *grbs, FILE *f, grbs_2net_t *tn);
void grbs_dump_2net(grbs_t *grbs, FILE *f, grbs_2net_t *tn);

void rbsr_map_debug_draw(rbsr_map_t *rbs, const char *fn)
{
	FILE *f = rnd_fopen(&rbs->pcb->hidlib, fn, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open debug output '%s' for write\n", fn);
		return;
	}

	grbs_draw_zoom = 0.001;

	grbs_draw_begin(&rbs->grbs, f);
	grbs_draw_points(&rbs->grbs, f);
	grbs_draw_wires(&rbs->grbs, f);
	grbs_draw_end(&rbs->grbs, f);

	grbs_draw_zoom = 1;

	fclose(f);
}

void rbsr_map_debug_dump(rbsr_map_t *rbs, const char *fn)
{
	FILE *f = rnd_fopen(&rbs->pcb->hidlib, fn, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open debug output '%s' for write\n", fn);
		return;
	}

	grbs_dump_points(&rbs->grbs, f);
	grbs_dump_wires(&rbs->grbs, f);

	fclose(f);
}


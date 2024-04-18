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

#include <libgrbs/route.h>
#include "install.h"

int rbsr_seq_begin_at(rbsr_seq_t *rbsq, pcb_board_t *pcb, rnd_layer_id_t lid, rnd_coord_t tx, rnd_coord_t ty, rnd_coord_t copper, rnd_coord_t clearance)
{
	grbs_point_t *start;

	rbsr_map_pcb(&rbsq->map, pcb, lid);
	rbsr_map_debug_draw(&rbsq->map, "rbsq1.svg");
	rbsr_map_debug_dump(&rbsq->map, "rbsq1.dump");

	rbsq->map.grbs.user_data = rbsq;
#if 0
	rbsq->map.grbs.coll_ingore_tn_line = coll_ingore_tn_line;
	rbsq->map.grbs.coll_ingore_tn_arc = coll_ingore_tn_arc;
	rbsq->map.grbs.coll_ingore_tn_point = coll_ingore_tn_point;
#endif

	start = rbsr_find_point_by_center(&rbsq->map, tx, ty);
	if (start == NULL) {
		rnd_message(RND_MSG_ERROR, "No suitable starting point\n");
		rbsr_map_uninit(&rbsq->map);
		return -1;
	}

	rbsq->tn = grbs_2net_new(&rbsq->map.grbs, RBSR_R2G(copper), RBSR_R2G(clearance));

	rbsq->snap = grbs_snapshot_save(&rbsq->map.grbs);

	rbsq->last_x = RBSR_G2R(start->x);
	rbsq->last_y = RBSR_G2R(start->y);

	rbsq->path[0].dir = GRBS_ADIR_INC;
	rbsq->path[0].pt = start;
	rbsq->used = 1;

	return 0;
}

RND_INLINE int rbsr_seq_redraw(rbsr_seq_t *rbsq)
{
	grbs_t *grbs = &rbsq->map.grbs;
	grbs_addr_t *last, *curr = NULL, *cons = NULL;
	int n, broken = 0, res = 0;

	grbs_path_remove_2net_addrs(grbs, rbsq->tn);
	grbs_snapshot_restore(rbsq->snap);

	rnd_trace("-- route path\n");
	last = grbs_addr_new(grbs, ADDR_POINT, rbsq->path[0].pt);
	last->last_real = NULL;
	rnd_trace(" strt=%p\n", last);

	for(n = 1; n < rbsq->used; n++) {
		curr = grbs_path_next(grbs, rbsq->tn, last, rbsq->path[n].pt, rbsq->path[n].dir);
		rnd_trace(" curr=%p\n", curr);
		if (curr == NULL) {
			curr = last;
			broken = 1;
			break;
		}
		last = curr;
	}

	if (!broken && (rbsq->consider.dir != RBS_ADIR_invalid)) {
		cons = grbs_path_next(grbs, rbsq->tn, last, rbsq->consider.pt, rbsq->consider.dir);
		if (cons != NULL)
			curr = cons;
		else
			res = -1;

		rnd_trace(" cons=%p\n", cons);
	}

	if (curr != NULL) {
		double ex, ey;

		if ((curr->type & 0x0F) == ADDR_POINT) {
			ex = curr->obj.pt->x;
			ey = curr->obj.pt->y;
		}
		else {
			grbs_arc_t *arc = curr->obj.arc;
			if (arc->new_in_use) {
				ex = arc->parent_pt->x + cos(arc->new_sa + arc->new_da) * arc->new_r;
				ey = arc->parent_pt->y + sin(arc->new_sa + arc->new_da) * arc->new_r;
			}
			else {
				ex = arc->parent_pt->x + cos(arc->sa + arc->da) * arc->r;
				ey = arc->parent_pt->y + sin(arc->sa + arc->da) * arc->r;
			}
		}
		rbsq->rlast_x = RBSR_G2R(ex);
		rbsq->rlast_y = RBSR_G2R(ey);
	}

	rnd_trace("realize:\n");
	for(; curr != NULL; curr = curr->last_real) {
		rnd_trace(" r %p\n", curr);
		grbs_path_realize(grbs, rbsq->tn, curr, 0);
	}
	rnd_trace("--\n");

	/* turn the last section into wireframe */
	if (cons != NULL) {
		grbs_arc_t *arc = gdl_first(&rbsq->tn->arcs);
		if (arc != NULL) {
			arc->RBSR_WIREFRAME_FLAG = 1;
			if (arc->da == 0) {
				/* make the consider-arc non-zero in length to make it visible */
				if (rbsq->consider.dir == GRBS_ADIR_CONVEX_CW)
					arc->da = 1;
				else if (rbsq->consider.dir == GRBS_ADIR_CONVEX_CCW)
					arc->da = -1;
			}
			if (arc->eline != NULL) {
				arc->eline->RBSR_WIREFRAME_FLAG = 1;
				arc = gdl_next(&rbsq->tn->arcs, arc);
				if (arc != NULL)
					arc->RBSR_WIREFRAME_FLAG = 1;
			}
		}
	}

	return res;
}

RND_INLINE grbs_point_t *rbsr_find_point_by_arc_thick(rbsr_map_t *rbs, rnd_coord_t cx_, rnd_coord_t cy_, double bestd2, double delta)
{
	double cx = RBSR_R2G(cx_), cy = RBSR_R2G(cy_);
	grbs_arc_t *arc;
	grbs_point_t *best = NULL;
	grbs_rtree_it_t it;
	grbs_rtree_box_t bbox;

	bbox.x1 = cx - delta;
	bbox.y1 = cy - delta;
	bbox.x2 = cx + delta;
	bbox.y2 = cy + delta;

	for(arc = grbs_rtree_first(&it, &rbs->grbs.arc_tree, &bbox); arc != NULL; arc = grbs_rtree_next(&it)) {
		grbs_point_t *pt = arc->parent_pt;
		double dx = cx - pt->x, dy = cy - pt->y, d2 = dx*dx + dy*dy;
		if (d2 < bestd2) {
			bestd2 = d2;
			best = pt;
		}
	}

	return best;
}


int rbsr_seq_consider(rbsr_seq_t *rbsq, rnd_coord_t tx, rnd_coord_t ty, int *need_redraw_out)
{
	grbs_point_t *end;
	rnd_coord_t ptcx, ptcy;
	double l1x, l1y, l2x, l2y, px, py, side, dist2, dx, dy, cop2, gslop;
	grbs_arc_dir_t dir;
	int refuse_incident = 0;

	gslop = RBSR_R2G((double)rnd_pixel_slop*10.0);
	end = rbsr_find_point_thick(&rbsq->map, tx, ty, gslop);
	if (end == NULL) {
		refuse_incident = 0;
		end = rbsr_find_point_by_arc_thick(&rbsq->map, tx, ty, RND_COORD_MAX, gslop);
	}

	if (end == NULL) {
		refuse: {
		int need_redraw = 0;
		if (rbsq->consider.dir != RBS_ADIR_invalid)
			need_redraw = 1;

		rbsq->consider.dir = RBS_ADIR_invalid;
		if (need_redraw)
			rbsr_seq_redraw(rbsq);
		*need_redraw_out = need_redraw;
		return -1;
		}
	}

	ptcx = RBSR_G2R(end->x);
	ptcy = RBSR_G2R(end->y);

	cop2 = RBSR_G2R(end->copper);
	cop2 = cop2 * cop2;

	dx = tx - ptcx; dy = ty - ptcy;
	dist2 = dx*dx + dy*dy;

	if (dist2 > cop2) {
		/* project target point onto the line between last end and center of the
		   point; if it is to the right, go ccw, if to the left go cw, if in the
		   center go incident */
		/* decide direction from cross product; line is rbsq->last_* -> ptc* */
		l1x = rbsq->last_x; l1y = rbsq->last_y;
		l2x = ptcx, l2y = ptcy;
		px = tx; py = ty;

		side = (l2x - l1x) * (py - l1y) - (l2y - l1y) * (px - l1x);
		rnd_trace(" side: %f %s\n", side, (side < 0) ? "cw" : "ccw");
		dir = (side < 0) ? GRBS_ADIR_CONVEX_CW : GRBS_ADIR_CONVEX_CCW;
	}
	else {
		if (refuse_incident)
			goto refuse;

		rnd_trace(" incident\n");
		dir = GRBS_ADIR_INC;
	}

	if ((rbsq->consider.pt == end) && (rbsq->consider.dir == dir)) {
		*need_redraw_out = 0;
		return 0; /* do not redraw if there's no change */
	}

	TODO("step back if end:dir matches the previous");


	rbsq->consider.pt = end;
	rbsq->consider.dir = dir;

	*need_redraw_out = 1;
	return rbsr_seq_redraw(rbsq);
}

rbsr_seq_accept_t rbsr_seq_accept(rbsr_seq_t *rbsq)
{
	rbsr_seq_accept_t res = RBSR_SQA_CONTINUE;


	rbsq->path[rbsq->used] = rbsq->consider;
	rbsq->used++;
	rbsq->last_x = rbsq->rlast_x;
	rbsq->last_y = rbsq->rlast_y;

	if (rbsq->consider.dir == GRBS_ADIR_INC) {
		res = RBSR_SQA_TERMINATE;
		rbsq->consider.dir = 0;
	}

	rbsr_seq_redraw(rbsq);
	return res;
}


void rbsr_seq_end(rbsr_seq_t *rbsq)
{
	pcb_layer_t *ly = pcb_get_layer(rbsq->map.pcb->Data, rbsq->map.lid);

	/* tune existing objects and install new objects */
	rbsr_install_2net(ly, rbsq->tn);
}


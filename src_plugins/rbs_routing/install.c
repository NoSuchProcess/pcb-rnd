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

RND_INLINE int angeq(double a1, double a2)
{
	double d = a1 - a2;
	return (d >= -0.01) && (d < +0.01);
}

static int rbsr_install_line(pcb_layer_t *ly, grbs_2net_t *tn, grbs_line_t *line)
{
	pcb_2netmap_obj_t *obj = line->user_data;
	pcb_line_t *pl;
	rnd_coord_t l1x = RBSR_G2R(line->x1), l1y = RBSR_G2R(line->y1);
	rnd_coord_t l2x = RBSR_G2R(line->x2), l2y = RBSR_G2R(line->y2);

	if (line->RBSR_WIREFRAME_COPIED_BACK)
		return 0;
	line->RBSR_WIREFRAME_COPIED_BACK = 1;

	if (obj == NULL) { /* create new */
		if ((line->x1 == line->x2) && (line->y1 == line->y2))
			return 0;

		pl = pcb_line_new(ly, l1x, l1y, l2x, l2y,
			RBSR_G2R(tn->copper*2), RBSR_G2R(tn->clearance*2),
			pcb_flag_make(PCB_FLAG_CLEARLINE));
		if (pl == NULL) {
			rnd_message(RND_MSG_ERROR, "rbsr_install: failed to create line\n");
			return -1;
		}
	}
	else {
		pl = obj->orig;
		assert(pl->type == PCB_OBJ_LINE);

		/* don't touch lines that have no change */
		if (rcrdeq(l1x, pl->Point1.X) && rcrdeq(l1y, pl->Point1.Y) && rcrdeq(l2x, pl->Point2.X) && rcrdeq(l2y, pl->Point2.Y)) return 0;
		if (rcrdeq(l2x, pl->Point1.X) && rcrdeq(l2y, pl->Point1.Y) && rcrdeq(l1x, pl->Point2.X) && rcrdeq(l1y, pl->Point2.Y)) return 0;

		/* modify trying to keep original direction */
		if ((rcrdeq(l1x, pl->Point1.X) && rcrdeq(l1y, pl->Point1.Y)) || (rcrdeq(l2x, pl->Point2.X) && rcrdeq(l2y, pl->Point2.Y)))
			pcb_line_modify(pl, &l1x, &l1y, &l2x, &l2y, NULL, NULL, 1);
		else
			pcb_line_modify(pl, &l2x, &l2y, &l1x, &l1y, NULL, NULL, 1);
	}
	return 0;
}

static int rbsr_install_arc(pcb_layer_t *ly, grbs_2net_t *tn, grbs_arc_t *arc)
{
	pcb_2netmap_obj_t *obj = arc->user_data;
	pcb_arc_t *pa;
	double sa, da;
	rnd_coord_t cx, cy, r;

	if (arc->RBSR_WIREFRAME_COPIED_BACK)
		return 0;
	arc->RBSR_WIREFRAME_COPIED_BACK = 1;

	sa = 180.0 - (arc->sa * RND_RAD_TO_DEG);
	da = - (arc->da * RND_RAD_TO_DEG);
	cx = RBSR_G2R(arc->parent_pt->x);
	cy = RBSR_G2R(arc->parent_pt->y);
	r = RBSR_G2R(arc->r);

	if (obj == NULL) { /* create new */
		if (arc->r == 0)
			return 0; /* do not create dummy arcs used for incident lines */

		pa = pcb_arc_new(ly, cx, cy, r, r, sa, da,
			RBSR_G2R(tn->copper*2), RBSR_G2R(tn->clearance*2),
			pcb_flag_make(PCB_FLAG_CLEARLINE), 1);

		if (pa == NULL) {
			rnd_message(RND_MSG_ERROR, "rbsr_install: failed to create arc\n");
			return -1;
		}
	}
	else {
		double sa2 = sa+da, da2 = -da, *new_sa = NULL, *new_da = NULL;
		int match_cent, match_radi, match_ang1, match_ang2;
		rnd_coord_t *new_cx = NULL, *new_cy = NULL, *new_r = NULL;

		pa = obj->orig;
		assert(pa->type == PCB_OBJ_ARC);

		/* figure which parameters match */
		match_cent = (rcrdeq(cx, pa->X) && rcrdeq(cy, pa->Y));
		match_radi = rcrdeq(r, pa->Width);
		match_ang1 = (angeq(sa, pa->StartAngle) && angeq(da, pa->Delta));
		match_ang2 = (angeq(sa2, pa->StartAngle) && angeq(da2, pa->Delta));

		/* no change */
		if (match_cent && match_radi && (match_ang1 || match_ang2)) return 0;

		/* figure what needs change */
		if (!match_cent) {
			new_cx = &cx;
			new_cy = &cy;
		}
		if (!match_radi)
			new_r = &r;
		if (!match_ang1 && !match_ang2) {
			new_sa = &sa;
			new_da = &da;
		}

		pcb_arc_modify(pa, new_cx, new_cy, new_r, new_r, new_sa, new_da, NULL, NULL, 1);

		/* A change in an arc typically means change in connected lines; the lines
		   on our target tn are tuned but lines of innocent bystander twonets are
		   not. Arcs of those bystander twonets are updated and are getting
		   here because all arcs of an affected point-segment is checked */
		if (arc->sline != NULL)
			rbsr_install_line(ly, tn, arc->sline);
		if (arc->eline != NULL)
			rbsr_install_line(ly, tn, arc->eline);
	}
	return 0;
}

static int rbsr_install_pt_arcs(pcb_layer_t *ly, grbs_2net_t *tn, grbs_point_t *pt)
{
	int seg, res = 0;

	for(seg = 0; seg < GRBS_MAX_SEG; seg++) {
		grbs_arc_t *a = gdl_first(&pt->arcs[seg]);
		for(a = gdl_next(&pt->arcs[seg], a) ; a != NULL; a = gdl_next(&pt->arcs[seg], a))
			res |= rbsr_install_arc(ly, tn, a);
	}

	return res;
}

int rbsr_install_2net(pcb_layer_t *ly, grbs_2net_t *tn)
{
	grbs_arc_t *a, *prev = NULL;
	int res = 0;

	for(a = gdl_first(&tn->arcs); a != NULL; prev = a, a = gdl_next(&tn->arcs, a)) {
		if (prev != NULL)
			res |= rbsr_install_line(ly, tn, a->sline);

		/* verify all arcs, their radius and angles may have changed due to new arc
		   inserted */
		res |= rbsr_install_pt_arcs(ly, tn, a->parent_pt);
	}

	return res;
}

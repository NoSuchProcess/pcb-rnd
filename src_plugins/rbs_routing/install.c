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

static int rbsr_install_arc(pcb_layer_t *ly, grbs_2net_t *tn, grbs_arc_t *arc)
{
	if (arc->user_data == NULL) { /* create new */
		pcb_arc_t *pa;
		double cx, cy, sa, da;

		if (arc->r == 0)
			return 0; /* do not create dummy arcs used for incident lines */

		sa = 180.0 - (arc->sa * RND_RAD_TO_DEG);
		da = - (arc->da * RND_RAD_TO_DEG);
		cx = arc->parent_pt->x;
		cy = arc->parent_pt->y;

		pa = pcb_arc_new(ly, RBSR_G2R(cx), RBSR_G2R(cy),
			RBSR_G2R(arc->r), RBSR_G2R(arc->r), sa, da,
			RBSR_G2R(tn->copper*2), RBSR_G2R(tn->clearance*2),
			pcb_flag_make(PCB_FLAG_CLEARLINE), 1);
	}
	else {
		TODO("verify existing");
	}
	return 0;
}

static int rbsr_install_line(pcb_layer_t *ly, grbs_2net_t *tn, grbs_line_t *line)
{
	if (line->user_data == NULL) { /* create new */
		pcb_line_t *pl;

		if ((line->x1 == line->x2) && (line->y1 == line->y2))
			return 0;

		pl = pcb_line_new(ly,
			RBSR_G2R(line->x1), RBSR_G2R(line->y1),
			RBSR_G2R(line->x2), RBSR_G2R(line->y2),
			RBSR_G2R(tn->copper*2), RBSR_G2R(tn->clearance*2),
			pcb_flag_make(PCB_FLAG_CLEARLINE));
	}
	else {
		TODO("verify existing");
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

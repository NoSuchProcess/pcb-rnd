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
#include "obj_pstk_inlines.h"

RND_INLINE void make_point_from_pstk_shape(rbsr_map_t *rbs, pcb_pstk_t *ps, pcb_pstk_shape_t *shp, pcb_layer_t *ly)
{
	pcb_board_t *pcb = rbs->pcb;
	rnd_coord_t clr = obj_pstk_get_clearance(pcb, ps, ly);
	rnd_coord_t copdia, x, y;
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
			pt = grbs_point_new(&rbs->grbs, RBSR_R2G(x), RBSR_R2G(y), RBSR_R2G(copdia), RBSR_R2G(clr));
			x = ps->x + shp->data.line.x2;
			y = ps->y + shp->data.line.y2;
			pt2 = grbs_point_new(&rbs->grbs, RBSR_R2G(x), RBSR_R2G(y), RBSR_R2G(copdia), RBSR_R2G(clr));
			tn = grbs_2net_new(&rbs->grbs, copdia, clr);
			line = grbs_line_realize(&rbs->grbs, tn, pt, pt2);
			line->immutable = 1;
			break;

		case PCB_PSSH_CIRC:
			copdia = shp->data.circ.dia;
			x = ps->x + shp->data.circ.x;
			y = ps->y + shp->data.circ.y;
			grbs_point_new(&rbs->grbs, RBSR_R2G(x), RBSR_R2G(y), RBSR_R2G(copdia), RBSR_R2G(clr));
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
			
			for(n = 0; n < shp->data.poly.len; n++) {
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

int rbsr_map_pcb(rbsr_map_t *dst, pcb_board_t *pcb, rnd_layer_id_t lid)
{
	int res;

	grbs_uninit(&dst->grbs);
	grbs_init(&dst->grbs);
	dst->pcb = pcb;
	dst->lid = lid;

	res = map_pstks(dst);

	return res;
}


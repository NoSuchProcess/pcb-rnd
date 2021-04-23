/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  delayed creation of objects
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2021)
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

#include "delay_postproc.h"
#include "obj_pstk_inlines.h"
#include "netlist.h"

typedef struct {
	pcb_board_t *pcb;
	pcb_poly_t *poly;
	const char *netname;
	pcb_thermal_t t;
	const char *(*obj_netname)(void *uctx, pcb_any_obj_t *obj);
	void *uctx;
} ppr_t;

void pcb_dlcr_post_poly_thermal_obj(pcb_board_t *pcb, pcb_poly_t *poly, pcb_any_obj_t *obj, pcb_thermal_t t)
{
	printf("THERMAL!\n");
}

static rnd_r_dir_t ppr_poly_therm(const rnd_box_t *b, void *cl)
{
	ppr_t *ppr = cl;
	pcb_any_obj_t *o = (pcb_any_obj_t *)b;
	const char *vnn = ppr->obj_netname(ppr->uctx, o);

	if ((vnn != NULL) && (strcmp(vnn, ppr->netname) == 0))
		pcb_dlcr_post_poly_thermal_obj(ppr->pcb, ppr->poly, o, ppr->t);

	return RND_R_DIR_FOUND_CONTINUE;
}


void pcb_dlcr_post_poly_thermal_netname(pcb_board_t *pcb, pcb_poly_t *poly, const char *netname, pcb_thermal_t t, const char *(*obj_netname)(void *uctx, pcb_any_obj_t *obj), void *uctx)
{
	ppr_t ppr;

	ppr.pcb = pcb;
	ppr.poly = poly;
	ppr.t = t;
	ppr.obj_netname = obj_netname;
	ppr.uctx = uctx;

	rnd_r_search(poly->parent.layer->line_tree, &poly->BoundingBox, NULL, ppr_poly_therm, &ppr, NULL);
	rnd_r_search(poly->parent.layer->arc_tree,  &poly->BoundingBox, NULL, ppr_poly_therm, &ppr, NULL);
	rnd_r_search(pcb->Data->padstack_tree,      &poly->BoundingBox, NULL, ppr_poly_therm, &ppr, NULL);
}

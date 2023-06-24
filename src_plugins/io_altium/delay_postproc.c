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

/* Copied from io_pads (the way altium does it is very differnet) */

#include "obj_pstk_inlines.h"
#include "netlist.h"
#include "thermal.h"
#include "change.h"
#include "find.h"

#include "pcbdoc.h"

typedef struct {
	void *rctx;
	pcb_board_t *pcb;
	pcb_poly_t *poly;
	long netid;
	pcb_thermal_t t;
	void *uctx;
} ppr_t;

static void altium_post_poly_thermal_obj(pcb_board_t *pcb, pcb_poly_t *poly, pcb_any_obj_t *obj, pcb_thermal_t t)
{
	pcb_find_t fctx = {0};
	rnd_bool isc;

	/* deal with objects that intersect */
	fctx.ignore_clearance = 1;

	/* the polygon already has a clearance cutout for the object, compensate this
	   with bloating up the search */
	switch(obj->type) {
		case PCB_OBJ_PSTK: fctx.bloat = ((pcb_pstk_t *)obj)->Clearance + 2; break;
		case PCB_OBJ_LINE: fctx.bloat = ((pcb_line_t *)obj)->Clearance + 2; break;
		case PCB_OBJ_ARC:  fctx.bloat = ((pcb_arc_t *)obj)->Clearance + 2; break;
		default: return;
	}
	isc = pcb_intersect_obj_obj(&fctx, (pcb_any_obj_t *)poly, obj);
	pcb_find_free(&fctx);
	if (!isc) return;

	pcb_chg_obj_thermal(obj->type, obj, obj, obj, t, pcb_layer2id(pcb->Data, poly->parent.layer));
}

static rnd_rtree_dir_t altium_poly_therm(void *cl, void *obj, const rnd_rtree_box_t *box)
{
	ppr_t *ppr = cl;
	pcb_any_obj_t *o = (pcb_any_obj_t *)obj;
	long netid = io_altium_obj2netid(ppr->rctx, o);

	if (netid == ppr->netid)
		altium_post_poly_thermal_obj(ppr->pcb, ppr->poly, o, ppr->t);

	return rnd_RTREE_DIR_FOUND_CONT;
}


static void altium_post_poly_thermal_netname(void *rctx, pcb_board_t *pcb, pcb_poly_t *poly, long netid, pcb_thermal_t t, void *uctx)
{
	ppr_t ppr;

	ppr.rctx = rctx;
	ppr.pcb = pcb;
	ppr.poly = poly;
	ppr.netid = netid;
	ppr.t = t;
	ppr.uctx = uctx;

	rnd_rtree_search_any(poly->parent.layer->line_tree, (rnd_rtree_box_t *)&poly->BoundingBox, NULL, altium_poly_therm, &ppr, NULL);
	rnd_rtree_search_any(poly->parent.layer->arc_tree,  (rnd_rtree_box_t *)&poly->BoundingBox, NULL, altium_poly_therm, &ppr, NULL);
	rnd_rtree_search_any(pcb->Data->padstack_tree,      (rnd_rtree_box_t *)&poly->BoundingBox, NULL, altium_poly_therm, &ppr, NULL);
}

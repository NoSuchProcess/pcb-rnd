/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - electric test export
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <stdio.h>
#include <assert.h>

#include "board.h"
#include "data.h"
#include "parse.h"
#include "safe_fs.h"
#include "error.h"
#include "tetest.h"
#include "rtree.h"
#include "obj_subc_parent.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "netlist.h"

static const char *sides(pcb_layer_type_t lyt)
{
	if ((lyt & PCB_LYT_TOP) && (lyt & PCB_LYT_BOTTOM)) return "both";
	if (lyt & PCB_LYT_TOP) return "top";
	if (lyt & PCB_LYT_BOTTOM) return "bottom";
	return "-";
}

static void tedax_etest_fsave_pstk(FILE *f, pcb_pstk_t *ps, const char *netname, const char *refdes, const char *term)
{
	int n, has_slot = 0;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	pcb_pstk_tshape_t *ts;
	pcb_layer_type_t copper = 0, exposed = 0, side;
	pcb_pstk_shape_t *shp, *minshp = NULL;
	pcb_coord_t shx, shy, dia, mindia;

	if (proto == NULL)
		return;
	ts = &proto->tr.array[0];
	if (ts == NULL)
		return;

	if (netname == NULL)
		netname = "-";

	shp = ts->shape;
	for(n = 0; n < ts->len; n++,shp++) {
		int accept = 0;
		side = (shp->layer_mask & (PCB_LYT_TOP | PCB_LYT_BOTTOM));
		if (shp->layer_mask & PCB_LYT_MECH)
			has_slot = 1;
		if ((side != 0) && (shp->layer_mask & PCB_LYT_MASK)) {
			exposed |= side;
			accept = 1;
		}
		if ((side != 0) && (shp->layer_mask & PCB_LYT_COPPER)) {
			copper |= side;
			accept = 1;
		}
		if (accept) {
			TODO("calculate dia");
			dia = PCB_MM_TO_COORD(0.5);
			if ((minshp == NULL) || (dia < mindia)) {
				minshp = shp;
				shx = 0; shy = 0;
				mindia = dia;
			}
		}
	}

	if ((minshp == NULL) || (copper == 0) || has_slot)
		return;

	fprintf(f, "	pad ");
	tedax_fprint_escape(f, netname == NULL ? "-" : netname);

	fprintf(f, " ");
	tedax_fprint_escape(f, refdes == NULL ? "-" : refdes);

	fprintf(f, " ");
	tedax_fprint_escape(f, term == NULL ? "-" : term);

	pcb_fprintf(f, " %.06mm %.06mm %s round %.06mm %.06mm 0 ", ps->x + shx, ps->y + shy, sides(copper), dia, dia);
	if (proto->hdia > 0)
		pcb_fprintf(f, "%s %.06mm ", proto->hplated ? "plated" : "unplated", proto->hdia);
	else
		fprintf(f, "- - ");

	fprintf(f, " %s %s\n", sides(exposed), sides(exposed));
}

int tedax_etest_fsave(pcb_board_t *pcb, const char *etestid, FILE *f)
{
	pcb_box_t *b;
	pcb_rtree_it_t it;

	fprintf(f, "begin etest v1 ");
	tedax_fprint_escape(f, etestid);
	fputc('\n', f);

	for(b = pcb_r_first(pcb->Data->padstack_tree, &it); b != NULL; b = pcb_r_next(&it)) {
		pcb_pstk_t *ps = (pcb_pstk_t *)b;
		pcb_subc_t *sc;
		pcb_net_term_t *t;

		assert(ps->type == PCB_OBJ_PSTK);
		if (ps->term == NULL)
			continue;
		sc = pcb_gobj_parent_subc(ps->parent_type, &ps->parent);
		if ((sc == NULL) || (sc->refdes == NULL))
			continue;
		t = pcb_net_find_by_refdes_term(&pcb->netlist[PCB_NETLIST_EDITED], sc->refdes, ps->term);
		if (t == NULL) /* export named nets only, for now */
			continue;
		tedax_etest_fsave_pstk(f, ps, t->parent.net->name, sc->refdes, ps->term);
	}
	pcb_r_end(&it);


	fprintf(f, "end etest\n");
	return 0;
}

int tedax_etest_save(pcb_board_t *pcb, const char *etestid, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_etest_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_etest_fsave(pcb, etestid, f);
	fclose(f);
	return res;
}


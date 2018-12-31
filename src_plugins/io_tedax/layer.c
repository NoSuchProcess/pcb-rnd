/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - stackup import/export
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include "layer.h"
#include "safe_fs.h"
#include "error.h"
#include "compat_misc.h"

int tedax_layer_fsave(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, FILE *f)
{
	char lntmp[64];
	int lno;
	pcb_layergrp_t *g = pcb_get_layergrp(pcb, gid);

	if (g == NULL)
		return -1;

	if (layname == NULL)
		layname = g->name;
	if (layname == NULL) {
		layname = lntmp;
		sprintf(lntmp, "anon_%ld", gid);
	}
	fprintf(f, "layer v1 %s\n", layname);
	for(lno = 0; lno < g->len; lno++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[lno]);
		if (ly == NULL)
			continue;
		PCB_LINE_LOOP(ly) {
			pcb_fprintf(f, " line %.06mm %.06mm %.06mm %.06mm %.06mm %.06mm\n",
				line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y,
				line->Thickness, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line) ? pcb_round(line->Clearance/2) : 0);
		} PCB_END_LOOP;
		PCB_ARC_LOOP(ly) {
			pcb_coord_t sx, sy, ex, ey;
			pcb_arc_get_end(arc, 0, &sx, &sy);
			pcb_arc_get_end(arc, 1, &ex, &ey);
			pcb_fprintf(f, " arc %.06mm %.06mm %.06mm %f %f %.06mm %.06mm %.06mm %.06mm %.06mm %.06mm\n",
				arc->X, arc->Y, arc->Width, arc->StartAngle, arc->Delta,
				arc->Thickness, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, arc) ? pcb_round(arc->Clearance/2) : 0,
				sx, sy, ex, ey);
		} PCB_END_LOOP;
	}
	fprintf(f, "end layer\n");
	return -1;
}

int tedax_layer_save(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_layer_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_layer_fsave(pcb, gid, layname, f);
	fclose(f);
	return res;
}

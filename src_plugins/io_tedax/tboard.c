/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - stackup import/export
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

#include "board.h"
#include "tboard.h"
#include "parse.h"
#include "error.h"
#include "safe_fs.h"
#include "stackup.h"
#include "netlist.h"
#include "tdrc.h"
#include "tlayer.h"

int tedax_board_fsave(pcb_board_t *pcb, FILE *f)
{
	pcb_layergrp_id_t gid;
	int n;
	pcb_attribute_t *a;
	tedax_stackup_t ctx;
	static const char *stackupid = "board_stackup";
	static const char *netlistid = "board_netlist";
	static const char *drcid     = "board_drc";

	tedax_stackup_init(&ctx);

	fputc('\n', f);
	tedax_drc_fsave(pcb, drcid, f);

	fputc('\n', f);
	tedax_net_fsave(pcb, netlistid, f);

	fputc('\n', f);
	if (tedax_stackup_fsave(&ctx, pcb, stackupid, f) != 0) {
		pcb_message(PCB_MSG_ERROR, "internal error: failed to save the stackup\n");
		goto error;
	}

	for(gid = 0; gid < ctx.g2n.used; gid++) {
		char *name = ctx.g2n.array[gid];
		if (name != NULL) {
			fputc('\n', f);
			tedax_layer_fsave(pcb, gid, name, f);
		}
	}

	fprintf(f, "\nbegin board v1 ");
	tedax_fprint_escape(f, pcb->Name);
	fputc('\n', f);
	pcb_fprintf(f, " drawing_area 0 0 %.06mm %.06mm\n", pcb->MaxWidth, pcb->MaxHeight);
	for(n = 0, a = pcb->Attributes.List; n < pcb->Attributes.Number; n++,a++) {
		pcb_fprintf(f, " attr ");
		tedax_fprint_escape(f, a->name);
		fputc(' ', f);
		tedax_fprint_escape(f, a->value);
		fputc('\n', f);
	}
	pcb_fprintf(f, " stackup %s\n", stackupid);
	pcb_fprintf(f, " netlist %s\n", netlistid);
	pcb_fprintf(f, " drc %s\n", drcid);
	fprintf(f, "end board\n");
	tedax_stackup_uninit(&ctx);
	return 0;
	
	error:
	tedax_stackup_uninit(&ctx);
	return -1;
}


int tedax_board_save(pcb_board_t *pcb, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_board_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_board_fsave(pcb, f);
	fclose(f);
	return res;
}


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
#include <genht/htsp.h>
#include <genht/hash.h>

#include "../src_plugins/lib_netmap/placement.h"

#include "board.h"
#include "data.h"
#include "tboard.h"
#include "parse.h"
#include "error.h"
#include "safe_fs.h"
#include "stackup.h"
#include "netlist.h"
#include "footprint.h"
#include "tdrc.h"
#include "tlayer.h"
#include "ht_subc.h"
#include "obj_pstk.h"
#include "compat_misc.h"

#define ps2fpname(fpname, padstack) \
	pcb_snprintf(fpname, sizeof(fpname), "ps_glob_%ld%s", padstack->protoi, (!!padstack->smirror) != (!!padstack->xmirror) ? "m" : "")

#define subc2fpname(fpname, subc) \
	pcb_snprintf(fpname, sizeof(fpname), "sc_glob_%ld", subc->ID)

static int tedax_global_pstk_fwrite(pcb_board_t *pcb, FILE *f)
{
	htsp_t seen;
	htsp_entry_t *e;

	htsp_init(&seen, strhash, strkeyeq);

	PCB_PADSTACK_LOOP(pcb->Data) {
		char fpname[256];
		ps2fpname(fpname, padstack);
		if (!htsp_has(&seen, fpname)) {
			fprintf(f, "\nbegin footprint v1 %s\n", fpname);
			if (padstack->term != NULL)
				fprintf(f, "	term %s %s - -\n", padstack->term, padstack->term);
			tedax_pstk_fsave(padstack, padstack->x, padstack->y, f);
			htsp_set(&seen, pcb_strdup(fpname), padstack);
			fprintf(f, "end footprint\n");
		}
	}
	PCB_END_LOOP;

	for(e = htsp_first(&seen); e != NULL; e = htsp_next(&seen, e))
		free(e->key);
	htsp_uninit(&seen);
	return 0;
}

static int tedax_global_subc_fwrite(pcb_placement_t *ctx, FILE *f)
{
	htscp_entry_t *e;

	pcb_placement_build(ctx, ctx->pcb->Data);
	for(e = htscp_first(&ctx->subcs); e != NULL; e = htscp_next(&ctx->subcs, e)) {
		pcb_subc_t *subc = e->value;
		char fpname[256];
		subc2fpname(fpname, subc);
		fprintf(f, "\nbegin footprint v1 %s\n", fpname);
		fprintf(f, "end footprint\n");
	}
	return 0;
}


int tedax_board_fsave(pcb_board_t *pcb, FILE *f)
{
	pcb_layergrp_id_t gid;
	int n;
	pcb_attribute_t *a;
	tedax_stackup_t ctx;
	pcb_placement_t plc;
	static const char *stackupid = "board_stackup";
	static const char *netlistid = "board_netlist";
	static const char *drcid     = "board_drc";

	tedax_stackup_init(&ctx);

	fputc('\n', f);
	tedax_drc_fsave(pcb, drcid, f);

	fputc('\n', f);
	tedax_net_fsave(pcb, netlistid, f);

	fputc('\n', f);
	pcb_placement_init(&plc, pcb);
	tedax_global_subc_fwrite(&plc, f);

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

	fputc('\n', f);
	if (tedax_global_pstk_fwrite(pcb, f) != 0)
		goto error;

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

	PCB_PADSTACK_LOOP(pcb->Data) {
		char fpname[256];
		ps2fpname(fpname, padstack);
		pcb_fprintf(f, " place %ld %s %.06mm %.06mm %f %d via\n", padstack->ID, fpname, padstack->x, padstack->y, padstack->rot, !!padstack->smirror);
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(pcb->Data) {
		pcb_host_trans_t tr;
		char fpname[256];
		pcb_subc_t *proto = htscp_get(&plc.subcs, subc);
		subc2fpname(fpname, proto);

		pcb_subc_get_host_trans(subc,  &tr, 0);

		pcb_fprintf(f, " place %ld %s %.06mm %.06mm %f %d comp\n", subc->ID, fpname, tr.ox, tr.oy, tr.rot, tr.on_bottom);
	}
	PCB_END_LOOP;

	fprintf(f, "end board\n");
	tedax_stackup_uninit(&ctx);
	pcb_placement_uninit(&plc);
	return 0;
	
	error:
	tedax_stackup_uninit(&ctx);
	pcb_placement_uninit(&plc);
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


/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - autorouter import/export
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
#include <genht/htsi.h>
#include <genht/hash.h>

#include "../src_plugins/lib_netmap/placement.h"

#include "board.h"
#include "data.h"
#include "tboard.h"
#include "parse.h"
#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include "obj_pstk_inlines.h"
#include "stackup.h"
#include "tlayer.h"
#include "obj_pstk.h"
#include <librnd/core/compat_misc.h>
#include "plug_io.h"

#include "common_inlines.h"

#define LAYERNET(obj) tedax_finsert_layernet_tags(f, nmap, (pcb_any_obj_t *)obj)

static int tedax_global_via_fwrite(pcb_board_t *pcb, FILE *f, pcb_netmap_t *nmap)
{
	PCB_PADSTACK_LOOP(pcb->Data) {
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto(padstack);
		if (proto != NULL) {
			rnd_coord_t cx, cy, dia = 0;

			if (proto->hdia > 0) {
				cx = padstack->x;
				cy = padstack->y;
				dia = proto->hdia;
			}
			else {
				TODO("look for a slot");
			}
			
			if (dia > 0) {
				fprintf(f, " via");
				LAYERNET(padstack);
				rnd_fprintf(f, " %.06mm %.06mm %.06mm 0\n", cx, cy, dia);
				TODO("bbvia: two more arguments");
			}
		}
	}
	PCB_END_LOOP;

	return 0;
}

int tedax_route_req_fsave(pcb_board_t *pcb, FILE *f)
{
	rnd_layergrp_id_t gid;
	int res = -1;
	tedax_stackup_t ctx;
	pcb_netmap_t nmap;
	static const char *stackupid = "board_stackup";

	if (pcb_netmap_init(&nmap, pcb) != 0) {
		rnd_message(RND_MSG_ERROR, "internal error: failed to map networks\n");
		goto error;
	}

	tedax_stackup_init(&ctx);
	fputc('\n', f);
	if (tedax_stackup_fsave(&ctx, pcb, stackupid, f, PCB_LYT_COPPER) != 0) {
		rnd_message(RND_MSG_ERROR, "internal error: failed to save the stackup\n");
		goto error;
	}

	for(gid = 0; gid < ctx.g2n.used; gid++) {
		char *name = ctx.g2n.array[gid];
		if (name != NULL) {
			fputc('\n', f);
			tedax_layer_fsave(pcb, gid, name, f, &nmap);
		}
	}

	fputc('\n', f);

	fprintf(f, "\nbegin route_req v1 ");
	tedax_fprint_escape(f, pcb->hidlib.name);
	fputc('\n', f);

	rnd_fprintf(f, " stackup %s\n", stackupid);

	if (tedax_global_via_fwrite(pcb, f, &nmap) != 0)
		goto error;

	fprintf(f, "end route_req\n");

	res = 0;
	error:;
	tedax_stackup_uninit(&ctx);
	pcb_netmap_uninit(&nmap);
	return res;
}


int tedax_route_req_save(pcb_board_t *pcb, const char *fn)
{
	int res;
	FILE *f;

	f = rnd_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_route_req_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_route_req_fsave(pcb, f);
	fclose(f);
	return res;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format write
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

#include "plug_io.h"
#include "error.h"
#include "pcb_bool.h"
#include "board.h"
#include "data.h"
#include "layer.h"
#include "layer_grp.h"

#include "write.h"

#include "../src_plugins/lib_netmap/netmap.h"

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	pcb_netmap_t nmap;
} dsn_write_t;

#define GNAME_MAX 64

static void group_name(char *dst, const char *src, pcb_layergrp_id_t gid)
{
	int n;
	char *d;
	const char*s;

	n = sprintf(dst, "%d__", (int)gid);

	for(d = dst+n, s = src; (*s != '\0') && (n < GNAME_MAX - 1); s++,d++,n++) {
		if ((*s == '"') || (*s < 32) || (*s > 126))
			*d = '_';
		else
			*d = *s;
	}
	*d = '\0';
}

#define COORDX(x) (x)
#define COORDY(y) (PCB->MaxHeight - (y))

static int dsn_write_structure(dsn_write_t *wctx)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *lg;

	fprintf(wctx->f, "  (structure\n");
	for(gid = 0, lg = wctx->pcb->LayerGroups.grp; gid < wctx->pcb->LayerGroups.len; gid++,lg++) {
		char gname[GNAME_MAX];
		if (!(lg->ltype & PCB_LYT_COPPER))
			continue;
		group_name(gname, lg->name, gid);
		fprintf(wctx->f, "    (layer \"%s\" (type signal))\n", gname);
	}
	fprintf(wctx->f, "  )\n");

	return 0;
}

static int dsn_write_wiring(dsn_write_t *wctx)
{
	pcb_layer_id_t lid;
	pcb_layer_t *ly;

	fprintf(wctx->f, "  (wiring\n");
	for(ly = wctx->pcb->Data->Layer, lid = 0; lid < wctx->pcb->Data->LayerN; lid++,ly++) {
		char gname[GNAME_MAX];
		pcb_layergrp_id_t gid = pcb_layer_get_group_(ly);
		pcb_layergrp_t *lg = pcb_get_layergrp(wctx->pcb, gid);
		pcb_net_t *net;

		if ((lg == NULL) || !(lg->ltype & PCB_LYT_COPPER))
			continue;

		group_name(gname, lg->name, gid);

		PCB_LINE_LOOP(ly);
		{
			net = htpp_get(&wctx->nmap.o2n, (pcb_any_obj_t *)line);
			pcb_fprintf(wctx->f,"    (wire (path \"%s\" %[4] %[4] %[4] %[4] %[4])", gname, line->Thickness,
				COORDX(line->Point1.X), COORDY(line->Point1.Y),
				COORDX(line->Point2.X), COORDY(line->Point2.Y));
			if (net != NULL)
				fprintf(wctx->f, "(net \"%s\")", net->name);
			fprintf(wctx->f, "(type protect))\n");
		}
		PCB_END_LOOP;

	}
	fprintf(wctx->f, "  )\n");
	return 0;
}


static int dsn_write_board(dsn_write_t *wctx)
{
	const char *s;
	int res = 0;

	if (pcb_netmap_init(&wctx->nmap, wctx->pcb) != 0) {
		pcb_message(PCB_MSG_ERROR, "Can not set up net map\n");
		return -1;
	}

	fprintf(wctx->f, "(pcb ");
	if ((wctx->pcb->Name != NULL) && (*wctx->pcb->Name != '\0')) {
		for(s = wctx->pcb->Name; *s != '\0'; s++) {
			if (isalnum(*s))
				fputc(*s, wctx->f);
			else
				fputc('_', wctx->f);
		}
		fputc('\n', wctx->f);
	}
	else
		fprintf(wctx->f, "anon\n");

	fprintf(wctx->f, "  (parser\n");
	fprintf(wctx->f, "    (string_quote \")\n");
	fprintf(wctx->f, "    (space_in_quoted_tokens on)\n");
	fprintf(wctx->f, "    (host_cad \"pcb-rnd/io_dsn\")\n");
	fprintf(wctx->f, "    (host_version \"%s\")\n", PCB_VERSION);
	fprintf(wctx->f, "  )\n");

	/* set units to mm with nm resolution */
	fprintf(wctx->f, "  (resolution mm 1000000)\n");
	fprintf(wctx->f, "  (unit mm)\n");
	pcb_printf_slot[4] = "%.07mm";

	res |= dsn_write_structure(wctx);
	res |= dsn_write_wiring(wctx);

	fprintf(wctx->f, ")\n");

	pcb_netmap_uninit(&wctx->nmap);
	return res;
}


int io_dsn_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	dsn_write_t wctx;

	wctx.f = FP;
	wctx.pcb = PCB;

	return dsn_write_board(&wctx);
}

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
#include <genht/htsi.h>
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
#include "plug_io.h"

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
		int res;

		subc2fpname(fpname, subc);
		res = tedax_fp_fsave_subc(subc, fpname, 1, f);
		assert(res == 0);
	}
	return 0;
}


int tedax_board_fsave(pcb_board_t *pcb, FILE *f)
{
	htsi_t urefdes;
	htsi_entry_t *re;
	pcb_layergrp_id_t gid;
	int n;
	pcb_attribute_t *a;
	tedax_stackup_t ctx;
	pcb_placement_t plc;
	static const char *stackupid = "board_stackup";
	static const char *netlistid = "board_netlist";
	static const char *drcid     = "board_drc";

	htsi_init(&urefdes, strhash, strkeyeq);
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
		pcb_attribute_t *a;
		pcb_host_trans_t tr;
		char fpname[256], refdes[256];
		pcb_subc_t *proto = htscp_get(&plc.subcs, subc);
		int n;

		if (subc->refdes != NULL) {
			if (tedax_strncpy_escape(refdes, sizeof(refdes), subc->refdes) != 0) {
				pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)subc, "subc-refdes", "too long", "subc refdes too long, using an auto-generated one instead");
				goto fake_refdes;
			}
			if (htsi_has(&urefdes, refdes)) {
				pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)subc, "subc-refdes", "duplucate", "duplicate subc refdes; using an auto-generated one instead - netlist is most probably broken");
				goto fake_refdes;
			}
		}
		else {
			fake_refdes:;
			sprintf(refdes, "ANON%ld", subc->ID);
		}
		htsi_insert(&urefdes, pcb_strdup(refdes), 1);

		subc2fpname(fpname, proto);
		pcb_subc_get_host_trans(subc,  &tr, 0);
		pcb_fprintf(f, " place %s %s %.06mm %.06mm %f %d comp\n", refdes, fpname, tr.ox, tr.oy, tr.rot, tr.on_bottom);

		/* placement text */
		PCB_TEXT_ALL_LOOP(subc->data) {
			if (PCB_FLAG_TEST(PCB_FLAG_FLOATER, text)) {
				pcb_layer_t *rl = pcb_layer_get_real(layer); /* it is slower to resolve the layer here than in an outer per-layer-loop, but we expect only a few floater text objects, code simplicity is more important */
				if (rl != NULL) {
					const char **lyname = (const char **)vtp0_get(&ctx.g2n, rl->meta.real.grp, 0);
					if (lyname != NULL) {
						gds_t tmp;
						pcb_fprintf(f, " place_text %s %s %.06mm %.06mm %.06mm %.06mm %d %f ",
							refdes, *lyname, text->bbox_naked.X1, text->bbox_naked.Y1, text->bbox_naked.X2, text->bbox_naked.Y2,
							text->Scale, text->rot);
						gds_init(&tmp);
						pcb_append_dyntext(&tmp, (pcb_any_obj_t *)text, text->TextString);
						tedax_fprint_escape(f, tmp.array);
						gds_uninit(&tmp);
						fputc('\n', f);
					}
				}
			}
		}
		PCB_ENDALL_LOOP;

		/* placement attributes */
		for(n = 0, a = subc->Attributes.List; n < subc->Attributes.Number; n++,a++) {
			if ((strcmp(a->name, "footprint") == 0) || (strcmp(a->name, "value") == 0)) {
				pcb_fprintf(f, " place_fattr %s %s ", refdes, a->name);
				tedax_fprint_escape(f, a->value);
				fputc('\n', f);
			}
			else {
				pcb_fprintf(f, " place_attr %s ", refdes);
				tedax_fprint_escape(f, a->name);
				fputc(' ', f);
				tedax_fprint_escape(f, a->value);
				fputc('\n', f);
			}
		}
	}
	PCB_END_LOOP;

	fprintf(f, "end board\n");
	for(re = htsi_first(&urefdes); re != NULL; re = htsi_next(&urefdes, re)) free(re->key);
	htsi_uninit(&urefdes);
	tedax_stackup_uninit(&ctx);
	pcb_placement_uninit(&plc);
	return 0;
	
	error:
	for(re = htsi_first(&urefdes); re != NULL; re = htsi_next(&urefdes, re)) free(re->key);
	htsi_uninit(&urefdes);
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

#define errexit(msg) \
do { \
	if (!silent) \
		pcb_message(PCB_MSG_ERROR, msg); \
	res = -1; \
	goto error; \
} while(0)

#define reqarg(what, reqargc) \
do { \
	if (argc != reqargc) \
		errexit("tEDAx board load: too few or too many arguments for " #what " reference\n"); \
} while(0)

#define remember(what) \
do { \
	if (what != NULL) \
		errexit("tEDAx board load: multiple instances of " #what " reference\n"); \
	reqarg(what, 2); \
	what = pcb_strdup(argv[1]); \
} while(0)

static int tedax_board_parse(pcb_board_t *pcb, FILE *f, char *buff, int buff_size, char *argv[], int argv_size, int silent)
{
	char *stackup = NULL, *netlist = NULL, *drc = NULL;
	int res = 0, argc;
	tedax_stackup_t ctx;

	tedax_stackup_init(&ctx);
	while((argc = tedax_getline(f, buff, buff_size, argv, argv_size)) >= 0) {
		if (strcmp(argv[0], "drawing_area") == 0) {
			pcb_coord_t x1, y1, x2, y2;
			pcb_bool succ;

			reqarg("drawing_area", 5);
			x1 = pcb_get_value(argv[1], "mm", NULL, &succ);
			if (!succ) errexit("Invalid x1 coord in drawing_area");
			y1 = pcb_get_value(argv[2], "mm", NULL, &succ);
			if (!succ) errexit("Invalid y1 coord in drawing_area");
			x2 = pcb_get_value(argv[3], "mm", NULL, &succ);
			if (!succ) errexit("Invalid x2 coord in drawing_area");
			y2 = pcb_get_value(argv[4], "mm", NULL, &succ);
			if (!succ) errexit("Invalid y2 coord in drawing_area");
			if ((x1 >= x2) || (y1 >= y2)) errexit("Invalid (unordered, negative box) drawing area");
			if ((x1 < 0) || (y1 < 0)) pcb_message(PCB_MSG_WARNING, "drawing_area starts at negative coords; some objects may not display;\nyou may want to run autocrop()");
			PCB->MaxWidth = x2 - x1;
			PCB->MaxHeight = y2 - y1;
		}
		else if (strcmp(argv[0], "attr") == 0) {
			reqarg("attr", 3);
			pcb_attribute_put(&PCB->Attributes, argv[1], argv[2]);
		}
		else if (strcmp(argv[0], "stackup") == 0) {
			remember(stackup);
		}
		else if (strcmp(argv[0], "netlist") == 0) {
			remember(netlist);
		}
		else if (strcmp(argv[0], "drc") == 0) {
			remember(drc);
		}
		else if (strcmp(argv[0], "place") == 0) {
			reqarg("attr", 8);
		}
		else if (strcmp(argv[0], "place_attr") == 0) {
			reqarg("attr", 4);
		}
		else if (strcmp(argv[0], "place_fattr") == 0) {
			reqarg("attr", 4);
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "board") == 0))
			break;
	}

	if (stackup != NULL) {
		rewind(f);
		res |= tedax_stackup_fload(&ctx, pcb, f, stackup, silent);
	}

	if (netlist != NULL) {
		rewind(f);
		res |= tedax_net_fload(f, 0, netlist, silent);
	}

	if (drc != NULL) {
		rewind(f);
		res |= tedax_drc_fload(pcb, f, drc, silent);
	}

	error:;
	free(stackup);
	free(netlist);
	free(drc);
	tedax_stackup_uninit(&ctx);
	return res;
}


int tedax_board_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, int silent)
{
	char line[520];
	char *argv[16];

	if (tedax_seek_hdr(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	if (tedax_seek_block(f, "board", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	return tedax_board_parse(pcb, f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]), silent);
}


int tedax_board_load(pcb_board_t *pcb, const char *fn, const char *blk_id, int silent)
{
	int res;
	FILE *f;

	f = pcb_fopen(fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_board_load(): can't open %s for reading\n", fn);
		return -1;
	}
	res = tedax_board_fload(pcb, f, blk_id, silent);
	fclose(f);
	return res;
}

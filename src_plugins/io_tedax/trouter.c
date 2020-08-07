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
#include "../src_plugins/lib_compat_help/pstk_compat.h"

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
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>
#include "plug_io.h"
#include "conf_core.h"

#include "common_inlines.h"

#define LAYERNET(obj) tedax_finsert_layernet_tags(f, nmap, (pcb_any_obj_t *)obj)

static int tedax_global_via_fwrite(pcb_board_t *pcb, pcb_data_t *data, FILE *f, pcb_netmap_t *nmap)
{
	PCB_PADSTACK_LOOP(data) {
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

	PCB_SUBC_LOOP(data) {
		tedax_global_via_fwrite(pcb, subc->data, f, nmap);
	}
	PCB_END_LOOP;
	return 0;
}

static void write_conf(FILE *f, int argc, fgw_arg_t *argv)
{
	int n;

	for(n = 0; n < argc; n++) {
		const char *key, *sep;
		if (fgw_arg_conv(&rnd_fgw, &argv[n], FGW_STR) != 0) {
			rnd_message(RND_MSG_ERROR, "Error: route_req: confkey #%d can not be converted to string and is ignored\n", n);
			continue;
		}
		key = argv[n].val.str;
		sep = strchr(key, '=');
		if (sep == NULL) {
			rnd_message(RND_MSG_ERROR, "Error: route_req: confkey %s: no '=' and no value\n", key);
			continue;
		}
		if (strlen(key) > 500) {
			rnd_message(RND_MSG_ERROR, "Error: route_req: confkey %s: value too long\n", key);
			continue;
		}
		sep++;
		fprintf(f, " conf ");
		tedax_fnprint_escape(f, key, sep-key-1);
		fprintf(f, " ");
		tedax_fprint_escape(f, sep);
		fprintf(f, "\n");
	}
}

int tedax_route_req_fsave(pcb_board_t *pcb, FILE *f, int cfg_argc, fgw_arg_t *cfg_argv)
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
	ctx.include_grp_id = 1;
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

	write_conf(f, cfg_argc, cfg_argv);

	rnd_fprintf(f, " stackup %s\n", stackupid);

	if (tedax_global_via_fwrite(pcb, pcb->Data, f, &nmap) != 0)
		goto error;

	/* For now only full-routing is specified */
	fprintf(f, " route_all\n");

	fprintf(f, "end route_req\n");

	res = 0;
	error:;
	tedax_stackup_uninit(&ctx);
	pcb_netmap_uninit(&nmap);
	return res;
}


int tedax_route_req_save(pcb_board_t *pcb, const char *fn, int cfg_argc, fgw_arg_t *cfg_argv)
{
	int res;
	FILE *f;

	f = rnd_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_route_req_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_route_req_fsave(pcb, f, cfg_argc, cfg_argv);
	fclose(f);
	return res;
}



#define PARSE_COORD(dst, src) \
	dst = rnd_get_value(src, "mm", NULL, &succ); \
	if (!succ) { \
		rnd_message(RND_MSG_ERROR, "External autorouter: invalid coordinate '%s'\n", src); \
		continue; \
	}

int tedax_route_res_fload(FILE *fn, const char *blk_id, int silent)
{
	char line[520];
	char *argv[16];
	int argc;
	long cnt_add = 0, cnt_del = 0;
	rnd_bool succ;

	if (tedax_seek_hdr(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	if (tedax_seek_block(fn, "route_res", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	while((argc = tedax_getline(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if ((argc > 5) && (strcmp(argv[0], "add") == 0)) {
			rnd_layergrp_id_t gid;
			pcb_layer_t *ly;
			pcb_layergrp_t *grp;
			char *end;

			if ((argc == 9) && (strcmp(argv[2], "via") == 0)) { /* special case: won't have a layer */
				pcb_pstk_t *ps;
				rnd_coord_t cx, cy, th, clr, hole;

				PARSE_COORD(cx, argv[5]);
				PARSE_COORD(cy, argv[6]);
				PARSE_COORD(th, argv[7]);
				PARSE_COORD(clr, argv[8]);

				hole = conf_core.design.via_drilling_hole;
				if (hole > th*4/5)
					hole = th*4/5;

				ps = pcb_pstk_new_compat_via(PCB->Data, -1, cx, cy, hole, th, clr, 0, PCB_PSTK_COMPAT_ROUND, rnd_true);
				pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, ps, ps, ps);

				PCB_FLAG_SET(PCB_FLAG_AUTO, ps);
				cnt_add++;
				continue;
			}

			/* convert the layer */
			gid = strtol(argv[1], &end, 10);
			if ((*end != '.') || (gid < 0) || (gid >= PCB->LayerGroups.len)) {
				rnd_message(RND_MSG_ERROR, "External autorouter: invalid layer group '%s'\n", argv[1]);
				continue;
			}
			grp = &PCB->LayerGroups.grp[gid];
			if ((!(grp->ltype & PCB_LYT_COPPER)) || (grp->len < 1)) {
				rnd_message(RND_MSG_ERROR, "External autorouter: on layer group '%s' which is either not copper or doesn't have layers\n", argv[1]);
				continue;
			}
			ly = pcb_get_layer(PCB->Data, grp->lid[0]);
			if (ly == NULL) {
				rnd_message(RND_MSG_ERROR, "External autorouter: on layer group '%s' which is broken\n", argv[1]);
				continue;
			}

			if ((argc == 11) && (strcmp(argv[2], "line") == 0)) {
				rnd_coord_t x1, y1, x2, y2, th, cl;
				pcb_line_t *line;
				PARSE_COORD(x1, argv[5]);
				PARSE_COORD(y1, argv[6]);
				PARSE_COORD(x2, argv[7]);
				PARSE_COORD(y2, argv[8]);
				PARSE_COORD(th, argv[9]);
				PARSE_COORD(cl, argv[10]);
				line = pcb_line_new_merge(ly, x1, y1, x2, y2, th, cl, pcb_flag_make(PCB_FLAG_CLEARLINE | PCB_FLAG_AUTO));
				pcb_undo_add_obj_to_create(PCB_OBJ_LINE, ly, line, line);
			}
			else
				rnd_message(RND_MSG_ERROR, "External autorouter: can't add object type '%s' with %d args\n", argv[2], argc);
			cnt_add++;
		}
		else if (strcmp(argv[0], "del") == 0) {
		}
		else if ((argc == 3) && (strcmp(argv[0], "log") == 0)) {
			rnd_message_level_t level = RND_MSG_DEBUG;
			switch(argv[1][0]) {
				case 'I': level = RND_MSG_INFO; break;
				case 'W': level = RND_MSG_WARNING; break;
				case 'E': level = RND_MSG_ERROR; break;
			}
			if (level == RND_MSG_DEBUG) break;
			rnd_message(level, "%s\n", argv[2]);
		}
		else if (strcmp(argv[0], "stat") == 0) {
		}
		else if (strcmp(argv[0], "confkey") == 0) {
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "route_res") == 0))
			break;
	}

	if ((cnt_add > 0) || (cnt_del > 0))
		pcb_undo_inc_serial();

	rnd_message(RND_MSG_INFO, "External autorouter: imported %ld objects, removed %ld objets\n", cnt_add, cnt_del);

	return 0;
}


int tedax_route_res_load(const char *fname, const char *blk_id, int silent)
{
	FILE *fn;
	int ret = 0;

	fn = rnd_fopen(&PCB->hidlib, fname, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname);
		return -1;
	}

	ret = tedax_route_res_fload(fn, blk_id, silent);

	fclose(fn);
	return ret;
}


#define LOAD_MINMAX(s) \
do { \
	cfg->min_val = -HUGE_VAL; cfg->max_val = HUGE_VAL; \
	v = strtod(s, &end); \
	if (*end == ':') { \
		cfg->min_val = strtod(s, &end); \
		if (*end == ':') \
			cfg->max_val = strtod(s, &end); \
	} \
} while(0)

void *tedax_route_conf_keys_fload(FILE *fn, const char *blk_id, int silent)
{
	char line[520];
	char *argv[16], *end;
	int argc;
	long start, cnt = 0;
	rnd_export_opt_t *table, *cfg;
	double v;


	if (tedax_seek_hdr(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return NULL;

	if (tedax_seek_block(fn, "route_res", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return NULL;

	start = ftell(fn);
	while((argc = tedax_getline(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if (strcmp(argv[0], "confkey") == 0)
			cnt++;
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "route_res") == 0))
			break;
	}

	cfg = table = calloc(sizeof(rnd_hid_attribute_t), cnt+1);

	fseek(fn, start, SEEK_SET);
	while((argc = tedax_getline(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if ((argc == 5) && (strcmp(argv[0], "confkey") == 0)) {
			cfg->name = rnd_strdup(argv[1]);
			cfg->help_text = rnd_strdup(argv[4]);
			if (strcmp(argv[2], "boolean") == 0) cfg->type = RND_HATT_BOOL;
			else if (strcmp(argv[2], "integer") == 0) cfg->type = RND_HATT_INTEGER;
			else if (strcmp(argv[2], "double") == 0) cfg->type = RND_HATT_REAL;
			else if (strcmp(argv[2], "coord") == 0) cfg->type = RND_HATT_COORD;
			else if (strcmp(argv[2], "string") == 0) cfg->type = RND_HATT_STRING;
			else cfg->type = RND_HATT_LABEL;

			switch(cfg->type) {
				case RND_HATT_BOOL:
					cfg->default_val.lng = rnd_istrue(argv[3]);
					break;
				case RND_HATT_INTEGER:
					LOAD_MINMAX(argv[3]);
					cfg->default_val.lng = v;
					break;
				case RND_HATT_REAL:
					LOAD_MINMAX(argv[3]);
					cfg->default_val.dbl = v;
					break;
				case RND_HATT_COORD:
					LOAD_MINMAX(argv[3]);
					cfg->default_val.crd = RND_MM_TO_COORD(v);
					break;
				case RND_HATT_STRING:
					cfg->default_val.str = rnd_strdup(argv[3]);
					break;
				default:
					break;
			}
			cfg++;
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "route_res") == 0))
			break;
	}

	return table;
}

void *tedax_route_conf_keys_load(const char *fname, const char *blk_id, int silent)
{
	FILE *fn;
	void *ret;

	fn = rnd_fopen(&PCB->hidlib, fname, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname);
		return NULL;
	}

	ret = tedax_route_conf_keys_fload(fn, blk_id, silent);

	fclose(fn);
	return ret;
}

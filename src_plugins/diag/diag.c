/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
 *
 *  This module, diag, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "flag_str.h"
#include "layer.h"
#include "diag_conf.h"
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/conf.h>
#include <librnd/core/error.h>
#include <librnd/core/event.h>
#include "integrity.h"
#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_dad.h>
#include "search.h"
#include "plug_footprint.h"
#include "plug_io.h"
#include "funchash_core.h"
#include "conf_core.h"

conf_diag_t conf_diag;

static const char pcb_acts_DumpConf[] =
	"dumpconf(native, [verbose], [prefix]) - dump the native (binary) config tree to stdout\n"
	"dumpconf(lihata, role, [prefix]) - dump in-memory lihata representation of a config tree\n"
	;
static const char pcb_acth_DumpConf[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *pcb_conf_main_root[];
extern lht_doc_t *pcb_conf_plug_root[];
static fgw_error_t pcb_act_DumpConf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;

	RND_ACT_CONVARG(1, FGW_KEYWORD, DumpConf, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_Native:
		{
			int verbose = 0;
			const char *prefix = "";
			RND_ACT_MAY_CONVARG(2, FGW_INT, DumpConf, verbose = argv[2].val.nat_int);
			RND_ACT_MAY_CONVARG(3, FGW_STR, DumpConf, prefix = argv[3].val.str);
			conf_dump(stdout, prefix, verbose, NULL);
		}
		break;
		case F_Lihata:
		{
		rnd_conf_role_t role;
		const char *srole, *prefix = "";
		RND_ACT_CONVARG(2, FGW_STR, DumpConf, srole = argv[2].val.str);
		RND_ACT_MAY_CONVARG(3, FGW_STR, DumpConf, prefix = argv[3].val.str);
		role = rnd_conf_role_parse(srole);
		if (role == RND_CFR_invalid) {
			rnd_message(RND_MSG_ERROR, "Invalid role: '%s'\n", argv[1]);
			RND_ACT_IRES(1);
			return 0;
		}
		if (pcb_conf_main_root[role] != NULL) {
			printf("%s### main\n", prefix);
			if (pcb_conf_main_root[role] != NULL)
				lht_dom_export(pcb_conf_main_root[role]->root, stdout, prefix);
			printf("%s### plugin\n", prefix);
			if (pcb_conf_plug_root[role] != NULL)
				lht_dom_export(pcb_conf_plug_root[role]->root, stdout, prefix);
		}
		else
			printf("%s <empty>\n", prefix);
		}
		break;
		default:
			RND_ACT_FAIL(DumpConf);
			return 1;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_EvalConf[] =
	"EvalConf(path) - evaluate a config path in different config sources to figure how it ended up in the native database\n"
	;
static const char pcb_acth_EvalConf[] = "Perform various operations on the configuration tree.";

static fgw_error_t pcb_act_EvalConf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;
	rnd_conf_native_t *nat;
	int role;

	RND_ACT_CONVARG(1, FGW_STR, EvalConf, path = argv[1].val.str);

	nat = rnd_conf_get_field(path);
	if (nat == NULL) {
		rnd_message(RND_MSG_ERROR, "EvalConf: invalid path %s - no such config setting\n", path);
		RND_ACT_IRES(-1);
		return 0;
	}

	printf("Conf node %s\n", path);
	for(role = 0; role < RND_CFR_max_real; role++) {
		lht_node_t *n;
		printf(" Role: %s\n", rnd_conf_role_name(role));
		n = rnd_conf_lht_get_at(role, path, 0);
		if (n != NULL) {
			rnd_conf_policy_t pol = -1;
			long prio = rnd_conf_default_prio[role];


			if (rnd_conf_get_policy_prio(n, &pol, &prio) == 0)
				printf("  * policy=%s\n  * prio=%ld\n", rnd_conf_policy_name(pol), prio);

			if (n->file_name != NULL)
				printf("  * from=%s:%d.%d\n", n->file_name, n->line, n->col);
			else
				printf("  * from=(unknown)\n");

			lht_dom_export(n, stdout, "  ");
		}
		else
			printf("  * not present\n");
	}

	printf(" Native:\n");
	rnd_conf_print_native((rnd_conf_pfn)rnd_fprintf, stdout, "  ", 1, nat);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpLayers[] = "dumplayers([all])\n";
static const char pcb_acth_DumpLayers[] = "Print info about each layer";

extern lht_doc_t *pcb_conf_main_root[];
static fgw_error_t pcb_act_DumpLayers(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = -2, g, n, used;
	rnd_layer_id_t arr[128]; /* WARNING: this assumes we won't have more than 128 layers */
	rnd_layergrp_id_t garr[128]; /* WARNING: this assumes we won't have more than 128 layers */

	RND_ACT_MAY_CONVARG(1, FGW_KEYWORD, DumpLayers, op = fgw_keyword(&argv[1]));

	if (op == F_All) {
		printf("Per group:\n");
		for(g = 0; g < PCB->LayerGroups.len; g++) {
			pcb_layergrp_t *grp = &PCB->LayerGroups.grp[g];
			printf(" Group %d: '%s' %x\n", g, grp->name, grp->ltype);
			for(n = 0; n < grp->len; n++) {
				pcb_layer_t *ly = pcb_get_layer(PCB->Data, grp->lid[n]);
				if (ly != NULL) {
					printf("  layer %d: '%s'\n", n, ly->name);
					if (ly->meta.real.grp != g)
						printf("   ERROR: invalid back-link to group: %ld should be %d\n", ly->meta.real.grp, g);
				}
				else
					printf("  layer %d: <invalid>\n", g);
			}
		}

		printf("Per layer:\n");
		for(n = 0; n < PCB->Data->LayerN; n++) {
			pcb_layer_t *ly = &PCB->Data->Layer[n];
			printf(" layer %d: '%s'\n", n, ly->name);
			if (ly->meta.real.grp >= 0) {
				pcb_layergrp_t *grp = &PCB->LayerGroups.grp[ly->meta.real.grp];
				int i, ok = 0;
				for(i = 0; i < grp->len; i++) {
					if (grp->lid[i] == n) {
						ok = 1;
						break;
					}
				}
				if (!ok)
					printf("   ERROR: invalid back-link to group: %ld\n", ly->meta.real.grp);
			}
		}

		RND_ACT_IRES(0);
		return 0;
	}

	printf("Max: theoretical=%d current_board=%d\n", PCB_MAX_LAYER, pcb_max_layer(PCB));
	used = pcb_layer_list_any(PCB, PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL, arr, sizeof(arr)/sizeof(arr[0]));
	for(n = 0; n < used; n++) {
		rnd_layer_id_t layer_id = arr[n];
		rnd_layergrp_id_t grp = pcb_layer_get_group(PCB, layer_id);
		printf(" [%lx] %04x group=%ld %s\n", layer_id, pcb_layer_flags(PCB, layer_id), grp, pcb_layer_name(PCB->Data, layer_id));
	}

	/* query by logical layer: any bottom copper */
	used = pcb_layer_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, arr, sizeof(arr)/sizeof(arr[0]));
	printf("All %d bottom copper layers are:\n", used);
	for(n = 0; n < used; n++) {
		rnd_layer_id_t layer_id = arr[n];
		printf(" [%lx] %s \n", layer_id, PCB->Data->Layer[layer_id].name);
	}

	/* query by groups (physical layers): any copper in group */
	used = pcb_layergrp_list(PCB, PCB_LYT_COPPER, garr, sizeof(garr)/sizeof(garr[0]));
	printf("All %d groups containing copper layers are:\n", used);
	for(g = 0; g < used; g++) {
		rnd_layergrp_id_t group_id = garr[g];
		printf(" group %ld (%d layers)\n", group_id, PCB->LayerGroups.grp[group_id].len);
		for(n = 0; n < PCB->LayerGroups.grp[group_id].len; n++) {
			rnd_layer_id_t layer_id = PCB->LayerGroups.grp[group_id].lid[n];
			printf("  [%lx] %s\n", layer_id, PCB->Data->Layer[layer_id].name);
		}
	}

	RND_ACT_IRES(0);
	return 0;
}


static void print_font(pcb_font_t *f, const char *prefix)
{
	int n, g = 0, gletter = 0, gdigit = 0;
	const char *name;

	/* count valid glyphs and classes */
	for(n = 0; n < PCB_MAX_FONTPOSITION + 1; n++) {
		if (f->Symbol[n].Valid) {
			g++;
			if (isalpha(n)) gletter++;
			if (isdigit(n)) gdigit++;
		}
	}

	name = f->name == NULL ? "<anon>" : f->name;
	rnd_printf("%s: %d %s; dim: %$$mm * %$$mm glyphs: %d (letter: %d, digit: %d)\n", prefix, f->id, name, f->MaxWidth, f->MaxHeight, g, gletter, gdigit);
}

static const char dump_fonts_syntax[] = "dumpfonts()\n";
static const char dump_fonts_help[] = "Print info about fonts";
static fgw_error_t pcb_act_DumpFonts(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	printf("Font summary:\n");
	print_font(&PCB->fontkit.dflt, " Default");
	if (PCB->fontkit.hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&PCB->fontkit.fonts); e; e = htip_next(&PCB->fontkit.fonts, e))
			print_font(e->value, " Extra  ");
	}
	else
		printf(" <no extra font loaded>\n");
	RND_ACT_IRES(0);
	return 0;
}

#ifndef NDEBUG
extern void undo_dump(void);
static const char dump_undo_syntax[] = "dumpfonts()\n";
static const char dump_undo_help[] = "Print info about fonts";
static fgw_error_t pcb_act_DumpUndo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	printf("Undo:\n");
	undo_dump();
	RND_ACT_IRES(0);
	return 0;
}
#endif

typedef enum { /* bitfield */
	DD_DRC = 1,
	DD_COPPER_ONLY = 2
} dd_flags;

static void dump_data(pcb_data_t *data, dd_flags what, int ind, const char *parent)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	ind++;
	for(o = pcb_data_first(&it, data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		const char *type = pcb_obj_type_name(o->type);
		rnd_coord_t cx, cy;

		if ((what & DD_COPPER_ONLY) && (o->type == PCB_OBJ_SUBC))
			goto skip;

		if ((what & DD_COPPER_ONLY) && (o->parent_type == PCB_PARENT_LAYER)) {
			pcb_layer_type_t lyt = pcb_layer_flags_(o->parent.layer);
			if (!(lyt & PCB_LYT_COPPER))
				goto skip;
		}

		cx = (o->BoundingBox.X1+o->BoundingBox.X2)/2;
		cy = (o->BoundingBox.Y1+o->BoundingBox.Y2)/2;
		printf("%*s %s", ind, "", type);
		rnd_printf(" #%ld %mm;%mm ", o->ID, cx, cy);

		if (parent != NULL)
			printf("%s", parent);
		printf("-");
		if (o->term != NULL)
			printf("%s", o->term);

		if (what & DD_DRC)
			printf(" DRC=%c%c", PCB_FLAG_TEST(PCB_FLAG_FOUND, o) ? 'f':'.', PCB_FLAG_TEST(PCB_FLAG_SELECTED, o) ? 's':'.');

		printf("\n");
		
		skip:;
		if (o->type == PCB_OBJ_SUBC)
			dump_data(((pcb_subc_t *)o)->data, what, ind, ((pcb_subc_t *)o)->refdes);
	}
}

static const char dump_data_syntax[] = "dumpdata()\n";
static const char dump_data_help[] = "Dump an aspect of the data";
static fgw_error_t pcb_act_DumpData(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	dd_flags what = DD_DRC | DD_COPPER_ONLY;
	printf("DumpData:\n");
	dump_data(PCB->Data, what, 0, NULL);
	printf("\n");
	RND_ACT_IRES(0);
	return 0;
}

static const char integrity_syntax[] = "integrity()\n";
static const char integrity_help[] = "perform integrirty check on the current board and generate errors if needed";
static fgw_error_t pcb_act_integrity(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_check_integrity(PCB);
	RND_ACT_IRES(0);
	return 0;
}

static int dumpflag_cb(void *ctx, gds_t *s, const char **input)
{
	pcb_flag_bits_t *flag = (pcb_flag_bits_t *)ctx;
	switch(**input) {
		case 'm': (*input)++; rnd_append_printf(s, "%lx", flag->mask); break;
		case 'M': (*input)++; gds_append_str(s, flag->mask_name); break;
		case 'N': (*input)++; gds_append_str(s, flag->name); break;
		case 't': (*input)++; rnd_append_printf(s, "%lx", flag->object_types); break;
		case 'H': (*input)++; gds_append_str(s, flag->name); break;
		default:
			return -1;
	}
	return 0;
}

static const char pcb_acts_dumpflags[] = "dumpflags([fmt])\n";
static const char pcb_acth_dumpflags[] = "dump flags, optionally using the format string provided by the user";
static fgw_error_t pcb_act_dumpflags(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int n;
	const char *default_fmt = "%m (%M %N) for %t:\n  %H\n";
	const char *fmt = default_fmt;

	RND_ACT_MAY_CONVARG(1, FGW_STR, dumpflags, fmt = argv[1].val.str);

	for(n = 0; n < pcb_object_flagbits_len; n++) {
		char *tmp;
		tmp = rnd_strdup_subst(fmt, dumpflag_cb, &pcb_object_flagbits[n], RND_SUBST_PERCENT /*| RND_SUBST_BACKSLASH*/);
		printf("%s", tmp);
		free(tmp);
	}

	RND_ACT_IRES(0);
	return 0;
}

static void ev_ui_post(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{

	if (conf_diag.plugins.diag.auto_integrity) {
		static int cnt = 0;
		if ((cnt++ % 100) == 0) {
			rnd_trace("Number of integrity checks so far: %d\n", cnt);
		}
		pcb_check_integrity(PCB);
	}
}

static const char pcb_acts_d1[] = "d1()\n";
static const char pcb_acth_d1[] = "debug action for development";
static fgw_error_t pcb_act_d1(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpIDs[] = "DumpIDs()\n";
static const char pcb_acth_DumpIDs[] = "Dump the ID hash";
static fgw_error_t pcb_act_DumpIDs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_data_t *data = PCB->Data;
	htip_entry_t *e;

	for(e = htip_first(&data->id2obj); e; e = htip_next(&data->id2obj, e)) {
		pcb_any_obj_t *o = e->value;
		if (o == NULL)
			printf("%ld: NULL\n", e->key);
		else
			printf("%ld: %p %ld %s%s\n", e->key, (void *)o, o->ID, pcb_obj_type_name(o->type), (o->ID == e->key) ? "" : " BROKEN");
	}

	RND_ACT_IRES(0);
	return 0;
}

#include "find.h"
static const char pcb_acts_Find2Perf[] = "find2perf()\n";
static const char pcb_acth_Find2Perf[] = "Measure the peformance of find2.c";
static fgw_error_t pcb_act_Find2Perf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	double from, now, end, duration = 4.0;
	long its = 0, pins = 0;
	pcb_find_t fctx;

	memset(&fctx, 0, sizeof(fctx));

	PCB_SUBC_LOOP(PCB->Data) {
		PCB_PADSTACK_LOOP(subc->data) {
			pins++;
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	rnd_message(RND_MSG_INFO, "Measuring find.c peformance for %f seconds starting from %ld pins...\n", duration, pins);

	from = rnd_dtime();
	end = from + duration;
	do {
		PCB_SUBC_LOOP(PCB->Data) {
			PCB_PADSTACK_LOOP(subc->data) {
				pcb_find_from_obj(&fctx, PCB->Data, (pcb_any_obj_t *)padstack);
				pcb_find_free(&fctx);
			}
			PCB_END_LOOP;
		}
		PCB_END_LOOP;
		its++;
		now = rnd_dtime();
	} while(now < end);
	rnd_message(RND_MSG_INFO, "find2.c peformance: %d %f pin find per second\n", its, (double)its * (double)pins / (now-from));
	RND_ACT_IRES(0);
	return 0;
}


#define DLF_PREFIX "<DumpLibFootprint> "
#define SCRATCH pcb_buffers[PCB_MAX_BUFFER-1]
static const char pcb_acts_DumpLibFootprint[] = "DumpLibFootprint(footprintname, [bbox|origin])\n";
static const char pcb_acth_DumpLibFootprint[] = "print footprint file and metadata to stdout";
static fgw_error_t pcb_act_DumpLibFootprint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fpn, *opt;
	FILE *f;
	pcb_fp_fopen_ctx_t fctx;
	int n, want_bbox = 0, want_origin = 0;

	RND_ACT_CONVARG(1, FGW_STR, DumpLibFootprint, fpn = argv[1].val.str);

	for(n = 2; n < argc; n++) {
		RND_ACT_CONVARG(n, FGW_STR, DumpLibFootprint, opt = argv[n].val.str);
		if (strcmp(opt, "bbox") == 0) want_bbox = 1;
		else if (strcmp(opt, "origin") == 0) want_origin = 1;
		else RND_ACT_FAIL(DumpLibFootprint);
	}

	f = pcb_fp_fopen(&conf_core.rc.library_search_paths, fpn, &fctx, PCB->Data);
	if ((f != PCB_FP_FOPEN_IN_DST) && (f != NULL)) {

		/* dump file content */
		printf(DLF_PREFIX "data begin\n");
		while(!feof(f)) {
			char buff[1024];
			int len = fread(buff, 1, sizeof(buff), f);
			if (len > 0)
				fwrite(buff, 1, len, stdout);
		}
		printf(DLF_PREFIX "data end\n");
		pcb_fp_fclose(f, &fctx);

		/* print exrtas */
		if (want_bbox || want_origin) {
			pcb_buffer_clear(PCB, &SCRATCH);
			if (!pcb_buffer_load_footprint(&SCRATCH, fctx.filename, NULL)) {
				RND_ACT_IRES(1);
				return 0;
			}
		}

		if (want_bbox)
			rnd_printf(DLF_PREFIX "bbox mm %mm %mm %mm %mm\n", SCRATCH.BoundingBox.X1, SCRATCH.BoundingBox.Y1, SCRATCH.BoundingBox.X2, SCRATCH.BoundingBox.Y2);
		if (want_origin)
			rnd_printf(DLF_PREFIX "origin mm %mm %mm\n", SCRATCH.X, SCRATCH.Y);

		RND_ACT_IRES(0);
	}
	else {
		pcb_fp_fclose(f, &fctx);
		printf(DLF_PREFIX "error file not found\n");
		RND_ACT_IRES(1);
	}

	return 0;
}

#define	PCB_FORCECOLOR_TYPES        \
	(PCB_OBJ_PSTK | PCB_OBJ_TEXT | PCB_OBJ_SUBC | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_SUBC_PART | PCB_OBJ_SUBC | PCB_OBJ_RAT)

static const char pcb_acts_forcecolor[] = "forcecolor(#RRGGBB)\n";
static const char pcb_acth_forcecolor[] = "change selected objects' color to #RRGGBB, reset if does not start with '#'";
static fgw_error_t pcb_act_forcecolor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t x, y;
	int type;
	void *ptr1, *ptr2, *ptr3;
	const char *new_color;

	RND_ACT_CONVARG(1, FGW_STR, forcecolor, new_color = argv[1].val.str);

	rnd_hid_get_coords("Click on object to change", &x, &y, 0);

	if ((type = pcb_search_screen(x, y, PCB_FORCECOLOR_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
		pcb_any_obj_t *o = (pcb_any_obj_t *)ptr2;
		if (o->override_color == NULL)
			o->override_color = malloc(sizeof(rnd_color_t));
		rnd_color_load_str(o->override_color, new_color);
	}

	RND_ACT_IRES(0);
	return 0;
}

rnd_action_t diag_action_list[] = {
	{"dumpconf", pcb_act_DumpConf, pcb_acth_DumpConf, pcb_acts_DumpConf},
	{"dumplayers", pcb_act_DumpLayers, pcb_acth_DumpLayers, pcb_acts_DumpLayers},
	{"dumpfonts", pcb_act_DumpFonts, dump_fonts_help, dump_fonts_syntax},
	{"dumpdata", pcb_act_DumpData, dump_data_help, dump_data_syntax},
#ifndef NDEBUG
	{"dumpundo", pcb_act_DumpUndo, dump_undo_help, dump_undo_syntax},
#endif
	{"EvalConf", pcb_act_EvalConf, pcb_acth_EvalConf, pcb_acts_EvalConf},
	{"d1", pcb_act_d1, pcb_acth_d1, pcb_acts_d1},
	{"find2perf", pcb_act_Find2Perf, pcb_acth_Find2Perf, pcb_acts_Find2Perf},
	{"integrity", pcb_act_integrity, integrity_help, integrity_syntax},
	{"dumpflags", pcb_act_dumpflags, pcb_acth_dumpflags, pcb_acts_dumpflags},
	{"dumpids", pcb_act_DumpIDs, pcb_acth_DumpIDs, pcb_acts_DumpIDs},
	{"DumpLibFootprint", pcb_act_DumpLibFootprint, pcb_acth_DumpLibFootprint, pcb_acts_DumpLibFootprint},
	{"forcecolor", pcb_act_forcecolor, pcb_acth_forcecolor, pcb_acts_forcecolor}
};

static const char *diag_cookie = "diag plugin";

int pplg_check_ver_diag(int ver_needed) { return 0; }

void pplg_uninit_diag(void)
{
	rnd_remove_actions_by_cookie(diag_cookie);
	rnd_conf_unreg_fields("plugins/diag/");
	rnd_event_unbind_allcookie(diag_cookie);
}

int pplg_init_diag(void)
{
	RND_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_diag, field,isarray,type_name,cpath,cname,desc,flags);
#include "diag_conf_fields.h"

	rnd_event_bind(RND_EVENT_USER_INPUT_POST, ev_ui_post, NULL, diag_cookie);
	RND_REGISTER_ACTIONS(diag_action_list, diag_cookie)
	return 0;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "flag_str.h"
#include "layer.h"
#include "diag_conf.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "conf.h"
#include "error.h"
#include "event.h"
#include "integrity.h"
#include "hid.h"
#include "hid_attrib.h"
#include "hid_dad.h"
#include "search.h"
#include "macro.h"

conf_diag_t conf_diag;

static const char dump_conf_syntax[] =
	"dumpconf(native, [verbose], [prefix]) - dump the native (binary) config tree to stdout\n"
	"dumpconf(lihata, role, [prefix]) - dump in-memory lihata representation of a config tree\n"

	;

static const char dump_conf_help[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_main_root[];
extern lht_doc_t *conf_plug_root[];
static int pcb_act_DumpConf(int argc, const char **argv)
{
	const char *cmd = argc > 0 ? argv[0] : NULL;

	if (PCB_NSTRCMP(cmd, "native") == 0) {
		int verbose;
		const char *prefix = "";
		if (argc > 1)
			verbose = atoi(argv[1]);
		if (argc > 2)
			prefix = argv[2];
		conf_dump(stdout, prefix, verbose, NULL);
	}

	else if (PCB_NSTRCMP(cmd, "lihata") == 0) {
		conf_role_t role;
		const char *prefix = "";
		if (argc <= 1) {
			pcb_message(PCB_MSG_ERROR, "conf(dumplht) needs a role\n");
			return 1;
		}
		role = conf_role_parse(argv[1]);
		if (role == CFR_invalid) {
			pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'\n", argv[1]);
			return 1;
		}
		if (argc > 2)
			prefix = argv[2];
		if (conf_main_root[role] != NULL) {
			printf("%s### main\n", prefix);
			if (conf_main_root[role] != NULL)
				lht_dom_export(conf_main_root[role]->root, stdout, prefix);
			printf("%s### plugin\n", prefix);
			if (conf_plug_root[role] != NULL)
				lht_dom_export(conf_plug_root[role]->root, stdout, prefix);
		}
		else
			printf("%s <empty>\n", prefix);
	}

	else {
		pcb_message(PCB_MSG_ERROR, "Invalid conf command '%s'\n", argv[0]);
		return 1;
	}
	return 0;
}

static const char eval_conf_syntax[] =
	"EvalConf(path) - evaluate a config path in different config sources to figure how it ended up in the native database\n"
	;

static const char eval_conf_help[] = "Perform various operations on the configuration tree.";

static int pcb_act_EvalConf(int argc, const char **argv)
{
	const char *path = argc > 0 ? argv[0] : NULL;
	conf_native_t *nat;
	int role;

	if (path == NULL) {
		pcb_message(PCB_MSG_ERROR, "EvalConf needs a path\n");
		return 1;
	}

	nat = conf_get_field(path);
	if (nat == NULL) {
		pcb_message(PCB_MSG_ERROR, "EvalConf: invalid path %s - no such config setting\n", path);
		return 1;
	}

	printf("Conf node %s\n", path);
	for(role = 0; role < CFR_max_real; role++) {
		lht_node_t *n;
		printf(" Role: %s\n", conf_role_name(role));
		n = conf_lht_get_at(role, path, 0);
		if (n != NULL) {
			conf_policy_t pol = -1;
			long prio = conf_default_prio[role];


			if (conf_get_policy_prio(n, &pol, &prio) == 0)
				printf("  * policy=%s\n  * prio=%ld\n", conf_policy_name(pol), prio);

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
	conf_print_native((conf_pfn)pcb_fprintf, stdout, "  ", 1, nat);

	return 0;
}

static const char dump_layers_syntax[] =
	"dumplayers([all])\n"
	;

static const char dump_layers_help[] = "Print info about each layer";

extern lht_doc_t *conf_main_root[];
static int pcb_act_DumpLayers(int argc, const char **argv)
{
	int g, n, used;
	pcb_layer_id_t arr[128]; /* WARNING: this assumes we won't have more than 128 layers */
	pcb_layergrp_id_t garr[128]; /* WARNING: this assumes we won't have more than 128 layers */

	if ((argc > 0) && (strcmp(argv[0], "all") == 0)) {
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

		return 0;
	}

	printf("Max: theoretical=%d current_board=%d\n", PCB_MAX_LAYER+2, pcb_max_layer);
	used = pcb_layer_list_any(PCB, PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL, arr, sizeof(arr)/sizeof(arr[0]));
	for(n = 0; n < used; n++) {
		pcb_layer_id_t layer_id = arr[n];
		pcb_layergrp_id_t grp = pcb_layer_get_group(PCB, layer_id);
		printf(" [%lx] %04x group=%ld %s\n", layer_id, pcb_layer_flags(PCB, layer_id), grp, pcb_layer_name(PCB->Data, layer_id));
	}

	/* query by logical layer: any bottom copper */
	used = pcb_layer_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, arr, sizeof(arr)/sizeof(arr[0]));
	printf("All %d bottom copper layers are:\n", used);
	for(n = 0; n < used; n++) {
		pcb_layer_id_t layer_id = arr[n];
		printf(" [%lx] %s \n", layer_id, PCB->Data->Layer[layer_id].name);
	}

	/* query by groups (physical layers): any copper in group */
	used = pcb_layergrp_list(PCB, PCB_LYT_COPPER, garr, sizeof(garr)/sizeof(garr[0]));
	printf("All %d groups containing copper layers are:\n", used);
	for(g = 0; g < used; g++) {
		pcb_layergrp_id_t group_id = garr[g];
		printf(" group %ld (%d layers)\n", group_id, PCB->LayerGroups.grp[group_id].len);
		for(n = 0; n < PCB->LayerGroups.grp[group_id].len; n++) {
			pcb_layer_id_t layer_id = PCB->LayerGroups.grp[group_id].lid[n];
			printf("  [%lx] %s\n", layer_id, PCB->Data->Layer[layer_id].name);
		}
	}

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
	pcb_printf("%s: %d %s; dim: %$$mm * %$$mm glyphs: %d (letter: %d, digit: %d)\n", prefix, f->id, name, f->MaxWidth, f->MaxHeight, g, gletter, gdigit);
}

static const char dump_fonts_syntax[] = "dumpfonts()\n";
static const char dump_fonts_help[] = "Print info about fonts";
static int pcb_act_DumpFonts(int argc, const char **argv)
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
	return 0;
}

#ifndef NDEBUG
extern void undo_dump(void);
static const char dump_undo_syntax[] = "dumpfonts()\n";
static const char dump_undo_help[] = "Print info about fonts";
static int pcb_act_DumpUndo(int argc, const char **argv)
{
	printf("Undo:\n");
	undo_dump();
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
		pcb_coord_t cx, cy;

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
		pcb_printf(" #%ld %mm;%mm ", o->ID, cx, cy);

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
static int pcb_act_DumpData(int argc, const char **argv)
{
	dd_flags what = DD_DRC | DD_COPPER_ONLY;
	printf("DumpData:\n");
	dump_data(PCB->Data, what, 0, NULL);
	printf("\n");
	return 0;
}

static const char integrity_syntax[] = "integrity()\n";
static const char integrity_help[] = "perform integrirty check on the current board and generate errors if needed";
static int pcb_act_integrity(int argc, const char **argv)
{
	pcb_check_integrity(PCB);
	return 0;
}

static int dumpflag_cb(void *ctx, gds_t *s, const char **input)
{
	pcb_flag_bits_t *flag = (pcb_flag_bits_t *)ctx;
	switch(**input) {
		case 'm': (*input)++; pcb_append_printf(s, "%lx", flag->mask); break;
		case 'M': (*input)++; gds_append_str(s, flag->mask_name); break;
		case 'N': (*input)++; gds_append_str(s, flag->name); break;
		case 't': (*input)++; pcb_append_printf(s, "%lx", flag->object_types); break;
		case 'H': (*input)++; gds_append_str(s, flag->name); break;
		default:
			return -1;
	}
	return 0;
}

static const char dumpflags_syntax[] = "dumpflags([fmt])\n";
static const char dumpflags_help[] = "dump flags, optionally using the format string provided by the user";
static int pcb_act_dumpflags(int argc, const char **argv)
{
	int n;
	const char *default_fmt = "%m (%M %N) for %t:\n  %H\n";
	const char *fmt;

	if (argc > 0)
		fmt = argv[0];
	else
		fmt = default_fmt;

	for(n = 0; n < pcb_object_flagbits_len; n++) {
		char *tmp;
		tmp = pcb_strdup_subst(fmt, dumpflag_cb, &pcb_object_flagbits[n], PCB_SUBST_PERCENT /*| PCB_SUBST_BACKSLASH*/);
		printf("%s", tmp);
		free(tmp);
	}

	return 0;
}

static void ev_ui_post(void *user_data, int argc, pcb_event_arg_t argv[])
{

	if (conf_diag.plugins.diag.auto_integrity) {
		static int cnt = 0;
		if ((cnt++ % 100) == 0) {
			pcb_trace("Number of integrity checks so far: %d\n", cnt);
		}
		pcb_check_integrity(PCB);
	}
}

static const char d1_syntax[] = "d1()\n";
static const char d1_help[] = "debug action for development";
static int pcb_act_d1(int argc, const char **argv)
{
	printf("D1!\n");
	return 0;
}

#define	PCB_FORCECOLOR_TYPES        \
	(PCB_OBJ_PSTK | PCB_OBJ_TEXT | PCB_OBJ_SUBC | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_SUBC_PART | PCB_OBJ_SUBC | PCB_OBJ_RAT)

static const char forcecolor_syntax[] = "forcecolor(#RRGGBB)\n";
static const char forcecolor_help[] = "change selected objects' color to #RRGGBB, reset if does not start with '#'";
static int pcb_act_forcecolor(int argc, const char **argv)
{
	pcb_coord_t x, y;
	int type;
	void *ptr1, *ptr2, *ptr3;

	const char *new_color = PCB_ACTION_ARG(0);

	pcb_hid_get_coords("Click on object to change", &x, &y);

	if ((type = pcb_search_screen(x, y, PCB_FORCECOLOR_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID){
		strncpy(((pcb_any_obj_t *)ptr2)->override_color, new_color, sizeof(((pcb_any_obj_t *)ptr2)->override_color)-1);
	}
	return 0;
}

pcb_hid_action_t diag_action_list[] = {
	{"dumpconf", 0, pcb_act_DumpConf,
	 dump_conf_help, dump_conf_syntax},
	{"dumplayers", 0, pcb_act_DumpLayers,
	 dump_layers_help, dump_layers_syntax},
	{"dumpfonts", 0, pcb_act_DumpFonts,
	 dump_fonts_help, dump_fonts_syntax},
	{"dumpdata", 0, pcb_act_DumpData,
	 dump_data_help, dump_data_syntax},
#ifndef NDEBUG
	{"dumpundo", 0, pcb_act_DumpUndo,
	 dump_undo_help, dump_undo_syntax},
#endif
	{"EvalConf", 0, pcb_act_EvalConf,
	 eval_conf_help, eval_conf_syntax},
	{"d1", 0, pcb_act_d1,
	 d1_help, d1_syntax},
	{"integrity", 0, pcb_act_integrity,
	 integrity_help, integrity_syntax},
	{"dumpflags", 0, pcb_act_dumpflags,
	 dumpflags_help, dumpflags_syntax},
	{"forcecolor", 0, pcb_act_forcecolor,
	 forcecolor_help, forcecolor_syntax}
};

static const char *diag_cookie = "diag plugin";

PCB_REGISTER_ACTIONS(diag_action_list, diag_cookie)

int pplg_check_ver_diag(int ver_needed) { return 0; }

void pplg_uninit_diag(void)
{
	pcb_hid_remove_actions_by_cookie(diag_cookie);
	conf_unreg_fields("plugins/diag/");
	pcb_event_unbind_allcookie(diag_cookie);
}

#include "dolists.h"
int pplg_init_diag(void)
{
	PCB_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_diag, field,isarray,type_name,cpath,cname,desc,flags);
#include "diag_conf_fields.h"

	pcb_event_bind(PCB_EVENT_USER_INPUT_POST, ev_ui_post, NULL, diag_cookie);
	PCB_REGISTER_ACTIONS(diag_action_list, diag_cookie)
	return 0;
}

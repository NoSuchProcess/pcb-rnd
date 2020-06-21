/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2018,2019 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include "board.h"
#include "data.h"
#include <librnd/core/plugins.h>
#include "config.h"
#include "props.h"
#include "propsel.h"
#include "propdlg.h"
#include <librnd/core/actions.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/error.h>
#include "layer.h"
#include "layer_addr.h"
#include "layer_grp.h"
#include "search.h"
#include "crosshair.h"
#include <librnd/core/compat_misc.h>

extern rnd_layergrp_id_t pcb_actd_EditGroup_gid;

int prop_scope_add(pcb_propedit_t *pe, const char *cmd, int quiet)
{
	pcb_idpath_t *idp;
	long id;
	void *o2;


	if (strcmp(cmd, "subc") == 0) {
		void *o1, *o3;
		pcb_objtype_t type = pcb_search_obj_by_location(PCB_OBJ_SUBC, &o1, &o2, &o3, pcb_crosshair.X, pcb_crosshair.Y, PCB_SLOP);
		if (type == 0) {
			if (!quiet)
				rnd_message(RND_MSG_ERROR, "No object under the cursor\n");
			return FGW_ERR_ARG_CONV;
		}
		goto object_scope;
	}
	if (strncmp(cmd, "object", 6) == 0) {
		if (cmd[6] == ':') {
			idp = pcb_str2idpath(pe->pcb, cmd+7);
			if (idp == NULL) {
				if (!quiet)
					rnd_message(RND_MSG_ERROR, "Failed to convert object ID: '%s'\n", cmd+7);
				return FGW_ERR_ARG_CONV;
			}
			pcb_idpath_list_append(&pe->objs, idp);
		}
		else {
			void *o1, *o3;
			pcb_objtype_t type;

			type = pcb_search_obj_by_location(PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &o1, &o2, &o3, pcb_crosshair.X, pcb_crosshair.Y, PCB_SLOP);
			if (type == 0)
				type = pcb_search_obj_by_location(PCB_OBJ_CLASS_REAL | PCB_LOOSE_SUBC(pe->pcb), &o1, &o2, &o3, pcb_crosshair.X, pcb_crosshair.Y, PCB_SLOP);
			if (type == 0) {
				if (!quiet)
					rnd_message(RND_MSG_ERROR, "No object under the cursor\n");
				return FGW_ERR_ARG_CONV;
			}
			object_scope:;
			idp = pcb_obj2idpath(o2);
			if (idp == NULL) {
				if (!quiet)
					rnd_message(RND_MSG_ERROR, "Object under the cursor has no idpath\n");
				return FGW_ERR_ARG_CONV;
			}
			pcb_idpath_list_append(&pe->objs, idp);
		}
	}
	else if (strncmp(cmd, "layergrp", 8) == 0) {
		if (cmd[8] == ':') {
			if (strcmp(cmd+9, "editgroup") != 0) {
				id = pcb_layergrp_str2id(pe->pcb, cmd+9);
				if (id < 0) {
					if (!quiet)
						rnd_message(RND_MSG_ERROR, "Invalid layergrp ID '%s'\n", cmd+9);
					return FGW_ERR_ARG_CONV;
				}
			}
			else
				id = pcb_actd_EditGroup_gid;
			vtl0_append(&pe->layergrps, id);
		}
		else {
			vtl0_append(&pe->layergrps, PCB_CURRLAYER(PCB)->meta.real.grp);
		}
	}
	else if (strncmp(cmd, "layer", 5) == 0) {
		if (cmd[5] == ':') {
			id = pcb_layer_str2id(pe->data, cmd+6);
			if (id < 0) {
				if (!quiet)
					rnd_message(RND_MSG_ERROR, "Invalid layer ID '%s'\n", cmd+6);
				return FGW_ERR_ARG_CONV;
			}
			vtl0_append(&pe->layers, id);
		}
		else {
			vtl0_append(&pe->layers, PCB_CURRLID(pe->pcb));
		}
	}
	else if (strncmp(cmd, "net:", 4) == 0) {
		char *name = rnd_strdup(cmd+4);
		if (!pe->nets_inited) {
			htsp_init(&pe->nets, strhash, strkeyeq);
			pe->nets_inited = 1;
		}
		htsp_set(&pe->nets, name, name);
	}
	else if ((strcmp(cmd, "board") == 0) || (strcmp(cmd, "pcb") == 0))
		pe->board = 1;
	else if (strcmp(cmd, "selection") == 0)
		pe->selection = 1;
	else if (strcmp(cmd, "geo") == 0)
		pe->geo = 1;
	else {
		char *end;
		pcb_idpath_t *idp = (void *)strtol(cmd, &end, 0);
		if ((*end == '\0') && (idp != NULL) && (htpp_get(&rnd_fgw.ptr_tbl, idp) == RND_PTR_DOMAIN_IDPATH)) {
			pcb_idpath_list_append(&pe->objs, idp);
			return 0;
		}
		if (!quiet)
			rnd_message(RND_MSG_ERROR, "Invalid scope: %s\n", cmd);
		return FGW_ERR_ARG_CONV;
	}
	return 0;
}

/* pair of prop_scope_add() - uninits the scope */
static void prop_scope_finish(pcb_propedit_t *pe)
{
	pcb_idpath_t *idp;

	/* need to remove idpaths from the scope list, else it won't be possible
	   to add them again */
	while((idp = pcb_idpath_list_first(&pe->objs)) != NULL)
		pcb_idpath_list_remove(idp);

	if (pe->nets_inited) {
		htsp_entry_t *e;
		for(e = htsp_first(&pe->nets); e != NULL; e = htsp_next(&pe->nets, e))
			free(e->key);
		htsp_uninit(&pe->nets);
		pe->nets_inited = 0;
	}
}

extern rnd_layergrp_id_t pcb_actd_EditGroup_gid;

static const char pcb_acts_propset[] = "propset([scope], name, value)";
static const char pcb_acth_propset[] = "Change the named property of scope or all selected objects to/by value. Scope is documented at PropEdit().";
fgw_error_t pcb_act_propset(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *first, *name, *val;
	pcb_propedit_t ctx;

	pcb_props_init(&ctx, PCB);

	RND_ACT_CONVARG(1, FGW_STR, propset, first = argv[1].val.str);
	if (prop_scope_add(&ctx, first, 1) == 0) {
		RND_ACT_CONVARG(2, FGW_STR, propset, name = argv[2].val.str);
		RND_ACT_CONVARG(3, FGW_STR, propset, val = argv[3].val.str);
	}
	else {
		name = first;
		ctx.selection = 1;
		RND_ACT_CONVARG(2, FGW_STR, propset, val = argv[2].val.str);
	}

	RND_ACT_IRES(pcb_propsel_set_str(&ctx, name, val));

	prop_scope_finish(&ctx);
	pcb_props_uninit(&ctx);
	return 0;
}

static const char pcb_acts_proptoggle[] = "proptoggle([scope], name, [create])";
static const char pcb_acth_proptoggle[] = "Toggle the named property of scope or all selected objects, assuming the property is boolean. Scope is documented at PropEdit(). If create is true, non-existing attributes are created as true.";
fgw_error_t pcb_act_proptoggle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *first, *name, *create = NULL;
	pcb_propedit_t ctx;

	pcb_props_init(&ctx, PCB);

	RND_ACT_CONVARG(1, FGW_STR, proptoggle, first = argv[1].val.str);
	if (prop_scope_add(&ctx, first, 1) == 0) {
		RND_ACT_CONVARG(2, FGW_STR, proptoggle, name = argv[2].val.str);
		RND_ACT_MAY_CONVARG(3, FGW_STR, proptoggle, create = argv[3].val.str);
	}
	else {
		name = first;
		RND_ACT_MAY_CONVARG(2, FGW_STR, proptoggle, create = argv[2].val.str);
		ctx.selection = 1;
	}

	RND_ACT_IRES(pcb_propsel_toggle(&ctx, name, rnd_istrue(create)));

	prop_scope_finish(&ctx);
	pcb_props_uninit(&ctx);
	return 0;
}

static const char pcb_acts_propget[] = "propget([scope], name, [stattype])";
static const char pcb_acth_propget[] = "Return the named property of scope or all selected objects to/by value. Scope is documented at PropEdit().";
fgw_error_t pcb_act_propget(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *first, *name, *stty = "common";
	pcb_propedit_t ctx;
	pcb_props_t *p;
	pcb_propval_t *out, most_common, min, max, avg;
	htprop_entry_t *e;
	int r;

	pcb_props_init(&ctx, PCB);

	RND_ACT_CONVARG(1, FGW_STR, propget, first = argv[1].val.str);
	if (prop_scope_add(&ctx, first, 1) == 0) {
		RND_ACT_CONVARG(2, FGW_STR, propget, name = argv[2].val.str);
		RND_ACT_MAY_CONVARG(3, FGW_STR, propget, stty = argv[3].val.str);
	}
	else {
		name = first;
		ctx.selection = 1;
		RND_ACT_CONVARG(2, FGW_STR, propget, stty = argv[2].val.str);
	}

	ctx.geo = 1;
	pcb_propsel_map_core(&ctx);

	p = htsp_get(&ctx.props, name);
	if (p == NULL)
		goto error;

	if (p->type == PCB_PROPT_STRING)
		r = pcb_props_stat(&ctx, p, &most_common, NULL, NULL, NULL);
	else
		r = pcb_props_stat(&ctx, p, &most_common, &min, &max, &avg);

	if (r != 0)
		goto error;

	if (strcmp(stty, "common") == 0) out = &most_common;
	else if (strcmp(stty, "min") == 0) out = &min;
	else if (strcmp(stty, "max") == 0) out = &max;
	else if (strcmp(stty, "avg") == 0) out = &avg;
	else goto error;

	switch(p->type) {
		case PCB_PROPT_STRING:
			if (out != &most_common)
				goto error;

			/* get the first one for now */
			e = htprop_first(&p->values);
			most_common = e->key;
			res->type = FGW_STR | FGW_DYN;
			res->val.str = rnd_strdup(out->string == NULL ? "" : out->string);
			break;
		case PCB_PROPT_COORD: res->type = FGW_COORD; fgw_coord(res) = out->coord; break;
		case PCB_PROPT_ANGLE: res->type = FGW_DOUBLE; res->val.nat_double = out->angle; break;
		case PCB_PROPT_DOUBLE: res->type = FGW_DOUBLE; res->val.nat_double = out->d; break;
		case PCB_PROPT_BOOL:
		case PCB_PROPT_INT:   res->type = FGW_INT; res->val.nat_int = out->i; break;
		case PCB_PROPT_COLOR: res->type = FGW_STR | FGW_DYN; res->val.str = rnd_strdup(out->clr.str); break;
		case PCB_PROPT_invalid:
		case PCB_PROPT_max: goto error;
	}
	prop_scope_finish(&ctx);
	pcb_props_uninit(&ctx);
	return 0;

	error:;
	prop_scope_finish(&ctx);
	pcb_props_uninit(&ctx);
	return FGW_ERR_ARG_CONV;
}

static const char pcb_acts_propprint[] = "PropPrint([scope])";
static const char pcb_acth_propprint[] = "Print a property map of objects matching the scope. Scope is documented at PropEdit().";
fgw_error_t pcb_act_propprint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *scope = NULL;
	pcb_propedit_t ctx;
	htsp_entry_t *e, *sorted;

	pcb_props_init(&ctx, PCB);

	RND_ACT_MAY_CONVARG(1, FGW_STR, propprint, scope = argv[1].val.str);
	if (scope != NULL) {
		if (prop_scope_add(&ctx, scope, 0) != 0)
			return FGW_ERR_ARG_CONV;
	}
	else
		ctx.selection = 1;

	pcb_propsel_map_core(&ctx);
	sorted = pcb_props_sort(&ctx);
	for(e = sorted; e->key != NULL; e++) {
		pcb_props_t *p = e->value;
		pcb_propval_t com, min, max, avg;

		printf("%s\n", e->key);
		if (p->type == PCB_PROPT_STRING)
			pcb_props_stat(&ctx, p, &com, NULL, NULL, NULL);
		else
			pcb_props_stat(&ctx, p, &com, &min, &max, &avg);
		switch(p->type) {
			case PCB_PROPT_STRING: printf("	common='%s'\n", com.string); break;
			case PCB_PROPT_COORD:
				rnd_printf("	common='%$$mm'\n", com.coord);
				rnd_printf("	min/avg/max=%$$mm/%$$mm/%$$mm\n", min.coord, avg.coord, max.coord);
				break;
			case PCB_PROPT_ANGLE:
				rnd_printf("	common='%f'\n", com.angle);
				rnd_printf("	min/avg/max=%f/%f/%f\n", min.angle, avg.angle, max.angle);
				break;
			case PCB_PROPT_DOUBLE:
				rnd_printf("	common='%f'\n", com.d);
				rnd_printf("	min/avg/max=%f/%f/%f\n", min.d, avg.d, max.d);
				break;
			case PCB_PROPT_INT:
			case PCB_PROPT_BOOL:
				rnd_printf("	common='%d'\n", com.i);
				rnd_printf("	min/avg/max=%d/%d/%d\n", min.i, avg.i, max.i);
				break;
			case PCB_PROPT_COLOR: printf("	common=#%02d%02d%02d\n", com.clr.r, com.clr.g, com.clr.b); break;
			case PCB_PROPT_invalid:
			case PCB_PROPT_max:
				break;
		}
	}
	free(sorted);

	prop_scope_finish(&ctx);
	pcb_props_uninit(&ctx);
	RND_ACT_IRES(0);
	return 0;
}

static const char *propedit_cookie = "propedit";

rnd_action_t propedit_action_list[] = {
	{"propedit", pcb_act_propedit, pcb_acth_propedit, pcb_acts_propedit},
	{"propprint", pcb_act_propprint, pcb_acth_propprint, pcb_acts_propprint},
	{"propset", pcb_act_propset, pcb_acth_propset, pcb_acts_propset},
	{"proptoggle", pcb_act_proptoggle, pcb_acth_proptoggle, pcb_acts_proptoggle},
	{"propget", pcb_act_propget, pcb_acth_propget, pcb_acts_propget}
};

int pplg_check_ver_propedit(int ver_needed) { return 0; }

void pplg_uninit_propedit(void)
{
	pcb_propdlg_uninit();
	rnd_remove_actions_by_cookie(propedit_cookie);
}

int pplg_init_propedit(void)
{
	RND_API_CHK_VER;

	if (sizeof(long) < sizeof(rnd_layer_id_t)) {
		rnd_message(RND_MSG_ERROR, "can't load propedig: layer id type wider than long\n");
		return -1;
	}

	if (sizeof(long) < sizeof(rnd_layergrp_id_t)) {
		rnd_message(RND_MSG_ERROR, "can't load propedig: layergrp id type wider than long\n");
		return -1;
	}

	RND_REGISTER_ACTIONS(propedit_action_list, propedit_cookie)
	pcb_propdlg_init();
	return 0;
}

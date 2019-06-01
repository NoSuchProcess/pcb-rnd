/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
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

/* Query language - actions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "actions.h"
#include "query.h"
#include "query_y.h"
#include "query_exec.h"
#include "query_access.h"
#include "draw.h"
#include "select.h"
#include "board.h"
#include "macro.h"
#include "idpath.h"
#include "compat_misc.h"

static const char pcb_acts_query[] =
	"query(dump, expr) - dry run: compile and dump an expression\n"
	"query(append, idplist, expr) - compile and run expr and append the idpath of resulting objects on idplist\n"
	;
static const char pcb_acth_query[] = "Perform various queries on PCB data.";

typedef struct {
	int trues, falses;
} eval_stat_t;

static void eval_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	char *resv;
	eval_stat_t *st = (eval_stat_t *)user_ctx;
	int t;

	if (res->type == PCBQ_VT_VOID) {
		printf(" <void>\n");
		st->falses++;
		return;
	}

	if (res->type != PCBQ_VT_COORD) {
		st->trues++;
		resv = pcb_query_sprint_val(res);
		printf(" %s\n", resv);
		free(resv);
		return;
	}

	t = pcb_qry_is_true(res);
	printf(" %s", t ? "true" : "false");
	if (t) {
		resv = pcb_query_sprint_val(res);
		printf(" (%s)\n", resv);
		free(resv);
		st->trues++;
	}
	else {
		printf("\n");
		st->falses++;
	}
}

typedef struct {
	pcb_cardinal_t cnt;
	pcb_change_flag_t how;
} select_t;

static void select_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	select_t *sel = (select_t *)user_ctx;
	if (!pcb_qry_is_true(res))
		return;
	if (PCB_OBJ_IS_CLASS(current->type, PCB_OBJ_CLASS_OBJ)) {
		int state_wanted = (sel->how == PCB_CHGFLG_SET);
		int state_is     = PCB_FLAG_TEST(PCB_FLAG_SELECTED, current);
		if (state_wanted != state_is) {
			PCB_FLAG_CHANGE(sel->how, PCB_FLAG_SELECTED, current);
			sel->cnt++;
		}
	}
}

static void append_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	pcb_idpath_list_t *list = user_ctx;
	pcb_idpath_t *idp;

	if (!pcb_qry_is_true(res))
		return;
	if (!PCB_OBJ_IS_CLASS(current->type, PCB_OBJ_CLASS_OBJ))
		return;

	idp = pcb_obj2idpath(current);
	pcb_idpath_list_append(list, idp);
}

static int run_script(const char *script, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current), void *user_ctx)
{
	pcb_qry_node_t *prg = NULL;

	pcb_qry_set_input(script);
	qry_parse(&prg);

	if (prg == NULL) {
		pcb_message(PCB_MSG_ERROR, "Compilation error.\n");
		return -1;
	}

	return pcb_qry_run(prg, cb, user_ctx);
}

static fgw_error_t pcb_act_query(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *arg = NULL;
	select_t sel;

	sel.cnt = 0;

	PCB_ACT_CONVARG(1, FGW_STR, query, cmd = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);

	if (strcmp(cmd, "version") == 0) {
		PCB_ACT_IRES(0100); /* 1.0 */
		return 0;
	}

	if (strcmp(cmd, "dump") == 0) {
		pcb_qry_node_t *prg = NULL;

		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);
		printf("Script dump: '%s'\n", arg);
		pcb_qry_set_input(arg);
		qry_parse(&prg);
		pcb_qry_dump_tree(" ", prg);
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "eval") == 0) {
		int errs;
		eval_stat_t st;

		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);

		memset(&st, 0, sizeof(st));
		printf("Script eval: '%s'\n", arg);
		errs = run_script(arg, eval_cb, &st);

		if (errs < 0)
			printf("Failed to run the query\n");
		else
			printf("eval statistics: true=%d false=%d errors=%d\n", st.trues, st.falses, errs);
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "select") == 0) {
		sel.how = PCB_CHGFLG_SET;

		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);

		if (run_script(arg, select_cb, &sel) < 0)
			printf("Failed to run the query\n");
		if (sel.cnt > 0) {
			pcb_board_set_changed_flag(pcb_true);
			pcb_hid_redraw(PCB);
		}
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "unselect") == 0) {
		sel.how = PCB_CHGFLG_CLEAR;

		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);

		if (run_script(arg, select_cb, &sel) < 0)
			printf("Failed to run the query\n");
		if (sel.cnt > 0) {
			pcb_board_set_changed_flag(pcb_true);
			pcb_hid_redraw(PCB);
		}
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "append") == 0) {
		pcb_idpath_list_t *list;

		PCB_ACT_CONVARG(2, FGW_IDPATH_LIST, query, list = fgw_idpath_list(&argv[2]));
		PCB_ACT_MAY_CONVARG(3, FGW_STR, query, arg = argv[3].val.str);

		if (!fgw_ptr_in_domain(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH_LIST))
			return FGW_ERR_PTR_DOMAIN;

		if (run_script(arg, append_cb, list) < 0)
			PCB_ACT_IRES(1);
		else
			PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_IRES(-1);
	return 0;
}

static const char *PTR_DOMAIN_PCFIELD = "ptr_domain_query_precompiled_field";

static pcb_qry_node_t *field_comp(const char *fields)
{
	char fname[64], *fno;
	const char *s;
	int len = 1, flen = 0, idx = 0;
	pcb_qry_node_t *res;

	if (*fields == '.') fields++;
	if (*fields == '\0')
		return NULL;

	for(s = fields; *s != '\0'; s++) {
		if (*s == '.') {
			len++;
			if (len > 16)
				return NULL; /* too many fields chained */
			if (flen >= sizeof(fname))
				return NULL; /* field name segment too long */
			flen = 0;
		}
		else
			flen++;
	}

	res = calloc(sizeof(pcb_qry_node_t), len);
	fno = fname;
	for(s = fields;; s++) {
		if ((*s == '.') || (*s == '\0')) {
			*fno = '\0';
			if (idx > 0)
				res[idx-1].next = &res[idx];
			res[idx].type = PCBQ_FIELD;
			res[idx].precomp.fld = query_fields_sphash(fname);
/*pcb_trace("[%d/%d] '%s' -> %d\n", idx, len, fname, res[idx].precomp.fld);*/
			if (res[idx].precomp.fld < 0) /* if compilation failed, this will need to be evaluated run-time, save as string */
				res[idx].data.str = pcb_strdup(fname);
			fno = fname;
			if (*s == '\0')
				break;
			idx++;
		}
		else
			*fno++ = *s;
	}

	return res;
}

static void field_free(pcb_qry_node_t *fld)
{
	pcb_qry_node_t *f;
	for(f = fld; f != NULL; f = f->next)
		if (f->data.str != NULL)
			free((char *)f->data.str);
	free(fld);
}


static void val2fgw(fgw_arg_t *dst, pcb_qry_val_t *src)
{
	switch(src->type) {
		case PCBQ_VT_COORD:
			dst->type = FGW_COORD_;
			fgw_coord(dst) = src->data.crd;
			break;
		case PCBQ_VT_DOUBLE:
			dst->type = FGW_DOUBLE;
			dst->val.nat_double = src->data.dbl;
			break;
		case PCBQ_VT_STRING:
			if (src->data.str != NULL) {
				dst->type = FGW_STR | FGW_DYN;
				dst->val.str = pcb_strdup(src->data.str);
			}
			else {
				dst->type = FGW_PTR;
				dst->val.ptr_void = NULL;
			}
			break;
		case PCBQ_VT_VOID:
		case PCBQ_VT_OBJ:
		case PCBQ_VT_LST:
		default:;
			break;
	}
}

static const char pcb_acts_QueryObj[] = "QueryObj(idpath, [.fieldname|fieldID])";
static const char pcb_acth_QueryObj[] = "Return the value of a field of an object, addressed by the object's idpath and the field's name or precompiled ID. Returns NIL on error.";
static fgw_error_t pcb_act_QueryObj(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_idpath_t *idp;
	pcb_qry_node_t *fld = NULL;
	pcb_qry_val_t obj;
	pcb_qry_val_t val;
	int free_fld = 0;

	PCB_ACT_CONVARG(1, FGW_IDPATH, QueryObj, idp = fgw_idpath(&argv[1]));

	if ((argv[2].type & FGW_STR) == FGW_STR) {
		const char *field;
		PCB_ACT_CONVARG(2, FGW_STR, QueryObj, field = argv[2].val.str);
		if (field == NULL)
			goto err;
		if (*field != '.')
			goto id;
		fld = field_comp(field);
		free_fld = 1;
	}
	else if ((argv[2].type & FGW_PTR) == FGW_PTR) {
		id:;
		PCB_ACT_CONVARG(2, FGW_PTR, QueryObj, fld = argv[2].val.ptr_void);
		if (!fgw_ptr_in_domain(&pcb_fgw, &argv[2], PTR_DOMAIN_PCFIELD))
			return FGW_ERR_PTR_DOMAIN;
	}

	if ((fld == NULL) || (!fgw_ptr_in_domain(&pcb_fgw, &argv[1], PCB_PTR_DOMAIN_IDPATH))) {
		if (free_fld)
			field_free(fld);
		return FGW_ERR_PTR_DOMAIN;
	}

	obj.type = PCBQ_VT_OBJ;
	obj.data.obj = pcb_idpath2obj(PCB->Data, idp);
	if (obj.data.obj == NULL)
		goto err;
	if (pcb_qry_obj_field(&obj, fld, &val) != 0)
		goto err;

	if (free_fld)
		field_free(fld);

	val2fgw(res, &val);
	return 0;

	err:;
		if (free_fld)
			field_free(fld);
		res->type = FGW_PTR;
		res->val.ptr_void = NULL;
		return 0;
}

static const char pcb_acts_QueryCompileField[] =
	"QueryCompileField(compile, fieldname)\n"
	"QueryCompileField(free, fieldID)";
static const char pcb_acth_QueryCompileField[] = "With \"compile\": precompiles textual field name to field ID; with \"free\": frees the memory allocated for a previously precompiled fieldID.";
static fgw_error_t pcb_act_QueryCompileField(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *fname;
	pcb_qry_node_t *fld;

	PCB_ACT_CONVARG(1, FGW_STR, QueryCompileField, cmd = argv[1].val.str);
	switch(*cmd) {
		case 'c': /* compile */
			PCB_ACT_CONVARG(2, FGW_STR, QueryCompileField, fname = argv[2].val.str);
			fld = field_comp(fname);
			fgw_ptr_reg(&pcb_fgw, res, PTR_DOMAIN_PCFIELD, FGW_PTR | FGW_STRUCT, fld);
			return 0;
		case 'f': /* free */
			if (!fgw_ptr_in_domain(&pcb_fgw, &argv[2], PTR_DOMAIN_PCFIELD))
				return FGW_ERR_PTR_DOMAIN;
			PCB_ACT_CONVARG(2, FGW_PTR, QueryCompileField, fld = argv[2].val.ptr_void);
			fgw_ptr_unreg(&pcb_fgw, &argv[2], PTR_DOMAIN_PCFIELD);
			field_free(fld);
			break;
		default:
			return FGW_ERR_ARG_CONV;
	}
	res->type = FGW_PTR;
	res->val.ptr_void = fld;
	return 0;
}

pcb_action_t query_action_list[] = {
	{"query", pcb_act_query, pcb_acth_query, pcb_acts_query},
	{"QueryObj", pcb_act_QueryObj, pcb_acth_QueryObj, pcb_acts_QueryObj},
	{"QueryCompileField", pcb_act_QueryCompileField, pcb_acth_QueryCompileField, pcb_acts_QueryCompileField}
};

PCB_REGISTER_ACTIONS(query_action_list, NULL)

#include "dolists.h"
void query_action_reg(const char *cookie)
{
	PCB_REGISTER_ACTIONS(query_action_list, cookie)
}

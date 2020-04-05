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
#include "conf_core.h"
#include <librnd/core/actions.h>
#include "query.h"
#include "query_y.h"
#include "query_exec.h"
#include "query_access.h"
#include "draw.h"
#include "select.h"
#include "board.h"
#include "idpath.h"
#include "view.h"
#include "actions_pcb.h"
#include <librnd/core/compat_misc.h>

static const char pcb_acts_query[] =
	"query(dump, expr) - dry run: compile and dump an expression\n"
	"query(eval|evalidp, expr) - compile and evaluate an expression and print a list of results on stdout\n"
	"query(select|unselect|view, expr) - select or unselect or build a view of objects matching an expression\n"
	"query(setflag:flag|unsetflag:flag, expr) - set or unset a named flag on objects matching an expression\n"
	"query(append, idplist, expr) - compile and run expr and append the idpath of resulting objects on idplist\n"
	;
static const char pcb_acth_query[] = "Perform various queries on PCB data.";

typedef struct {
	int trues, falses;
	unsigned print_idpath:1;
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

	if (st->print_idpath && (res->type == PCBQ_VT_OBJ)) {
		pcb_idpath_t *idp = pcb_obj2idpath(res->data.obj);
		char *idps = pcb_idpath2str(idp, 0);
		st->trues++;
		printf(" <obj %s>\n", idps);
		free(idps);
		pcb_idpath_destroy(idp);
		return;
	}

	if ((res->type != PCBQ_VT_COORD) && (res->type != PCBQ_VT_LONG)) {
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
	pcb_flag_values_t what;
} flagop_t;

static void flagop_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	flagop_t *sel = (flagop_t *)user_ctx;
	if (!pcb_qry_is_true(res))
		return;
	if (PCB_OBJ_IS_CLASS(current->type, PCB_OBJ_CLASS_OBJ)) {
		int state_wanted = (sel->how == PCB_CHGFLG_SET);
		int state_is     = PCB_FLAG_TEST(sel->what, current);
		if (state_wanted != state_is) {
			PCB_FLAG_CHANGE(sel->how, sel->what, current);
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

static void view_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	pcb_view_list_t *view = user_ctx;
	pcb_view_t *v;

	if (!pcb_qry_is_true(res))
		return;
	if (!PCB_OBJ_IS_CLASS(current->type, PCB_OBJ_CLASS_OBJ))
		return;

	v = pcb_view_new(&PCB->hidlib, "search result", NULL, NULL);
	pcb_view_append_obj(v, 0, current);
	pcb_view_set_bbox_by_objs(PCB->Data, v);
	pcb_view_list_append(view, v);
}

int pcb_qry_run_script(pcb_qry_exec_t *ec, pcb_board_t *pcb, const char *script, const char *scope, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current), void *user_ctx)
{
	pcb_qry_node_t *prg = NULL;
	int res, bufno = -1; /* empty scope means board */

	if (script == NULL) {
		pcb_message(PCB_MSG_ERROR, "Compilation error: no script specified.\n");
		return -1;
	}

	while(isspace(*script)) script++;
	pcb_qry_set_input(script);
	qry_parse(&prg);

	if (prg == NULL) {
		pcb_message(PCB_MSG_ERROR, "Compilation error.\n");
		return -1;
	}

	/* decode scope and set bufno */
	if ((scope != NULL) && (*scope != '\0')) {
		if (strcmp(scope, "board") == 0) bufno = -1;
		else if (strncmp(scope, "buffer", 6) == 0) {
			scope += 6;
			if (*scope != '\0') {
				char *end;
				bufno = strtol(scope, &end, 10);
				if (*end != '\0') {
					pcb_message(PCB_MSG_ERROR, "Invalid buffer number: '%s': not an integer\n", scope);
					pcb_qry_n_free(prg);
					return -1;
				}
				bufno--;
				if ((bufno < 0) || (bufno >= PCB_MAX_BUFFER)) {
					pcb_message(PCB_MSG_ERROR, "Invalid buffer number: '%d' out of range 1..%d\n", bufno+1, PCB_MAX_BUFFER);
					pcb_qry_n_free(prg);
					return -1;
				}
			}
			else
				bufno = conf_core.editor.buffer_number;
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Invalid scope: '%s': must be board or buffer or bufferN\n", scope);
			pcb_qry_n_free(prg);
			return -1;
		}
	}
	
	res = pcb_qry_run(ec, pcb, prg, bufno, cb, user_ctx);
	pcb_qry_n_free(prg);
	return res;
}

static fgw_error_t pcb_act_query(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *arg = NULL, *scope = NULL;
	flagop_t sel;

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
		pcb_qry_n_free(prg);
		PCB_ACT_IRES(0);
		return 0;
	}

	if ((strcmp(cmd, "eval") == 0) || (strcmp(cmd, "evalidp") == 0)) {
		int errs;
		eval_stat_t st;

		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);
		PCB_ACT_MAY_CONVARG(3, FGW_STR, query, scope = argv[3].val.str);

		memset(&st, 0, sizeof(st));
		st.print_idpath = (cmd[4] != '\0');
		printf("Script eval: '%s' scope='%s'\n", arg, scope == NULL ? "" : scope);
		errs = pcb_qry_run_script(NULL, PCB_ACT_BOARD, arg, scope, eval_cb, &st);

		if (errs < 0)
			printf("Failed to run the query\n");
		else
			printf("eval statistics: true=%d false=%d errors=%d\n", st.trues, st.falses, errs);
		PCB_ACT_IRES(0);
		return 0;
	}

	if ((strcmp(cmd, "select") == 0) || (strncmp(cmd, "setflag:", 8) == 0)) {
		sel.how = PCB_CHGFLG_SET;
		sel.what = PCB_FLAG_SELECTED;

		if (cmd[3] == 'f') {
			pcb_flag_t flg = pcb_strflg_s2f(cmd + 8, NULL, NULL, 0);
			sel.what = flg.f;
			if (sel.what == 0) {
				pcb_message(PCB_MSG_ERROR, "Invalid flag '%s'\n", cmd+8);
				PCB_ACT_IRES(0);
				return 0;
			}
		}

		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);
		PCB_ACT_MAY_CONVARG(3, FGW_STR, query, scope = argv[3].val.str);

		if (pcb_qry_run_script(NULL, PCB_ACT_BOARD, arg, scope, flagop_cb, &sel) < 0)
			printf("Failed to run the query\n");
		if (sel.cnt > 0) {
			pcb_board_set_changed_flag(pcb_true);
			if (PCB_HAVE_GUI_ATTR_DLG)
				pcb_hid_redraw(PCB);
		}
		PCB_ACT_IRES(0);
		return 0;
	}

	if ((strcmp(cmd, "unselect") == 0) || (strncmp(cmd, "unsetflag:", 10) == 0)) {
		sel.how = PCB_CHGFLG_CLEAR;
		sel.what = PCB_FLAG_SELECTED;

		if (cmd[5] == 'f') {
			pcb_flag_t flg = pcb_strflg_s2f(cmd + 10, NULL, NULL, 0);
			sel.what = flg.f;
			if (sel.what == 0) {
				pcb_message(PCB_MSG_ERROR, "Invalid flag '%s'\n", cmd+8);
				PCB_ACT_IRES(0);
				return 0;
			}
		}

		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);
		PCB_ACT_MAY_CONVARG(3, FGW_STR, query, scope = argv[3].val.str);

		if (pcb_qry_run_script(NULL, PCB_ACT_BOARD, arg, scope, flagop_cb, &sel) < 0)
			printf("Failed to run the query\n");
		if (sel.cnt > 0) {
			pcb_board_set_changed_flag(pcb_true);
			if (PCB_HAVE_GUI_ATTR_DLG)
				pcb_hid_redraw(PCB);
		}
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "append") == 0) {
		pcb_idpath_list_t *list;

		PCB_ACT_CONVARG(2, FGW_IDPATH_LIST, query, list = fgw_idpath_list(&argv[2]));
		PCB_ACT_MAY_CONVARG(3, FGW_STR, query, arg = argv[3].val.str);
		PCB_ACT_MAY_CONVARG(4, FGW_STR, query, scope = argv[4].val.str);

		if (!fgw_ptr_in_domain(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH_LIST))
			return FGW_ERR_PTR_DOMAIN;

		if (pcb_qry_run_script(NULL, PCB_ACT_BOARD, arg, scope, append_cb, list) < 0)
			PCB_ACT_IRES(1);
		else
			PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "view") == 0) {
		fgw_arg_t args[4], ares;
		pcb_view_list_t *view = calloc(sizeof(pcb_view_list_t), 1);
		PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);
		if (pcb_qry_run_script(NULL, PCB_ACT_BOARD, arg, scope, view_cb, view) >= 0) {
			args[1].type = FGW_STR; args[1].val.str = "advanced search results";
			args[2].type = FGW_STR; args[2].val.str = "search_res";
			fgw_ptr_reg(&pcb_fgw, &args[3], PCB_PTR_DOMAIN_VIEWLIST, FGW_PTR | FGW_STRUCT, view);
			pcb_actionv_bin(PCB_ACT_HIDLIB, "viewlist", &ares, 4, args);
			PCB_ACT_IRES(0);
		}
		else {
			free(view);
			PCB_ACT_IRES(1);
		}
		return 0;
	}

	PCB_ACT_FAIL(query);
}

static const char *PTR_DOMAIN_PCFIELD = "ptr_domain_query_precompiled_field";

static pcb_qry_node_t *field_comp(const char *fields)
{
	char fname[64], *fno;
	const char *s;
	int len = 1, flen = 0, idx = 0, quote = 0;
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
		if ((quote == 0) && ((*s == '.') || (*s == '\0'))) {
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
			if (s[1] == '\"') {
				quote = s[1];
				s++;
			}
		}
		else {
			if (*s == quote)
				quote = 0;
			else
				*fno++ = *s;
		}
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
		case PCBQ_VT_LONG:
			dst->type = FGW_LONG;
			dst->val.nat_long = src->data.lng;
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
	obj.data.obj = pcb_idpath2obj_in(PCB->Data, idp);
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

void query_action_reg(const char *cookie)
{
	PCB_REGISTER_ACTIONS(query_action_list, cookie)
}

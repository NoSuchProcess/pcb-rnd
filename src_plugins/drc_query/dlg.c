/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_tree.h>
#include <genlist/gendlist.h>
#include <liblihata/dom.h>
#include <liblihata/tree.h>
#include "actions_pcb.h"

#define PCB dont_use

static void rlist_select(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row);

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	rnd_conf_role_t role;
	char *rule, *path;
	int wtype, wtitle, wsource, wdisable, wdesc, wquery, wsave, wsaveroles;
	gdl_elem_t link;
} rule_edit_ctx_t;

static const char *save_roles[] = {"user", "project", "design", "cli", NULL};
static rnd_conf_role_t save_rolee[] = { RND_CFR_USER, RND_CFR_PROJECT, RND_CFR_DESIGN, RND_CFR_CLI};
#define save_role_defaulti 2

gdl_list_t rule_edit_dialogs;

static void rule_edit_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	rule_edit_ctx_t *ctx = caller_data;

	gdl_remove(&rule_edit_dialogs, ctx, link);

	free(ctx->path);
	free(ctx->rule);
	RND_DAD_FREE(ctx->dlg);
	free(ctx);
}

static const char *textval_empty(lht_node_t *nd, const char *fname)
{
	lht_node_t *nt = lht_dom_hash_get(nd, fname);

	if ((nt == NULL) || (nt->type != LHT_TEXT))
		return "";

	return nt->data.text.value;
}

static void drc_rule_pcb2dlg(rule_edit_ctx_t *ctx)
{
	rnd_dad_retovr_t retovr;
	lht_node_t *nd = rnd_conf_lht_get_at_mainplug(ctx->role, ctx->path, 1, 0);
	if (nd != NULL) {
		rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wquery];
		rnd_hid_text_t *txt = atxt->wdata;
		int *dis, dis_ = 0;

		dis = drc_get_disable(ctx->rule);
		if (dis == NULL)
			dis = &dis_;

		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wtype,    str, textval_empty(nd, "type"));
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wtitle,   str, textval_empty(nd, "title"));
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wsource,  str, textval_empty(nd, "source"));
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdisable, str, *dis ? "yes" : "no");
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdesc,    str, textval_empty(nd, "desc"));

		txt->hid_set_text(atxt, ctx->dlg_hid_ctx, RND_HID_TEXT_REPLACE, textval_empty(nd, "query"));
	}
	else {
		rnd_message(RND_MSG_ERROR, "Rule %s disappeared from the config tree.\n", ctx->rule);
		rnd_hid_dad_close(ctx->dlg_hid_ctx, &retovr, -1);
	}
}

static void drcq_open_view_win(rnd_hidlib_t *hidlib, pcb_view_list_t *view)
{
	fgw_arg_t args[4], ares;
	args[1].type = FGW_STR; args[1].val.str = "drc_query: manual run";
	args[2].type = FGW_STR; args[2].val.str = "drc_query_run";
	fgw_ptr_reg(&rnd_fgw, &args[3], PCB_PTR_DOMAIN_VIEWLIST, FGW_PTR | FGW_STRUCT, view);
	rnd_actionv_bin(hidlib, "viewlist", &ares, 4, args);

}

static void rule_btn_run_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	rule_edit_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wquery];
	rnd_hid_text_t *txt = atxt->wdata;
	char *script = txt->hid_get_text(atxt, hid_ctx);
	pcb_view_list_t *view = calloc(sizeof(pcb_view_list_t), 1);
	pcb_board_t *pcb = (pcb_board_t *)rnd_gui->get_dad_hidlib(hid_ctx);

	drc_qry_exec(NULL, pcb, view, ctx->rule,
		ctx->dlg[ctx->wtype].val.str,
		ctx->dlg[ctx->wtitle].val.str,
		ctx->dlg[ctx->wdesc].val.str,
		script);
	drcq_open_view_win(&pcb->hidlib, view);

	free(script);
	drc_rlist_pcb2dlg(); /* for the run time */
}

#define MKDIR_ND(outnode, parent, ntype, nname) \
do { \
	lht_node_t *nnew; \
	lht_err_t err; \
	char *nname0 = nname; \
	if (parent->type == LHT_LIST) nname0 = rnd_concat(nname, ":0", NULL); \
	nnew = lht_tree_path_(parent->doc, parent, nname0, 1, 1, &err); \
	if (parent->type == LHT_LIST) free(nname0); \
	if ((nnew != NULL) && (nnew->type != ntype)) { \
		rnd_message(RND_MSG_ERROR, "Internal error: invalid existing node type for %s: %d, rule is NOT saved\n", nname, nnew->type); \
		return; \
	} \
	else if (nnew == NULL) { \
		nnew = lht_dom_node_alloc(ntype, nname); \
		switch(parent->type) { \
			case LHT_HASH: err = lht_dom_hash_put(parent, nnew); break; \
			case LHT_LIST: err = lht_dom_list_append(parent, nnew); break; \
			default: \
				rnd_message(RND_MSG_ERROR, "Internal error: invalid parent node type for %s: %d, rule is NOT saved\n", parent->name, parent->type); \
				return; \
		} \
	} \
	outnode = nnew; \
} while(0)

#define MKDIR_ND_SET_TEXT(parent, nname, nval) \
do { \
	lht_node_t *ntxt; \
	MKDIR_ND(ntxt, parent, LHT_TEXT, nname); \
	if (ntxt == NULL) { \
		rnd_message(RND_MSG_ERROR, "Internal error: new text node for %s is NULL, rule is NOT saved\n", nname); \
		return; \
	} \
	free(ntxt->data.text.value); \
	ntxt->data.text.value = rnd_strdup(nval == NULL ? "" : nval); \
} while(0)

static void rule_btn_save_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	rule_edit_ctx_t *ctx = caller_data;
	int ri = ctx->dlg[ctx->wsaveroles].val.lng;
	lht_node_t *nd;
	rnd_conf_role_t role;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wquery];
	rnd_hid_text_t *txt = atxt->wdata;

	if ((ri < 0) || (ri >= sizeof(save_rolee)/sizeof(save_rolee[0]))) {
		rnd_message(RND_MSG_ERROR, "Internal error: role out of range, rule is NOT saved\n");
		return;
	}

	role = save_rolee[ri];
	nd = rnd_conf_lht_get_at_mainplug(role, ctx->path, 1, 0);
	if (nd == NULL) {
		nd = rnd_conf_lht_get_first(role, 1);
		if (nd == NULL) {
			rnd_message(RND_MSG_ERROR, "Internal error: failed to create role root, rule is NOT saved\n");
			return;
		}
		MKDIR_ND(nd, nd, LHT_HASH, "plugins");
		MKDIR_ND(nd, nd, LHT_HASH, "drc_query");
		MKDIR_ND(nd, nd, LHT_LIST, "rules");
		if ((nd->data.list.first == NULL) && (role != RND_CFR_USER)) {
			gdl_iterator_t it;
			rnd_conf_listitem_t *i;

			/* need to copy all rules! */
			rnd_conflist_foreach(&conf_drc_query.plugins.drc_query.rules, &it, i) {
				lht_node_t *nnew = lht_dom_duptree(i->prop.src);
				lht_dom_list_append(nd, nnew);
			}
			rnd_message(RND_MSG_WARNING, "NOTE: Copying ALL drc rule to config role %s\n", ctx->rule, save_roles[ri]);
		}
		MKDIR_ND(nd, nd, LHT_HASH, ctx->rule);
		rnd_message(RND_MSG_INFO, "NOTE: Copying drc rule '%s' to config role %s\n", ctx->rule, save_roles[ri]);
	}

	MKDIR_ND_SET_TEXT(nd, "type", ctx->dlg[ctx->wtype].val.str);
	MKDIR_ND_SET_TEXT(nd, "desc", ctx->dlg[ctx->wdesc].val.str);
	MKDIR_ND_SET_TEXT(nd, "title", ctx->dlg[ctx->wtitle].val.str);
	MKDIR_ND_SET_TEXT(nd, "source", ctx->dlg[ctx->wsource].val.str);
	MKDIR_ND_SET_TEXT(nd, "query", txt->hid_get_text(atxt, hid_ctx));

	rnd_conf_update(NULL, -1);
	drc_rlist_pcb2dlg();
}

#undef MKDIR_ND

static int pcb_dlg_rule_edit(rnd_conf_role_t role, const char *rule)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	char *info, *path;
	rule_edit_ctx_t *ctx;
	lht_node_t *nd;
	int n, srolei = save_role_defaulti;

	for(ctx = gdl_first(&rule_edit_dialogs); ctx != NULL; ctx = gdl_next(&rule_edit_dialogs, ctx))
	{
		if (strcmp(rule, ctx->rule) == 0) {
			rnd_message(RND_MSG_ERROR, "An edit dialog for rule %s is already open.\n", rule);
			return 0;
		}
	}

	path = rnd_concat(DRC_CONF_PATH_RULES, rule, ":0", NULL);
	nd = rnd_conf_lht_get_at_mainplug(role, path, 1, 0);
	if (nd == NULL) {
		rnd_message(RND_MSG_ERROR, "Rule %s not found on this role.\n", rule);
		return -1;
	}

	ctx = calloc(sizeof(rule_edit_ctx_t), 1);
	ctx->role = role;
	ctx->rule = rnd_strdup(rule);
	ctx->path = path;

	gdl_insert(&rule_edit_dialogs, ctx, link);

	/* attempt to set save role to input role, if input role is not read-only */
	for(n = 0; n < sizeof(save_rolee)/sizeof(save_rolee[0]); n++) {
		if (save_rolee[n] == role) {
			srolei = n;
			break;
		}
	}

	info = rnd_strdup_printf("DRC rule edit: %s on role %s", rule, rnd_conf_role_name(role));
	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(ctx->dlg, info);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "DRC violation type (group):");
			RND_DAD_STRING(ctx->dlg);
				RND_DAD_WIDTH_CHR(ctx->dlg, 24);
				ctx->wtype = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "DRC violation title:");
			RND_DAD_STRING(ctx->dlg);
			RND_DAD_WIDTH_CHR(ctx->dlg, 32);
			ctx->wtitle = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Rule source:");
			RND_DAD_STRING(ctx->dlg);
			RND_DAD_WIDTH_CHR(ctx->dlg, 32);
			ctx->wsource = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Disable drc rule:");
			RND_DAD_BOOL(ctx->dlg, "");
			ctx->wdisable = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_LABEL(ctx->dlg, "DRC violation description:");
		RND_DAD_STRING(ctx->dlg);
				RND_DAD_WIDTH_CHR(ctx->dlg, 48);
				ctx->wdesc = RND_DAD_CURRENT(ctx->dlg);

		RND_DAD_LABEL(ctx->dlg, "DRC rule query script:");
		RND_DAD_TEXT(ctx->dlg, ctx);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			ctx->wquery = RND_DAD_CURRENT(ctx->dlg);


		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Run");
				RND_DAD_CHANGE_CB(ctx->dlg, rule_btn_run_cb);

			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME | RND_HATF_TIGHT);
				RND_DAD_BUTTON(ctx->dlg, "Save");
					RND_DAD_CHANGE_CB(ctx->dlg, rule_btn_save_cb);
					ctx->wsave = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_HELP(ctx->dlg, "Save rule at the selected conf role\nFor roles other than 'user', a full copy of all drc rules are saved!\n");
				RND_DAD_ENUM(ctx->dlg, save_roles);
					ctx->wsaveroles = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_DEFAULT_NUM(ctx->dlg, srolei);
					RND_DAD_HELP(ctx->dlg, "Save rule at the selected conf role\nFor roles other than 'user', a full copy of all drc rules are saved!\n");
			RND_DAD_END(ctx->dlg);

			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_END(ctx->dlg);
			RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);
	free(info);

	RND_DAD_DEFSIZE(ctx->dlg, 200, 400);
	RND_DAD_NEW("drc_query_rule_edit", ctx->dlg, "drc_query: rule editor", ctx, rnd_false, rule_edit_close_cb);

	drc_rule_pcb2dlg(ctx);

	return 0;
}

static const char pcb_acts_DrcQueryEditRule[] = "DrcQueryEditRule(role, path, rule)\nDrcQueryEditRule(role, rule)\n";
static const char pcb_acth_DrcQueryEditRule[] = "Interactive, GUI based DRC rule editor";
static fgw_error_t pcb_act_DrcQueryEditRule(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *srole, *spath, *srule = NULL;
	rnd_conf_role_t role;

	RND_ACT_CONVARG(1, FGW_STR, DrcQueryEditRule, srole = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, DrcQueryEditRule, spath = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, DrcQueryEditRule, srule = argv[3].val.str);

	if (srule == NULL)
		srule = spath;

	role = rnd_conf_role_parse(srole);
	if (role == RND_CFR_invalid)
		RND_ACT_FAIL(DrcQueryEditRule);

	RND_ACT_IRES(pcb_dlg_rule_edit(role, srule));
	return 0;
}



typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int wlist, wrule, wtype, wtitle, wdesc, wstat;
} drc_rlist_ctx_t;

static drc_rlist_ctx_t drc_rlist_ctx;

static void drc_rlist_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	drc_rlist_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(drc_rlist_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void drc_rlist_pcb2dlg(void)
{
	drc_rlist_ctx_t *ctx = &drc_rlist_ctx;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[5], *cursor_path = NULL;
	gdl_iterator_t it;
	rnd_conf_listitem_t *i;
	pcb_drcq_stat_t *st;

	if (!ctx->active)
		return;

	attr = &ctx->dlg[ctx->wlist];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	cell[4] = NULL;

	rnd_conflist_foreach(&conf_drc_query.plugins.drc_query.rules, &it, i) {
		const char *src;
		int *dis, dis_ = 0;
		rnd_conf_role_t role;
		lht_node_t *rule = i->prop.src;
		st = pcb_drcq_stat_get(rule->name);

		if (rule->type != LHT_HASH)
			continue;

		dis = drc_get_disable(rule->name);
		if (dis == NULL)
			dis = &dis_;

		role = rnd_conf_lookup_role(rule);
		src = textval_empty(rule, "source");
		cell[0] = rule->name;
		cell[1] = rnd_strdup(rnd_conf_role_name(role));
		cell[2] = *dis ? "YES" : "no";
		if (st->run_cnt > 0)
			cell[3] = rnd_strdup_printf("%.3fs", st->last_run_time);
		else
			cell[3] = "-";

		if (*src != '\0') {
			rnd_hid_tree_t *tree = attr->wdata;
			rnd_hid_row_t *parent = htsp_get(&tree->paths, src);
			char *pcell[5];

			if (parent == NULL) {
				pcell[0] = src;
				pcell[1] = "";
				pcell[2] = "";
				pcell[3] = "";
				pcell[4] = NULL;
				parent = rnd_dad_tree_append(tree->attrib, NULL, pcell);
			}
			rnd_dad_tree_append_under(tree->attrib, parent, cell);
		}
		else
			rnd_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
		free(cursor_path);

		r = rnd_dad_tree_get_selected(attr);
		rlist_select(&ctx->dlg[ctx->wlist], ctx->dlg_hid_ctx, r);
	}
}

static void rlist_btn_toggle_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	drc_rlist_ctx_t *ctx = caller_data;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&(ctx->dlg[ctx->wlist]));
	int *dis;

	if (row == NULL) {
		rnd_message(RND_MSG_ERROR, "Select a rule first!\n");
		return;
	}

	dis = drc_get_disable(row->cell[0]);
	if (dis == NULL) {
		rnd_message(RND_MSG_ERROR, "internal error: no disable conf node for %s\n", row->cell[0]);
		return;
	}

	*dis = !*dis;
	drc_rlist_pcb2dlg();
}

#define rlist_fetch() \
do { \
	if (row == NULL) { \
		rnd_message(RND_MSG_ERROR, "Select a rule first!\n"); \
		return; \
	} \
	if (*row->cell[1] == '\0') \
		return; \
	role = rnd_conf_role_parse(row->cell[1]); \
	if (role == RND_CFR_invalid) { \
		rnd_message(RND_MSG_ERROR, "internal error: invalid role %s\n", row->cell[0]); \
		return; \
	} \
} while(0)

static const char *rule_basename(const char *cell0)
{
	const char *basename = strchr(cell0, '/');
	if (basename == NULL)
		return cell0;
	return basename+1;
}

#define rlist_fetch_nd() \
do { \
	if (*row->cell[1] != '\0') { \
		const char *basename = rule_basename(row->cell[0]); \
		char *path = rnd_concat(DRC_CONF_PATH_RULES, basename, ":0", NULL); \
		nd = rnd_conf_lht_get_at_mainplug(role, path, 1, 0); \
		if (nd == NULL) { \
			rnd_message(RND_MSG_ERROR, "internal error: rule not found at %s\n", path); \
			return; \
		} \
		free(path); \
	} \
	else \
		return; \
} while(0)

static void rlist_btn_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	drc_rlist_ctx_t *ctx = caller_data;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&(ctx->dlg[ctx->wlist]));
	rnd_conf_role_t role;

	rlist_fetch();

	pcb_dlg_rule_edit(role, row->cell[0]);
}

static void rlist_btn_run_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	drc_rlist_ctx_t *ctx = caller_data;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&(ctx->dlg[ctx->wlist]));
	lht_node_t *nd;
	const char *script;
	rnd_conf_role_t role;
	pcb_view_list_t *view;
	pcb_board_t *pcb = (pcb_board_t *)rnd_gui->get_dad_hidlib(hid_ctx);

	rlist_fetch();
	rlist_fetch_nd();

	script = textval_empty(nd, "query");
	if (script == NULL) {
		rnd_message(RND_MSG_ERROR, "Can not run rule %s: no query specified\n", row->cell[0]);
		return;
	}

	view = calloc(sizeof(pcb_view_list_t), 1);
	drc_qry_exec(NULL, pcb, view, rule_basename(row->cell[0]), textval_empty(nd, "type"), textval_empty(nd, "title"), textval_empty(nd, "desc"), script);
	drcq_open_view_win(&pcb->hidlib, view);
	drc_rlist_pcb2dlg(); /* for the run time */
}

static void rlist_select(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_attr_val_t hv;
	rnd_hid_tree_t *tree = attrib->wdata;
	drc_rlist_ctx_t *ctx = tree->user_ctx;
	lht_node_t *nd;
	gds_t tmp;
	rnd_conf_role_t role;
	pcb_drcq_stat_t *st;

	rlist_fetch();
	rlist_fetch_nd();

	hv.str = nd->name;
	if (hv.str == NULL) hv.str = "<n/a>";
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wrule, &hv);

	hv.str = textval_empty(nd, "type");
	if (hv.str == NULL) hv.str = "<n/a>";
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtype, &hv);

	hv.str = textval_empty(nd, "title");
	if (hv.str == NULL) hv.str = "<n/a>";
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtitle, &hv);

	hv.str = textval_empty(nd, "desc");
	if (hv.str == NULL) hv.str = "<n/a>";
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdesc, &hv);

	gds_init(&tmp);
	st = pcb_drcq_stat_get(nd->name);
	rnd_append_printf(&tmp, "Ran %ld times", st->run_cnt);
	if (st->run_cnt > 0) {
		rnd_append_printf(&tmp, "\nLast run took: %.6f s", st->last_run_time);
		rnd_append_printf(&tmp, "\nTotal run time: %.6f s", st->sum_run_time);
		rnd_append_printf(&tmp, "\nAverage run time: %.6f s", st->sum_run_time / (double)st->run_cnt);
		rnd_append_printf(&tmp, "\nLast run violations: %ld", st->last_hit_cnt);
		rnd_append_printf(&tmp, "\nTotal violations: %ld", st->sum_hit_cnt);
		rnd_append_printf(&tmp, "\nAverage violations: %.2f", (double)st->sum_hit_cnt / (double)st->run_cnt);
	}
	hv.str = tmp.array;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wstat, &hv);
	gds_uninit(&tmp);
}


static int pcb_dlg_drc_rlist(void)
{
	const char *lst_hdr[] = {"rule name", "role", "disabled", "cost", NULL};
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	int wpane;

	if (drc_rlist_ctx.active)
		return 0; /* do not open another */

	RND_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg);
		RND_DAD_COMPFLAG(drc_rlist_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_HPANE(drc_rlist_ctx.dlg);
			wpane = RND_DAD_CURRENT(drc_rlist_ctx.dlg);

			RND_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg); /* left */
				RND_DAD_COMPFLAG(drc_rlist_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_LABEL(drc_rlist_ctx.dlg, "Rules available:");
				RND_DAD_TREE(drc_rlist_ctx.dlg, 4, 1, lst_hdr);
					RND_DAD_COMPFLAG(drc_rlist_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					drc_rlist_ctx.wlist = RND_DAD_CURRENT(drc_rlist_ctx.dlg);
					RND_DAD_TREE_SET_CB(drc_rlist_ctx.dlg, selected_cb, rlist_select);
					RND_DAD_TREE_SET_CB(drc_rlist_ctx.dlg, ctx, &drc_rlist_ctx);
				RND_DAD_BEGIN_HBOX(drc_rlist_ctx.dlg);
					RND_DAD_BUTTON(drc_rlist_ctx.dlg, "Run");
						RND_DAD_CHANGE_CB(drc_rlist_ctx.dlg, rlist_btn_run_cb);
					RND_DAD_BUTTON(drc_rlist_ctx.dlg, "Edit...");
						RND_DAD_CHANGE_CB(drc_rlist_ctx.dlg, rlist_btn_edit_cb);
					RND_DAD_BUTTON(drc_rlist_ctx.dlg, "Toggle disable");
						RND_DAD_CHANGE_CB(drc_rlist_ctx.dlg, rlist_btn_toggle_cb);
				RND_DAD_END(drc_rlist_ctx.dlg);
			RND_DAD_END(drc_rlist_ctx.dlg);

			RND_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg); /* right */
				RND_DAD_COMPFLAG(drc_rlist_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_BEGIN_HBOX(drc_rlist_ctx.dlg);
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "Rule: ");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "<no rule selected>");
						drc_rlist_ctx.wrule = RND_DAD_CURRENT(drc_rlist_ctx.dlg);
				RND_DAD_END(drc_rlist_ctx.dlg);

				RND_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg);
					RND_DAD_COMPFLAG(drc_rlist_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "Type/group:");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "-");
						drc_rlist_ctx.wtype = RND_DAD_CURRENT(drc_rlist_ctx.dlg);
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "Title:");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "-");
						drc_rlist_ctx.wtitle = RND_DAD_CURRENT(drc_rlist_ctx.dlg);
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "Description:");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "-");
						drc_rlist_ctx.wdesc = RND_DAD_CURRENT(drc_rlist_ctx.dlg);
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "Statistics:");
					RND_DAD_LABEL(drc_rlist_ctx.dlg, "-");
						drc_rlist_ctx.wstat = RND_DAD_CURRENT(drc_rlist_ctx.dlg);
				RND_DAD_END(drc_rlist_ctx.dlg);
			RND_DAD_END(drc_rlist_ctx.dlg);

		RND_DAD_END(drc_rlist_ctx.dlg);
		RND_DAD_BUTTON_CLOSES(drc_rlist_ctx.dlg, clbtn);
	RND_DAD_END(drc_rlist_ctx.dlg);

	/* set up the context */
	drc_rlist_ctx.active = 1;

	RND_DAD_DEFSIZE(drc_rlist_ctx.dlg, 550, 400);
	RND_DAD_NEW("drc_query_list", drc_rlist_ctx.dlg, "drc_query: list of rules", &drc_rlist_ctx, rnd_false, drc_rlist_close_cb);
	RND_DAD_SET_VALUE(drc_rlist_ctx.dlg_hid_ctx, wpane,    dbl, 0.5);
	drc_rlist_pcb2dlg();
	return 0;
}


static const char pcb_acts_DrcQueryListRules[] = "DrcQueryListRules()\n";
static const char pcb_acth_DrcQueryListRules[] = "List all drc rules implemented in drc_query";
static fgw_error_t pcb_act_DrcQueryListRules(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_ACT_IRES(pcb_dlg_drc_rlist());
	return 0;
}

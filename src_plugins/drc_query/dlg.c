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

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	conf_role_t role;
	char *rule, *path;
	int wtype, wtitle, wdisable, wdesc, wquery, wsave;
	gdl_elem_t link;
} rule_edit_ctx_t;

gdl_list_t rule_edit_dialogs;

static void rule_edit_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	rule_edit_ctx_t *ctx = caller_data;

	gdl_remove(&rule_edit_dialogs, ctx, link);

	free(ctx->path);
	free(ctx->rule);
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static const char *textval(lht_node_t *nd, const char *fname)
{
	lht_node_t *nt = lht_dom_hash_get(nd, fname);

	if ((nt == NULL) || (nt->type != LHT_TEXT))
		return "";

	return nt->data.text.value;
}

static int boolval(lht_node_t *nd, const char *fname)
{
	return str2bool(textval(nd, fname));
}

static void drc_rule_pcb2dlg(rule_edit_ctx_t *ctx)
{
	pcb_dad_retovr_t retovr;
	lht_node_t *nd = pcb_conf_lht_get_at_mainplug(ctx->role, ctx->path, 1, 0);
	if (nd != NULL) {
		pcb_hid_attribute_t *atxt = &ctx->dlg[ctx->wquery];
		pcb_hid_text_t *txt = atxt->wdata;
		int *dis, dis_ = 0;

		dis = drc_get_disable(ctx->rule);
		if (dis == NULL)
			dis = &dis_;

		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wtype,    str, textval(nd, "type"));
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wtitle,   str, textval(nd, "title"));
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdisable, str, *dis);
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdesc,    str, textval(nd, "desc"));

		txt->hid_set_text(atxt, ctx->dlg_hid_ctx, PCB_HID_TEXT_REPLACE, textval(nd, "query"));
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Rule %s disappeared from the config tree.\n", ctx->rule);
		pcb_hid_dad_close(ctx->dlg_hid_ctx, &retovr, -1);
	}

	pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wsave, 0 && !rnd_conf_is_read_only(ctx->role));
}

static void rule_btn_run_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	rule_edit_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *atxt = &ctx->dlg[ctx->wquery];
	pcb_hid_text_t *txt = atxt->wdata;
	char *s;
	s = txt->hid_get_text(atxt, hid_ctx);
	pcb_actionva(&PCB->hidlib, "query", "view", s, NULL);
	free(s);
}

static void rule_btn_save_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	rule_edit_ctx_t *ctx = caller_data;
	
}

static int pcb_dlg_rule_edit(conf_role_t role, const char *rule)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	char *info, *path;
	rule_edit_ctx_t *ctx;
	lht_node_t *nd;

	for(ctx = gdl_first(&rule_edit_dialogs); ctx != NULL; ctx = gdl_next(&rule_edit_dialogs, ctx))
	{
		if (strcmp(rule, ctx->rule) == 0) {
			pcb_message(PCB_MSG_ERROR, "An edit dialog for rule %s is already open.\n", rule);
			return 0;
		}
	}

	path = pcb_concat(DRC_CONF_PATH_RULES, rule, ":0", NULL);
	nd = pcb_conf_lht_get_at_mainplug(role, path, 1, 0);
	if (nd == NULL) {
		pcb_message(PCB_MSG_ERROR, "Rule %s not found on this role.\n", rule);
		return -1;
	}

	ctx = calloc(sizeof(rule_edit_ctx_t), 1);
	ctx->role = role;
	ctx->rule = pcb_strdup(rule);
	ctx->path = path;

	gdl_insert(&rule_edit_dialogs, ctx, link);


	info = pcb_strdup_printf("DRC rule edit: %s on role %s", rule, pcb_conf_role_name(role));
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(ctx->dlg, info);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "DRC violation type (group):");
			PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_WIDTH_CHR(ctx->dlg, 24);
				ctx->wtype = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "DRC violation title:");
			PCB_DAD_STRING(ctx->dlg);
			PCB_DAD_WIDTH_CHR(ctx->dlg, 32);
			ctx->wtitle = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Disable drc rule:");
			PCB_DAD_BOOL(ctx->dlg, "");
			ctx->wdisable = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_LABEL(ctx->dlg, "DRC violation description:");
		PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_WIDTH_CHR(ctx->dlg, 48);
				ctx->wdesc = PCB_DAD_CURRENT(ctx->dlg);

		PCB_DAD_LABEL(ctx->dlg, "DRC rule query script:");
		PCB_DAD_TEXT(ctx->dlg, ctx);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			ctx->wquery = PCB_DAD_CURRENT(ctx->dlg);


		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Run");
				PCB_DAD_CHANGE_CB(ctx->dlg, rule_btn_run_cb);
			PCB_DAD_BUTTON(ctx->dlg, "Save");
				PCB_DAD_CHANGE_CB(ctx->dlg, rule_btn_save_cb);
				ctx->wsave = PCB_DAD_CURRENT(ctx->dlg);;
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
	free(info);

	PCB_DAD_DEFSIZE(ctx->dlg, 200, 400);
	PCB_DAD_NEW("drc_query_rule_edit", ctx->dlg, "drc_query: rule editor", ctx, pcb_false, rule_edit_close_cb);

	drc_rule_pcb2dlg(ctx);

	return 0;
}

static const char pcb_acts_DrcQueryEditRule[] = "DrcQueryEditRule(role, path, rule)\nDrcQueryEditRule(role, rule)\n";
static const char pcb_acth_DrcQueryEditRule[] = "Interactive, GUI based DRC rule editor";
static fgw_error_t pcb_act_DrcQueryEditRule(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *srole, *spath, *srule = NULL;
	conf_role_t role;

	PCB_ACT_CONVARG(1, FGW_STR, DrcQueryEditRule, srole = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, DrcQueryEditRule, spath = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, DrcQueryEditRule, srule = argv[3].val.str);

	if (srule == NULL)
		srule = spath;

	role = pcb_conf_role_parse(srole);
	if (role == CFR_invalid)
		PCB_ACT_FAIL(DrcQueryEditRule);

	PCB_ACT_IRES(pcb_dlg_rule_edit(role, srule));
	return 0;
}



typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int wlist, wrule, wtype, wtitle, wdesc;
} drc_rlist_ctx_t;

static drc_rlist_ctx_t drc_rlist_ctx;

static void drc_rlist_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	drc_rlist_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(drc_rlist_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void drc_rlist_pcb2dlg(void)
{
	drc_rlist_ctx_t *ctx = &drc_rlist_ctx;
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;
	gdl_iterator_t it;
	pcb_conf_listitem_t *i;


	attr = &ctx->dlg[ctx->wlist];
	tree = attr->wdata;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	cell[3] = NULL;

	pcb_conflist_foreach(&conf_drc_query.plugins.drc_query.rules, &it, i) {
		int *dis, dis_ = 0;
		conf_role_t role;
		lht_node_t *rule = i->prop.src;
		if (rule->type != LHT_HASH)
			continue;

		dis = drc_get_disable(rule->name);
		if (dis == NULL)
			dis = &dis_;

		role = pcb_conf_lookup_role(rule);
		cell[0] = rule->name;
		cell[1] = pcb_strdup(pcb_conf_role_name(role));
		cell[2] = *dis ? "YES" : "no";
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
		free(cursor_path);
	}
}

static void rlist_btn_toggle_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	drc_rlist_ctx_t *ctx = caller_data;
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&(ctx->dlg[ctx->wlist]));
	int *dis;

	if (row == NULL) {
		pcb_message(PCB_MSG_ERROR, "Select a rule first!\n");
		return;
	}

	dis = drc_get_disable(row->cell[0]);
	if (dis == NULL) {
		pcb_message(PCB_MSG_ERROR, "internal error: no disable conf node for %s\n", row->cell[0]);
		return;
	}

	*dis = !*dis;
	drc_rlist_pcb2dlg();
}

#define rlist_fetch() \
do { \
	if (row == NULL) { \
		pcb_message(PCB_MSG_ERROR, "Select a rule first!\n"); \
		return; \
	} \
	role = pcb_conf_role_parse(row->cell[1]); \
	if (role == CFR_invalid) { \
		pcb_message(PCB_MSG_ERROR, "internal error: invalid role %s\n", row->cell[0]); \
		return; \
	} \
} while(0)

#define rlist_fetch_nd() \
do { \
	char *path = pcb_concat(DRC_CONF_PATH_RULES, row->cell[0], ":0", NULL); \
	nd = pcb_conf_lht_get_at_mainplug(role, path, 1, 0); \
	if (nd == NULL) { \
		pcb_message(PCB_MSG_ERROR, "internal error: rule not found at %s\n", path); \
		return; \
	} \
	free(path); \
} while(0)

static void rlist_btn_edit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	drc_rlist_ctx_t *ctx = caller_data;
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&(ctx->dlg[ctx->wlist]));
	conf_role_t role;

	rlist_fetch();

	pcb_dlg_rule_edit(role, row->cell[0]);
}

static void rlist_btn_run_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	drc_rlist_ctx_t *ctx = caller_data;
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&(ctx->dlg[ctx->wlist]));
	lht_node_t *nd;
	const char *s;
	conf_role_t role;

	rlist_fetch();
	rlist_fetch_nd();

	s = textval(nd, "query");
	if (s == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not run rule %s: no query specified\n", row->cell[0]);
		return;
	}

	pcb_actionva(&PCB->hidlib, "query", "view", s, NULL);
}

static void rlist_select(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_attr_val_t hv;
	pcb_hid_tree_t *tree = attrib->wdata;
	drc_rlist_ctx_t *ctx = tree->user_ctx;
	lht_node_t *nd;
	conf_role_t role;

	rlist_fetch();
	rlist_fetch_nd();

	hv.str = nd->name;
	if (hv.str == NULL) hv.str = "<n/a>";
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wrule, &hv);

	hv.str = textval(nd, "type");
	if (hv.str == NULL) hv.str = "<n/a>";
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtype, &hv);

	hv.str = textval(nd, "title");
	if (hv.str == NULL) hv.str = "<n/a>";
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtitle, &hv);

	hv.str = textval(nd, "desc");
	if (hv.str == NULL) hv.str = "<n/a>";
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdesc, &hv);
}


static int pcb_dlg_drc_rlist(void)
{
	const char *lst_hdr[] = {"rule name", "role", "disabled", NULL};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	int wpane;

	if (drc_rlist_ctx.active)
		return 0; /* do not open another */

	PCB_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg);
		PCB_DAD_COMPFLAG(drc_rlist_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HPANE(drc_rlist_ctx.dlg);
			wpane = PCB_DAD_CURRENT(drc_rlist_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg); /* left */
				PCB_DAD_COMPFLAG(drc_rlist_ctx.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(drc_rlist_ctx.dlg, "Rules available:");
				PCB_DAD_TREE(drc_rlist_ctx.dlg, 3, 0, lst_hdr);
					PCB_DAD_COMPFLAG(drc_rlist_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					drc_rlist_ctx.wlist = PCB_DAD_CURRENT(drc_rlist_ctx.dlg);
					PCB_DAD_TREE_SET_CB(drc_rlist_ctx.dlg, selected_cb, rlist_select);
					PCB_DAD_TREE_SET_CB(drc_rlist_ctx.dlg, ctx, &drc_rlist_ctx);
				PCB_DAD_BEGIN_HBOX(drc_rlist_ctx.dlg);
					PCB_DAD_BUTTON(drc_rlist_ctx.dlg, "Run");
						PCB_DAD_CHANGE_CB(drc_rlist_ctx.dlg, rlist_btn_run_cb);
					PCB_DAD_BUTTON(drc_rlist_ctx.dlg, "Edit...");
						PCB_DAD_CHANGE_CB(drc_rlist_ctx.dlg, rlist_btn_edit_cb);
					PCB_DAD_BUTTON(drc_rlist_ctx.dlg, "Toggle disable");
						PCB_DAD_CHANGE_CB(drc_rlist_ctx.dlg, rlist_btn_toggle_cb);
				PCB_DAD_END(drc_rlist_ctx.dlg);
			PCB_DAD_END(drc_rlist_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg); /* right */
				PCB_DAD_COMPFLAG(drc_rlist_ctx.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_BEGIN_HBOX(drc_rlist_ctx.dlg);
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "Rule: ");
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "<no rule selected>");
						drc_rlist_ctx.wrule = PCB_DAD_CURRENT(drc_rlist_ctx.dlg);
				PCB_DAD_END(drc_rlist_ctx.dlg);

				PCB_DAD_BEGIN_VBOX(drc_rlist_ctx.dlg);
					PCB_DAD_COMPFLAG(drc_rlist_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "Type/group:");
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "-");
						drc_rlist_ctx.wtype = PCB_DAD_CURRENT(drc_rlist_ctx.dlg);
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "");
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "Title:");
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "-");
						drc_rlist_ctx.wtitle = PCB_DAD_CURRENT(drc_rlist_ctx.dlg);
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "");
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "Description:");
					PCB_DAD_LABEL(drc_rlist_ctx.dlg, "-");
						drc_rlist_ctx.wdesc = PCB_DAD_CURRENT(drc_rlist_ctx.dlg);
				PCB_DAD_END(drc_rlist_ctx.dlg);
			PCB_DAD_END(drc_rlist_ctx.dlg);

		PCB_DAD_END(drc_rlist_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(drc_rlist_ctx.dlg, clbtn);
	PCB_DAD_END(drc_rlist_ctx.dlg);

	/* set up the context */
	drc_rlist_ctx.active = 1;

	PCB_DAD_DEFSIZE(drc_rlist_ctx.dlg, 550, 400);
	PCB_DAD_NEW("drc_query_list", drc_rlist_ctx.dlg, "drc_query: list of rules", &drc_rlist_ctx, pcb_false, drc_rlist_close_cb);
	PCB_DAD_SET_VALUE(drc_rlist_ctx.dlg_hid_ctx, wpane,    dbl, 0.5);
	drc_rlist_pcb2dlg();
	return 0;
}


static const char pcb_acts_DrcQueryListRules[] = "DrcQueryListRules()\n";
static const char pcb_acth_DrcQueryListRules[] = "List all drc rules implemented in drc_query";
static fgw_error_t pcb_act_DrcQueryListRules(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(pcb_dlg_drc_rlist());
	return 0;
}

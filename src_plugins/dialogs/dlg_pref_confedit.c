/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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

/* Preferences dialog, conf tree tab -> edit conf node (input side) popup */

#include <librnd/core/hid_dad_unit.h>

#define is_read_only(ctx)   rnd_conf_is_read_only(ctx->role)

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)

	rnd_conf_native_t *nat;
	int idx;
	rnd_conf_role_t role;
	
	int wnewval, winsa;
} confedit_ctx_t;

static void pref_conf_edit_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	pref_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	free(ctx);
}

/* Returns true if the value being edited is not yet created in the conf tree */
static int confedit_node_is_uninitialized(confedit_ctx_t *ctx)
{
	return (rnd_conf_lht_get_at(ctx->role, ctx->nat->hash_path, 0) == NULL);
}

static void confedit_brd2dlg(confedit_ctx_t *ctx)
{
	rnd_hid_attr_val_t hv;
	lht_node_t *nl, *nd = rnd_conf_lht_get_at(ctx->role, ctx->nat->hash_path, 1);
	const char *val;

	if (ctx->idx >= ctx->nat->array_size)
		return; /* shouldn't ever happen - we have checked this before creating the dialog! */

	val = pref_conf_get_val(nd, ctx->nat, ctx->idx);

	switch(ctx->nat->type) {
		case RND_CFN_STRING:
			hv.str = val;
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case RND_CFN_BOOLEAN:
		case RND_CFN_INTEGER:
			hv.lng = atoi(val);
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case RND_CFN_REAL:
			hv.dbl = strtod(val, NULL);
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case RND_CFN_COORD:
			hv.crd = rnd_get_value(val, NULL, NULL, NULL);
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case RND_CFN_UNIT:
			{
				const rnd_unit_t *u = rnd_get_unit_struct(val);
				if (u != NULL)
					hv.lng = u - rnd_units;
				else
					hv.lng = -1;
				rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			}
			break;
		case RND_CFN_COLOR:
			hv.clr = ctx->nat->val.color[ctx->idx];
			rnd_color_load_str(&hv.clr, val);
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case RND_CFN_LIST:
			{
				rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
				rnd_hid_tree_t *tree = attr->wdata;
			
				rnd_dad_tree_clear(tree);
				if (nd->type != LHT_LIST)
					return;
				for(nl = nd->data.list.first; nl != NULL; nl = nl->next) {
					char *cell[2] = {NULL};
					if (nl->type == LHT_TEXT)
						cell[0] = rnd_strdup(nl->data.text.value);
					rnd_dad_tree_append(attr, NULL, cell);
				}
			}
			break;
		case RND_CFN_HLIST:
/*			rnd_message(RND_MSG_ERROR, "ERROR: can not import hash lists on GUI\n");*/
			/* Nothing to do, for now it's just a bunch of buttons */
			break;
		case RND_CFN_max:
			rnd_message(RND_MSG_ERROR, "ERROR: invalid conf node type\n");
			break;
	}
}

static void pref_conf_editval_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr;
	char buf[128];
	const char *val = buf;

	if (ctx->idx >= ctx->nat->array_size)
		return; /* shouldn't ever happen - we have checked this before creating the dialog! */

	attr = &ctx->dlg[ctx->wnewval];

	switch(ctx->nat->type) {
		case RND_CFN_STRING:  val = attr->val.str; break;
		case RND_CFN_BOOLEAN:
		case RND_CFN_INTEGER: sprintf(buf, "%ld", attr->val.lng); break;
		case RND_CFN_REAL:    sprintf(buf, "%f", attr->val.dbl); break;
		case RND_CFN_COORD:   rnd_snprintf(buf, sizeof(buf), "%.08$mH", attr->val.crd); break;
		case RND_CFN_UNIT:
			if ((attr->val.lng < 0) || (attr->val.lng >= rnd_get_n_units(0)))
				return;
			val = rnd_units[attr->val.lng].suffix;
			break;
		case RND_CFN_COLOR:   val = attr->val.clr.str; break;
		case RND_CFN_LIST:
			{
				rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
				rnd_hid_tree_t *tree = attr->wdata;
				rnd_hid_row_t *r;
				lht_node_t *nd = rnd_conf_lht_get_at(ctx->role, ctx->nat->hash_path, 0);

				if (nd == NULL) {
					rnd_message(RND_MSG_ERROR, "Internal error: can't copy back to non-existing list!\n");
					return;
				}

				if (nd->type != LHT_LIST) {
					rnd_message(RND_MSG_ERROR, "Internal error: can't copy back list into non-list!\n");
					return;
				}

				/* empty the list so that we insert to an empty list which is overwriting the list */
				while(nd->data.list.first != NULL)
					lht_tree_del(nd->data.list.first);

				for(r = gdl_first(&tree->rows); r != NULL; r = gdl_next(&tree->rows, r)) {
					lht_node_t *n = lht_dom_node_alloc(LHT_TEXT, NULL);
					lht_dom_list_append(nd, n);
					n->data.text.value = rnd_strdup(r->cell[0]);
				}
				rnd_conf_makedirty(ctx->role);
				rnd_conf_update(ctx->nat->hash_path, ctx->idx);
			}
			return;
		case RND_CFN_HLIST:
		case RND_CFN_max:
			return;
	}

	if (val == NULL)
		val = "";
	rnd_conf_set(ctx->role, ctx->nat->hash_path, ctx->idx,  val, RND_POL_OVERWRITE);

	if ((ctx->role == RND_CFR_USER) || (ctx->role == RND_CFR_PROJECT))
		rnd_conf_save_file(&PCB->hidlib, NULL, (PCB == NULL ? NULL : PCB->hidlib.filename), ctx->role, NULL);
	else if (ctx->role == RND_CFR_DESIGN)
		pcb_board_set_changed_flag(PCB, 1);

	rnd_gui->invalidate_all(rnd_gui);
}

static void pref_conf_editval_del_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);

	if (r != NULL) {
		rnd_dad_tree_remove(attr, r);
		pref_conf_editval_cb(hid_ctx, caller_data, trigger_attr);
	}
}

static void pref_conf_editval_edit(void *hid_ctx, confedit_ctx_t *ctx, rnd_hid_attribute_t *attr, rnd_hid_row_t *r)
{
	char *nv = rnd_hid_prompt_for(&PCB->hidlib, "list item value:", r->cell[0], "Edit config list item");
	if (nv == NULL)
		return;

	if (rnd_dad_tree_modify_cell(attr, r, 0, rnd_strdup(nv)) == 0)
		pref_conf_editval_cb(hid_ctx, ctx, attr);
}

static void pref_conf_editval_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);

	if (r != NULL)
		pref_conf_editval_edit(hid_ctx, ctx, attr, r);
}

static void pref_conf_editval_ins_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);
	char *cols[] = {NULL, NULL};

	cols[0] = rnd_strdup("");

	if (trigger_attr == &ctx->dlg[ctx->winsa])
		r = rnd_dad_tree_append(attr, r, cols);
	else
		r = rnd_dad_tree_insert(attr, r, cols);
	if (r != NULL)
		pref_conf_editval_edit(hid_ctx, ctx, attr, r);
}

static void pref_conf_editval_hlist_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	rnd_actionva(&PCB->hidlib, ctx->nat->gui_edit_act,
		rnd_conf_role_name(ctx->role), ctx->nat->hash_path, trigger_attr->val.str,
		NULL);
}


static void pref_conf_edit_dlg(rnd_conf_native_t *nat, long idx, rnd_conf_role_t role, rnd_conflist_t *hlist, rnd_bool modal)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	confedit_ctx_t *ctx;
	int b[4] = {0};

	ctx = calloc(sizeof(confedit_ctx_t), 1);
	ctx->nat = nat;
	ctx->idx = idx;
	ctx->role = role;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

		RND_DAD_LABEL(ctx->dlg, ctx->nat->hash_path);

		switch(ctx->nat->type) {
			case RND_CFN_STRING:
				RND_DAD_STRING(ctx->dlg);
					ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = RND_DAD_CURRENT(ctx->dlg);
				break;
			case RND_CFN_BOOLEAN:
				RND_DAD_BOOL(ctx->dlg);
					ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case RND_CFN_INTEGER:
				RND_DAD_INTEGER(ctx->dlg);
					ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_MINMAX(ctx->dlg, -(1<<30), +(1<<30));
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = RND_DAD_CURRENT(ctx->dlg);
				break;
			case RND_CFN_REAL:
				RND_DAD_REAL(ctx->dlg);
					ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_MINMAX(ctx->dlg, -(1<<30), +(1<<30));
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = RND_DAD_CURRENT(ctx->dlg);
				break;
			case RND_CFN_COORD:
				RND_DAD_COORD(ctx->dlg);
					ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_MINMAX(ctx->dlg, -RND_MM_TO_COORD(1000), +RND_MM_TO_COORD(1000));
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = RND_DAD_CURRENT(ctx->dlg);
				break;
			case RND_CFN_UNIT:
				RND_DAD_UNIT(ctx->dlg, 0x3fff);
					ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case RND_CFN_COLOR:
				RND_DAD_COLOR(ctx->dlg);
					ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case RND_CFN_LIST:
				RND_DAD_BEGIN_VBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
					RND_DAD_TREE(ctx->dlg, 1, 0, NULL);
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
						ctx->wnewval = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_BEGIN_HBOX(ctx->dlg);
						RND_DAD_BUTTON(ctx->dlg, "Edit...");
							RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_edit_cb);
							b[0] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_BUTTON(ctx->dlg, "Del");
							RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_del_cb);
							b[1] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_BUTTON(ctx->dlg, "Insert before");
							RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_ins_cb);
							b[2] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_BUTTON(ctx->dlg, "Insert after");
							RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_ins_cb);
							b[3] = ctx->winsa = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_END(ctx->dlg);
				RND_DAD_END(ctx->dlg);
				break;
			case RND_CFN_HLIST:
				{
					gdl_iterator_t it;
					rnd_conf_listitem_t *i;

					if (hlist == NULL) {
						RND_DAD_LABEL(ctx->dlg, "ERROR: hlist not available");
						break;
					}

					RND_DAD_BEGIN_VBOX(ctx->dlg);
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
						rnd_conflist_foreach(hlist, &it, i) {
							lht_node_t *rule = i->prop.src;
							RND_DAD_BUTTON(ctx->dlg, rule->name);
								RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_hlist_cb);
						}
					RND_DAD_END(ctx->dlg);
				}
				break;
			case RND_CFN_max:
				RND_DAD_LABEL(ctx->dlg, "ERROR: invalid conf node type");
		}

		if (is_read_only(ctx)) {
			RND_DAD_LABEL(ctx->dlg, "NOTE: this value is read-only");
				RND_DAD_HELP(ctx->dlg, "Config value with this config role\ncan not be modified.\nPlease pick another config role\nand try to edit or create\nthe value there!\n(If that role has higher priority,\nthat value will override this one)");
		}
		else if (confedit_node_is_uninitialized(ctx)) {
			RND_DAD_LABEL(ctx->dlg, "NOTE: change the value to create the config node");
				RND_DAD_HELP(ctx->dlg, "This config node does not exist\non the selected role.\nTo create it, change the value.\nSpecial case for boolean values:\nto create an unticked value,\nfirst tick then untick the checkbox.");
		}

		RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	RND_DAD_END(ctx->dlg);

	RND_DAD_NEW("pref_confedit", ctx->dlg, "pcb-rnd conf item", ctx, modal, pref_conf_edit_close_cb);

	if (is_read_only(ctx)) {
		int n;
		rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wnewval, 0);
		for(n = 0; n < sizeof(b) / sizeof(b[0]); n++)
			if (b[n] != 0)
				rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, b[n], 0);
	}


	confedit_brd2dlg(ctx);
}


static void pref_conf_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *pctx = caller_data;
	rnd_hid_row_t *r;

	if (pctx->conf.selected_nat == NULL) {
		rnd_message(RND_MSG_ERROR, "You need to select a conf leaf node to edit\nTry the tree on the left.\n");
		return;
	}

	r = rnd_dad_tree_get_selected(&pctx->dlg[pctx->conf.wintree]);
	if (r == NULL) {
		rnd_message(RND_MSG_ERROR, "You need to select a role (upper right list)\n");
		return;
	}

	if (pctx->conf.selected_idx >= pctx->conf.selected_nat->array_size) {
		rnd_message(RND_MSG_ERROR, "Internal error: array index out of bounds\n");
		return;
	}

	if (pctx->conf.selected_nat->type == RND_CFN_HLIST) {
		if (pctx->conf.selected_nat->gui_edit_act == NULL) {
			rnd_message(RND_MSG_ERROR, "ERROR: can not edit hash lists on GUI\n");
			return;
		}
	}

	pref_conf_edit_dlg(pctx->conf.selected_nat, pctx->conf.selected_idx, r->user_data2.lng, pctx->conf.selected_nat->val.list, rnd_false);
}

const char pcb_acts_dlg_confval_edit[] = "dlg_confval_edit(path, idx, role, [modal])\n";
const char pcb_acth_dlg_confval_edit[] = "Present a dialog box for editing the value of a conf node at path.";
fgw_error_t pcb_act_dlg_confval_edit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path, *srole, *smodal = NULL;
	rnd_bool modal;
	long idx;
	rnd_conf_native_t *nat;
	rnd_conf_role_t role;

	RND_ACT_CONVARG(1, FGW_STR, dlg_confval_edit, path = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_LONG, dlg_confval_edit, idx = argv[2].val.nat_long);
	RND_ACT_CONVARG(3, FGW_STR, dlg_confval_edit, srole = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, dlg_confval_edit, smodal = argv[4].val.str);

	nat = rnd_conf_get_field(path);
	if (nat == NULL) {
		rnd_message(RND_MSG_ERROR, "ERROR: no such config path: '%s'\n", path);
		return -1;
	}


	modal = rnd_istrue(smodal);
	role = rnd_conf_role_parse(srole);
	if (role == RND_CFR_invalid) {
		rnd_message(RND_MSG_ERROR, "ERROR: no such config role: '%s'\n", srole);
		return -1;
	}

	pref_conf_edit_dlg(nat, idx, role, NULL, modal);
	RND_ACT_IRES(0);
	return 0;
}


static void pref_conf_del_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *pctx = caller_data;
	rnd_hid_row_t *r;

	if (pctx->conf.selected_nat == NULL) {
		rnd_message(RND_MSG_ERROR, "You need to select a conf leaf node to remove\nTry the tree on the left.\n");
		return;
	}

	r = rnd_dad_tree_get_selected(&pctx->dlg[pctx->conf.wintree]);
	if (r == NULL) {
		rnd_message(RND_MSG_ERROR, "You need to select a role (upper right list)\n");
		return;
	}

	if (pctx->conf.selected_idx >= pctx->conf.selected_nat->array_size) {
		rnd_message(RND_MSG_ERROR, "Internal error: array index out of bounds\n");
		return;
	}

	if (rnd_conf_is_read_only(r->user_data2.lng)) {
		rnd_message(RND_MSG_ERROR, "Role is read-only, can not remove item\n");
		return;
	}

	rnd_conf_del(r->user_data2.lng, pctx->conf.selected_nat->hash_path, pctx->conf.selected_idx);
}

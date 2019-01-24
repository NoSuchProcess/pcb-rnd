/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#define is_read_only(ctx) ((ctx->role == CFR_INTERNAL) || (ctx->role == CFR_SYSTEM) || (ctx->role == CFR_DEFAULTPCB))

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)

	conf_native_t *nat;
	int idx;
	conf_role_t role;
	
	int wnewval, winsa;
} confedit_ctx_t;

static void pref_conf_edit_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pref_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void confedit_brd2dlg(confedit_ctx_t *ctx)
{
	pcb_hid_attr_val_t hv;
	lht_node_t *nl, *nd = conf_lht_get_at(ctx->role, ctx->nat->hash_path, 1);
	const char *val;

	if (ctx->idx >= ctx->nat->array_size)
		return; /* shouldn't ever happen - we have checked this before creating the dialog! */

	val = pref_conf_get_val(nd, ctx->nat, ctx->idx);

	switch(ctx->nat->type) {
		case CFN_STRING:
			hv.str_value = val;
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case CFN_BOOLEAN:
		case CFN_INTEGER:
			hv.int_value = atoi(val);
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case CFN_REAL:
			hv.real_value = strtod(val, NULL);
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case CFN_COORD:
			hv.coord_value = pcb_get_value(val, NULL, NULL, NULL);
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case CFN_UNIT:
			{
				const pcb_unit_t *u = get_unit_struct(val);
				if (u != NULL)
					hv.int_value = u - pcb_units;
				else
					hv.int_value = -1;
				pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			}
			break;
		case CFN_COLOR:
			hv.clr_value = ctx->nat->val.color[ctx->idx];
			pcb_color_load_str(&hv.clr_value, val);
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnewval, &hv);
			break;
		case CFN_LIST:
			{
				pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
				pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
			
				pcb_dad_tree_clear(tree);
				if (nd->type != LHT_LIST)
					return;
				for(nl = nd->data.list.first; nl != NULL; nl = nl->next) {
					char *cell[2] = {NULL};
					if (nl->type == LHT_TEXT)
						cell[0] = pcb_strdup(nl->data.text.value);
					pcb_dad_tree_append(attr, NULL, cell);
				}
			}
			break;
		case CFN_max:
			PCB_DAD_LABEL(ctx->dlg, "ERROR: invalid conf node type");
	}
}

static void pref_conf_editval_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr;
	char buf[128];
	const char *val = buf;

	if (ctx->idx >= ctx->nat->array_size)
		return; /* shouldn't ever happen - we have checked this before creating the dialog! */

	attr = &ctx->dlg[ctx->wnewval];

	switch(ctx->nat->type) {
		case CFN_STRING:  val = attr->default_val.str_value; break;
		case CFN_BOOLEAN:
		case CFN_INTEGER: sprintf(buf, "%d", attr->default_val.int_value); break;
		case CFN_REAL:    sprintf(buf, "%f", attr->default_val.real_value); break;
		case CFN_COORD:   pcb_snprintf(buf, sizeof(buf), "%.08$mH", attr->default_val.coord_value); break;
		case CFN_UNIT:
			if ((attr->default_val.int_value < 0) || (attr->default_val.int_value >= pcb_get_n_units()))
				return;
			val = pcb_units[attr->default_val.int_value].suffix;
			break;
		case CFN_COLOR:   val = attr->default_val.clr_value.str; break;
		case CFN_LIST:
			{
				pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
				pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
				pcb_hid_row_t *r;
				lht_node_t *nd = conf_lht_get_at(ctx->role, ctx->nat->hash_path, 0);

				if (nd == NULL) {
					pcb_message(PCB_MSG_ERROR, "Internal error: can't copy back to non-existing list!\n");
					return;
				}

				if (nd->type != LHT_LIST) {
					pcb_message(PCB_MSG_ERROR, "Internal error: can't copy back list into non-list!\n");
					return;
				}

				/* empty the list so that we insert to an empty list which is overwriting the list */
				while(nd->data.list.first != NULL)
					lht_tree_del(nd->data.list.first);

				for(r = gdl_first(&tree->rows); r != NULL; r = gdl_next(&tree->rows, r)) {
					lht_node_t *n = lht_dom_node_alloc(LHT_TEXT, NULL);
					lht_dom_list_append(nd, n);
					n->data.text.value = pcb_strdup(r->cell[0]);
				}
				conf_makedirty(ctx->role);
				conf_update(ctx->nat->hash_path, ctx->idx);
			}
			return;
		case CFN_max:
			return;
	}

	if (val == NULL)
		val = "";
	conf_set(ctx->role, ctx->nat->hash_path, ctx->idx,  val, POL_OVERWRITE);

	if ((ctx->role == CFR_USER) || (ctx->role == CFR_PROJECT))
		conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), ctx->role, NULL);
	else if (ctx->role == CFR_DESIGN)
		pcb_board_set_changed_flag(1);

	pcb_gui->invalidate_all();
}

static void pref_conf_editval_del_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(attr);

	if (r != NULL) {
		pcb_dad_tree_remove(attr, r);
		pref_conf_editval_cb(hid_ctx, caller_data, trigger_attr);
	}
}

static void pref_conf_editval_edit(void *hid_ctx, confedit_ctx_t *ctx, pcb_hid_attribute_t *attr, pcb_hid_row_t *r)
{
	char *nv = pcb_hid_prompt_for("list item value:", r->cell[0], "Edit config list item");
	if (nv == NULL)
		return;

	if (pcb_dad_tree_modify_cell(attr, r, 0, pcb_strdup(nv)) == 0)
		pref_conf_editval_cb(hid_ctx, ctx, attr);
}

static void pref_conf_editval_edit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(attr);

	if (r != NULL)
		pref_conf_editval_edit(hid_ctx, ctx, attr, r);
}

static void pref_conf_editval_ins_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *trigger_attr)
{
	confedit_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wnewval];
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(attr);
	char *cols[] = {NULL, NULL};

	cols[0] = pcb_strdup("");

	if (trigger_attr == &ctx->dlg[ctx->winsa])
		r = pcb_dad_tree_append(attr, r, cols);
	else
		r = pcb_dad_tree_insert(attr, r, cols);
	if (r != NULL)
		pref_conf_editval_edit(hid_ctx, ctx, attr, r);
}


static void pref_conf_edit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	pref_ctx_t *pctx = caller_data;
	confedit_ctx_t *ctx;
	pcb_hid_row_t *r;
	int b[4] = {0};

	if (pctx->conf.selected_nat == NULL) {
		pcb_message(PCB_MSG_ERROR, "You need to select a conf leaf node to edit\nTry the tree on the left.\n");
		return;
	}

	r = pcb_dad_tree_get_selected(&pctx->dlg[pctx->conf.wintree]);
	if (r == NULL) {
		pcb_message(PCB_MSG_ERROR, "You need to select a role (upper right list)\n");
		return;
	}

	if (pctx->conf.selected_idx >= pctx->conf.selected_nat->array_size) {
		pcb_message(PCB_MSG_ERROR, "Internal error: array index out of bounds\n");
		return;
	}

	ctx = calloc(sizeof(confedit_ctx_t), 1);
	ctx->nat = pctx->conf.selected_nat;
	ctx->idx = pctx->conf.selected_idx;
	ctx->role = r->user_data2.lng;

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

		PCB_DAD_LABEL(ctx->dlg, ctx->nat->hash_path);

		switch(ctx->nat->type) {
			case CFN_STRING:
				PCB_DAD_STRING(ctx->dlg);
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = PCB_DAD_CURRENT(ctx->dlg);
				break;
			case CFN_BOOLEAN:
				PCB_DAD_BOOL(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case CFN_INTEGER:
				PCB_DAD_INTEGER(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_MINMAX(ctx->dlg, -(1<<30), +(1<<30));
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = PCB_DAD_CURRENT(ctx->dlg);
				break;
			case CFN_REAL:
				PCB_DAD_REAL(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_MINMAX(ctx->dlg, -(1<<30), +(1<<30));
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = PCB_DAD_CURRENT(ctx->dlg);
				break;
			case CFN_COORD:
				PCB_DAD_COORD(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_MINMAX(ctx->dlg, -PCB_MM_TO_COORD(1000), +PCB_MM_TO_COORD(1000));
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
					b[0] = PCB_DAD_CURRENT(ctx->dlg);
				break;
			case CFN_UNIT:
				PCB_DAD_UNIT(ctx->dlg);
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case CFN_COLOR:
				PCB_DAD_COLOR(ctx->dlg);
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case CFN_LIST:
				PCB_DAD_BEGIN_VBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
					PCB_DAD_TREE(ctx->dlg, 1, 0, NULL);
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
						ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_BEGIN_HBOX(ctx->dlg);
						PCB_DAD_BUTTON(ctx->dlg, "Edit...");
							PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_edit_cb);
							b[0] = PCB_DAD_CURRENT(ctx->dlg);
						PCB_DAD_BUTTON(ctx->dlg, "Del");
							PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_del_cb);
							b[1] = PCB_DAD_CURRENT(ctx->dlg);
						PCB_DAD_BUTTON(ctx->dlg, "Insert before");
							PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_ins_cb);
							b[2] = PCB_DAD_CURRENT(ctx->dlg);
						PCB_DAD_BUTTON(ctx->dlg, "Insert after");
							PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_ins_cb);
							b[3] = ctx->winsa = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_END(ctx->dlg);
				PCB_DAD_END(ctx->dlg);
				break;
			case CFN_max:
				PCB_DAD_LABEL(ctx->dlg, "ERROR: invalid conf node type");
		}

		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW("pref_confedit", ctx->dlg, "pcb-rnd conf item", ctx, pcb_false, pref_conf_edit_close_cb);

	if (is_read_only(ctx)) {
		int n;
		pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wnewval, 0);
		for(n = 0; n < sizeof(b) / sizeof(b[0]); n++)
			if (b[n] != 0)
				pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, b[n], 0);
	}


	confedit_brd2dlg(ctx);
}

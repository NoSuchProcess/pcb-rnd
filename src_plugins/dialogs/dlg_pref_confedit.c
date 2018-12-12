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

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)

	conf_native_t *nat;
	int idx;
	conf_role_t role;
	
	int wnewval;
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
	lht_node_t *nd = conf_lht_get_at(ctx->role, ctx->nat->hash_path, 1);
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
TODO("needs more code")
			PCB_DAD_LABEL(ctx->dlg, "ERROR: TODO: list");
		case CFN_max:
			PCB_DAD_LABEL(ctx->dlg, "ERROR: invalid conf node type");
	}
}

static void pref_conf_editval_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

}

static void pref_conf_edit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	pref_ctx_t *pctx = caller_data;
	confedit_ctx_t *ctx;
	pcb_hid_row_t *r;

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

	PCB_DAD_BEGIN_VBOX(pref_ctx.dlg);
		PCB_DAD_COMPFLAG(pref_ctx.dlg, PCB_HATF_EXPFILL);

		PCB_DAD_LABEL(ctx->dlg, ctx->nat->hash_path);

		switch(ctx->nat->type) {
			case CFN_STRING:
				PCB_DAD_STRING(ctx->dlg);
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case CFN_BOOLEAN:
				PCB_DAD_BOOL(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case CFN_INTEGER:
				PCB_DAD_INTEGER(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case CFN_REAL:
				PCB_DAD_REAL(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
				break;
			case CFN_COORD:
				PCB_DAD_COORD(ctx->dlg, "");
					ctx->wnewval = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "apply");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_editval_cb);
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
TODO("needs more code")
				PCB_DAD_LABEL(ctx->dlg, "ERROR: TODO: list");
				break;
			case CFN_max:
				PCB_DAD_LABEL(ctx->dlg, "ERROR: invalid conf node type");
		}

		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW(ctx->dlg, "pcb-rnd conf item", ctx, pcb_false, pref_conf_edit_close_cb);

	confedit_brd2dlg(ctx);
}

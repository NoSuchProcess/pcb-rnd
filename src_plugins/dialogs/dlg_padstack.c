/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "obj_pstk.h"
#include "obj_pstk_op.h"
#include "obj_pstk_inlines.h"
#include "operation.h"

typedef struct pse_proto_layer_s {
	const char *name;
	pcb_layer_type_t mask;
	pcb_layer_combining_t comb;

	int auto_from[2];
	pcb_coord_t auto_bloat;
} pse_proto_layer_t;

static const pse_proto_layer_t pse_layer[] = {
	{"top paste",            PCB_LYT_TOP | PCB_LYT_PASTE,     PCB_LYC_AUTO,             {2,-1}, 0},
	{"top mask",             PCB_LYT_TOP | PCB_LYT_MASK,      PCB_LYC_SUB|PCB_LYC_AUTO, {2,-1}, PCB_MIL_TO_COORD(4)},
	{"top copper",           PCB_LYT_TOP | PCB_LYT_COPPER,    0,                        {4,3},  0},
	{"any internal copper",  PCB_LYT_INTERN | PCB_LYT_COPPER, 0,                        {2,4},  0},
	{"bottom copper",        PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0,                        {2,3},  0},
	{"bottom mask",          PCB_LYT_BOTTOM | PCB_LYT_MASK,   PCB_LYC_SUB|PCB_LYC_AUTO, {4,-1}, PCB_MIL_TO_COORD(4)},
	{"bottom paste",         PCB_LYT_BOTTOM | PCB_LYT_PASTE,  PCB_LYC_AUTO,             {4,-1},  0}
};
#define pse_num_layers (sizeof(pse_layer) / sizeof(pse_layer[0]))

typedef struct pse_s {
	pcb_hid_attribute_t *attrs;
	pcb_board_t *pcb;
	pcb_pstk_t *ps;
	int tab;

	/* widget IDs */
	int tab_instance, tab_prototype;
	int but_instance, but_prototype;
	int proto_id, clearance, rot, xmirror, smirror;
	int proto_shape[pse_num_layers];
	int proto_info[pse_num_layers];
	int proto_change[pse_num_layers];
	pcb_coord_t proto_clr[pse_num_layers];
	int hole_header;
	int hdia, hplated;
	int htop_val, htop_text, htop_layer;
	int hbot_val, hbot_text, hbot_layer;

	/* sub-dialog: shape change */
	void *parent_hid_ctx;
	int editing_shape; /* index of the shape being edited */
	pcb_hid_attribute_t *shape_chg;
	int text_shape, del, derive;
	int copy_do, copy_from;
	int shrink, amount, grow;
} pse_t;

static void pse_tab_update(void *hid_ctx, pse_t *pse)
{
	switch(pse->tab) {
		case 0:
			pcb_gui->attr_dlg_widget_hide(hid_ctx, pse->tab_instance, pcb_false);
			pcb_gui->attr_dlg_widget_hide(hid_ctx, pse->tab_prototype, pcb_true);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_instance, pcb_false);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_prototype, pcb_true);
			break;
		case 1:
			pcb_gui->attr_dlg_widget_hide(hid_ctx, pse->tab_instance, pcb_true);
			pcb_gui->attr_dlg_widget_hide(hid_ctx, pse->tab_prototype, pcb_false);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_instance, pcb_true);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_prototype, pcb_false);
			break;
	}
}

/* build a group/layer name string in tmp */
char *pse_group_string(pcb_board_t *pcb, pcb_layergrp_t *grp, char *out, int size)
{
	const char *lname = "", *gname = "";

	if (grp != NULL) {
		gname = grp->name;
		if (grp->len > 0) {
			pcb_layer_t *l = pcb_get_layer(pcb->Data, grp->lid[0]);
			if (l != NULL)
				lname = l->name;
		}
	}

	pcb_snprintf(out, size, "%s\n[%s]", gname, lname);
	return out;
}

/* Convert from padstack to dialog */
static void pse_ps2dlg(void *hid_ctx, pse_t *pse)
{
	char tmp[256], *s;
	int n;
	pcb_pstk_proto_t *proto;
	pcb_layergrp_id_t top_gid, bottom_gid;
	pcb_layergrp_t *top_grp, *bottom_grp;
	pcb_bb_type_t htype;

	htype = pcb_pstk_bbspan(pse->pcb, pse->ps, &top_gid, &bottom_gid, &proto);
	top_grp = pcb_get_layergrp(pse->pcb, top_gid);
	bottom_grp = pcb_get_layergrp(pse->pcb, bottom_gid);

	/* instance */
	sprintf(tmp, "#%ld, %d", (long int)pse->ps->proto, pse->ps->protoi);
	PCB_DAD_SET_VALUE(hid_ctx, pse->proto_id, str_value, tmp);
	PCB_DAD_SET_VALUE(hid_ctx, pse->clearance, coord_value, pse->ps->Clearance);
	PCB_DAD_SET_VALUE(hid_ctx, pse->rot, real_value, pse->ps->rot);
	PCB_DAD_SET_VALUE(hid_ctx, pse->xmirror, int_value, pse->ps->xmirror);
	PCB_DAD_SET_VALUE(hid_ctx, pse->smirror, int_value, pse->ps->smirror);

	/* proto - layers */
	for(n = 0; n < pse_num_layers; n++) {
		pcb_pstk_shape_t *shape = pcb_pstk_shape(pse->ps, pse_layer[n].mask, pse_layer[n].comb);
		if (shape != NULL) {
			switch(shape->shape) {
				case PCB_PSSH_CIRC:
					PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "circle");
					if ((shape->data.circ.x != 0) || (shape->data.circ.y != 0))
						pcb_snprintf(tmp, sizeof(tmp), "dia=%.06$mm\nat %.06$mm;%.06$mm", shape->data.circ.dia, shape->data.circ.x, shape->data.circ.y);
					else
						pcb_snprintf(tmp, sizeof(tmp), "dia=%.06$mm", shape->data.circ.dia);
					break;
				case PCB_PSSH_LINE:
					if (shape->data.line.square)
						PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "square line");
					else
						PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "round line");
					pcb_snprintf(tmp, sizeof(tmp), "thickness=%.06$mm", shape->data.line.thickness);
					break;
				case PCB_PSSH_POLY:
					PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "polygon");
					pcb_snprintf(tmp, sizeof(tmp), "corners=%d", shape->data.poly.len);
					break;
				default:
					PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "<unknown>");
					strcpy(tmp, "<unknown>");
			}
			PCB_DAD_SET_VALUE(hid_ctx, pse->proto_info[n], str_value, tmp);
			PCB_DAD_SET_VALUE(hid_ctx, pse->proto_clr[n], coord_value, shape->clearance);
		}
		else {
			PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "");
			PCB_DAD_SET_VALUE(hid_ctx, pse->proto_info[n], str_value, "");
			PCB_DAD_SET_VALUE(hid_ctx, pse->proto_clr[n], coord_value, 0);
		}
	}

	/* proto - hole */
	s = "Hole geometry (<unknown>):";
	switch(htype) {
		case PCB_BB_NONE: s = "Hole geometry (there is no hole in this padstack):"; break;
		case PCB_BB_THRU: s = "Hole geometry (all-way-through hole):"; break;
		case PCB_BB_BB: s = "Hole geometry (blind and/or buried hole):"; break;
		case PCB_BB_INVALID: s = "Hole geometry (INVALID HOLE):"; break;
	}
	PCB_DAD_SET_VALUE(hid_ctx, pse->hole_header, str_value, s);

	PCB_DAD_SET_VALUE(hid_ctx, pse->hdia, coord_value, proto->hdia);
	PCB_DAD_SET_VALUE(hid_ctx, pse->hplated, int_value, proto->hplated);
	PCB_DAD_SET_VALUE(hid_ctx, pse->htop_val, int_value, proto->htop);
	PCB_DAD_SET_VALUE(hid_ctx, pse->hbot_val, int_value, proto->hbottom);

	if (proto->htop == 0)
		strcpy(tmp, "top copper group");
	else
		sprintf(tmp, "%d groups from\nthe %s copper group", proto->htop, proto->htop > 0 ? "top" : "bottom");
	PCB_DAD_SET_VALUE(hid_ctx, pse->htop_text, str_value, tmp);
	PCB_DAD_SET_VALUE(hid_ctx, pse->htop_layer, str_value, pse_group_string(pse->pcb, top_grp, tmp, sizeof(tmp)));

	if (proto->hbottom == 0)
		strcpy(tmp, "bottom copper group");
	else
		sprintf(tmp, "%d groups from\nthe %s copper group", proto->hbottom, proto->hbottom > 0 ? "bottom" : "top");
	PCB_DAD_SET_VALUE(hid_ctx, pse->hbot_text, str_value, tmp);
	PCB_DAD_SET_VALUE(hid_ctx, pse->hbot_layer, str_value, pse_group_string(pse->pcb, bottom_grp, tmp, sizeof(tmp)));

}

static void pse_tab_ps(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pse->tab = 0;
	pse_tab_update(hid_ctx, pse);
}

static void pse_tab_proto(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pse->tab = 1;
	pse_tab_update(hid_ctx, pse);
}

static void pse_chg_instance(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	static int lock = 0;

	if (lock != 0)
		return;

	pcb_pstk_change_instance(pse->ps,
		NULL,
		&pse->attrs[pse->clearance].default_val.coord_value,
		&pse->attrs[pse->rot].default_val.real_value,
		&pse->attrs[pse->xmirror].default_val.int_value,
		&pse->attrs[pse->smirror].default_val.int_value);

	lock++;
	pse_ps2dlg(hid_ctx, pse); /* to get calculated text fields updated */
	lock--;

	pcb_gui->invalidate_all();
}

static void pse_chg_hole(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	static int lock = 0;

	if (lock != 0)
		return;

	if (proto != NULL) {
		pcb_pstk_proto_change_hole(proto,
			&pse->attrs[pse->hplated].default_val.int_value,
			&pse->attrs[pse->hdia].default_val.coord_value,
			&pse->attrs[pse->htop_val].default_val.int_value,
			&pse->attrs[pse->hbot_val].default_val.int_value);
	}

	lock++;
	pse_ps2dlg(hid_ctx, pse); /* to get calculated text fields updated */
	lock--;

	pcb_gui->invalidate_all();
}

static void pse_chg_proto_clr(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	static int lock = 0;

	if (lock != 0)
		return;

	if (proto != NULL) {
		int n, idx = -1, sidx;
		pcb_opctx_t ctx;

		for(n = 0; n < pse_num_layers; n++)
			if (pse->proto_clr[n] == (attr - pse->attrs))
				idx = n;
		if (idx < 0) {
			pcb_message(PCB_MSG_ERROR, "Can't find shape - clearance unchanged (a)\n");
			return;
		}

		sidx = pcb_pstk_get_shape_idx(&proto->tr.array[0], pse_layer[idx].mask, pse_layer[idx].comb);
		if (sidx < 0) {
			pcb_message(PCB_MSG_ERROR, "Can't find shape - clearance unchanged (b)\n");
			return;
		}

		ctx.clip.clear = 0;
		ctx.clip.restore = 1;
		pcb_pstkop_clip(&ctx, pse->ps);

		for(n = 0; n < proto->tr.used; n++)
			pcb_pstk_shape_clr_grow(&proto->tr.array[n].shape[sidx], pcb_true, pse->attrs[pse->proto_clr[idx]].default_val.coord_value);

		ctx.clip.clear = 1;
		ctx.clip.restore = 0;
		pcb_pstkop_clip(&ctx, pse->ps);
	}

	lock++;
	pse_ps2dlg(hid_ctx, pse); /* to get calculated text fields updated */
	lock--;

	pcb_gui->invalidate_all();
}

static void pse_shape_del(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_proto_del_shape(proto, pse_layer[pse->editing_shape].mask, pse_layer[pse->editing_shape].comb);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_auto(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	int n, src_idx = -1;
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];
	int dst_idx = pcb_pstk_get_shape_idx(ts, pse_layer[pse->editing_shape].mask, pse_layer[pse->editing_shape].comb);
	char src_shape_names[128];
	char *end = src_shape_names;

	for(n = 0; n < 2; n++) {
		int from = pse_layer[pse->editing_shape].auto_from[n];
		if (from < 0)
			continue;
		src_idx = pcb_pstk_get_shape_idx(ts, pse_layer[from].mask, pse_layer[from].comb);
		if (src_idx >= 0)
			break;
		strcpy(end, pse_layer[from].name);
		end += strlen(pse_layer[from].name);
		*end = ',';
		end++;
	}

	if (src_idx < 0) {
		if (end > src_shape_names)
			end--;
		*end = 0;
		pcb_message(PCB_MSG_ERROR, "Can't derive shape: source shapes (%s) are empty\n", src_shape_names);
		return;
	}

	pcb_pstk_shape_derive(proto, dst_idx, src_idx, pse_layer[pse->editing_shape].auto_bloat, pse_layer[pse->editing_shape].mask, pse_layer[pse->editing_shape].comb);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_copy(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];
	int from = pse->shape_chg[pse->copy_from].default_val.int_value;
	int dst_idx = pcb_pstk_get_shape_idx(ts, pse_layer[pse->editing_shape].mask, pse_layer[pse->editing_shape].comb);
	int src_idx = pcb_pstk_get_shape_idx(ts, pse_layer[from].mask, pse_layer[from].comb);

	if (src_idx < 0) {
		pcb_message(PCB_MSG_ERROR, "Can't copy shape: source shape (%s) is empty\n", pse_layer[from].name);
		return;
	}

	if (src_idx == dst_idx) {
		pcb_message(PCB_MSG_ERROR, "Can't copy shape: source shape and destination shape are the same layer type\n");
		return;
	}

	pcb_pstk_shape_derive(proto, dst_idx, src_idx, 0, pse_layer[pse->editing_shape].mask, pse_layer[pse->editing_shape].comb);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pcb_gui->invalidate_all();
}


static void pse_shape_bloat(void *hid_ctx, void *caller_data, pcb_coord_t sign)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];
	int n, dst_idx = pcb_pstk_get_shape_idx(ts, pse_layer[pse->editing_shape].mask, pse_layer[pse->editing_shape].comb);
	pcb_coord_t bloat = pse->shape_chg[pse->amount].default_val.coord_value;

	if (bloat <= 0)
		return;

	if (dst_idx < 0) {
		pcb_message(PCB_MSG_ERROR, "Can't copy shape: source shape (%s) is empty\n", pse_layer[pse->editing_shape].name);
		return;
	}

	bloat *= sign;
	for(n = 0; n < proto->tr.used; n++)
		pcb_pstk_shape_grow(&proto->tr.array[n].shape[dst_idx], pcb_false, bloat);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_shrink(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_shape_bloat(hid_ctx, caller_data, -1);
}

static void pse_shape_grow(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_shape_bloat(hid_ctx, caller_data, +1);
}

static void pse_chg_shape(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	int n;
	char tmp[256];
	const char *copy_from_names[pse_num_layers+1];
	PCB_DAD_DECL(dlg);

	pse->parent_hid_ctx = hid_ctx;

	for(n = 0; n < pse_num_layers; n++)
		copy_from_names[n] = pse_layer[n].name;
	copy_from_names[n] = NULL;

	pse->editing_shape = -1;
	for(n = 0; n < pse_num_layers; n++) {
		if (pse->proto_change[n] == (attr - pse->attrs)) {
			pse->editing_shape = n;
			break;
		}
	}

	PCB_DAD_BEGIN_VBOX(dlg);
		sprintf(tmp, "Automatically generate shape for ...");
		PCB_DAD_LABEL(dlg, tmp);
		PCB_DAD_LABEL(dlg, "");
			pse->text_shape = PCB_DAD_CURRENT(dlg);
		PCB_DAD_BUTTON(dlg, "Delete (no shape)");
			pse->del = PCB_DAD_CURRENT(dlg);
			PCB_DAD_CHANGE_CB(dlg, pse_shape_del);
			PCB_DAD_HELP(dlg, "Remove the shape from this layer type");
		PCB_DAD_BUTTON(dlg, "Derive automatically");
			pse->derive = PCB_DAD_CURRENT(dlg);
			PCB_DAD_CHANGE_CB(dlg, pse_shape_auto);
			PCB_DAD_HELP(dlg, "Derive the shape for this layer type\nfrom other, existing shapes of this padstack\n(automatic)");
		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BUTTON(dlg, "Copy shape from");
				pse->copy_do = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_shape_copy);
				PCB_DAD_HELP(dlg, "Copy the shape for this layer type\nfrom other, existing shapes of this padstack\nfrom the layer type selected");
			PCB_DAD_ENUM(dlg, copy_from_names); /* coposite */
				pse->copy_from = PCB_DAD_CURRENT(dlg);
			PCB_DAD_HELP(dlg, "Select the source layer type for manual shape copy");
		PCB_DAD_END(dlg);

		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BUTTON(dlg, "Shrink");
				pse->shrink = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_shape_shrink);
				PCB_DAD_HELP(dlg, "Make the shape smaller by the selected amount");
			PCB_DAD_COORD(dlg, "");
				pse->amount = PCB_DAD_CURRENT(dlg);
				PCB_DAD_MINMAX(dlg, 1, PCB_MM_TO_COORD(100));
			PCB_DAD_BUTTON(dlg, "Grow");
				pse->grow = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_shape_grow);
				PCB_DAD_HELP(dlg, "Make the shape larger by the selected amount");
		PCB_DAD_END(dlg);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW(dlg, "dlg_padstack_edit_shape", "Edit padstack shape", pse, pcb_true, NULL);
	pse->shape_chg = dlg;

/*	pse_ps2dlg(dlg_hid_ctx, pse);*/
	PCB_DAD_RUN(dlg);

	pse->shape_chg = NULL;
	PCB_DAD_FREE(dlg);
}

static const char pcb_acts_PadstackEdit[] = "PadstackEdit(object)\n";
static const char pcb_acth_PadstackEdit[] = "interactive pad stack editor";
static int pcb_act_PadstackEdit(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int n;
	pse_t pse;
	PCB_DAD_DECL(dlg);

	memset(&pse, 0, sizeof(pse));

	if ((argc == 0) || (pcb_strcasecmp(argv[0], "object") == 0)) {
		void *ptr1, *ptr2 = NULL, *ptr3;
		long type;
		pcb_gui->get_coords("Click on a padstack to edit", &x, &y);
		type = pcb_search_screen(x, y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART | PCB_LOOSE_SUBC, &ptr1, &ptr2, &ptr3);
		if (type != PCB_OBJ_PSTK) {
			pcb_message(PCB_MSG_ERROR, "Need a padstack.\n");
			return 1;
		}
		pse.ps = ptr2;
	}
	else
		PCB_ACT_FAIL(PadstackEdit);

	pse.pcb = pcb_data_get_top(pse.ps->parent.data);
	if (pse.pcb == NULL)
		pse.pcb = PCB;

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BUTTON(dlg, "this instance");
				pse.but_instance = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_tab_ps);
				PCB_DAD_HELP(dlg, "Change the properties of\nthis one padstack instance");
			PCB_DAD_BUTTON(dlg, "prototype");
				pse.but_prototype = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_tab_proto);
				PCB_DAD_HELP(dlg, "Change the properties of\nthe padstack prototype used by\nthis padstack\n(affects all padstacks using the same prototype)");
		PCB_DAD_END(dlg);

		/* this instance */
		PCB_DAD_BEGIN_VBOX(dlg);
			pse.tab_instance = PCB_DAD_CURRENT(dlg);
			PCB_DAD_BEGIN_VBOX(dlg);
				PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
				PCB_DAD_LABEL(dlg, "Setting that affect only this padstack instance");
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_LABEL(dlg, "prototype");
					PCB_DAD_BUTTON(dlg, "#5");
						pse.proto_id = PCB_DAD_CURRENT(dlg);
						PCB_DAD_HELP(dlg, "Padstack prototype ID\n(click to use a different prototype)");
				PCB_DAD_END(dlg);
				PCB_DAD_BEGIN_TABLE(dlg, 2);
					PCB_DAD_LABEL(dlg, "Clearance");
					PCB_DAD_COORD(dlg, "");
						pse.clearance = PCB_DAD_CURRENT(dlg);
						PCB_DAD_MINVAL(dlg, 1);
						PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
						PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
						PCB_DAD_HELP(dlg, "global clearance (affects all layers)");
					PCB_DAD_LABEL(dlg, "Rotation");
					PCB_DAD_REAL(dlg, "");
						pse.rot = PCB_DAD_CURRENT(dlg);
						PCB_DAD_MINVAL(dlg, 0);
						PCB_DAD_MAXVAL(dlg, 360);
						PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
					PCB_DAD_LABEL(dlg, "X-mirror");
					PCB_DAD_BOOL(dlg, "");
						pse.xmirror = PCB_DAD_CURRENT(dlg);
						PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
					PCB_DAD_LABEL(dlg, "S-mirror");
					PCB_DAD_BOOL(dlg, "");
						pse.smirror = PCB_DAD_CURRENT(dlg);
						PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);
		PCB_DAD_END(dlg);

		/* prototype */
		PCB_DAD_BEGIN_VBOX(dlg);
			pse.tab_prototype = PCB_DAD_CURRENT(dlg);
			PCB_DAD_BEGIN_VBOX(dlg);
				PCB_DAD_LABEL(dlg, "Setting that affect all padstacks with the same prototype");
				PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
				PCB_DAD_LABEL(dlg, "Pad geometry per layer type:");
				PCB_DAD_BEGIN_TABLE(dlg, 5);
					PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
					for(n = 0; n < pse_num_layers; n++) {
						PCB_DAD_LABEL(dlg, pse_layer[n].name);
						PCB_DAD_LABEL(dlg, "-");
							pse.proto_shape[n] = PCB_DAD_CURRENT(dlg);
						PCB_DAD_LABEL(dlg, "-");
							pse.proto_info[n] = PCB_DAD_CURRENT(dlg);
						PCB_DAD_BUTTON(dlg, "change...");
							pse.proto_change[n] = PCB_DAD_CURRENT(dlg);
							PCB_DAD_CHANGE_CB(dlg, pse_chg_shape);
							PCB_DAD_HELP(dlg, "Change the shape on this layer type");
						PCB_DAD_COORD(dlg, "");
							pse.proto_clr[n] = PCB_DAD_CURRENT(dlg);
							PCB_DAD_MINVAL(dlg, 1);
							PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
							PCB_DAD_CHANGE_CB(dlg, pse_chg_proto_clr);
							PCB_DAD_HELP(dlg, "local, per layer type clearance\n(only when global padstack clearance is 0)");
					}
				PCB_DAD_END(dlg);
			
				PCB_DAD_LABEL(dlg, "Hole properties:");
					pse.hole_header = PCB_DAD_CURRENT(dlg);

				PCB_DAD_BEGIN_TABLE(dlg, 4);

					PCB_DAD_LABEL(dlg, "Diameter:");
					PCB_DAD_COORD(dlg, "");
						pse.hdia = PCB_DAD_CURRENT(dlg);
						PCB_DAD_MINVAL(dlg, 1);
						PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
						PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
					PCB_DAD_LABEL(dlg, ""); /* dummy */
					PCB_DAD_LABEL(dlg, ""); /* dummy */

					PCB_DAD_LABEL(dlg, "Plating:");
					PCB_DAD_BOOL(dlg, "");
						pse.hplated = PCB_DAD_CURRENT(dlg);
						PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
						PCB_DAD_HELP(dlg, "A plated hole galvanically connects layers");
					PCB_DAD_LABEL(dlg, ""); /* dummy */
					PCB_DAD_LABEL(dlg, ""); /* dummy */

					PCB_DAD_LABEL(dlg, "Hole top:");
					PCB_DAD_INTEGER(dlg, "");
						pse.htop_val = PCB_DAD_CURRENT(dlg);
						PCB_DAD_MINVAL(dlg, -(pse.pcb->LayerGroups.cache.copper_len-1));
						PCB_DAD_MAXVAL(dlg, pse.pcb->LayerGroups.cache.copper_len-1);
						PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
						PCB_DAD_HELP(dlg, "Blind/buried via: top end of the hole");
					PCB_DAD_LABEL(dlg, "<text>");
						pse.htop_text = PCB_DAD_CURRENT(dlg);
					PCB_DAD_LABEL(dlg, "<layer>");
						pse.htop_layer = PCB_DAD_CURRENT(dlg);

					PCB_DAD_LABEL(dlg, "Hole bottom:");
					PCB_DAD_INTEGER(dlg, "");
						pse.hbot_val = PCB_DAD_CURRENT(dlg);
						PCB_DAD_MINVAL(dlg, -(pse.pcb->LayerGroups.cache.copper_len-1));
						PCB_DAD_MAXVAL(dlg, pse.pcb->LayerGroups.cache.copper_len-1);
						PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
						PCB_DAD_HELP(dlg, "Blind/buried via: bottom end of the hole");
					PCB_DAD_LABEL(dlg, "<text>");
						pse.hbot_text = PCB_DAD_CURRENT(dlg);
					PCB_DAD_LABEL(dlg, "<layer>");
						pse.hbot_layer = PCB_DAD_CURRENT(dlg);
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);
		PCB_DAD_END(dlg);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW(dlg, "dlg_padstack_edit", "Edit padstack", &pse, pcb_true, NULL);
	pse.attrs = dlg;
	pse_tab_update(dlg_hid_ctx, &pse);
	pse_ps2dlg(dlg_hid_ctx, &pse);
	PCB_DAD_RUN(dlg);

	PCB_DAD_FREE(dlg);
	return 0;
}


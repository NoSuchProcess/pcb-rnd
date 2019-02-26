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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "obj_pstk.h"
#include "obj_pstk_op.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_parent.h"
#include "search.h"
#include "operation.h"
#include "dlg_lib_pstk.h"
#include "dlg_padstack.h"

static const char *shapes[] = { "circle", "square", NULL };
static const char *sides[] = { "all (top, bottom, intern)", "top & bottom only", "top only", "bottom only", "none", NULL };
static pcb_layer_type_t sides_lyt[] = { PCB_LYT_TOP | PCB_LYT_BOTTOM | PCB_LYT_INTERN, PCB_LYT_TOP | PCB_LYT_BOTTOM, PCB_LYT_TOP, PCB_LYT_BOTTOM, 0 };

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
	const char *prn = "";
	int n;
	pcb_pstk_proto_t *proto;
	pcb_layergrp_id_t top_gid, bottom_gid;
	pcb_layergrp_t *top_grp, *bottom_grp;
	pcb_bb_type_t htype;
	char shp_found[32];
	pcb_pstk_tshape_t *ts = pcb_pstk_get_tshape(pse->ps);

	if (ts == NULL)
		return;

	htype = pcb_pstk_bbspan(pse->pcb, pse->ps, &top_gid, &bottom_gid, &proto);
	top_grp = pcb_get_layergrp(pse->pcb, top_gid);
	bottom_grp = pcb_get_layergrp(pse->pcb, bottom_gid);

	/* instance */
	if ((proto != NULL) && (proto->name != NULL))
		prn = proto->name;
	pcb_snprintf(tmp, sizeof(tmp), "#%ld:%d (%s)", (long int)pse->ps->proto, pse->ps->protoi, prn);
	PCB_DAD_SET_VALUE(hid_ctx, pse->proto_id, str_value, tmp);
	PCB_DAD_SET_VALUE(hid_ctx, pse->clearance, coord_value, pse->ps->Clearance);
	PCB_DAD_SET_VALUE(hid_ctx, pse->rot, real_value, pse->ps->rot);
	PCB_DAD_SET_VALUE(hid_ctx, pse->xmirror, int_value, pse->ps->xmirror);
	PCB_DAD_SET_VALUE(hid_ctx, pse->smirror, int_value, pse->ps->smirror);

	/* proto - layers */
	memset(shp_found, 0, sizeof(shp_found));
	for(n = 0; n < pcb_proto_num_layers; n++) {
		pcb_pstk_shape_t *shape = pcb_pstk_shape(pse->ps, pcb_proto_layers[n].mask, pcb_proto_layers[n].comb);

		if (shape != NULL) {
			int shape_idx = shape - ts->shape;
			if ((shape_idx >= 0) && (shape_idx < sizeof(shp_found)))
				shp_found[shape_idx] = 1;
			switch(shape->shape) {
				case PCB_PSSH_HSHADOW:
					*tmp = '\0';
					PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "hshadow");
					break;
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

	{
		int not_found = 0;
		for(n = 0; n < MIN((int)ts->len, (int)sizeof(shp_found)); n++)
			if (shp_found[n] == 0)
				not_found++;
		if (not_found)
			pcb_message(PCB_MSG_ERROR, "This padstack has %d shape(s) that are not listed in the padstack editor\nFor editing all shapes, please break the padstack up\n", not_found);
	}

	/* proto - hole */
	if (proto->mech_idx >= 0) {
		s = "Slot geometry (<unknown>):";
		switch(htype) {
			case PCB_BB_NONE: s = "Slot geometry (there is no slot in this padstack):"; break;
			case PCB_BB_THRU: s = "Slot geometry (all-way-through slot):"; break;
			case PCB_BB_BB: s = "Slot geometry (blind and/or buried slot):"; break;
			case PCB_BB_INVALID: s = "Slot geometry (INVALID SLOT):"; break;
		}
		pcb_gui->attr_dlg_widget_state(hid_ctx, pse->hdia, 0);
	}
	else {
		s = "Hole geometry (<unknown>):";
		switch(htype) {
			case PCB_BB_NONE: s = "Hole geometry (there is no hole in this padstack):"; break;
			case PCB_BB_THRU: s = "Hole geometry (all-way-through hole):"; break;
			case PCB_BB_BB: s = "Hole geometry (blind and/or buried hole):"; break;
			case PCB_BB_INVALID: s = "Hole geometry (INVALID HOLE):"; break;
		}
		pcb_gui->attr_dlg_widget_state(hid_ctx, pse->hdia, 1);
	}
	PCB_DAD_SET_VALUE(hid_ctx, pse->hole_header, str_value, s);

	free((char *)pse->attrs[pse->prname].default_val.str_value);
	pse->attrs[pse->prname].default_val.str_value = NULL;
	PCB_DAD_SET_VALUE(hid_ctx, pse->prname, str_value, pcb_strdup(proto->name == NULL ? "" : proto->name));
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

static void pse_change_callback(pse_t *pse)
{
	if (pse->change_cb != NULL)
		pse->change_cb(pse);
}

static void pse_chg_protoid(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_cardinal_t proto_id;
	pcb_subc_t *subc;
	static int lock = 0;

	if (lock != 0)
		return;

	subc = pcb_obj_parent_subc((pcb_any_obj_t *)pse->ps);
	proto_id = pcb_dlg_pstklib(pse->pcb, (subc == NULL ? 0 : subc->ID), pcb_true, "Select a new prototype to be used on the padstack");
	if (proto_id == PCB_PADSTACK_INVALID)
		return;

	pcb_pstk_change_instance(pse->ps, &proto_id, NULL, NULL, NULL, NULL);

	lock++;
	pse_ps2dlg(hid_ctx, pse); /* to get the button text updated with proto name */
	lock--;

	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

static void pse_chg_protodup(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_cardinal_t proto_id;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);

	assert(pse->ps->parent_type == PCB_PARENT_DATA);
	if (proto == NULL) {
		pcb_message(PCB_MSG_ERROR, "Internal error: can't determine prototype\n");
		return;
	}
	proto_id = pcb_pstk_proto_insert_forcedup(pse->ps->parent.data, proto, 0);
	pcb_pstk_change_instance(pse->ps, &proto_id, NULL, NULL, NULL, NULL);
	pse_ps2dlg(hid_ctx, pse);
	pse_change_callback(pse);
	pcb_gui->invalidate_all();
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

	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

static void pse_chg_prname(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	static int lock = 0;
	const char *new_name = pse->attrs[pse->prname].default_val.str_value;

	if ((lock != 0) || (proto == NULL))
		return;

	if (proto->name == NULL) {
		if ((new_name == NULL) || (*new_name == '\0'))
			return;
	}
	else {
		if (strcmp(proto->name, new_name) == 0)
			return;
	}

	pcb_pstk_proto_change_name(proto, new_name);

	lock++;
	pse_ps2dlg(hid_ctx, pse); /* to get calculated text fields updated */
	lock--;

	pse_change_callback(pse);
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

	pse_change_callback(pse);
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

		for(n = 0; n < pcb_proto_num_layers; n++)
			if (pse->proto_clr[n] == (attr - pse->attrs))
				idx = n;
		if (idx < 0) {
			pcb_message(PCB_MSG_ERROR, "Can't find shape - clearance unchanged (a)\n");
			return;
		}

		sidx = pcb_pstk_get_shape_idx(&proto->tr.array[0], pcb_proto_layers[idx].mask, pcb_proto_layers[idx].comb);
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

	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_del(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_proto_del_shape(proto, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);

	pse_ps2dlg(pse->parent_hid_ctx, pse);

	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_hshadow(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_proto_del_shape(proto, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);
	pcb_pstk_shape_add_hshadow(proto, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);
	pse_ps2dlg(pse->parent_hid_ctx, pse);

	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_auto(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	int n, src_idx = -1;
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];
	int dst_idx;
	char src_shape_names[128];
	char *end = src_shape_names;


	if (ts == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't derive shape: no shapes (empty padstack)\n");
		return;
	}

	dst_idx = pcb_pstk_get_shape_idx(ts, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);

	for(n = 0; n < 2; n++) {
		int from = pcb_proto_layers[pse->editing_shape].auto_from[n];
		if (from < 0)
			continue;
		src_idx = pcb_pstk_get_shape_idx(ts, pcb_proto_layers[from].mask, pcb_proto_layers[from].comb);
		if (src_idx >= 0)
			break;
		strcpy(end, pcb_proto_layers[from].name);
		end += strlen(pcb_proto_layers[from].name);
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

	pcb_pstk_shape_derive(proto, dst_idx, src_idx, pcb_proto_layers[pse->editing_shape].auto_bloat, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_copy(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];
	int from = pse->shape_chg[pse->copy_from].default_val.int_value;
	int dst_idx;
	int src_idx;

	if (ts == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't copy shape: no such source shape (empty padstack)\n");
		return;
	}

	dst_idx = pcb_pstk_get_shape_idx(ts, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);
	src_idx = pcb_pstk_get_shape_idx(ts, pcb_proto_layers[from].mask, pcb_proto_layers[from].comb);

	if (src_idx < 0) {
		pcb_message(PCB_MSG_ERROR, "Can't copy shape: source shape (%s) is empty\n", pcb_proto_layers[from].name);
		return;
	}

	if (src_idx == dst_idx) {
		pcb_message(PCB_MSG_ERROR, "Can't copy shape: source shape and destination shape are the same layer type\n");
		return;
	}

	pcb_pstk_shape_derive(proto, dst_idx, src_idx, 0, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

static void pse_shape_swap(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];
	int from = pse->shape_chg[pse->copy_from].default_val.int_value;
	int dst_idx;
	int src_idx;

	if (ts == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't swap shape: no such shapes (empty padstack)\n");
		return;
	}

	dst_idx = pcb_pstk_get_shape_idx(ts, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);
	src_idx = pcb_pstk_get_shape_idx(ts, pcb_proto_layers[from].mask, pcb_proto_layers[from].comb);

	if (src_idx < 0) {
		pcb_message(PCB_MSG_ERROR, "Can't swap shape: source shape (%s) is empty\n", pcb_proto_layers[from].name);
		return;
	}

	if (src_idx == dst_idx) {
		pcb_message(PCB_MSG_ERROR, "Can't swap shape: source shape and destination shape are the same layer type\n");
		return;
	}

	pcb_pstk_shape_swap_layer(proto, dst_idx, src_idx);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}


static void pse_shape_bloat(void *hid_ctx, void *caller_data, pcb_coord_t sign)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pse->ps);
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];
	int n, dst_idx = pcb_pstk_get_shape_idx(ts, pcb_proto_layers[pse->editing_shape].mask, pcb_proto_layers[pse->editing_shape].comb);
	pcb_coord_t bloat = pse->shape_chg[pse->amount].default_val.coord_value;

	if (bloat <= 0)
		return;

	if (dst_idx < 0) {
		pcb_message(PCB_MSG_ERROR, "Can't copy shape: source shape (%s) is empty\n", pcb_proto_layers[pse->editing_shape].name);
		return;
	}

	bloat *= sign;
	for(n = 0; n < proto->tr.used; n++)
		pcb_pstk_shape_grow(&proto->tr.array[n].shape[dst_idx], pcb_false, bloat);

	pcb_pstk_proto_update(proto);

	pse_ps2dlg(pse->parent_hid_ctx, pse);
	pse_change_callback(pse);
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
	const char *copy_from_names[pcb_proto_num_layers+1];
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	PCB_DAD_DECL(dlg);

	pse->parent_hid_ctx = hid_ctx;

	for(n = 0; n < pcb_proto_num_layers; n++)
		copy_from_names[n] = pcb_proto_layers[n].name;
	copy_from_names[n] = NULL;

	pse->editing_shape = -1;
	for(n = 0; n < pcb_proto_num_layers; n++) {
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
		PCB_DAD_BUTTON(dlg, "Replace shape with hshadow");
			pse->hshadow = PCB_DAD_CURRENT(dlg);
			PCB_DAD_CHANGE_CB(dlg, pse_shape_hshadow);
			PCB_DAD_HELP(dlg, "Set shape to hshadow on this layer type\n(invisible shape with the same\ngeometry as the hole or slot, for proper clearance)");
		PCB_DAD_BUTTON(dlg, "Derive automatically");
			pse->derive = PCB_DAD_CURRENT(dlg);
			PCB_DAD_CHANGE_CB(dlg, pse_shape_auto);
			PCB_DAD_HELP(dlg, "Derive the shape for this layer type\nfrom other, existing shapes of this padstack\n(automatic)");
		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BEGIN_VBOX(dlg);
				PCB_DAD_BUTTON(dlg, "Copy shape from");
					pse->copy_do = PCB_DAD_CURRENT(dlg);
					PCB_DAD_CHANGE_CB(dlg, pse_shape_copy);
					PCB_DAD_HELP(dlg, "Copy the shape for this layer type\nfrom other, existing shapes of this padstack\nfrom the layer type selected");
				PCB_DAD_BUTTON(dlg, "Swap shape with");
					pse->copy_do = PCB_DAD_CURRENT(dlg);
					PCB_DAD_CHANGE_CB(dlg, pse_shape_swap);
					PCB_DAD_HELP(dlg, "Swap the shape for this layer type\nwith another, existing shapes of this padstack\nfrom the layer type selected");
					PCB_DAD_HELP(dlg, "Select the other layer type for swapping shape");
			PCB_DAD_END(dlg);
			PCB_DAD_ENUM(dlg, copy_from_names); /* coposite */
				pse->copy_from = PCB_DAD_CURRENT(dlg);
				PCB_DAD_HELP(dlg, "Select the source layer type for\nmanual shape copy or shape swap");
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
		PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW("padstack_shape", dlg, "Edit padstack shape", pse, pcb_true, NULL);
	pse->shape_chg = dlg;

/*	pse_ps2dlg(dlg_hid_ctx, pse);*/
	PCB_DAD_RUN(dlg);

	pse->shape_chg = NULL;
	PCB_DAD_FREE(dlg);
	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

/* Auto gen shape on a single layer */
static void pse_gen_shape(pcb_pstk_tshape_t *ts, pcb_layer_type_t lyt, int shape, pcb_coord_t size)
{
	int idx = ts->len;

	if (size <= 0) {
		pcb_message(PCB_MSG_ERROR, "Invalid size - has to be larger than 0\n");
		return;
	}

	ts->len++;
	ts->shape = realloc(ts->shape, ts->len * sizeof(pcb_pstk_shape_t));

	ts->shape[idx].layer_mask = lyt;
	ts->shape[idx].comb = 0;
	ts->shape[idx].clearance = 0;

	switch(shape) {
		case 0:
			ts->shape[idx].shape = PCB_PSSH_CIRC;
			ts->shape[idx].data.circ.x = ts->shape[idx].data.circ.y = 0;
			ts->shape[idx].data.circ.dia = size;
			break;
		case 1:
			ts->shape[idx].shape = PCB_PSSH_POLY;
			pcb_pstk_shape_alloc_poly(&ts->shape[idx].data.poly, 4);
			ts->shape[idx].data.poly.x[0] = -size/2;
			ts->shape[idx].data.poly.y[0] = -size/2;
			ts->shape[idx].data.poly.x[1] = ts->shape[idx].data.poly.x[0];
			ts->shape[idx].data.poly.y[1] = ts->shape[idx].data.poly.y[0] + size;
			ts->shape[idx].data.poly.x[2] = ts->shape[idx].data.poly.x[0] + size;
			ts->shape[idx].data.poly.y[2] = ts->shape[idx].data.poly.y[0] + size;
			ts->shape[idx].data.poly.x[3] = ts->shape[idx].data.poly.x[0] + size;
			ts->shape[idx].data.poly.y[3] = ts->shape[idx].data.poly.y[0];
			break;
	}
}

/* Auto derive shape from the related copper layer */
static void pse_drv_shape(pcb_pstk_proto_t *proto, pcb_pstk_tshape_t *ts, pcb_layer_type_t lyt, int paste)
{
	int srci = (lyt & PCB_LYT_TOP) ? 0 : 1;
	pcb_pstk_shape_derive(proto, -1, srci, PCB_PROTO_MASK_BLOAT, lyt | PCB_LYT_MASK, PCB_LYC_SUB|PCB_LYC_AUTO);
	if (paste)
		pcb_pstk_shape_derive(proto, -1, srci, 0, lyt | PCB_LYT_PASTE, PCB_LYC_AUTO);
}

/* Auto gen shapes for all layers the user selected on the dialog plus add the hole */
static void pse_gen(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pcb_pstk_proto_t proto;
	int sides = pse->attrs[pse->gen_sides].default_val.int_value;
	int shape = pse->attrs[pse->gen_shp].default_val.int_value;
	int expose = pse->attrs[pse->gen_expose].default_val.int_value;
	int paste = pse->attrs[pse->gen_paste].default_val.int_value;
	pcb_coord_t size = pse->attrs[pse->gen_size].default_val.coord_value;
	pcb_layer_type_t lyt = sides_lyt[sides];
	pcb_pstk_tshape_t *ts;
	pcb_cardinal_t pid;

	memset(&proto, 0, sizeof(proto));

	ts = pcb_vtpadstack_tshape_alloc_append(&proto.tr, 1);
	ts->rot = 0.0;
	ts->xmirror = 0;
	ts->smirror = 0;
	ts->len = 0;

	if (lyt & PCB_LYT_TOP)     pse_gen_shape(ts, PCB_LYT_COPPER | PCB_LYT_TOP, shape, size);
	if (lyt & PCB_LYT_BOTTOM)  pse_gen_shape(ts, PCB_LYT_COPPER | PCB_LYT_BOTTOM, shape, size);
	if (lyt & PCB_LYT_INTERN)  pse_gen_shape(ts, PCB_LYT_COPPER | PCB_LYT_INTERN, shape, size);
	if (expose) {
		if (lyt & PCB_LYT_TOP)    pse_drv_shape(&proto, ts, PCB_LYT_TOP, paste);
		if (lyt & PCB_LYT_BOTTOM) pse_drv_shape(&proto, ts, PCB_LYT_BOTTOM, paste);
	}

	proto.hdia = pse->attrs[pse->gen_drill].default_val.coord_value;
	proto.hplated = 1;

	pcb_pstk_proto_update(&proto);
	if (pse->gen_shape_in_place) {
		if (pcb_pstk_proto_replace(pse->data, pse->ps->proto, &proto) == PCB_PADSTACK_INVALID)
			pcb_message(PCB_MSG_ERROR, "Internal error: pse_gen() failed to raplace padstack prototype\n");
	}
	else {
		pid = pcb_pstk_proto_insert_dup(pse->data, &proto, 1);
		if (pid == PCB_PADSTACK_INVALID)
			pcb_message(PCB_MSG_ERROR, "Internal error: pse_gen() failed to insert padstack prototype\n");
		else
			pcb_pstk_change_instance(pse->ps, &pid, NULL, NULL, NULL, NULL);
	}

	pse_ps2dlg(hid_ctx, pse);
	PCB_DAD_SET_VALUE(hid_ctx, pse->tab, int_value, 1); /* switch to the prototype view where the new attributes are visible */
	pse_change_callback(pse);
	pcb_gui->invalidate_all();
}

void pcb_pstkedit_dialog(pse_t *pse, int target_tab)
{
	int n;
	const char *tabs[] = { "this instance", "prototype", "generate common geometry", NULL };
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	PCB_DAD_DECL(dlg);

	pse->disable_instance_tab = !!pse->disable_instance_tab;
	target_tab -= pse->disable_instance_tab;

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_COMPFLAG(dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABBED(dlg, tabs + pse->disable_instance_tab);
			pse->tab = PCB_DAD_CURRENT(dlg);

			if (!pse->disable_instance_tab) {
				/* Tab 0: this instance */
				PCB_DAD_BEGIN_VBOX(dlg);
					PCB_DAD_BEGIN_VBOX(dlg);
						PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
						PCB_DAD_LABEL(dlg, "Settings that affect only this padstack instance");
						PCB_DAD_BEGIN_HBOX(dlg);
							PCB_DAD_LABEL(dlg, "prototype");
							PCB_DAD_BUTTON(dlg, "#5");
								pse->proto_id = PCB_DAD_CURRENT(dlg);
								PCB_DAD_CHANGE_CB(dlg, pse_chg_protoid);
								PCB_DAD_HELP(dlg, "Padstack prototype ID\n(click to use a different prototype)");
							PCB_DAD_BUTTON(dlg, "Duplicate");
								PCB_DAD_HELP(dlg, "Copy padstack prototype to create a new prototype\nfor this padstack instance. Changes to the new\nprototype will not affect other users of the\noriginal prototye.");
								PCB_DAD_CHANGE_CB(dlg, pse_chg_protodup);
						PCB_DAD_END(dlg);
						PCB_DAD_BEGIN_TABLE(dlg, 2);
							PCB_DAD_LABEL(dlg, "Clearance");
							PCB_DAD_COORD(dlg, "");
								pse->clearance = PCB_DAD_CURRENT(dlg);
								PCB_DAD_MINVAL(dlg, 0);
								PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
								PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
								PCB_DAD_HELP(dlg, "global clearance (affects all layers)");
							PCB_DAD_LABEL(dlg, "Rotation");
							PCB_DAD_REAL(dlg, "");
								pse->rot = PCB_DAD_CURRENT(dlg);
								PCB_DAD_MINVAL(dlg, 0);
								PCB_DAD_MAXVAL(dlg, 360);
								PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
							PCB_DAD_LABEL(dlg, "X-mirror");
							PCB_DAD_BOOL(dlg, "");
								pse->xmirror = PCB_DAD_CURRENT(dlg);
								PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
							PCB_DAD_LABEL(dlg, "S-mirror");
							PCB_DAD_BOOL(dlg, "");
								pse->smirror = PCB_DAD_CURRENT(dlg);
								PCB_DAD_CHANGE_CB(dlg, pse_chg_instance);
						PCB_DAD_END(dlg);
					PCB_DAD_END(dlg);
				PCB_DAD_END(dlg);
			}

			/* Tab 1: prototype */
			PCB_DAD_BEGIN_VBOX(dlg);
				PCB_DAD_BEGIN_VBOX(dlg);
					PCB_DAD_LABEL(dlg, "Settings that affect all padstacks with the same prototype");
					PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
					PCB_DAD_LABEL(dlg, "Pad geometry per layer type:");
					PCB_DAD_BEGIN_TABLE(dlg, 5);
						PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
						for(n = 0; n < pcb_proto_num_layers; n++) {
							const char *layname = pcb_proto_layers[n].name;
							char *layname_tmp = NULL;
							char *help = NULL;

							if (pcb_proto_board_layer_for(PCB->Data, pcb_proto_layers[n].mask, pcb_proto_layers[n].comb) == -1) {
								layname = layname_tmp = pcb_strdup_printf("(%s)", pcb_proto_layers[n].name);
								help = "No board layer available for this layer type.\nThis shape will not show on the board\nuntil the corresponding layer in the matching layer group type is created.";
							}
							PCB_DAD_LABEL(dlg, layname);
								if (help != NULL)
									PCB_DAD_HELP(dlg, help);
							PCB_DAD_LABEL(dlg, "-");
								pse->proto_shape[n] = PCB_DAD_CURRENT(dlg);
							PCB_DAD_LABEL(dlg, "-");
								pse->proto_info[n] = PCB_DAD_CURRENT(dlg);
							PCB_DAD_BUTTON(dlg, "change...");
								pse->proto_change[n] = PCB_DAD_CURRENT(dlg);
								PCB_DAD_CHANGE_CB(dlg, pse_chg_shape);
								PCB_DAD_HELP(dlg, "Change the shape on this layer type");
							PCB_DAD_COORD(dlg, "");
								pse->proto_clr[n] = PCB_DAD_CURRENT(dlg);
								PCB_DAD_MINVAL(dlg, 1);
								PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
								PCB_DAD_CHANGE_CB(dlg, pse_chg_proto_clr);
								PCB_DAD_HELP(dlg, "local, per layer type clearance\n(only when global padstack clearance is 0)");
							free(layname_tmp);
						}
					PCB_DAD_END(dlg);

					PCB_DAD_BEGIN_HBOX(dlg);
						PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
						PCB_DAD_LABEL(dlg, "Prototype name:");
						PCB_DAD_STRING(dlg);
							PCB_DAD_CHANGE_CB(dlg, pse_chg_prname);
							pse->prname = PCB_DAD_CURRENT(dlg);
					PCB_DAD_END(dlg);

					PCB_DAD_LABEL(dlg, "Hole properties:");
						pse->hole_header = PCB_DAD_CURRENT(dlg);

					PCB_DAD_BEGIN_TABLE(dlg, 4);

						PCB_DAD_LABEL(dlg, "Diameter:");
						PCB_DAD_COORD(dlg, "");
							pse->hdia = PCB_DAD_CURRENT(dlg);
							PCB_DAD_MINVAL(dlg, 0);
							PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
							PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
						PCB_DAD_LABEL(dlg, ""); /* dummy */
						PCB_DAD_LABEL(dlg, ""); /* dummy */

						PCB_DAD_LABEL(dlg, "Hole/slot plating:");
						PCB_DAD_BOOL(dlg, "");
							pse->hplated = PCB_DAD_CURRENT(dlg);
							PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
							PCB_DAD_HELP(dlg, "A plated hole galvanically connects layers");
						PCB_DAD_LABEL(dlg, ""); /* dummy */
						PCB_DAD_LABEL(dlg, ""); /* dummy */

						PCB_DAD_LABEL(dlg, "Hole/slot top:");
						PCB_DAD_INTEGER(dlg, "");
							pse->htop_val = PCB_DAD_CURRENT(dlg);
							PCB_DAD_MINVAL(dlg, -(pse->pcb->LayerGroups.cache.copper_len-1));
							PCB_DAD_MAXVAL(dlg, pse->pcb->LayerGroups.cache.copper_len-1);
							PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
							PCB_DAD_HELP(dlg, "Blind/buried via/slot: top end of the hole");
						PCB_DAD_LABEL(dlg, "<text>");
							pse->htop_text = PCB_DAD_CURRENT(dlg);
						PCB_DAD_LABEL(dlg, "<layer>");
							pse->htop_layer = PCB_DAD_CURRENT(dlg);

						PCB_DAD_LABEL(dlg, "Hole/slot bottom:");
						PCB_DAD_INTEGER(dlg, "");
							pse->hbot_val = PCB_DAD_CURRENT(dlg);
							PCB_DAD_MINVAL(dlg, -(pse->pcb->LayerGroups.cache.copper_len-1));
							PCB_DAD_MAXVAL(dlg, pse->pcb->LayerGroups.cache.copper_len-1);
							PCB_DAD_CHANGE_CB(dlg, pse_chg_hole);
							PCB_DAD_HELP(dlg, "Blind/buried via/slot: bottom end of the hole");
						PCB_DAD_LABEL(dlg, "<text>");
							pse->hbot_text = PCB_DAD_CURRENT(dlg);
						PCB_DAD_LABEL(dlg, "<layer>");
							pse->hbot_layer = PCB_DAD_CURRENT(dlg);
					PCB_DAD_END(dlg);
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);

			/* Tab 2: generate common geometry */
			PCB_DAD_BEGIN_VBOX(dlg);
				PCB_DAD_BEGIN_VBOX(dlg);
					PCB_DAD_LABEL(dlg, "Generate a new prototype using a few numeric input");

					PCB_DAD_BEGIN_TABLE(dlg, 2);
						PCB_DAD_LABEL(dlg, "Copper shape:");
						PCB_DAD_ENUM(dlg, shapes);
							pse->gen_shp = PCB_DAD_CURRENT(dlg);

						PCB_DAD_LABEL(dlg, "Size (circle diameter or square side):");
						PCB_DAD_COORD(dlg, "");
							pse->gen_size = PCB_DAD_CURRENT(dlg);
							PCB_DAD_MINVAL(dlg, 1);
							PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));

						PCB_DAD_LABEL(dlg, "Drill diameter (0 means no hole):");
						PCB_DAD_COORD(dlg, "");
							pse->gen_drill = PCB_DAD_CURRENT(dlg);
							PCB_DAD_MINVAL(dlg, 1);
							PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));

						PCB_DAD_LABEL(dlg, "Copper shapes on:");
						PCB_DAD_ENUM(dlg, sides);
							pse->gen_sides = PCB_DAD_CURRENT(dlg);

						PCB_DAD_LABEL(dlg, "Expose top/bottom copper:");
						PCB_DAD_BOOL(dlg, "");
							pse->gen_expose = PCB_DAD_CURRENT(dlg);

						PCB_DAD_LABEL(dlg, "Paste exposed copper:");
						PCB_DAD_BOOL(dlg, "");
							pse->gen_paste = PCB_DAD_CURRENT(dlg);

						PCB_DAD_LABEL(dlg, "");
						PCB_DAD_BUTTON(dlg, "Generate!");
							pse->gen_do = PCB_DAD_CURRENT(dlg);
							PCB_DAD_CHANGE_CB(dlg, pse_gen);
					PCB_DAD_END(dlg);
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);
		PCB_DAD_END(dlg);
		PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW("padstack", dlg, "Edit padstack", pse, pcb_true, NULL);
	pse->attrs = dlg;
	pse_ps2dlg(dlg_hid_ctx, pse);
	if (target_tab > 0)
		PCB_DAD_SET_VALUE(dlg_hid_ctx, pse->tab, int_value, target_tab);
	PCB_DAD_RUN(dlg);

	free((char *)pse->attrs[pse->prname].default_val.str_value);

	PCB_DAD_FREE(dlg);
}

static const char pcb_acts_PadstackEdit[] = "PadstackEdit(object, [tab])\n";
static const char pcb_acth_PadstackEdit[] = "interactive pad stack editor";
static fgw_error_t pcb_act_PadstackEdit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = F_Object, target_tab = -1;
	pse_t pse;

	memset(&pse, 0, sizeof(pse));

	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, PadstackEdit, op = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_INT, PadstackEdit, target_tab = argv[2].val.nat_int);
	PCB_ACT_IRES(0);

	if (op == F_Object) {
		pcb_coord_t x, y;
		void *ptr1, *ptr2 = NULL, *ptr3;
		long type;
		pcb_hid_get_coords("Click on a padstack to edit", &x, &y, 0);
		type = pcb_search_screen(x, y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART | PCB_LOOSE_SUBC, &ptr1, &ptr2, &ptr3);
		if (type != PCB_OBJ_PSTK) {
			pcb_message(PCB_MSG_ERROR, "Need a padstack.\n");
			PCB_ACT_IRES(1);
			return 0;
		}
		pse.ps = ptr2;
	}
	else
		PCB_ACT_FAIL(PadstackEdit);

	pse.pcb = pcb_data_get_top(pse.ps->parent.data);
	if (pse.pcb == NULL)
		pse.pcb = PCB;

	pse.data = pse.ps->parent.data;
	assert(pse.ps->parent_type == PCB_PARENT_DATA);

	pcb_pstkedit_dialog(&pse, target_tab);

	PCB_ACT_IRES(0);
	return 0;
}


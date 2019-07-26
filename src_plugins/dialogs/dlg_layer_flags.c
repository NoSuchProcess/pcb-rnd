/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "board.h"
#include "actions.h"
#include "event.h"
#include "hid_dad.h"
#include "dlg_layer_binding.h"
#include "dlg_layer_flags.h"

const char pcb_acts_LayerPropGui[] = "LayerPropGui(layerid)";
const char pcb_acth_LayerPropGui[] = "Change layer flags and properties";
fgw_error_t pcb_act_LayerPropGui(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_DAD_DECL(dlg)
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"OK", 0}, {NULL, 0}};
	int wname, wsub, wauto, failed, ar = 0;
	pcb_layer_t *ly;
	pcb_layer_id_t lid;


	PCB_ACT_MAY_CONVARG(1, FGW_LONG, LayerPropGui, lid = argv[1].val.nat_long);
	ly = pcb_get_layer(PCB->Data, lid);

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_BEGIN_TABLE(dlg, 2);
			PCB_DAD_LABEL(dlg, "name");
			PCB_DAD_STRING(dlg);
				PCB_DAD_HELP(dlg, "logical layer name");
				wname = PCB_DAD_CURRENT(dlg);
			PCB_DAD_LABEL(dlg, "sub");
			PCB_DAD_BOOL(dlg, "");
				PCB_DAD_HELP(dlg, "Layer is drawn negatively in composition\n(will not work on copper)");
				wsub = PCB_DAD_CURRENT(dlg);
			PCB_DAD_LABEL(dlg, "auto");
			PCB_DAD_BOOL(dlg, "");
				PCB_DAD_HELP(dlg, "Layer is target for autogenerated objects\nand side effects, e.g. padstack shapes");
				wauto = PCB_DAD_CURRENT(dlg);
		PCB_DAD_END(dlg);
		PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
	PCB_DAD_END(dlg);
	

	dlg[wname].val.str_value = pcb_strdup(ly->name);
	dlg[wsub].val.lng = ly->comb & PCB_LYC_SUB;
	dlg[wauto].val.lng = ly->comb & PCB_LYC_AUTO;

	PCB_DAD_AUTORUN("layer_prop", dlg, "Properties of a logical layer", NULL, failed);

	if (failed == 0) {
		pcb_layer_combining_t comb = 0;
		if (strcmp(ly->name, dlg[wname].val.str_value) != 0) {
			ar |= pcb_layer_rename_(ly, (char *)dlg[wname].val.str_value);
			pcb_board_set_changed_flag(pcb_true);
		}
		if (dlg[wsub].val.lng) comb |= PCB_LYC_SUB;
		if (dlg[wauto].val.lng) comb |= PCB_LYC_AUTO;
		if (ly->comb != comb) {
			ly->comb = comb;
			pcb_board_set_changed_flag(pcb_true);
		}
	}
	else
		ar = 1;
	PCB_DAD_FREE(dlg);

	PCB_ACT_IRES(ar);
	return 0;
}

const char pcb_acts_GroupPropGui[] = "GroupPropGui(groupid)";
const char pcb_acth_GroupPropGui[] = "Change group flags and properties";
fgw_error_t pcb_act_GroupPropGui(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_DAD_DECL(dlg)
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"OK", 0}, {NULL, 0}};
	int wname, wtype, wpurp, wloc;
	int failed, n, ar = 0, orig_type, changed = 0, omit_loc = 0, orig_loc = -1, def_loc;
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;
	static const char *ltypes[] = { "top", "bottom", "any intern", "global", NULL };
	pcb_layer_type_t ltype_bits[] = { PCB_LYT_TOP, PCB_LYT_BOTTOM, PCB_LYT_INTERN, 0 };
#define LOC_TYPES (PCB_LYT_DOC)

	PCB_ACT_MAY_CONVARG(1, FGW_LONG, GroupPropGui, gid = argv[1].val.nat_long);
	g = pcb_get_layergrp(PCB, gid);

	if (g->ltype & LOC_TYPES) {
		for(n = 0; ltype_bits[n] != 0; n++)
			if (g->ltype & ltype_bits[n])
				def_loc = n;
	}
	else
		omit_loc = 1;

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_BEGIN_TABLE(dlg, 2);
			PCB_DAD_LABEL(dlg, "name");
			PCB_DAD_STRING(dlg);
				PCB_DAD_HELP(dlg, "group name");
				wname = PCB_DAD_CURRENT(dlg);
			PCB_DAD_LABEL(dlg, "type");
			PCB_DAD_ENUM(dlg, pcb_lb_types);
				PCB_DAD_HELP(dlg, "type/material of the group");
				wtype = PCB_DAD_CURRENT(dlg);
			if (!omit_loc) {
				PCB_DAD_LABEL(dlg, "location");
				PCB_DAD_ENUM(dlg, ltypes);
					PCB_DAD_HELP(dlg, "location of the group in the stack");
					wloc = PCB_DAD_CURRENT(dlg);
			}
			PCB_DAD_LABEL(dlg, "purpose");
			PCB_DAD_STRING(dlg);
				PCB_DAD_HELP(dlg, "purpose");
				wpurp = PCB_DAD_CURRENT(dlg);
				PCB_DAD_HELP(dlg, "subtype of the layer\nmeaning depends on the main type");
		PCB_DAD_END(dlg);
		PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
	PCB_DAD_END(dlg);


	dlg[wname].val.str_value = pcb_strdup(g->name);
	dlg[wtype].val.lng = orig_type = pcb_ly_type2enum(g->ltype);
	dlg[wpurp].val.str_value = pcb_strdup(g->purpose == NULL ? "" : g->purpose);
	if (!omit_loc)
		dlg[wloc].val.lng = def_loc;

	if (!omit_loc) {
		pcb_layer_type_t loc = g->ltype & PCB_LYT_ANYWHERE;
		dlg[wloc].val.lng = 3;
		if (loc != 0) {
			for(n = 0; ltypes[n] != NULL; n++) {
				if ((loc & ltype_bits[n]) == loc) {
					dlg[wloc].val.lng = n;
					break;
				}
			}
		}
		orig_loc = dlg[wloc].val.lng;
	}

	PCB_DAD_AUTORUN("layer_grp_prop", dlg, "Edit the properties of a layer group (physical layer)", NULL, failed);
	if (failed == 0) {
		if (strcmp(g->name, dlg[wname].val.str_value) != 0) {
			ar |= pcb_layergrp_rename_(g, (char *)dlg[wname].val.str_value);
			dlg[wname].val.str_value = NULL;
			pcb_board_set_changed_flag(pcb_true);
		}

		if (dlg[wtype].val.lng != orig_type) {
			pcb_layer_type_t lyt = 0;
			pcb_get_ly_type_(dlg[wtype].val.lng, &lyt);
			g->ltype &= ~PCB_LYT_ANYTHING;
			g->ltype |= lyt;
			changed = 1;
		}

		if ((!omit_loc) && (dlg[wloc].val.lng != orig_loc)) {
			if (PCB_LAYER_SIDED(g->ltype)) {
				g->ltype &= ~PCB_LYT_ANYWHERE;
				if (dlg[wloc].val.lng >= 0)
					g->ltype |= ltype_bits[dlg[wloc].val.lng];
				changed = 1;
			}
			else
				pcb_message(PCB_MSG_ERROR, "Ignoring location - for this layer group type it is determined by the stackup\n");
		}

		if (dlg[wpurp].val.str_value == NULL) {
			if (g->purpose != NULL) {
				pcb_layergrp_set_purpose__(g, NULL);
				changed = 1;
			}
		}
		else if ((g->purpose == NULL) || (strcmp(g->purpose, dlg[wpurp].val.str_value) != 0)) {
			if (*dlg[wpurp].val.str_value == '\0')
				pcb_layergrp_set_purpose__(g, NULL);
			else
				pcb_layergrp_set_purpose__(g, pcb_strdup(dlg[wpurp].val.str_value));
			changed = 1;
		}

		if (changed) {
			pcb_board_set_changed_flag(pcb_true);
			pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
		}
	}
	else
		ar = 1;

	PCB_DAD_FREE(dlg);

	PCB_ACT_IRES(ar);
	return 0;
}
#undef LOC_TYPES

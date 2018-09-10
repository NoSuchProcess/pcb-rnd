#include "event.h"

static const char pcb_acts_LayerPropGui[] = "LayerPropGui(layerid)";
static const char pcb_acth_LayerPropGui[] = "Change layer flags and properties";
static fgw_error_t pcb_act_LayerPropGui(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ar;
	pcb_layer_t *ly;
	pcb_layer_id_t lid;
	pcb_hid_attr_val_t rv[16];
	pcb_hid_attribute_t attr[] = {
		{"name", "logical layer name",          PCB_HATT_STRING, 0, 0, {0}, NULL, NULL, 0, NULL, NULL},
		{"sub: drawn using subtraction", NULL,  PCB_HATT_BOOL, 0, 0, {0}, NULL, NULL, 0, NULL, NULL},
		{"auto: auto-generated layer", NULL,    PCB_HATT_BOOL, 0, 0, {0}, NULL, NULL, 0, NULL, NULL}
	};

	PCB_ACT_MAY_CONVARG(1, FGW_LONG, LayerPropGui, lid = argv[1].val.nat_long);
	ly = pcb_get_layer(PCB->Data, lid);

	attr[0].default_val.str_value = pcb_strdup(ly->name);
	attr[1].default_val.int_value = ly->comb & PCB_LYC_SUB;
	attr[2].default_val.int_value = ly->comb & PCB_LYC_AUTO;

	ar = pcb_attribute_dialog(attr,sizeof(attr)/sizeof(attr[0]), rv, "Edit layer properties", "Edit the properties of a logical layer", NULL);

	if (ar == 0) {
		pcb_layer_combining_t comb = 0;
		if (strcmp(ly->name, attr[0].default_val.str_value) != 0) {
			ar |= pcb_layer_rename_(ly, (char *)attr[0].default_val.str_value);
			attr[0].default_val.str_value = NULL;
			pcb_board_set_changed_flag(pcb_true);
		}
		if (attr[1].default_val.int_value) comb |= PCB_LYC_SUB;
		if (attr[2].default_val.int_value) comb |= PCB_LYC_AUTO;
		if (ly->comb != comb) {
			ly->comb = comb;
			pcb_board_set_changed_flag(pcb_true);
		}
	}
	free((char *)attr[0].default_val.str_value);

	PCB_ACT_IRES(ar);
	return 0;
}

static const char pcb_acts_GroupPropGui[] = "GroupPropGui(groupid)";
static const char pcb_acth_GroupPropGui[] = "Change group flags and properties";
static fgw_error_t pcb_act_GroupPropGui(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ar, orig_type, changed = 0;
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;
	pcb_hid_attr_val_t rv[16];
	pcb_hid_attribute_t attr[] = {
		{"name", "group (physical layer) name",          PCB_HATT_STRING, 0, 0, {0}, NULL, NULL, 0, NULL, NULL},
		{"type", "type/material of the group",           PCB_HATT_ENUM,   0, 0, {0}, NULL, NULL, 0, NULL, NULL},
		{"purpose", "purpose or subtype",                  PCB_HATT_STRING, 0, 0, {0}, NULL, NULL, 0, NULL, NULL},
	};

	PCB_ACT_MAY_CONVARG(1, FGW_LONG, GroupPropGui, gid = argv[1].val.nat_long);
	g = pcb_get_layergrp(PCB, gid);

	attr[0].default_val.str_value = pcb_strdup(g->name);
	attr[1].enumerations = lb_types;
	attr[1].default_val.int_value = orig_type = ly_type2enum(g->ltype);
	attr[2].default_val.str_value = pcb_strdup(g->purpose);

	ar = pcb_attribute_dialog(attr,sizeof(attr)/sizeof(attr[0]), rv, "Edit group properties", "Edit the properties of a layer group (physical layer)", NULL);

	if (ar == 0) {
		if (strcmp(g->name, attr[0].default_val.str_value) != 0) {
			ar |= pcb_layergrp_rename_(g, (char *)attr[0].default_val.str_value);
			attr[0].default_val.str_value = NULL;
			pcb_board_set_changed_flag(pcb_true);
		}

		if (attr[1].default_val.int_value != orig_type) {
			pcb_layer_type_t lyt = 0;
			get_ly_type_(attr[1].default_val.int_value, &lyt);
			g->ltype &= ~PCB_LYT_ANYTHING;
			g->ltype |= lyt;
			if (PCB_LAYER_SIDED(lyt)) {
#warning TODO
			}
			changed = 1;
		}

		if (strcmp(g->purpose, attr[2].default_val.str_value) != 0) {
			pcb_layergrp_set_purpose__(g, pcb_strdup(attr[2].default_val.str_value));
			changed = 1;
		}

		if (changed) {
			pcb_board_set_changed_flag(pcb_true);
			pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
		}
	}

	free((char *)attr[0].default_val.str_value);

	PCB_ACT_IRES(ar);
	return 0;
}

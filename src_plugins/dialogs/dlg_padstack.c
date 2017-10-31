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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "obj_padstack.h"
#include "obj_padstack_inlines.h"

typedef struct pse_proto_layer_s {
	const char *name;
	pcb_layer_type_t mask;
	pcb_layer_combining_t comb;
} pse_proto_layer_t;

static const pse_proto_layer_t pse_layer[] = {
	{"top paste",            PCB_LYT_TOP | PCB_LYT_PASTE, 0},
	{"top mask",             PCB_LYT_TOP | PCB_LYT_MASK, PCB_LYC_SUB},
	{"top copper",           PCB_LYT_TOP | PCB_LYT_COPPER, 0},
	{"any internal copper",  PCB_LYT_INTERN | PCB_LYT_COPPER, 0},
	{"bottom copper",        PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0},
	{"bottom mask",          PCB_LYT_BOTTOM | PCB_LYT_MASK, PCB_LYC_SUB},
	{"bottom paste",         PCB_LYT_BOTTOM | PCB_LYT_PASTE, 0}
};
#define pse_num_layers (sizeof(pse_layer) / sizeof(pse_layer[0]))

typedef struct pse_s {
	pcb_padstack_t *ps;
	int tab;

	/* widget IDs */
	int tab_instance, tab_prototype;
	int but_instance, but_prototype;
	int proto_id, clearance;
	int proto_shape[pse_num_layers];
	int proto_info[pse_num_layers];
	int hdia;
	int htop_val, htop_text, htop_layer;
	int hbot_val, hbot_text, hbot_layer;
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

/* Convert from padstack to dialog */
static void pse_ps2dlg(void *hid_ctx, pse_t *pse)
{
	char tmp[128];
	int n;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(pse->ps);

	/* instance */
	sprintf(tmp, "#%ld", (long int)pse->ps->proto);
	PCB_DAD_SET_VALUE(hid_ctx, pse->proto_id, str_value, tmp);
	PCB_DAD_SET_VALUE(hid_ctx, pse->clearance, coord_value, pse->ps->Clearance);

	/* proto - layers */
	for(n = 0; n < pse_num_layers; n++) {
		pcb_padstack_shape_t *shape = pcb_padstack_shape(pse->ps, pse_layer[n].mask, pse_layer[n].comb);
		if (shape != NULL) {
			switch(shape->shape) {
				case PCB_PSSH_CIRC:
					PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "circle");
					pcb_snprintf(tmp, sizeof(tmp), "dia=$%mm at $%mm;$%mm", shape->data.circ.dia, shape->data.circ.x, shape->data.circ.y);
					break;
				case PCB_PSSH_LINE:
					if (shape->data.line.square)
						PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "square line");
					else
						PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "round line");
					pcb_snprintf(tmp, sizeof(tmp), "thickness=%mm", shape->data.line.thickness);
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
		}
		else {
			PCB_DAD_SET_VALUE(hid_ctx, pse->proto_shape[n], str_value, "");
			PCB_DAD_SET_VALUE(hid_ctx, pse->proto_info[n], str_value, "");
		}
	}

	/* proto - holes */
	PCB_DAD_SET_VALUE(hid_ctx, pse->hdia, coord_value, proto->hdia);
	PCB_DAD_SET_VALUE(hid_ctx, pse->htop_val, int_value, proto->htop);
	PCB_DAD_SET_VALUE(hid_ctx, pse->hbot_val, int_value, proto->hbottom);

	if (proto->htop == 0)
		strcpy(tmp, "top copper group");
	else
		sprintf(tmp, "%d groups from\nthe %s copper group", proto->htop, proto->htop > 0 ? "top" : "bottom");
	PCB_DAD_SET_VALUE(hid_ctx, pse->htop_text, str_value, tmp);

	if (proto->hbottom == 0)
		strcpy(tmp, "bottom copper group");
	else
		sprintf(tmp, "%d groups from\nthe %s copper group", proto->hbottom, proto->hbottom > 0 ? "bottom" : "top");
	PCB_DAD_SET_VALUE(hid_ctx, pse->hbot_text, str_value, tmp);

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


static const char pcb_acts_PadstackEdit[] = "PadstackEdit(object)\n";
static const char pcb_acth_PadstackEdit[] = "interactive pad stack editor";
static int pcb_act_PadstackEdit(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int n;
	pse_t pse;
	pcb_board_t *pcb;
	PCB_DAD_DECL(dlg);

	memset(&pse, 0, sizeof(pse));

	if ((argc == 0) || (pcb_strcasecmp(argv[0], "object") == 0)) {
		void *ptr1, *ptr2 = NULL, *ptr3;
		long type;
		pcb_gui->get_coords("Click on a padstack to edit", &x, &y);
		type = pcb_search_screen(x, y, PCB_TYPE_PADSTACK, &ptr1, &ptr2, &ptr3);
		if (type != PCB_TYPE_PADSTACK) {
			pcb_message(PCB_MSG_ERROR, "Need a padstack.\n");
			return 1;
		}
		pse.ps = ptr2;
	}
	else
		PCB_ACT_FAIL(PadstackEdit);

	pcb = pcb_data_get_top(pse.ps->parent.data);
	if (pcb == NULL)
		pcb = PCB;

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BUTTON(dlg, "this instance");
				pse.but_instance = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_tab_ps);
			PCB_DAD_BUTTON(dlg, "prototype");
				pse.but_prototype = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_tab_proto);
		PCB_DAD_END(dlg);

		/* this instance */
		PCB_DAD_BEGIN_VBOX(dlg);
			pse.tab_instance = PCB_DAD_CURRENT(dlg);
			PCB_DAD_BEGIN_VBOX(dlg);
				PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_LABEL(dlg, "prototype");
					PCB_DAD_BUTTON(dlg, "#5");
						pse.proto_id = PCB_DAD_CURRENT(dlg);
				PCB_DAD_END(dlg);
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_LABEL(dlg, "Clearance");
					PCB_DAD_COORD(dlg, "");
						pse.clearance = PCB_DAD_CURRENT(dlg);
						PCB_DAD_MINVAL(dlg, 1);
						PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);
		PCB_DAD_END(dlg);

		/* prototype */
		PCB_DAD_BEGIN_VBOX(dlg);
			pse.tab_prototype = PCB_DAD_CURRENT(dlg);
			PCB_DAD_BEGIN_VBOX(dlg);
				PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
				PCB_DAD_LABEL(dlg, "Pad geometry:");
				PCB_DAD_BEGIN_TABLE(dlg, 3);
					PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
					for(n = 0; n < pse_num_layers; n++) {
						PCB_DAD_LABEL(dlg, pse_layer[n].name);
						PCB_DAD_LABEL(dlg, "-");
							pse.proto_shape[n] = PCB_DAD_CURRENT(dlg);
						PCB_DAD_LABEL(dlg, "-");
							pse.proto_info[n] = PCB_DAD_CURRENT(dlg);
					}
				PCB_DAD_END(dlg);
			
				PCB_DAD_LABEL(dlg, "Hole geometry:");

				PCB_DAD_BEGIN_TABLE(dlg, 4);

				PCB_DAD_LABEL(dlg, "diameter:");
				PCB_DAD_COORD(dlg, "");
					pse.hdia = PCB_DAD_CURRENT(dlg);
					PCB_DAD_MINVAL(dlg, 1);
					PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
				PCB_DAD_LABEL(dlg, ""); /* dummy */
				PCB_DAD_LABEL(dlg, ""); /* dummy */

				PCB_DAD_LABEL(dlg, "Hole top:");
				PCB_DAD_INTEGER(dlg, "");
					pse.htop_val = PCB_DAD_CURRENT(dlg);
					PCB_DAD_MINVAL(dlg, -(pcb->LayerGroups.cache.copper_len-1));
					PCB_DAD_MAXVAL(dlg, pcb->LayerGroups.cache.copper_len-1);
				PCB_DAD_LABEL(dlg, "<text>");
					pse.htop_text = PCB_DAD_CURRENT(dlg);
				PCB_DAD_LABEL(dlg, "<layer>");
					pse.htop_layer = PCB_DAD_CURRENT(dlg);

				PCB_DAD_LABEL(dlg, "Hole bottom:");
				PCB_DAD_INTEGER(dlg, "");
					pse.hbot_val = PCB_DAD_CURRENT(dlg);
					PCB_DAD_MINVAL(dlg, -(pcb->LayerGroups.cache.copper_len-1));
					PCB_DAD_MAXVAL(dlg, pcb->LayerGroups.cache.copper_len-1);
				PCB_DAD_LABEL(dlg, "<text>");
					pse.hbot_text = PCB_DAD_CURRENT(dlg);
				PCB_DAD_LABEL(dlg, "<layer>");
					pse.hbot_layer = PCB_DAD_CURRENT(dlg);

			PCB_DAD_END(dlg);
		PCB_DAD_END(dlg);
	PCB_DAD_END(dlg);


	PCB_DAD_NEW(dlg, "dlg_padstack_edit", "Edit padstack", &pse);
	pse_tab_update(dlg_hid_ctx, &pse);
	pse_ps2dlg(dlg_hid_ctx, &pse);
	PCB_DAD_RUN(dlg);

	PCB_DAD_FREE(dlg);
	return 0;
}


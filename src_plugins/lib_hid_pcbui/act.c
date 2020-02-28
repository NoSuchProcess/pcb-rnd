/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017..2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/actions.h>
#include "board.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include "draw.h"
#include <librnd/core/hid.h>
#include "layer.h"
#include "layer_vis.h"
#include "search.h"
#include "obj_subc_parent.h"

#include "util.h"
#include "act.h"

#define NOGUI() \
do { \
	if ((pcb_gui == NULL) || (!pcb_gui->gui)) { \
		PCB_ACT_IRES(1); \
		return 0; \
	} \
	PCB_ACT_IRES(0); \
} while(0)

const char pcb_acts_Zoom[] =
	"Zoom()\n"
	"Zoom([+|-|=]factor)\n"
	"Zoom(x1, y1, x2, y2)\n"
	"Zoom(selected)\n"
	"Zoom(?)\n"
	"Zoom(get)\n"
	"Zoom(found)\n";
const char pcb_acth_Zoom[] = "GUI zoom";
/* DOC: zoom.html */
fgw_error_t pcb_act_Zoom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *vp, *ovp;
	double v;
	pcb_coord_t x = 0, y = 0;

	NOGUI();

	if (argc < 2) {
		pcb_gui->zoom_win(pcb_gui, 0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y, 1);
		return 0;
	}

	if (argc == 5) {
		pcb_coord_t x1, y1, x2, y2;

		PCB_ACT_CONVARG(1, FGW_COORD, Zoom, x1 = fgw_coord(&argv[1]));
		PCB_ACT_CONVARG(2, FGW_COORD, Zoom, y1 = fgw_coord(&argv[2]));
		PCB_ACT_CONVARG(3, FGW_COORD, Zoom, x2 = fgw_coord(&argv[3]));
		PCB_ACT_CONVARG(4, FGW_COORD, Zoom, y2 = fgw_coord(&argv[4]));

		pcb_gui->zoom_win(pcb_gui, x1, y1, x2, y2, 1);
		return 0;
	}

	if (argc > 2)
		PCB_ACT_FAIL(Zoom);

	PCB_ACT_CONVARG(1, FGW_STR, Zoom, ovp = vp = argv[1].val.str);

	if (pcb_strcasecmp(vp, "selected") == 0) {
		pcb_box_t sb;
		if (pcb_get_selection_bbox(&sb, PCB->Data) > 0)
			pcb_gui->zoom_win(pcb_gui, sb.X1, sb.Y1, sb.X2, sb.Y2, 1);
		else
			pcb_message(PCB_MSG_ERROR, "Can't zoom to selection: nothing selected\n");
		return 0;
	}

	if (pcb_strcasecmp(vp, "found") == 0) {
		pcb_box_t sb;
		if (pcb_get_found_bbox(&sb, PCB->Data) > 0)
			pcb_gui->zoom_win(pcb_gui, sb.X1, sb.Y1, sb.X2, sb.Y2, 1);
		else
			pcb_message(PCB_MSG_ERROR, "Can't zoom to 'found': nothing found\n");
		return 0;
	}

	if (*vp == '?') {
		pcb_message(PCB_MSG_INFO, "Current zoom level (coord-per-pix): %$mm\n", pcb_gui->coord_per_pix);
		return 0;
	}

	if (pcb_strcasecmp(argv[1].val.str, "get") == 0) {
		res->type = FGW_DOUBLE;
		res->val.nat_double = pcb_gui->coord_per_pix;
		return 0;
	}

	if (*vp == '+' || *vp == '-' || *vp == '=')
		vp++;
	v = strtod(vp, NULL);
	if (v <= 0)
		return FGW_ERR_ARG_CONV;

	pcb_hid_get_coords("Select zoom center", &x, &y, 0);
	switch (ovp[0]) {
	case '-':
		pcb_gui->zoom(pcb_gui, x, y, 1 / v, 1);
		break;
	default:
	case '+':
		pcb_gui->zoom(pcb_gui, x, y, v, 1);
		break;
	case '=':
		pcb_gui->zoom(pcb_gui, x, y, v, 0);
		break;
	}

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Pan[] = "Pan(Mode)";
const char pcb_acth_Pan[] = "Start or stop panning (Mode = 1 to start, 0 to stop)\n";
/* DOC: pan.html */
fgw_error_t pcb_act_Pan(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int mode;
	pcb_coord_t x, y;

	NOGUI();

	pcb_hid_get_coords("Click on a place to pan", &x, &y, 0);

	PCB_ACT_CONVARG(1, FGW_INT, Pan, mode = argv[1].val.nat_int);
	pcb_gui->pan_mode(pcb_gui, x, y, mode);

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Center[] = "Center()\n";
const char pcb_acth_Center[] = "Moves the pointer to the center of the window.";
/* DOC: center.html */
fgw_error_t pcb_act_Center(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t x, y;
	NOGUI();

	pcb_hid_get_coords("Click to center", &x, &y, 0);

	if (argc != 1)
		PCB_ACT_FAIL(Center);

	pcb_gui->pan(pcb_gui, x, y, 0);

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Scroll[] = "Scroll(up|down|left|right, [pixels])";
const char pcb_acth_Scroll[] = "Scroll the viewport.";
/* DOC: scroll.html */
fgw_error_t pcb_act_Scroll(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op;
	double dx = 0.0, dy = 0.0, pixels = 100.0;

	PCB_ACT_CONVARG(1, FGW_STR, Scroll, op = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_DOUBLE, Scroll, pixels = argv[2].val.nat_double);

	if (pcb_strcasecmp(op, "up") == 0)
		dy = -pcb_gui->coord_per_pix * pixels;
	else if (pcb_strcasecmp(op, "down") == 0)
		dy = pcb_gui->coord_per_pix * pixels;
	else if (pcb_strcasecmp(op, "right") == 0)
		dx = pcb_gui->coord_per_pix * pixels;
	else if (pcb_strcasecmp(op, "left") == 0)
		dx = -pcb_gui->coord_per_pix * pixels;
	else
		PCB_ACT_FAIL(Scroll);

	pcb_gui->pan(pcb_gui, dx, dy, 1);

	PCB_ACT_IRES(0);
	return 0;
}


const char pcb_acts_SwapSides[] = "SwapSides(|v|h|r, [S])";
const char pcb_acth_SwapSides[] = "Swaps the side of the board you're looking at.";
/* DOC: swapsides.html */
fgw_error_t pcb_act_SwapSides(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layergrp_id_t active_group = pcb_layer_get_group(PCB, pcb_layer_stack[0]);
	pcb_layergrp_id_t comp_group = -1, solder_group = -1;
	pcb_bool comp_on = pcb_false, solder_on = pcb_false;
	pcb_box_t vb;
	pcb_coord_t x, y;
	double xcent, ycent, xoffs, yoffs;

	NOGUI();

	pcb_hid_get_coords("Click to center of flip", &x, &y, 0);

	x = pcb_crosshair.X;
	y = pcb_crosshair.Y;

	pcb_gui->view_get(pcb_gui, &vb);
	xcent = (double)(vb.X1 + vb.X2)/2.0;
	ycent = (double)(vb.Y1 + vb.Y2)/2.0;
	xoffs = xcent - x;
	yoffs = ycent - y;
/*	pcb_trace("SwapSides: xy=%mm;%mm cent=%mm;%mm ofs=%mm;%mm\n", x, y, (pcb_coord_t)xcent, (pcb_coord_t)ycent, (pcb_coord_t)xoffs, (pcb_coord_t)yoffs);*/

	if (pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder_group, 1) > 0)
		solder_on = pcb_get_layer(PCB->Data, PCB->LayerGroups.grp[solder_group].lid[0])->meta.real.vis;

	if (pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp_group, 1) > 0)
		comp_on = pcb_get_layer(PCB->Data, PCB->LayerGroups.grp[comp_group].lid[0])->meta.real.vis;

	pcb_draw_inhibit_inc();
	if (argc > 1) {
		const char *a, *b = "";
		pcb_layer_id_t lid;
		pcb_layer_type_t lyt;

		PCB_ACT_CONVARG(1, FGW_STR, SwapSides, a = argv[1].val.str);
		PCB_ACT_MAY_CONVARG(2, FGW_STR, SwapSides, b = argv[2].val.str);
		switch (a[0]) {
			case 'h': case 'H':
				conf_toggle_heditor_("view/flip_x", view.flip_x);
				xoffs = 0;
				break;
			case 'v': case 'V':
				if (!pcbhl_conf.editor.view.flip_y)
					yoffs = -yoffs;
				else
					yoffs = 0;
				conf_toggle_heditor_("view/flip_y", view.flip_y);
				break;
			case 'r': case 'R':
				xoffs = 0;
				if (!pcbhl_conf.editor.view.flip_y)
					yoffs = -yoffs;
				else
					yoffs = 0;

				conf_toggle_heditor_("view/flip_x", view.flip_x);
				conf_toggle_heditor_("view/flip_y", view.flip_y);
				conf_toggle_editor(show_solder_side); /* Swapped back below */
				break;

			default:
				pcb_draw_inhibit_dec();
				PCB_ACT_IRES(1);
				return 0;
		}

		switch (b[0]) {
			case 'S':
			case 's':
				lyt = (pcb_layer_flags_(PCB_CURRLAYER(PCB)) & PCB_LYT_ANYTHING) | (!conf_core.editor.show_solder_side ?  PCB_LYT_BOTTOM : PCB_LYT_TOP);
				lid = pcb_layer_vis_last_lyt(lyt);
				if (lid >= 0)
					pcb_layervis_change_group_vis(&PCB->hidlib, lid, 1, 1);
		}
	}

	conf_toggle_editor(show_solder_side);

	if ((active_group == comp_group && comp_on && !solder_on) || (active_group == solder_group && solder_on && !comp_on)) {
		pcb_bool new_solder_vis = conf_core.editor.show_solder_side;

		if (comp_group >= 0)
			pcb_layervis_change_group_vis(&PCB->hidlib, PCB->LayerGroups.grp[comp_group].lid[0], !new_solder_vis, !new_solder_vis);
		if (solder_group >= 0)
			pcb_layervis_change_group_vis(&PCB->hidlib, PCB->LayerGroups.grp[solder_group].lid[0], new_solder_vis, new_solder_vis);
	}

	pcb_draw_inhibit_dec();

/*pcb_trace("-jump-> %mm;%mm -> %mm;%mm\n", x, y, (pcb_coord_t)(x + xoffs), (pcb_coord_t)(y + yoffs));*/
	pcb_gui->pan(pcb_gui, pcb_round(x + xoffs), pcb_round(y + yoffs), 0);
	pcb_gui->set_crosshair(pcb_gui, x, y, HID_SC_PAN_VIEWPORT);

	pcb_gui->invalidate_all(pcb_gui);

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Command[] = "Command()";
const char pcb_acth_Command[] = "Displays the command line input in the status area.";
/* DOC: command */
fgw_error_t pcb_act_Command(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	NOGUI();
	pcb_gui->open_command(pcb_gui);
	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Popup[] = "Popup(MenuName, [obj-type])";
const char pcb_acth_Popup[] = "Bring up the popup menu specified by MenuName, optionally modified with the object type under the cursor.\n";
/* DOC: popup.html */
fgw_error_t pcb_act_Popup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char name[256], name2[256];
	const char *tn = NULL, *a0, *a1 = NULL;
	int r = 1;
	enum {
		CTX_NONE,
		CTX_OBJ_TYPE
	} ctx_sens = CTX_NONE;

	NOGUI();

	if (argc != 2 && argc != 3)
		PCB_ACT_FAIL(Popup);

	PCB_ACT_CONVARG(1, FGW_STR, Popup, a0 = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, Popup, a1 = argv[2].val.str);

	*name = '\0';
	*name2 = '\0';

	if (argc == 3) {
		if (strcmp(a1, "obj-type") == 0) ctx_sens = CTX_OBJ_TYPE;
	}

	if (strlen(a0) < sizeof(name) - 32) {
		switch(ctx_sens) {
			case CTX_OBJ_TYPE:
				{
					pcb_coord_t x, y;
					pcb_objtype_t type;
					void *o1, *o2, *o3;
					pcb_any_obj_t *o;

					pcb_hid_get_coords("context sensitive popup: select object", &x, &y, 0);
					type = pcb_search_screen(x, y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &o1, &o2, &o3);
					o = o2;
					if ((type == 0) || ((o != NULL) && (pcb_gobj_parent_subc(o->parent_type, &o->parent) == NULL))) {
						type = pcb_search_screen(x, y, PCB_OBJ_CLASS_REAL, &o1, &o2, &o3);

						switch(type) {
							case 0: tn = "none"; break;
							case PCB_OBJ_SUBC:
								if (pcb_attribute_get(&((pcb_any_obj_t *)o2)->Attributes, "extobj") != 0) {
									tn = "extobj-subcircuit";
									break;
								}
								/* fall through */
							default:
								tn = pcb_obj_type_name(type);
								break;
						}

						sprintf(name, "/popups/%s-%s", a0, tn);
					}
					else
						sprintf(name, "/popups/%s-padstack-in-subc", a0);
					sprintf(name2, "/popups/%s-misc", a0);
				}
				break;
			case CTX_NONE:
				sprintf(name, "/popups/%s", a0);
				break;
				
		}
	}

	if (*name != '\0')
		r = pcb_gui->open_popup(pcb_gui, name);
	if ((r != 0) && (*name2 != '\0'))
		r = pcb_gui->open_popup(pcb_gui, name2);

	PCB_ACT_IRES(r);
	return 0;
}

const char pcb_acts_LayerHotkey[] = "LayerHotkey(layer, select|vis)";
const char pcb_acth_LayerHotkey[] = "Change the key binding for a layer";
/* DOC: layerhotkey.html */
fgw_error_t pcb_act_LayerHotkey(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layer_t *ly;
	char *op;
	const char *key, *val, *title, *msg;
	fgw_error_t er;
	fgw_arg_t r, args[4];

	PCB_ACT_CONVARG(1, FGW_LAYER, LayerHotkey, ly = fgw_layer(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_STR, LayerHotkey, op = argv[2].val.str);

	if (pcb_strcasecmp(op, "select") == 0)   { key = "pcb-rnd::key::select"; title = "set layer selection hotkey"; }
	else if (pcb_strcasecmp(op, "vis") == 0) { key = "pcb-rnd::key::vis"; title = "set layer visibility hotkey"; }
	else PCB_ACT_FAIL(LayerHotkey);

	msg =
		"Layer hotkey syntax is the same as\n"
		"the 'a' field in the menu file: it is\n"
		"a semicolon separated sequence of keys,\n"
		"each is specified as modifier<Key>k,\n"
		"where modifier is empty, Alt, Ctrl, Shift\n"
		"and k is the name of the key. For example\n"
		"{l shift-t} is written as:\n"
		"<Key>l; Shift<Key>t\n";

	val = pcb_attribute_get(&ly->Attributes, key);
	args[1].type = FGW_STR; args[1].val.str = msg;
	args[2].type = FGW_STR; args[2].val.str = val;
	args[3].type = FGW_STR; args[3].val.str = title;
	er = pcb_actionv_bin(PCB_ACT_HIDLIB, "promptfor", &r, 4, args);

	if ((er != NULL) || ((r.type & FGW_STR) != FGW_STR)) {
		fgw_arg_free(&pcb_fgw, &r);
		PCB_ACT_IRES(1);
		return 0;
	}
	
	pcb_attribute_put(&ly->Attributes, key, r.val.str);
	fgw_arg_free(&pcb_fgw, &r);

	PCB_ACT_IRES(0);
	return 0;
}

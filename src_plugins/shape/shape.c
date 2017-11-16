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

#include "config.h"
#include "action_helper.h"
#include "plugins.h"
#include "hid_actions.h"

#include "board.h"
#include "buffer.h"
#include "compat_misc.h"
#include "conf_core.h"
#include "data.h"
#include "error.h"
#include "layer.h"
#include "math_helper.h"
#include "obj_poly.h"
#include "obj_poly_draw.h"
#include "rotate.h"

const char *pcb_shape_cookie = "shape plugin";

static pcb_poly_t *regpoly(pcb_layer_t *layer, int corners, pcb_coord_t rx, pcb_coord_t ry, double rot_deg, pcb_coord_t cx, pcb_coord_t cy)
{
	int n, flags = PCB_FLAG_CLEARPOLY;
	double cosra, sinra, a, da = 2*M_PI / (double)corners;
	pcb_poly_t *p;

	if (corners < 3)
		return NULL;

	if ((rot_deg >= 360.0) || (rot_deg <= -360.0))
		rot_deg = fmod(rot_deg, 360.0);

	if (conf_core.editor.full_poly)
		flags |= PCB_FLAG_FULLPOLY;
	if (conf_core.editor.clear_polypoly)
		flags |= PCB_FLAG_CLEARPOLYPOLY;

	p = pcb_poly_new(layer, 2 * conf_core.design.clearance, pcb_flag_make(flags));
	if (p == NULL)
		return NULL;

	if (rot_deg != 0.0) {
		cosra = cos(rot_deg / PCB_RAD_TO_DEG);
		sinra = sin(rot_deg / PCB_RAD_TO_DEG);
	}

	for(n = 0,a = 0; n < corners; n++,a+=da) {
		pcb_coord_t x,  y;
		x = pcb_round(cos(a) * (double)rx + (double)cx);
		y = pcb_round(sin(a) * (double)ry + (double)cy);
		if (rot_deg != 0.0)
			pcb_rotate(&x, &y, cx, cy, cosra, sinra);
		pcb_poly_point_new(p, x, y);
	}

	pcb_add_poly_on_layer(layer, p);

	return p;
}

static pcb_poly_t *regpoly_place(pcb_data_t *data, pcb_layer_t *layer, int corners, pcb_coord_t rx, pcb_coord_t ry, double rot_deg, pcb_coord_t cx, pcb_coord_t cy)
{
	pcb_poly_t *p = regpoly(CURRENT, corners, rx, ry, rot_deg, cx, cy);

	if (p == NULL)
		return NULL;

	pcb_poly_init_clip(PCB->Data, CURRENT, p);
	pcb_poly_invalidate_draw(CURRENT, p);

	if (data != PCB->Data) {
		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		pcb_copy_obj_to_buffer(PCB, data, PCB->Data, PCB_TYPE_POLY, CURRENT, p, p);
		pcb_r_delete_entry(CURRENT->polygon_tree, (pcb_box_t *)p);
		pcb_poly_free_fields(p);
		pcb_poly_free(p);
		pcb_crosshair_set_mode(PCB_MODE_PASTE_BUFFER);
	}
	return p;
}

static const char pcb_acts_regpoly[] = "regpoly([where,] corners, radius [,rotation])";
static const char pcb_acth_regpoly[] = "Generate regular polygon.";
int pcb_act_regpoly(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	double rot = 0;
	pcb_coord_t rx, ry = 0;
	pcb_bool succ, have_coords = pcb_false;
	int corners = 0, a;
	pcb_data_t *data = PCB->Data;
	const char *dst;
	char *end;

	if (argc < 2) {
		pcb_message(PCB_MSG_ERROR, "regpoly() needs at least two parameters\n");
		return -1;
	}
	if (argc > 4) {
		pcb_message(PCB_MSG_ERROR, "regpoly(): too many arguments\n");
		return -1;
	}

	a = 0;
	dst = argv[0];
	if (pcb_strncasecmp(dst, "buffer", 6) == 0) {
		data = PCB_PASTEBUFFER->Data;
		dst += 6;
		a = 1;
	}
	
	end = strchr(dst, ';');
	if (end != NULL) {
		char *sx, *sy, *tmp;
		int offs = end - dst;
		have_coords = 1;
		a = 1;
		
		tmp = pcb_strdup(dst);
		tmp[offs] = '\0';
		sx = tmp;
		sy = tmp + offs + 1;
		x = pcb_get_value(sx, NULL, NULL, &succ);
		if (succ)
			y = pcb_get_value(sy, NULL, NULL, &succ);
		free(tmp);
		if (!succ) {
			pcb_message(PCB_MSG_ERROR, "regpoly(): invalid center coords '%s'\n", dst);
			return -1;
		}
	}

	corners = strtol(argv[a], &end, 10);
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "regpoly(): invalid number of corners '%s'\n", argv[a]);
		return -1;
	}
	a++;

	/* convert radii */
	dst = argv[a];
	end = strchr(dst, ';');
	if (end != NULL) {
		char *sx, *sy, *tmp = pcb_strdup(dst);
		int offs = end - dst;
		tmp[offs] = '\0';
		sx = tmp;
		sy = tmp + offs + 1;
		rx = pcb_get_value(sx, NULL, NULL, &succ);
		if (succ)
			ry = pcb_get_value(sy, NULL, NULL, &succ);
		free(tmp);
	}
	else
		rx = ry = pcb_get_value(dst, NULL, NULL, &succ);

	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "regpoly(): invalid radius '%s'\n", argv[a]);
		return -1;
	}
	a++;

	if (a < argc) {
		rot = strtod(argv[a], &end);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "regpoly(): invalid rotation '%s'\n", argv[a]);
			return -1;
		}
	}

	if ((data == PCB->Data) && (!have_coords))
		pcb_gui->get_coords("Click on the center of the polygon", &x, &y);

	if (regpoly_place(data, CURRENT, corners, rx, ry, rot, x, y) == NULL)
		pcb_message(PCB_MSG_ERROR, "regpoly(): failed to create the polygon\n");

	return 0;
}

static const char pcb_acts_shape[] = "shape()";
static const char pcb_acth_shape[] = "Interactive shape generator.";
int pcb_act_shape(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

pcb_hid_action_t shape_action_list[] = {
	{"regpoly", 0, pcb_act_regpoly, pcb_acth_regpoly, pcb_acts_regpoly},
	{"shape", 0, pcb_act_shape, pcb_acth_shape, pcb_acts_shape}
};

PCB_REGISTER_ACTIONS(shape_action_list, pcb_shape_cookie)

int pplg_check_ver_shape(int ver_needed) { return 0; }

void pplg_uninit_shape(void)
{
	pcb_hid_remove_actions_by_cookie(pcb_shape_cookie);
}


#include "dolists.h"
int pplg_init_shape(void)
{
	PCB_REGISTER_ACTIONS(shape_action_list, pcb_shape_cookie)
	return 0;
}

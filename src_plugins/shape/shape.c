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

const char *pcb_shape_cookie = "shape plugin";

static const char pcb_acts_regpoly[] = "regpoly([where,] corners, radius [,rotation])";
static const char pcb_acth_regpoly[] = "Generate regular polygon.";
int pcb_act_regpoly(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	double rot = 0;
	pcb_coord_t rx, ry = 0;
	pcb_bool succ, have_coords = pcb_false;
	int corners = 0, a, offs;
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

pcb_trace("regpoly: %d c=%d rad=%$mm;%$mm rot=%f center=%$mm;%$mm\n", data == PCB->Data, corners, rx, ry, rot, x, y);

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

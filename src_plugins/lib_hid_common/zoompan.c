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

#include <stdlib.h>

#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/hidlib.h>

#include "zoompan.h"

const char *pcb_acts_Zoom;
const char pcb_acts_Zoom_default[] = pcb_gui_acts_zoom;
fgw_error_t pcb_gui_act_zoom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hidlib_t *hidlib = RND_ACT_HIDLIB;
	const char *vp, *ovp;
	double v;
	rnd_coord_t x = 0, y = 0;

	PCB_GUI_NOGUI();

	if (argc < 2) {
		rnd_gui->zoom_win(rnd_gui, 0, 0, hidlib->size_x, hidlib->size_y, 1);
		return 0;
	}

	if (argc == 5) {
		rnd_coord_t x1, y1, x2, y2;

		RND_PCB_ACT_CONVARG(1, FGW_COORD, Zoom, x1 = fgw_coord(&argv[1]));
		RND_PCB_ACT_CONVARG(2, FGW_COORD, Zoom, y1 = fgw_coord(&argv[2]));
		RND_PCB_ACT_CONVARG(3, FGW_COORD, Zoom, x2 = fgw_coord(&argv[3]));
		RND_PCB_ACT_CONVARG(4, FGW_COORD, Zoom, y2 = fgw_coord(&argv[4]));

		rnd_gui->zoom_win(rnd_gui, x1, y1, x2, y2, 1);
		return 0;
	}

	if (argc > 2)
		RND_ACT_FAIL(Zoom);

	RND_PCB_ACT_CONVARG(1, FGW_STR, Zoom, ovp = vp = argv[1].val.str);

	if (*vp == '?') {
		rnd_message(RND_MSG_INFO, "Current zoom level (coord-per-pix): %$mm\n", rnd_gui->coord_per_pix);
		return 0;
	}

	if (rnd_strcasecmp(argv[1].val.str, "get") == 0) {
		res->type = FGW_DOUBLE;
		res->val.nat_double = rnd_gui->coord_per_pix;
		return 0;
	}

	if (*vp == '+' || *vp == '-' || *vp == '=')
		vp++;
	v = strtod(vp, NULL);
	if (v <= 0)
		return FGW_ERR_ARG_CONV;

	rnd_hid_get_coords("Select zoom center", &x, &y, 0);
	switch (ovp[0]) {
	case '-':
		rnd_gui->zoom(rnd_gui, x, y, 1 / v, 1);
		break;
	default:
	case '+':
		rnd_gui->zoom(rnd_gui, x, y, v, 1);
		break;
	case '=':
		rnd_gui->zoom(rnd_gui, x, y, v, 0);
		break;
	}

	RND_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Pan[] = "Pan(Mode)";
const char pcb_acth_Pan[] = "Start or stop panning (Mode = 1 to start, 0 to stop)\n";
/* DOC: pan.html */
fgw_error_t pcb_act_Pan(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int mode;
	rnd_coord_t x, y;

	PCB_GUI_NOGUI();

	rnd_hid_get_coords("Click on a place to pan", &x, &y, 0);

	RND_PCB_ACT_CONVARG(1, FGW_INT, Pan, mode = argv[1].val.nat_int);
	rnd_gui->pan_mode(rnd_gui, x, y, mode);

	RND_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Center[] = "Center()\n";
const char pcb_acth_Center[] = "Moves the pointer to the center of the window.";
/* DOC: center.html */
fgw_error_t pcb_act_Center(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t x, y;
	PCB_GUI_NOGUI();

	rnd_hid_get_coords("Click to center", &x, &y, 0);

	if (argc != 1)
		RND_ACT_FAIL(Center);

	rnd_gui->pan(rnd_gui, x, y, 0);

	RND_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Scroll[] = "Scroll(up|down|left|right, [pixels])";
const char pcb_acth_Scroll[] = "Scroll the viewport.";
/* DOC: scroll.html */
fgw_error_t pcb_act_Scroll(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op;
	double dx = 0.0, dy = 0.0, pixels = 100.0;

	RND_PCB_ACT_CONVARG(1, FGW_STR, Scroll, op = argv[1].val.str);
	rnd_PCB_ACT_MAY_CONVARG(2, FGW_DOUBLE, Scroll, pixels = argv[2].val.nat_double);

	if (rnd_strcasecmp(op, "up") == 0)
		dy = -rnd_gui->coord_per_pix * pixels;
	else if (rnd_strcasecmp(op, "down") == 0)
		dy = rnd_gui->coord_per_pix * pixels;
	else if (rnd_strcasecmp(op, "right") == 0)
		dx = rnd_gui->coord_per_pix * pixels;
	else if (rnd_strcasecmp(op, "left") == 0)
		dx = -rnd_gui->coord_per_pix * pixels;
	else
		RND_ACT_FAIL(Scroll);

	rnd_gui->pan(rnd_gui, dx, dy, 1);

	RND_ACT_IRES(0);
	return 0;
}


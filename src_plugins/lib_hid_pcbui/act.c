/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017..2019 Tibor 'Igor2' Palinkas
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

#include "actions.h"
#include "board.h"
#include "compat_misc.h"
#include "hid.h"

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
		pcb_gui->zoom_win(0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y, 1);
		return 0;
	}

	if (argc == 5) {
		pcb_coord_t x1, y1, x2, y2;

		PCB_ACT_CONVARG(1, FGW_COORD, Zoom, x1 = fgw_coord(&argv[1]));
		PCB_ACT_CONVARG(2, FGW_COORD, Zoom, y1 = fgw_coord(&argv[2]));
		PCB_ACT_CONVARG(3, FGW_COORD, Zoom, x2 = fgw_coord(&argv[3]));
		PCB_ACT_CONVARG(4, FGW_COORD, Zoom, y2 = fgw_coord(&argv[4]));

		pcb_gui->zoom_win(x1, y1, x2, y2, 1);
		return 0;
	}

	if (argc > 2)
		PCB_ACT_FAIL(Zoom);

	PCB_ACT_CONVARG(1, FGW_STR, Zoom, ovp = vp = argv[1].val.str);

	if (pcb_strcasecmp(vp, "selected") == 0) {
		pcb_box_t sb;
		if (pcb_get_selection_bbox(&sb, PCB->Data) > 0)
			pcb_gui->zoom_win(sb.X1, sb.Y1, sb.X2, sb.Y2, 1);
		else
			pcb_message(PCB_MSG_ERROR, "Can't zoom to selection: nothing selected\n");
		return 0;
	}

	if (pcb_strcasecmp(vp, "found") == 0) {
		pcb_box_t sb;
		if (pcb_get_found_bbox(&sb, PCB->Data) > 0)
			pcb_gui->zoom_win(sb.X1, sb.Y1, sb.X2, sb.Y2, 1);
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
		pcb_gui->zoom(x, y, 1 / v, 1);
		break;
	default:
	case '+':
		pcb_gui->zoom(x, y, v, 1);
		break;
	case '=':
		pcb_gui->zoom(x, y, v, 0);
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
	pcb_gui->pan_mode(x, y, mode);

	PCB_ACT_IRES(0);
	return 0;
}

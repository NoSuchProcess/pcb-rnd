/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
#include "board.h"
#include "data.h"
#include "layer.h"
#include "layer_ui.h"
/*#include "acompnet_conf.h"*/
#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "conf.h"
#include "conf_core.h"
#include "error.h"

static pcb_layer_t *ly;

pcb_flag_t flg_mesh_pt;
static void acompnet_mesh_addpt(double x, double y)
{
	x = pcb_round(x);
	y = pcb_round(y);
	
	pcb_line_new(ly, x, y, x, y, conf_core.design.via_thickness, conf_core.design.clearance, flg_mesh_pt);
}

static void acompnet_mesh()
{
	double sep = conf_core.design.via_thickness + conf_core.design.clearance;
	PCB_LINE_LOOP(CURRENT) {
		double len, vx, vy, x1, y1, x2, y2;
		x1 = line->Point1.X;
		x2 = line->Point2.X;
		y1 = line->Point1.Y;
		y2 = line->Point2.Y;
		vx = x2 - x1;
		vy = y2 - y1;
		len = sqrt(vx*vx + vy*vy);
		vx = vx/len;
		vy = vy/len;
		acompnet_mesh_addpt(x1 - vx*sep, y1 - vy*sep);
		acompnet_mesh_addpt(x2 + vx*sep, y2 + vy*sep);
	}
	PCB_END_LOOP;
}


static const char pcb_acts_acompnet[] = "acompnet()\n" ;
static const char pcb_acth_acompnet[] = "Attempt to auto-complete the current network";

static int pcb_act_acompnet(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	acompnet_mesh();
	return 0;
}

pcb_hid_action_t acompnet_action_list[] = {
	{"acompnet", 0, pcb_act_acompnet,
	 pcb_acth_acompnet, pcb_acts_acompnet},
};

static const char *acompnet_cookie = "acompnet plugin";

PCB_REGISTER_ACTIONS(acompnet_action_list, acompnet_cookie)

static void hid_acompnet_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(acompnet_cookie);
	pcb_uilayer_free_all_cookie(acompnet_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_acompnet_init(void)
{
	PCB_REGISTER_ACTIONS(acompnet_action_list, acompnet_cookie)
	ly = pcb_uilayer_alloc(acompnet_cookie, "autocomp-net", "#c09920");

	return hid_acompnet_uninit;
}

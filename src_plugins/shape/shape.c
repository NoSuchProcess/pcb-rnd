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
#include "data.h"
#include "layer.h"

const char *pcb_shape_cookie = "shape plugin";

static const char pcb_acts_shape[] = "shape(where, type, [type-specific-params...])";
static const char pcb_acth_shape[] = "";
int pcb_act_shape(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

pcb_hid_action_t shape_action_list[] = {
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

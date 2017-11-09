/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, diag, was written and is Copyright (C) 2016 by Tibor Palinkas
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

#include "brave.h"
#include "hid_dad.h"

static struct {
	pcb_brave_t bit;
	const char *shrt, *lng;
} desc[] = {
	{PCB_BRAVE_PSTK_VIA, "use padstack for vias",
		"placing new vias will place padstacks instsad of old via objects" },
	{0, NULL, NULL}
};


static const char pcb_acts_Brave[] =
	"Brave()\n"
	"Brave(setting, on|off)\n";
static const char pcb_acth_Brave[] = "Changes brave settings.";
static int pcb_act_Brave(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

pcb_hid_action_t brave_action_list[] = {
	{"Brave", 0, pcb_act_Brave,
	 pcb_acth_Brave, pcb_acts_Brave}
};

PCB_REGISTER_ACTIONS(brave_action_list, NULL)


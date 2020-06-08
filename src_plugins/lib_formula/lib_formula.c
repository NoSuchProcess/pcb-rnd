/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <librnd/core/plugins.h>

#include "impedance.h"
#include "bisect.h"

static rnd_action_t lib_formula_action_list[] = {
	{"impedance_microstrip", pcb_act_impedance_microstrip, pcb_acth_impedance_microstrip, pcb_acts_impedance_microstrip},
	{"impedance_coplanar_waveguide", pcb_act_impedance_coplanar_waveguide, pcb_acth_impedance_coplanar_waveguide, pcb_acts_impedance_coplanar_waveguide},
	{"formula_bisect", pcb_act_formula_bisect, pcb_acth_formula_bisect, pcb_acts_formula_bisect}
};

char *lib_formula_cookie = "lib_formula plugin";

int pplg_check_ver_lib_formula(int ver_needed) { return 0; }

void pplg_uninit_lib_formula(void)
{
	rnd_remove_actions_by_cookie(lib_formula_cookie);
}

int pplg_init_lib_formula(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(lib_formula_action_list, lib_formula_cookie);
	return 0;
}

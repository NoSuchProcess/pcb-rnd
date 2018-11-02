/*
 *                            COPYRIGHT
 *
 *  bar - an external fungw plugin for pcb-rnd (example/template plugin)
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Contact: TODO: FILL THIS IN
 */

/* This is pcb-rnd's config.h from trunk: */
#include "config.h"

#include <stdio.h>

#include "src/actions.h"
#include "src/error.h"

static fgw_error_t pcb_act_ExtBar(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_message(PCB_MSG_ERROR, "PLEASE CONSIDER DEVELOPING A CORE PLUGIN INSTEAD!\n");
	PCB_ACT_IRES(0);
	return 0;
}

int pplg_check_ver_ext_bar(int ver_needed) { return 0; }

void pplg_uninit_ext_bar(void)
{
	fprintf(stderr, "EXT BAR uninit\n");
	pcb_remove_actions_by_cookie(ext_bar_cookie);
}

#include "src/dolists.h"

int pplg_init_ext_bar(void)
{
	PCB_API_CHK_VER; /* for external plugins this is CRITICAL */

	fgw_func_reg(&pcb_fgw, "extbar", ExtBar); /* need to register with lowercase name */

	fprintf(stderr, "EXT BAR init\n");
	return 0;
}

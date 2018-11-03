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
#include <libfungw/fungw.h>

#include "src/error.h"

static fgw_error_t extbar(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_message(PCB_MSG_ERROR, "PLEASE CONSIDER DEVELOPING A CORE PLUGIN INSTEAD!\n");
	res->val.nat_int = 0;
	res->type = FGW_INT;
	return 0;
}

void pcb_rnd_uninit(fgw_obj_t *obj)
{
	fprintf(stderr, "EXT BAR uninit\n");
}

int pcb_rnd_init(fgw_obj_t *obj, const char *opts)
{
	fgw_func_reg(obj, "extbar", extbar); /* need to register with lowercase name */

	fprintf(stderr, "EXT BAR init with '%s'\n", opts);
	return 0;
}

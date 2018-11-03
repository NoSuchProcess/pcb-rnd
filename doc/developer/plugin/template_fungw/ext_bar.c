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

#include <stdio.h>
#include <libfungw/fungw.h>

static fgw_error_t extbar(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_ctx_t *ctx = argv[0].val.func->obj->parent;
	fgw_arg_t call_res;
	int retval;

	/* pcb-rnd API shall be accessed through fungw calls */
	retval = fgw_vcall(ctx, &call_res, "message", FGW_STR, "PLEASE CONSIDER DEVELOPING A CORE PLUGIN INSTEAD!\n", 0);

	/* need to check and free the return value - any action call may fail */
	if (retval != 0)
		fprintf(stderr, "ERROR: failed to call message() (fungw level)\n");
	else if ((call_res.type != FGW_INT) || (call_res.val.nat_int != 0))
		fprintf(stderr, "ERROR: failed to call message() (action level)\n");
	fgw_arg_free(ctx, &call_res);

	/* Set the return value of this action */
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

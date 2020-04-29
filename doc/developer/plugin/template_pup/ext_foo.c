/*
 *                            COPYRIGHT
 *
 *  foo - an external plugin for pcb-rnd (example/template plugin)
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

/* These are pcb-rnd's headers from trunk: */
#include "src/actions.h"
#include "src/plugins.h"

static const char *ext_foo_cookie = "ext foo (external plugin example/template)";

static const char pcb_acts_ExtFoo[] = "ExtFoo(arg1, arg2, [arg3]...)";
static const char pcb_acth_ExtFoo[] = "Help text: short description of what the action does.";
static fgw_error_t pcb_act_ExtFoo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "PLEASE CONSIDER DEVELOPING A CORE PLUGIN INSTEAD!\n");
	RND_ACT_IRES(0);
	return 0;
}

static const rnd_action_t ext_foo_action_list[] = {
	{"ExtFoo", pcb_act_ExtFoo, pcb_acth_ExtFoo, pcb_acts_ExtFoo}
};

int pplg_check_ver_ext_foo(int ver_needed) { return 0; }

void pplg_uninit_ext_foo(void)
{
	fprintf(stderr, "EXT FOO uninit\n");
	rnd_remove_actions_by_cookie(ext_foo_cookie);
}


int pplg_init_ext_foo(void)
{
	PCB_API_CHK_VER; /* for external plugins this is CRITICAL */

	RND_REGISTER_ACTIONS(ext_foo_action_list, ext_foo_cookie);

	fprintf(stderr, "EXT FOO init\n");
	return 0;
}

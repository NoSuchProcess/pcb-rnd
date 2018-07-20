/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <genvector/gds_char.h>

#include "hid_attrib.h"
#include "hid_init.h"
#include "plugins.h"
#include "actions.h"

static const char *ddraft_cookie = "ddraft plugin";

static const char pcb_acts_trim[] = "trim(selected|found|object, selected|found|object)\n";
static const char pcb_acth_trim[] = "Use one or more objects as cutting edge and trim other objects. First argument is the cutting edge";
static fgw_error_t pcb_act_trim(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *scut, *sobj;
	int rs = -1;

	PCB_ACT_CONVARG(1, FGW_STR, trim, scut = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, trim, sobj = argv[2].val.str);

	PCB_ACT_IRES(rs);
	return 0;
}


static pcb_action_t ddraft_action_list[] = {
	{"trim", pcb_act_trim, pcb_acth_trim, pcb_acts_trim}
};

PCB_REGISTER_ACTIONS(ddraft_action_list, ddraft_cookie)

int pplg_check_ver_ddraft(int ver_needed) { return 0; }

void pplg_uninit_ddraft(void)
{
	pcb_remove_actions_by_cookie(ddraft_cookie);
}

#include "dolists.h"
int pplg_init_ddraft(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(ddraft_action_list, ddraft_cookie)

	return 0;
}

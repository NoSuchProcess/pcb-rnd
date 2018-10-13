/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  hand assembly assistant GUI
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

#include "board.h"
#include "data.h"
#include "plugins.h"
#include "actions.h"

static const char *asm_cookie = "asm plugin";

static const char pcb_acts_asm[] = "asm()";
static const char pcb_acth_asm[] = "Interactive assembly assistant";
fgw_error_t pcb_act_asm(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t asm_action_list[] = {
	{"asm", pcb_act_asm, pcb_acth_asm, pcb_acts_asm}
};

PCB_REGISTER_ACTIONS(asm_action_list, asm_cookie)

int pplg_check_ver_asm(int ver_needed) { return 0; }

void pplg_uninit_asm(void)
{
	pcb_remove_actions_by_cookie(asm_cookie);
}


#include "dolists.h"
int pplg_init_asm(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(asm_action_list, asm_cookie)

	return 0;
}

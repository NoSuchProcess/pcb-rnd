/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design - EM simulation
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "data.h"

#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/hid/hid_menu.h>
#include "menu_internal.c"

#include "src_plugins/export_emsim/export_emsim.h"

static const char pcb_acts_EmsDcpower[] = "EmsDcpower([port,name,refdes,terminal]*,[resistor|vsrc,port1,port2,value]*)";
static const char pcb_acth_EmsDcpower[] = "DC power simulation. ";
/* DOC: ... */
static fgw_error_t pcb_act_EmsDcpower(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int n, retv = 0;
	emsim_env_t env = {0};
	emsim_lumped_t *lump;

	for(n = 1; n < argc; ) {
		const char *cmd;
		RND_ACT_CONVARG(n, FGW_STR, EmsDcpower, cmd = argv[n].val.str);
		if (strcmp(cmd, "port") == 0) {
			lump = malloc(sizeof(emsim_lumped_t));
			lump->type = PORT;
			RND_ACT_CONVARG(n+1, FGW_STR, EmsDcpower, lump->data.port.name = argv[n+1].val.str);
			RND_ACT_CONVARG(n+2, FGW_STR, EmsDcpower, lump->data.port.refdes = argv[n+2].val.str);
			RND_ACT_CONVARG(n+3, FGW_STR, EmsDcpower, lump->data.port.term = argv[n+3].val.str);
			n+=4;
			emsim_lumped_link(&env, lump);
		}
		else if (strcmp(cmd, "resistor") == 0) {
			lump = malloc(sizeof(emsim_lumped_t));
			lump->type = RESISTOR;
			comp:;
			RND_ACT_CONVARG(n+1, FGW_STR, EmsDcpower, lump->data.comp.port1 = argv[n+1].val.str);
			RND_ACT_CONVARG(n+2, FGW_STR, EmsDcpower, lump->data.comp.port2 = argv[n+2].val.str);
			RND_ACT_CONVARG(n+3, FGW_STR, EmsDcpower, lump->data.comp.value = argv[n+3].val.str);
			n+=4;
			emsim_lumped_link(&env, lump);
		}
		else if (strcmp(cmd, "vsrc") == 0) {
			lump = malloc(sizeof(emsim_lumped_t));
			lump->type = VSRC;
			goto comp;
		}
		else {
			rnd_message(RND_MSG_ERROR, "EmsDcpower: invalid instruction '%s'\n", cmd);
			retv = FGW_ERR_ARG_CONV;
			goto quit;
		}
	}

	emsim_export_to_file(pcb, &env, stdout);

	quit:;
	return retv;
}


rnd_action_t emsim_action_list[] = {
	{"EmsDcpower", pcb_act_EmsDcpower, pcb_acth_EmsDcpower, pcb_acts_EmsDcpower}
};


static const char *emsim_cookie = "emsim plugin";

int pplg_check_ver_emsim(int ver_needed) { return 0; }

void pplg_uninit_emsim(void)
{
	rnd_remove_actions_by_cookie(emsim_cookie);
	rnd_hid_menu_unload(rnd_gui, emsim_cookie);
}

int pplg_init_emsim(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(emsim_action_list, emsim_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, emsim_cookie, 175, NULL, 0, emsim_menu, "plugin: emsim");
	return 0;
}

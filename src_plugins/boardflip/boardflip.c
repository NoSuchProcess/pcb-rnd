/*
 * BoardFlip plug-in for PCB.
 *
 * Copyright (C) 2008 DJ Delorie <dj@delorie.com>
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016 and generalized in 2017, 2018.
 *
 * Original source was: http://www.delorie.com/pcb/boardflip.c
 *
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "actions.h"
#include "plugins.h"
#include "board.h"
#include "data.h"
#include "funchash_core.h"


static const char pcb_acts_boardflip[] = "BoardFlip([sides])";
static const char pcb_acth_boardflip[] = "Mirror the board over the x axis, optionally mirroring sides as well.";
/* DOC: boardflip.html */
static fgw_error_t pcb_act_boardflip(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = -2;
	pcb_bool smirror;

	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, boardflip, op = fgw_keyword(&argv[1]));

	smirror = (op == F_Sides);
	pcb_data_mirror(PCB->Data, 0, smirror ? PCB_TXM_SIDE : PCB_TXM_COORD, smirror);

	PCB_ACT_IRES(0);
	return 0;
}


static pcb_action_t boardflip_action_list[] = {
	{"BoardFlip", pcb_act_boardflip, pcb_acth_boardflip, pcb_acts_boardflip}
};

char *boardflip_cookie = "boardflip plugin";

PCB_REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie)

int pplg_check_ver_boardflip(int ver_needed) { return 0; }

void pplg_uninit_boardflip(void)
{
	pcb_remove_actions_by_cookie(boardflip_cookie);
}

#include "dolists.h"
int pplg_init_boardflip(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie);
	return 0;
}


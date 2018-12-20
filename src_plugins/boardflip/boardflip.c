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




int pplg_check_ver_boardflip(int ver_needed) { return 0; }

void pplg_uninit_boardflip(void)
{
}

int pplg_init_boardflip(void)
{
	PCB_API_CHK_VER;
	return 0;
}


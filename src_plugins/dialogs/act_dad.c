/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include <genht/htsp.h>
#include <genht/hash.h>
#include "actions.h"

#include "act_dad.h"

typedef struct {
	char *name;
} dad_t;

htsp_t dads;

static void dad_destroy(dad_t *dad)
{
	htsp_pop(&dads, dad->name);
	free(dad->name);
	free(dad);
}

const char pcb_acts_dad[] =
	"dad()\n"
	;
const char pcb_acth_dad[] = "Manipulate Dynamic Attribute Dialogs";
fgw_error_t pcb_act_dad(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(0);
	return 0;
}

void pcb_act_dad_init(void)
{
	htsp_init(&dads, strhash, strkeyeq);
}

void pcb_act_dad_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&dads); e != NULL; e = htsp_next(&dads, e))
		dad_destroy(e->value);
	htsp_uninit(&dads);
}

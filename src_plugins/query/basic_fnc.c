/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "global.h"
#include "data.h"
#include "query_access.h"
#include "query_exec.h"

/* Query language - basic functions */
static int fnc_llen(int argc, pcb_qry_val_t *argv[], pcb_qry_val_t *res)
{
	if (argc != 1)
		return -1;
	if (argv[0]->type != PCBQ_VT_LST)
		PCB_QRY_RET_INV(res);
	PCB_QRY_RET_INT(res, pcb_objlist_length(&argv[0]->data.lst));
}



void pcb_qry_basic_fnc_init(void)
{
	pcb_qry_fnc_reg("llen", fnc_llen);
}

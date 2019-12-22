/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Basic, standard extended objects
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <stdio.h>

#include "board.h"
#include "data.h"
#include "plugins.h"
#include "actions.h"
#include "obj_subc.h"
#include "pcb-printf.h"
#include "extobj.h"
#include "extobj_helper.h"
#include "conf_core.h"
#include "hid_inlines.h"
#include "hid_dad.h"

#include "line_of_vias.c"
#include "dimension.c"

int pplg_check_ver_exto_std(int ver_needed) { return 0; }

void pplg_uninit_exto_std(void)
{
	pcb_extobj_unreg(&pcb_line_of_vias);
	pcb_extobj_unreg(&pcb_dimension);
}

int pplg_init_exto_std(void)
{
	PCB_API_CHK_VER;

	pcb_extobj_reg(&pcb_line_of_vias);
	pcb_extobj_reg(&pcb_dimension);

	return 0;
}

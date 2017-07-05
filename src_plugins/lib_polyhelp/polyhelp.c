/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <stdio.h>

#include "polyhelp.h"
#include "plugins.h"


void pcb_pline_fprint_anim(FILE *f, pcb_pline_t *pl)
{
	pcb_vnode_t *v, *n;
	fprintf(f, "!pline start\n");
	v = pl;
	do {
		n = v->next;
		pcb_fprintf(f, "line %#mm %#mm %#mm %#mm\n", v->point[0], v->point[1], n->point[0], n->point[1]);
	}
	while ((v = v->next) != pl);
	fprintf(f, "!pline end\n");
}

int pplg_check_ver_lib_polyhelp(int ver_needed) { return 0; }

void pplg_uninit_lib_polyhelp(void)
{
}

int pplg_init_lib_polyhelp(void)
{
	return 0;
}

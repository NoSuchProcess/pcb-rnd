/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "flag.h"
#include "operation.h"
#include "obj_all_op.h"

pcb_opfunc_t ChgFlagFunctions = {
	ChgFlagLine,
	ChgFlagText,
	ChgFlagPolygon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChgFlagArc,
	NULL,
	NULL,
	ChgFlagSubc
};




/* This just fills in a pcb_flag_t with current flags.  */
pcb_flag_t pcb_flag_make(unsigned int flags)
{
	pcb_flag_t rv;
	memset(&rv, 0, sizeof(rv));
	rv.f = flags;
	return rv;
}

pcb_flag_t pcb_flag_add(pcb_flag_t flag, unsigned int flags)
{
	flag.f |= flags;
	return flag;
}

pcb_flag_t pcb_flag_mask(pcb_flag_t flag, unsigned int flags)
{
	flag.f &= ~flags;
	return flag;
}

int pcb_mem_any_set(unsigned char *ptr, int bytes)
{
	while (bytes--)
		if (*ptr++)
			return 1;
	return 0;
}

void pcb_flag_erase(pcb_flag_t * f)
{
	pcb_unknown_flag_t *u, *next;
	for (u = f->unknowns; u != NULL; u = next) {
		free(u->str);
		next = u->next;
		free(u);
	}
	f->unknowns = NULL;
}

int pcb_flag_eq(pcb_flag_t *f1, pcb_flag_t *f2)
{
	if (f1->f != f2->f)
		return 0;

	if (f1->q != f2->q)
		return 0;

	if (f1->int_conn_grp != f2->int_conn_grp)
		return 0;

	/* WARNING: ignore unknowns for now: the only place where we use this function,
	undo.c, won't care */

	return (memcmp(f1->t, &f2->t, sizeof(f1->t)) == 0);
}

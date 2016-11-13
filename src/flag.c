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

/* This just fills in a pcb_flag_t with current flags.  */
pcb_flag_t pcb_flag_make(unsigned int flags)
{
	pcb_flag_t rv;
	memset(&rv, 0, sizeof(rv));
	rv.f = flags;
	return rv;
}

/* This converts old flag bits (from saved PCB files) to new format.  */
pcb_flag_t pcb_flag_old(unsigned int flags)
{
	pcb_flag_t rv;
	int i, f;
	memset(&rv, 0, sizeof(rv));
	/* If we move flag bits around, this is where we map old bits to them.  */
	rv.f = flags & 0xffff;
	f = 0x10000;
	for (i = 0; i < 8; i++) {
		/* use the closest thing to the old thermal style */
		if (flags & f)
			rv.t[i / 2] |= (1 << (4 * (i % 2)));
		f <<= 1;
	}
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

int mem_any_set(unsigned char *ptr, int bytes)
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

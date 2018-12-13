/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "flag.h"
#include "operation.h"

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

	/* WARNING: ignore unknowns for now: the only place where we use this function,
	undo.c, won't care */

	return (memcmp(f1->t, &f2->t, sizeof(f1->t)) == 0);
}

const char *pcb_dynflag_cookie[PCB_DYNFLAG_BLEN];

pcb_dynf_t pcb_dynflag_alloc(const char *cookie)
{
	pcb_dynf_t n;
	for(n = 0; n < PCB_DYNFLAG_BLEN; n++) {
		if (pcb_dynflag_cookie[n] == NULL) {
			pcb_dynflag_cookie[n] = cookie;
			return n;
		}
	}
	return PCB_DYNF_INVALID;
}

void pcb_dynflag_free(pcb_dynf_t dynf)
{
	if ((dynf >= 0) && (dynf < PCB_DYNFLAG_BLEN))
		pcb_dynflag_cookie[dynf] = NULL;
}


void pcb_dynflag_uninit(void)
{
	pcb_dynf_t n;
	for(n = 0; n < PCB_DYNFLAG_BLEN; n++)
		if (pcb_dynflag_cookie[n] != NULL)
			fprintf(stderr, "pcb-rnd: Internal error: dynamic flag %d (%s) not unregistered\n", n, pcb_dynflag_cookie[n]);
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
#include "ht_endp.h"
#include "ht_endp.c"

#include "endp.h"

RND_INLINE void hpgl_add_endp(htendp_t *ht, rnd_cheap_point_t ep, void *a)
{
	htendp_entry_t *e;
	e = htendp_getentry(ht, ep);
	if (e == NULL) {
		vtp0_t empty = {0};
		htendp_insert(ht, ep, empty);
		e = htendp_getentry(ht, ep);
	}
	else {
		long n;
		for(n = 0; n < e->value.used; n++)
			if (e->value.array[n] == a)
				return;
	}

	vtp0_append(&e->value, a);
}

void hpgl_add_arc(htendp_t *ht, pcb_arc_t *a, pcb_dynf_t dflg)
{
	rnd_cheap_point_t ep;

	PCB_DFLAG_SET(&a->Flags, dflg);

	pcb_arc_get_end(a, 0, &ep.X, &ep.Y);
	hpgl_add_endp(ht, ep, a);

	pcb_arc_get_end(a, 1, &ep.X, &ep.Y);
	hpgl_add_endp(ht, ep, a);
}

void hpgl_add_line(htendp_t *ht, pcb_line_t *l, pcb_dynf_t dflg)
{
	rnd_cheap_point_t ep;

	PCB_DFLAG_SET(&l->Flags, dflg);

	ep.X = l->Point1.X; ep.Y = l->Point1.Y;
	hpgl_add_endp(ht, ep, l);

	ep.X = l->Point2.X; ep.Y = l->Point2.Y;
	hpgl_add_endp(ht, ep, l);
}

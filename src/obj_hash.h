/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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

#include <genht/hash.h>
#include <genlist/gendlist.h>
#include "compat_misc.h"

/* compare two strings and return 0 if they are equal. NULL == NULL means equal. */
PCB_INLINE int pcb_neqs(const char *s1, const char *s2)
{
	if ((s1 == NULL) && (s2 == NULL)) return 0;
	if ((s1 == NULL) || (s2 == NULL)) return 1;
	return strcmp(s1, s2) != 0;
}

PCB_INLINE unsigned pcb_hash_coord(pcb_coord_t c)
{
	return murmurhash(&(c), sizeof(pcb_coord_t));
}

PCB_INLINE void pcb_hash_tr_coords(const pcb_host_trans_t *tr, pcb_coord_t *dstx, pcb_coord_t *dsty, pcb_coord_t srcx, pcb_coord_t srcy)
{
	pcb_coord_t px, py;

	px = srcx - tr->ox;
	py = srcy - tr->oy;

	if ((tr->rot == 0.0) && (!tr->on_bottom)) {
		*dstx = px;
		*dsty = py;
	}
	else {
		*dstx = pcb_round((double)px * tr->cosa + (double)py * tr->sina);
		*dsty = pcb_round((double)py * tr->cosa - (double)px * tr->sina);
	}
}


PCB_INLINE unsigned pcb_hash_angle(const pcb_host_trans_t *tr, pcb_angle_t ang)
{
	long l;
	ang -= tr->rot;
	ang *= 10000;
	l = floor(ang);
	return murmurhash(&l, sizeof(l));
}

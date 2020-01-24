/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

/* Resolve polygon self intersections */

#include <genvector/vtp0.h>
#include <librnd/poly/polyarea.h>

/* Returns whether a polyline is self-intersecting. O(n^2) */
pcb_bool pcb_pline_is_selfint(pcb_pline_t *pl);

/* Assume pl is self-intersecting split it up into a list of freshly allocated
   plines in out. O(n^2) */
void pcb_pline_split_selfint(const pcb_pline_t *pl, vtp0_t *out);


pcb_cardinal_t pcb_polyarea_split_selfint(pcb_polyarea_t *pa);




/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* Search functions with rtree callbacks; implementation: in search.c */

#ifndef PCB_SEARCH_R_H
#define PCB_SEARCH_R_H

#include <librnd/poly/rtree.h>

/* Search data for given object types within a box using the usual rtree conventions for the callback */
rnd_r_dir_t pcb_search_data_by_loc(pcb_data_t *data, pcb_objtype_t type, const rnd_rnd_box_t *query_box, rnd_r_dir_t (*cb_)(void *closure, pcb_any_obj_t *obj, void *box), void *closure);

#endif

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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_DATA_LIST_H
#define PCB_DATA_LIST_H

#include <genvector/vtp0.h>
#include "data.h"

/* append objects of type, with flags matching mask to dst; pointer are
   valid only until the first change to data; if flags is 0, everything matches. */
void pcb_data_list_by_flag(pcb_data_t *data, vtp0_t *dst, pcb_objtype_t type, unsigned long mask);

/* append objects of type that are terminals; pointer are
   valid only until the first change to data */
void pcb_data_list_terms(pcb_data_t *data, vtp0_t *dst, pcb_objtype_t type);

#endif

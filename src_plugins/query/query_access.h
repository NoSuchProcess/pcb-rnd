/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* Query language - access to / extract core data */

#ifndef PCB_QUERY_ACCESS_H
#define PCB_QUERY_ACCESS_H

#include "query.h"
#include "obj_any.h"

/* Append objects with matching type to lst */
void pcb_qry_list_all(pcb_qry_val_t *lst, pcb_objtype_t mask);

int pcb_qry_list_cmp(pcb_qry_val_t *lst1, pcb_qry_val_t *lst2);

void pcb_qry_list_free(pcb_qry_val_t *lst_);

#endif

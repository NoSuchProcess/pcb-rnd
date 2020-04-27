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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Query language - access to / extract core data */

#ifndef PCB_QUERY_ACCESS_H
#define PCB_QUERY_ACCESS_H

#include "query.h"

/* Append objects with matching type to lst */
void pcb_qry_list_all_pcb(pcb_qry_val_t *lst, pcb_objtype_t mask);
void pcb_qry_list_all_data(pcb_qry_val_t *lst, pcb_data_t *data, pcb_objtype_t mask);
void pcb_qry_list_all_subc(pcb_qry_val_t *lst, pcb_subc_t *sc, pcb_objtype_t mask); /* only first level objects */

int pcb_qry_list_cmp(pcb_qry_val_t *lst1, pcb_qry_val_t *lst2);

void pcb_qry_list_free(pcb_qry_val_t *lst_);

int pcb_qry_obj_field(pcb_qry_exec_t *ec, pcb_qry_val_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res);


#endif

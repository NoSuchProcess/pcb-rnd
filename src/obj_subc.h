/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#ifndef PCB_OBJ_SUBC_H
#define PCB_OBJ_SUBC_H

#include "obj_common.h"
#include "global_typedefs.h"

struct pcb_subc_s {
	PCB_ANYOBJECTFIELDS;
	char uid[64]; /* globally unique ID */
	pcb_data_t *data;
	gdl_elem_t link;
};

/* convert buffer contents into a subcircuit, in-place; returns 0 on success */
int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer);

void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY);

/*** loops ***/

#define PCB_SUBC_LOOP(top) do {                                     \
	pcb_subc_t *subc;                                                 \
	gdl_iterator_t __it__;                                            \
	subclist_foreach(&(top)->subc, &__it__, subc) {


#endif

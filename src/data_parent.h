/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016..2017 Tibor 'Igor2' Palinkas
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

#ifndef PCB_DATA_PARENT_H
#define PCB_DATA_PARENT_H

#include "global_typedefs.h"

/* which elem of the parent union is active */
typedef enum pcb_parenttype_e {
	PCB_PARENT_INVALID = 0,  /* invalid or unknown */
	PCB_PARENT_LAYER,        /* object is on a layer */
	PCB_PARENT_SUBC,         /* object is part of a subcircuit */
	PCB_PARENT_DATA,         /* global objects like via */
	PCB_PARENT_BOARD,        /* directly under a board (typical for pcb_data_t of a board) */
	PCB_PARENT_NET,          /* pcb_net_term_t's parent is a pcb_net_t */
	PCB_PARENT_UI            /* parent of UI layers */
} pcb_parenttype_t;

/* class is e.g. PCB_OBJ_CLASS_OBJ */
#define PCB_OBJ_IS_CLASS(type, class)  (((type) & PCB_OBJ_CLASS_MASK) == (class))

union pcb_parent_s {
	void           *any;
	pcb_layer_t    *layer;
	pcb_data_t     *data;
	pcb_subc_t     *subc;
	pcb_board_t    *board;
	pcb_net_t      *net;
};

#define PCB_PARENT_TYPENAME_layer    PCB_PARENT_LAYER
#define PCB_PARENT_TYPENAME_data     PCB_PARENT_DATA
#define PCB_PARENT_TYPENAME_subc     PCB_PARENT_SUBC
#define PCB_PARENT_TYPENAME_board    PCB_PARENT_BOARD

#define PCB_SET_PARENT(obj, ptype, parent_ptr) \
	do { \
		(obj)->parent_type = PCB_PARENT_TYPENAME_ ## ptype; \
		(obj)->parent.ptype = parent_ptr; \
	} while(0)

#define PCB_CLEAR_PARENT(obj) \
	do { \
		(obj)->parent_type = PCB_PARENT_INVALID; \
		(obj)->parent.any = NULL; \
	} while(0)

#endif

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for select routines */

#ifndef	PCB_SELECT_H
#define	PCB_SELECT_H

#include "config.h"
#include "operation.h"

#define SELECT_TYPES	\
	(PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_POLYGON | PCB_TYPE_ELEMENT |	\
	 PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_RATLINE | PCB_TYPE_ARC)

pcb_bool pcb_select_object(void);
pcb_bool pcb_select_block(pcb_box_t *, pcb_bool);
long int *pcb_list_block(pcb_box_t *Box, int *len);

void *pcb_object_operation(pcb_opfunc_t *F, pcb_opctx_t *ctx, int Type, void *Ptr1, void *Ptr2, void *Ptr3);
pcb_bool pcb_selected_operation(pcb_opfunc_t *F, pcb_opctx_t *ctx, pcb_bool Reset, int type);

pcb_bool pcb_select_connection(pcb_bool);


typedef enum {
	SM_REGEX = 0,
	SM_LIST = 1
} pcb_search_method_t;

pcb_bool pcb_select_object_by_name(int, const char *, pcb_bool, pcb_search_method_t);

/* New API */

/* Change the selection of an element or element name (these have side effects) */
void pcb_select_element(pcb_element_t *element, pcb_change_flag_t how, int redraw);
void pcb_select_element_name(pcb_element_t *element, pcb_change_flag_t how, int redraw);

#endif

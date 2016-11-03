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

#include "global.h"
#include "operation.h"

#define SELECT_TYPES	\
	(PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_POLYGON | PCB_TYPE_ELEMENT |	\
	 PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_RATLINE | PCB_TYPE_ARC)

pcb_bool SelectObject(void);
pcb_bool SelectBlock(BoxTypePtr, pcb_bool);
long int *ListBlock(BoxTypePtr Box, int *len);
pcb_bool SelectedOperation(ObjectFunctionTypePtr, pcb_bool, int);
void *ObjectOperation(ObjectFunctionTypePtr, int, void *, void *, void *);
pcb_bool SelectConnection(pcb_bool);

typedef enum {
	SM_REGEX = 0,
	SM_LIST = 1
} search_method_t;

pcb_bool SelectObjectByName(int, const char *, pcb_bool, search_method_t);

/* New API */

/* Change the selection of an element or element name (these have side effects) */
void pcb_select_element(ElementType *element, pcb_change_flag_t how, int redraw);
void pcb_select_element_name(ElementType *element, pcb_change_flag_t how, int redraw);

#endif

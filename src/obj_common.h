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

#ifndef PCB_OBJ_COMMON_H
#define PCB_OBJ_COMMON_H

/* memset object to 0, but keep the link field */
#define reset_obj_mem(type, obj) \
do { \
	gdl_elem_t __lnk__ = obj->link; \
	memset(obj, 0, sizeof(type)); \
	obj->link = __lnk__; \
} while(0) \


/* pcb_true during file loads, for example to allow overlapping vias.
   pcb_false otherwise, to stop the user from doing normally dangerous
   things.  */
void CreateBeLenient(pcb_bool);
extern pcb_bool pcb_create_be_lenient;

void CreateIDBump(int min_id);
void CreateIDReset(void);
long int CreateIDGet(void);


#endif

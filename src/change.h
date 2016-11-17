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

/* prototypes to change object properties */

#ifndef	PCB_CHANGE_H
#define	PCB_CHANGE_H

#include "config.h"

/* ---------------------------------------------------------------------------
 * some defines
 */
#define	PCB_CHANGENAME_TYPES        \
	(PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_LINE)

#define	PCB_CHANGESIZE_TYPES        \
	(PCB_TYPE_POLYGON | PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_LINE | \
	 PCB_TYPE_ARC | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_ELEMENT)

#define	PCB_CHANGE2NDSIZE_TYPES     \
	(PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_ELEMENT)

/* We include polygons here only to inform the user not to do it that way.  */
#define PCB_CHANGECLEARSIZE_TYPES	\
	(PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLYGON)

#define	PCB_CHANGENONETLIST_TYPES     \
	(PCB_TYPE_ELEMENT)

#define	PCB_CHANGESQUARE_TYPES     \
	(PCB_TYPE_ELEMENT | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_VIA)

#define	PCB_CHANGEOCTAGON_TYPES     \
	(PCB_TYPE_ELEMENT | PCB_TYPE_PIN | PCB_TYPE_VIA)

#define PCB_CHANGEJOIN_TYPES	\
	(PCB_TYPE_ARC | PCB_TYPE_LINE | PCB_TYPE_TEXT)

#define PCB_CHANGETHERMAL_TYPES	\
	(PCB_TYPE_PIN | PCB_TYPE_VIA)

#define PCB_CHANGEMASKSIZE_TYPES    \
        (PCB_TYPE_PIN | PCB_TYPE_VIA | PCB_TYPE_PAD)

pcb_bool pcb_chg_selected_size(int, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_selected_clear_size(int, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_selected_2nd_size(int, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_selected_mask_size(int, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_selected_join(int);
pcb_bool pcb_set_selected_join(int);
pcb_bool pcb_clr_selected_join(int);
pcb_bool pcb_chg_selected_nonetlist(int);
pcb_bool pcb_chg_selected_square(int);
pcb_bool pcb_set_selected_square(int);
pcb_bool pcb_clr_selected_square(int);
pcb_bool pcb_chg_selected_thermals(int, int);
pcb_bool pcb_chg_selected_hole(void);
pcb_bool pcb_chg_selected_paste(void);
pcb_bool pcb_chg_selected_octagon(int);
pcb_bool pcb_set_selected_octagon(int);
pcb_bool pcb_clr_selected_octagon(int);
pcb_bool pcb_chg_obj_size(int, void *, void *, void *, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_obj_1st_size(int, void *, void *, void *, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_obj_thermal(int, void *, void *, void *, int);
pcb_bool pcb_chg_obj_clear_size(int, void *, void *, void *, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_obj_2nd_size(int, void *, void *, void *, pcb_coord_t, pcb_bool, pcb_bool);
pcb_bool pcb_chg_obj_mask_size(int, void *, void *, void *, pcb_coord_t, pcb_bool);
pcb_bool pcb_chg_obj_join(int, void *, void *, void *);
pcb_bool pcb_set_obj_join(int, void *, void *, void *);
pcb_bool pcb_clr_obj_join(int, void *, void *, void *);
pcb_bool pcb_chg_obj_nonetlist(int Type, void *Ptr1, void *Ptr2, void *Ptr3);
pcb_bool pcb_chg_obj_square(int, void *, void *, void *, int);
pcb_bool pcb_set_obj_square(int, void *, void *, void *);
pcb_bool pcb_clr_obj_square(int, void *, void *, void *);
pcb_bool pcb_chg_obj_octagon(int, void *, void *, void *);
pcb_bool pcb_set_obj_octagon(int, void *, void *, void *);
pcb_bool pcb_clr_obj_octagon(int, void *, void *, void *);
void *pcb_chg_obj_name(int, void *, void *, void *, char *);
void *pcb_chg_obj_name_query(int, void *, void *, void *, int);
void *pcb_chg_obj_pinnum(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *Name);
pcb_bool pcb_chg_obj_radius(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_x, pcb_coord_t r, pcb_bool absolute);
pcb_bool pcb_chg_obj_angle(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_start, pcb_angle_t a, pcb_bool absolute);
pcb_bool pcb_chg_selected_angle(int types, int is_start, pcb_angle_t Difference, pcb_bool fixIt);
pcb_bool pcb_chg_selected_radius(int types, int is_start, pcb_angle_t Difference, pcb_bool fixIt);

#endif

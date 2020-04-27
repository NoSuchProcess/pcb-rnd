/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Generic, object-type independent change calls */

#ifndef	PCB_CHANGE_H
#define	PCB_CHANGE_H

#include "config.h"
#include "board.h"
#include "flag.h"
#include <librnd/core/pcb_bool.h>
#include <librnd/core/unit.h>

extern int defer_updates, defer_needs_update;

#define	PCB_CHANGENAME_TYPES        \
	(PCB_OBJ_PSTK | PCB_OBJ_TEXT | PCB_OBJ_SUBC | PCB_OBJ_LINE | \
	PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_SUBC_PART | PCB_OBJ_SUBC)

#define	PCB_CHANGESIZE_TYPES        \
	(PCB_OBJ_PSTK | PCB_OBJ_POLY | PCB_OBJ_LINE | PCB_OBJ_ARC | \
	PCB_OBJ_TEXT | PCB_OBJ_SUBC | PCB_OBJ_SUBC_PART)

#define	PCB_CHANGE2NDSIZE_TYPES     \
	(PCB_OBJ_PSTK | PCB_OBJ_SUBC | PCB_OBJ_SUBC_PART)

#define	PCB_CHANGEROT_TYPES     \
	(PCB_OBJ_PSTK | PCB_OBJ_TEXT | PCB_OBJ_GFX)

/* We include polygons here only to inform the user not to do it that way.  */
#define PCB_CHANGECLEARSIZE_TYPES \
	(PCB_OBJ_PSTK | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_SUBC | \
	PCB_OBJ_SUBC_PART)

#define	PCB_CHANGENONETLIST_TYPES     \
	(PCB_OBJ_SUBC)

#define	PCB_CHANGESQUARE_TYPES     \
	(PCB_OBJ_SUBC | PCB_OBJ_SUBC_PART)

#define	PCB_CHANGEOCTAGON_TYPES     \
	(PCB_OBJ_SUBC | PCB_OBJ_SUBC_PART)

#define PCB_CHANGEJOIN_TYPES	\
	(PCB_OBJ_ARC | PCB_OBJ_LINE | PCB_OBJ_TEXT | PCB_OBJ_POLY)

#define PCB_CHANGETHERMAL_TYPES	\
	(PCB_OBJ_SUBC_PART)

rnd_bool pcb_chg_selected_size(int, rnd_coord_t, rnd_bool);
rnd_bool pcb_chg_selected_clear_size(int, rnd_coord_t, rnd_bool);
rnd_bool pcb_chg_selected_2nd_size(int, rnd_coord_t, rnd_bool);
rnd_bool pcb_chg_selected_rot(int, double, rnd_bool);
rnd_bool pcb_chg_selected_join(int);
rnd_bool pcb_set_selected_join(int);
rnd_bool pcb_clr_selected_join(int);
rnd_bool pcb_chg_selected_nonetlist(int);
rnd_bool pcb_chg_selected_thermals(int types, int therm_style, unsigned long lid);
rnd_bool pcb_chg_obj_size(int, void *, void *, void *, rnd_coord_t, rnd_bool);
rnd_bool pcb_chg_obj_1st_size(int, void *, void *, void *, rnd_coord_t, rnd_bool);
rnd_bool pcb_chg_obj_thermal(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int therm_type, unsigned long lid);
rnd_bool pcb_chg_obj_clear_size(int, void *, void *, void *, rnd_coord_t, rnd_bool);
rnd_bool pcb_chg_obj_2nd_size(int, void *, void *, void *, rnd_coord_t, rnd_bool, rnd_bool);
rnd_bool pcb_chg_obj_rot(int, void *, void *, void *, double, rnd_bool, rnd_bool);
rnd_bool pcb_chg_obj_join(int, void *, void *, void *);
rnd_bool pcb_set_obj_join(int, void *, void *, void *);
rnd_bool pcb_clr_obj_join(int, void *, void *, void *);
rnd_bool pcb_chg_obj_nonetlist(int Type, void *Ptr1, void *Ptr2, void *Ptr3);
void *pcb_chg_obj_name(int, void *, void *, void *, char *);

/* queries the user for a new object name and changes it */
void *pcb_chg_obj_name_query(pcb_any_obj_t *obj);

rnd_bool pcb_chg_obj_radius(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_x, rnd_coord_t r, rnd_bool absolute);
rnd_bool pcb_chg_obj_angle(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_start, pcb_angle_t a, rnd_bool absolute);
rnd_bool pcb_chg_selected_angle(int types, int is_start, pcb_angle_t Difference, rnd_bool fixIt);
rnd_bool pcb_chg_selected_radius(int types, int is_start, pcb_angle_t Difference, rnd_bool fixIt);

/* Change flag flg of an object in a way dictated by 'how' */
void pcb_flag_change(pcb_board_t *pcb, pcb_change_flag_t how, pcb_flag_values_t flg, int Type, void *Ptr1, void *Ptr2, void *Ptr3);

/* Invalidate the term label of an object */
void *pcb_obj_invalidate_label(pcb_objtype_t Type, void *Ptr1, void *Ptr2, void *Ptr3);

/* Undoable obj attribute change; if new_value is NULL, delete the attribute */
int pcb_uchg_attr(pcb_board_t *pcb, pcb_any_obj_t *obj, const char *key, const char *new_value);


#endif

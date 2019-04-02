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

#ifndef	PCB_UNDO_H
#define	PCB_UNDO_H

#include <stdlib.h>
#include <libuundo/uundo.h>

/* Temporary for compatibility */
#include "undo_old.h"

typedef enum pcb_undo_ev_e {
	PCB_UNDO_EV_UNDO,
	PCB_UNDO_EV_REDO,
	PCB_UNDO_EV_CLEAR_LIST,
	PCB_UNDO_EV_TRUNCATE
} pcb_undo_ev_t;

void *pcb_undo_alloc(pcb_board_t *pcb, const uundo_oper_t *oper, size_t data_len);
int pcb_undo(pcb_bool);
int pcb_redo(pcb_bool);
int pcb_undo_above(uundo_serial_t s_min);

void pcb_undo_inc_serial(void);
void pcb_undo_save_serial(void);
void pcb_undo_restore_serial(void);
void pcb_undo_clear_list(pcb_bool);

void pcb_undo_lock(void);
void pcb_undo_unlock(void);
pcb_bool pcb_undoing(void);

uundo_serial_t pcb_undo_serial(void);
void pcb_undo_truncate_from(uundo_serial_t sfirst);

void pcb_undo_freeze_serial(void);
void pcb_undo_unfreeze_serial(void);
void pcb_undo_freeze_add(void);
void pcb_undo_unfreeze_add(void);

/* Return the number of undo slots in use */
size_t pcb_num_undo(void);

/* Returns 0 if undo integrity is not broken */
int undo_check(void);

void undo_dump(void);

/* temporary */
#include "pcb_bool.h"
extern pcb_data_t *pcb_removelist;
extern pcb_bool pcb_undo_and_draw;

#endif

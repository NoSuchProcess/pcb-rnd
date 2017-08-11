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

/* prototypes for undo routines */

#ifndef	PCB_UNDO_H
#define	PCB_UNDO_H

#include <stdlib.h>
#include <libuundo/uundo.h>

/* Temporary for compatibility */
#include "undo_old.h"

void *pcb_undo_alloc(pcb_board_t *pcb, const uundo_oper_t *oper, size_t data_len);
int pcb_undo(pcb_bool);
int pcb_redo(pcb_bool);
void pcb_undo_inc_serial(void);
void pcb_undo_save_serial(void);
void pcb_undo_restore_serial(void);
void pcb_undo_clear_list(pcb_bool);

void pcb_undo_lock(void);
void pcb_undo_unlock(void);
pcb_bool pcb_undoing(void);

/* Returns 0 if undo integrity is not broken */
int undo_check(void);

void undo_dump(void);

/* temporary */
#include "pcb_bool.h"
extern pcb_data_t *RemoveList;
extern pcb_bool pcb_undo_and_draw;

#endif

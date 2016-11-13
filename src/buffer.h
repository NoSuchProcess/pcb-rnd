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

/* prototypes for buffer handling routines */

#ifndef PCB_BUFFER_H
#define PCB_BUFFER_H

#include "obj_common.h"

struct pcb_buffer_s {								/* information about the paste buffer */
	pcb_coord_t X, Y;										/* offset */
	pcb_box_t BoundingBox;
	pcb_data_t *Data;							/* data; not all members of pcb_board_t */
	/* are used */
};

/* ---------------------------------------------------------------------------
 * prototypes
 */
void pcb_buffer_swap(pcb_buffer_t *Buffer);
void pcb_set_buffer_bbox(pcb_buffer_t *);
void pcb_buffer_clear(pcb_buffer_t *);
void pcb_buffer_add_selected(pcb_buffer_t *, pcb_coord_t, pcb_coord_t, pcb_bool);
pcb_bool pcb_buffer_load_layout(pcb_buffer_t *Buffer, const char *Filename, const char *fmt);
void pcb_buffer_rotate(pcb_buffer_t *, pcb_uint8_t);
void pcb_buffer_select_paste(int);
void pcb_swap_buffers(void);
void pcb_buffer_mirror(pcb_buffer_t *);
void pcb_init_buffers(void);
void pcb_uninit_buffers(void);
void *pcb_move_obj_to_buffer(pcb_data_t *, pcb_data_t *, int, void *, void *, void *);
void *pcb_copy_obj_to_buffer(pcb_data_t *, pcb_data_t *, int, void *, void *, void *);

/* This action is called from ActionElementAddIf() */
int pcb_load_footprint(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

/* pastes the contents of the buffer to the layout. Only visible objects
   are handled by the routine. */
pcb_bool pcb_buffer_copy_to_layout(pcb_coord_t X, pcb_coord_t Y);


pcb_data_t *pcb_buffer_new(void);


/* ---------------------------------------------------------------------------
 * access macro for current buffer
 */
#define	PCB_PASTEBUFFER (&Buffers[conf_core.editor.buffer_number])

#endif

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

/* Board independent paste buffers */

#ifndef PCB_BUFFER_H
#define PCB_BUFFER_H

#include "obj_common.h"
#include <libfungw/fungw.h>

struct pcb_buffer_s {     /* information about the paste buffer */
	pcb_coord_t X, Y;       /* offset */
	pcb_box_t BoundingBox;
	pcb_box_t bbox_naked;
	pcb_data_t *Data;       /* data; not all members of pcb_board_t */
	int from_outside;       /* data is coming from outside of the current board (lib, loaded board) */
};

/* Recalculates the bounding box of the buffer; returns 0 on success */
int pcb_set_buffer_bbox(pcb_buffer_t *);

/* clears the contents of the paste buffer: free's data, but preserves parent */
void pcb_buffer_clear(pcb_board_t *pcb, pcb_buffer_t *Buffer);

/* adds (copies) all selected and visible objects to the paste buffer */
void pcb_buffer_add_selected(pcb_board_t *pcb, pcb_buffer_t *, pcb_coord_t, pcb_coord_t, pcb_bool);

/* load a board into buffer parse the file with enabled 'PCB mode' */
pcb_bool pcb_buffer_load_layout(pcb_board_t *pcb, pcb_buffer_t *Buffer, const char *Filename, const char *fmt);

/* rotates the contents of the pastebuffer by Number * 90 degrees or free angle*/
void pcb_buffer_rotate90(pcb_buffer_t *Buffer, unsigned int Number);
void pcb_buffer_rotate(pcb_buffer_t *Buffer, pcb_angle_t angle);

/* flip elements/subcircuits/tracks from one side to the other; also swap
   the layer stackup of padstacks */
void pcb_buffers_flip_side(pcb_board_t *pcb);

/* graphically mirror the buffer on the current side */
void pcb_buffer_mirror(pcb_board_t *pcb, pcb_buffer_t *Buffer);

/* Scale all coords and sizes by sx;sy and thicknesses by sth;
   if recurse is non-zero, also scale subcircuits */
void pcb_buffer_scale(pcb_buffer_t *Buffer, double sx, double sy, double sth, int recurse);

/* Init/uninit all buffers (stored in board-independent global variables) */
void pcb_init_buffers(pcb_board_t *pcb);
void pcb_uninit_buffers(pcb_board_t *pcb);

/* Creates a new empty paste buffer */
pcb_data_t *pcb_buffer_new(pcb_board_t *pcb);

/* Moves the passed object to the passed buffer's data and removes it from its
   original data; returns the new object pointer on success or NULL on fail */
void *pcb_move_obj_to_buffer(pcb_board_t *pcb, pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3);

/* Adds/copies the passed object to the passed buffer's data;
   returns the new object pointer on success or NULL on fail */
void *pcb_copy_obj_to_buffer(pcb_board_t *pcb, pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3);


/* This action is called from ActionElementAddIf() */
fgw_error_t pcb_act_LoadFootprint(fgw_arg_t *res, int oargc, fgw_arg_t *oargv);

/* pastes the contents of the buffer to the layout. Only visible objects
   are handled by the routine. */
pcb_bool pcb_buffer_copy_to_layout(pcb_board_t *pcb, pcb_coord_t X, pcb_coord_t Y);

/* change the side of all objects in the buffer */
void pcb_buffer_flip_side(pcb_board_t *pcb, pcb_buffer_t *Buffer);

/* Load a footprint by name into a buffer; fmt is optional (may be NULL). */
pcb_bool pcb_buffer_load_footprint(pcb_buffer_t *Buffer, const char *Name, const char *fmt);

/* sets currently active buffer */
void pcb_buffer_set_number(int Number);

/* Address of the currently active paste buffer */
#define PCB_PASTEBUFFER (&pcb_buffers[conf_core.editor.buffer_number])

#endif

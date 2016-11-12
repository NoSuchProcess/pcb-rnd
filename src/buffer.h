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
	Coord X, Y;										/* offset */
	BoxType BoundingBox;
	pcb_data_t *Data;							/* data; not all members of pcb_board_t */
	/* are used */
};

/* ---------------------------------------------------------------------------
 * prototypes
 */
void SwapBuffer(pcb_buffer_t *Buffer);
void SetBufferBoundingBox(pcb_buffer_t *);
void ClearBuffer(pcb_buffer_t *);
void AddSelectedToBuffer(pcb_buffer_t *, Coord, Coord, pcb_bool);
pcb_bool LoadLayoutToBuffer(pcb_buffer_t *Buffer, const char *Filename, const char *fmt);
void RotateBuffer(pcb_buffer_t *, pcb_uint8_t);
void SelectPasteBuffer(int);
void pcb_swap_buffers(void);
void MirrorBuffer(pcb_buffer_t *);
void InitBuffers(void);
void UninitBuffers(void);
void *MoveObjectToBuffer(pcb_data_t *, pcb_data_t *, int, void *, void *, void *);
void *CopyObjectToBuffer(pcb_data_t *, pcb_data_t *, int, void *, void *, void *);

/* This action is called from ActionElementAddIf() */
int LoadFootprint(int argc, const char **argv, Coord x, Coord y);

pcb_data_t *CreateNewBuffer(void);


/* ---------------------------------------------------------------------------
 * access macro for current buffer
 */
#define	PASTEBUFFER		(&Buffers[conf_core.editor.buffer_number])

#endif

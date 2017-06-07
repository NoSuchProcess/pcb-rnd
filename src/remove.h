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

/* prototypes for remove routines */

#ifndef	PCB_REMOVE_H
#define	PCB_REMOVE_H

#include "config.h"

/* ---------------------------------------------------------------------------
 * some constants
 */
#define PCB_REMOVE_TYPES \
	(PCB_TYPE_VIA | PCB_TYPE_LINE_POINT | PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT | PCB_TYPE_SUBC | \
	PCB_TYPE_POLYGON_POINT | PCB_TYPE_POLYGON | PCB_TYPE_RATLINE | PCB_TYPE_ARC | PCB_TYPE_ARC_POINT)

pcb_bool pcb_remove_selected(void);
void *pcb_remove_object(int, void *, void *, void *);
void *pcb_destroy_object(pcb_data_t *, int, void *, void *, void *);

#endif

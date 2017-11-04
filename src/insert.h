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

/* prototypes for inserting points into objects */

#ifndef	PCB_INSERT_H
#define	PCB_INSERT_H

#include "config.h"

#define	PCB_INSERT_TYPES	(PCB_TYPE_POLY | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_RATLINE)

/* ---------------------------------------------------------------------------
 * prototypes
 */
void *pcb_insert_point_in_object(int, void *, void *, pcb_cardinal_t *, pcb_coord_t, pcb_coord_t, pcb_bool, pcb_bool);
pcb_point_t *pcb_adjust_insert_point(void);

#endif

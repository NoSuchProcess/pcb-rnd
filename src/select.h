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

#ifndef	PCB_SELECT_H
#define	PCB_SELECT_H

#include "config.h"
#include "operation.h"

#define PCB_SELECT_TYPES	\
	(PCB_OBJ_LINE | PCB_OBJ_TEXT | PCB_OBJ_POLY | PCB_OBJ_SUBC | \
	 PCB_OBJ_PSTK | PCB_OBJ_RAT | PCB_OBJ_ARC)

pcb_bool pcb_select_object(pcb_board_t *pcb);
pcb_bool pcb_select_block(pcb_board_t *pcb, pcb_box_t *Box, pcb_bool flag, pcb_bool vis_only);
long int *pcb_list_block(pcb_board_t *pcb, pcb_box_t *Box, int *len);

pcb_bool pcb_select_connection(pcb_board_t *pcb, pcb_bool);

#endif

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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
 */

#ifndef PCB_TOOL_LOGIC_H
#define PCB_TOOL_LOGIC_H

typedef enum { /* bitfield */
	PCB_TLF_RAT = 1   /* tool can be used on the rat layer */
} pcb_tool_user_flags_t;

void pcb_tool_logic_init(void);
void pcb_tool_logic_uninit(void);

void pcb_tool_attach_for_copy(pcb_hidlib_t *hl, pcb_coord_t PlaceX, pcb_coord_t PlaceY, pcb_bool do_rubberband);
void pcb_tool_notify_block(void); /* create first or second corner of a marked block (when clicked) */

extern pcb_bool pcb_tool_is_saved;

/**** old helpers ****/

/* does what's appropriate for the current mode setting (when clicked). This
   normally means creation of an object at the current crosshair location.
   new created objects are added to the create undo list of course */
void pcb_press_mode(pcb_hidlib_t *hidlib);

void pcb_release_mode(pcb_hidlib_t *hidlib);


#endif

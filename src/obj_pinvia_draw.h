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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/*** Standard draw on pins and vias ***/
void pcb_via_invalidate_erase(pcb_pin_t *Via);
void pcb_via_name_invalidate_erase(pcb_pin_t *Via);
void pcb_pin_invalidate_erase(pcb_pin_t *Pin);
void pcb_pin_name_invalidate_erase(pcb_pin_t *Pin);
void pcb_via_invalidate_draw(pcb_pin_t *Via);
void pcb_via_name_invalidate_draw(pcb_pin_t *Via);
void pcb_pin_invalidate_draw(pcb_pin_t *Pin);
void pcb_pin_name_invalidate_draw(pcb_pin_t *Pin);

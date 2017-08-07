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

/*** Standard draw on pins and vias ***/

/* Include rtree.h for these */
#ifdef PCB_RTREE_H
pcb_r_dir_t pcb_pin_draw_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t pcb_pin_name_draw_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t pcb_pin_clear_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t pcb_via_draw_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t pcb_hole_draw_callback(const pcb_box_t * b, void *cl);
#endif


void pcb_pin_draw(pcb_pin_t *pin, pcb_bool draw_hole);
void pcb_via_invalidate_erase(pcb_pin_t *Via);
void pcb_via_name_invalidate_erase(pcb_pin_t *Via);
void pcb_pin_invalidate_erase(pcb_pin_t *Pin);
void pcb_pin_name_invalidate_erase(pcb_pin_t *Pin);
void pcb_via_invalidate_draw(pcb_pin_t *Via);
void pcb_via_name_invalidate_draw(pcb_pin_t *Via);
void pcb_pin_invalidate_draw(pcb_pin_t *Pin);
void pcb_pin_name_invalidate_draw(pcb_pin_t *Pin);

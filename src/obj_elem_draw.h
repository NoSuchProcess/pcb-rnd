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

/*** Standard draw of elements ***/

/* Include rtree.h for these */
#ifdef PCB_RTREE_H
pcb_r_dir_t draw_element_name_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t draw_element_mark_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t draw_element_callback(const pcb_box_t * b, void *cl);
#endif

void draw_element_package(pcb_element_t * element);
void draw_element_name(pcb_element_t * element);
void draw_element_pins_and_pads(pcb_element_t * element);
void draw_element(pcb_element_t *element);

void EraseElement(pcb_element_t *Element);
void EraseElementPinsAndPads(pcb_element_t *Element);
void EraseElementName(pcb_element_t *Element);

void DrawElement(pcb_element_t *Element);
void DrawElementName(pcb_element_t *Element);
void DrawElementPackage(pcb_element_t *Element);
void DrawElementPinsAndPads(pcb_element_t *Element);

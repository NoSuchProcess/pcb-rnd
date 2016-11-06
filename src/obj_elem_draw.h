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
r_dir_t draw_element_name_callback(const BoxType * b, void *cl);
r_dir_t draw_element_mark_callback(const BoxType * b, void *cl);
r_dir_t draw_element_callback(const BoxType * b, void *cl);
#endif

void draw_element_package(ElementType * element);
void draw_element_name(ElementType * element);
void draw_element_pins_and_pads(ElementType * element);
void draw_element(ElementTypePtr element);

void EraseElement(ElementTypePtr Element);
void EraseElementPinsAndPads(ElementTypePtr Element);
void EraseElementName(ElementTypePtr Element);

void DrawElement(ElementTypePtr Element);
void DrawElementName(ElementTypePtr Element);
void DrawElementPackage(ElementTypePtr Element);
void DrawElementPinsAndPads(ElementTypePtr Element);

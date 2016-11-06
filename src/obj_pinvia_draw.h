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
r_dir_t draw_pin_callback(const BoxType * b, void *cl);
r_dir_t clear_pin_callback(const BoxType * b, void *cl);
r_dir_t draw_via_callback(const BoxType * b, void *cl);
r_dir_t draw_hole_callback(const BoxType * b, void *cl);
#endif


void draw_pin(PinTypePtr pin, pcb_bool draw_hole);
void EraseVia(PinTypePtr Via);
void EraseViaName(PinTypePtr Via);
void ErasePin(PinTypePtr Pin);
void ErasePinName(PinTypePtr Pin);
void DrawVia(PinTypePtr Via);
void DrawViaName(PinTypePtr Via);
void DrawPin(PinTypePtr Pin);
void DrawPinName(PinTypePtr Pin);

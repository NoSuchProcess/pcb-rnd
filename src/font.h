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

#ifndef PCB_FONT_H
#define PCB_FONT_H

#include "global_typedefs.h"
#include "box.h"

/* ---------------------------------------------------------------------------
 * symbol and font related stuff
 */
typedef struct symbol_st {     /* a single symbol */
	LineTypePtr Line;
	pcb_bool Valid;
	pcb_cardinal_t LineN;       /* number of lines */
	pcb_cardinal_t LineMax;     /* lines allocated */
	Coord Width, Height, Delta; /* size of cell, distance to next symbol */
} SymbolType, *SymbolTypePtr;

struct pcb_font_s {          /* complete set of symbols */
	Coord MaxHeight, MaxWidth; /* maximum cell width and height */
	BoxType DefaultSymbol;     /* the default symbol is a filled box */
	SymbolType Symbol[MAX_FONTPOSITION + 1];
	pcb_bool Valid;
};

void CreateDefaultFont(pcb_board_t *pcb);
void SetFontInfo(FontTypePtr Ptr);

LineTypePtr CreateNewLineInSymbol(SymbolTypePtr Symbol, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness);

#endif


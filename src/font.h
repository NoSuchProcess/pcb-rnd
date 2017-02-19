/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <genht/htip.h>
#include "global_typedefs.h"
#include "box.h"

/* ---------------------------------------------------------------------------
 * symbol and font related stuff
 */
typedef struct symbol_s {     /* a single symbol */
	pcb_line_t *Line;
	pcb_bool Valid;
	pcb_cardinal_t LineN;       /* number of lines */
	pcb_cardinal_t LineMax;     /* lines allocated */
	pcb_coord_t Width, Height, Delta; /* size of cell, distance to next symbol */
} pcb_symbol_t;

typedef long int pcb_font_id_t;      /* a font is referenced by a pcb_board_t:pcb_font_id_t pair */

struct pcb_font_s {          /* complete set of symbols */
	pcb_coord_t MaxHeight, MaxWidth; /* maximum cell width and height */
	pcb_box_t DefaultSymbol;     /* the default symbol is a filled box */
	pcb_symbol_t Symbol[PCB_MAX_FONTPOSITION + 1];
	char *name;
	pcb_font_id_t id;
};

struct pcb_fontkit_s {          /* a set of unrelated fonts */
	pcb_font_t dflt;              /* default, fallback font, also the sysfont */
	htip_t fonts;
	pcb_bool valid, hash_inited;
};

/* Look up font. If not found: Return NULL (fallback=0), or return the
   default font (fallback=1) */
pcb_font_t *pcb_font(pcb_board_t *pcb, pcb_font_id_t id, int fallback);

void pcb_font_create_default(pcb_board_t *pcb);
void pcb_font_set_info(pcb_font_t *Ptr);

pcb_line_t *pcb_font_new_line_in_sym(pcb_symbol_t *Symbol, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness);


/*** font kit handling ***/
void pcb_fontkit_free(pcb_fontkit_t *fk);
pcb_font_t *pcb_new_font(pcb_fontkit_t *fk, pcb_font_id_t id, const char *name);

#endif


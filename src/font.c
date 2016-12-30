/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

/* font support. Note: glyphs are called symbols here */

#include "config.h"

#include <string.h>

#include "font.h"
#include "board.h"
#include "conf_core.h"
#include "error.h"
#include "plug_io.h"
#include "paths.h"
#include "compat_nls.h"

#define STEP_SYMBOLLINE 10

typedef struct embf_line_s {
	int x1, y1, x2, y2, th;
} embf_line_t;

typedef struct embf_font_s {
	int delta;
	embf_line_t *lines;
	int num_lines;
} embf_font_t;

#include "font_internal.c"

static void pcb_font_load_internal(pcb_board_t *pcb)
{
	int n, l;
	memset(&pcb->Font, 0, sizeof(pcb->Font));
	pcb->Font.MaxWidth  = embf_maxx - embf_minx;
	pcb->Font.MaxHeight = embf_maxy - embf_miny;
	for(n = 0; n < sizeof(embf_font) / sizeof(embf_font[0]); n++) {
		if (embf_font[n].delta != 0) {
			pcb_symbol_t *s = pcb->Font.Symbol + n;
			embf_line_t *lines = embf_font[n].lines;

			for(l = 0; l < embf_font[n].num_lines; l++) {
				pcb_coord_t x1 = PCB_MIL_TO_COORD(lines[l].x1);
				pcb_coord_t y1 = PCB_MIL_TO_COORD(lines[l].y1);
				pcb_coord_t x2 = PCB_MIL_TO_COORD(lines[l].x2);
				pcb_coord_t y2 = PCB_MIL_TO_COORD(lines[l].y2);
				pcb_coord_t th = PCB_MIL_TO_COORD(lines[l].th);
				pcb_font_new_line_in_sym(s, x1, y1, x2, y2, th);
			}

			s->Valid = 1;
			s->Delta = PCB_MIL_TO_COORD(embf_font[n].delta);
		}
	}
	pcb_font_set_info(&pcb->Font);
}

/* parses a file with font information and installs it into the provided PCB
 * checks directories given as colon separated list by resource fontPath
 * if the fonts filename doesn't contain a directory component */
void pcb_font_create_default(pcb_board_t *pcb)
{
	int res = -1;
	pcb_io_err_inhibit_inc();
	conf_list_foreach_path_first(res, &conf_core.rc.default_font_file, pcb_parse_font(&pcb->Font, __path__));
	pcb_io_err_inhibit_dec();

	if (res != 0) {
		const char *s;
		gds_t buff;
		s = conf_concat_strlist(&conf_core.rc.default_font_file, &buff, NULL, ':');
		pcb_message(PCB_MSG_WARNING, _("Can't find font-symbol-file. Searched: '%s'; falling back to the embedded default font\n"), s);
		pcb_font_load_internal(pcb);
		gds_uninit(&buff);
	}
}

/* transforms symbol coordinates so that the left edge of each symbol
 * is at the zero position. The y coordinates are moved so that min(y) = 0 */
void pcb_font_set_info(pcb_font_t *Ptr)
{
	pcb_cardinal_t i, j;
	pcb_symbol_t *symbol;
	pcb_line_t *line;
	pcb_coord_t totalminy = PCB_MAX_COORD;

	/* calculate cell with and height (is at least PCB_DEFAULT_CELLSIZE)
	 * maximum cell width and height
	 * minimum x and y position of all lines
	 */
	Ptr->MaxWidth = PCB_DEFAULT_CELLSIZE;
	Ptr->MaxHeight = PCB_DEFAULT_CELLSIZE;
	for (i = 0, symbol = Ptr->Symbol; i <= PCB_MAX_FONTPOSITION; i++, symbol++) {
		pcb_coord_t minx, miny, maxx, maxy;

		/* next one if the index isn't used or symbol is empty (SPACE) */
		if (!symbol->Valid || !symbol->LineN)
			continue;

		minx = miny = PCB_MAX_COORD;
		maxx = maxy = 0;
		for (line = symbol->Line, j = symbol->LineN; j; j--, line++) {
			minx = MIN(minx, line->Point1.X);
			miny = MIN(miny, line->Point1.Y);
			minx = MIN(minx, line->Point2.X);
			miny = MIN(miny, line->Point2.Y);
			maxx = MAX(maxx, line->Point1.X);
			maxy = MAX(maxy, line->Point1.Y);
			maxx = MAX(maxx, line->Point2.X);
			maxy = MAX(maxy, line->Point2.Y);
		}

		/* move symbol to left edge */
		for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
			pcb_line_move(line, -minx, 0);

		/* set symbol bounding box with a minimum cell size of (1,1) */
		symbol->Width = maxx - minx + 1;
		symbol->Height = maxy + 1;

		/* check total min/max  */
		Ptr->MaxWidth = MAX(Ptr->MaxWidth, symbol->Width);
		Ptr->MaxHeight = MAX(Ptr->MaxHeight, symbol->Height);
		totalminy = MIN(totalminy, miny);
	}

	/* move coordinate system to the upper edge (lowest y on screen) */
	for (i = 0, symbol = Ptr->Symbol; i <= PCB_MAX_FONTPOSITION; i++, symbol++)
		if (symbol->Valid) {
			symbol->Height -= totalminy;
			for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
				pcb_line_move(line, 0, -totalminy);
		}

	/* setup the box for the default symbol */
	Ptr->DefaultSymbol.X1 = Ptr->DefaultSymbol.Y1 = 0;
	Ptr->DefaultSymbol.X2 = Ptr->DefaultSymbol.X1 + Ptr->MaxWidth;
	Ptr->DefaultSymbol.Y2 = Ptr->DefaultSymbol.Y1 + Ptr->MaxHeight;
}

/* creates a new line in a symbol */
pcb_line_t *pcb_font_new_line_in_sym(pcb_symbol_t *Symbol, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness)
{
	pcb_line_t *line = Symbol->Line;

	/* realloc new memory if necessary and clear it */
	if (Symbol->LineN >= Symbol->LineMax) {
		Symbol->LineMax += STEP_SYMBOLLINE;
		line = (pcb_line_t *) realloc(line, Symbol->LineMax * sizeof(pcb_line_t));
		Symbol->Line = line;
		memset(line + Symbol->LineN, 0, STEP_SYMBOLLINE * sizeof(pcb_line_t));
	}

	/* copy values */
	line = line + Symbol->LineN++;
	line->Point1.X = X1;
	line->Point1.Y = Y1;
	line->Point2.X = X2;
	line->Point2.Y = Y2;
	line->Thickness = Thickness;
	return (line);
}


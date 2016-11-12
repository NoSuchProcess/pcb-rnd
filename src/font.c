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

/* parses a file with font information and installs it into the provided PCB
 * checks directories given as colon separated list by resource fontPath
 * if the fonts filename doesn't contain a directory component */
void CreateDefaultFont(pcb_board_t *pcb)
{
	int res = -1;
	pcb_io_err_inhibit_inc();
	conf_list_foreach_path_first(res, &conf_core.rc.default_font_file, ParseFont(&pcb->Font, __path__));
	pcb_io_err_inhibit_dec();

	if (res != 0) {
		const char *s;
		gds_t buff;
		s = conf_concat_strlist(&conf_core.rc.default_font_file, &buff, NULL, ':');
		Message(PCB_MSG_ERROR, _("Can't find font-symbol-file - there won't be font in this design. Searched: '%s'\n"), s);
		gds_uninit(&buff);
	}
}

/* transforms symbol coordinates so that the left edge of each symbol
 * is at the zero position. The y coordinates are moved so that min(y) = 0 */
void SetFontInfo(FontTypePtr Ptr)
{
	pcb_cardinal_t i, j;
	SymbolTypePtr symbol;
	LineTypePtr line;
	Coord totalminy = MAX_COORD;

	/* calculate cell with and height (is at least DEFAULT_CELLSIZE)
	 * maximum cell width and height
	 * minimum x and y position of all lines
	 */
	Ptr->MaxWidth = DEFAULT_CELLSIZE;
	Ptr->MaxHeight = DEFAULT_CELLSIZE;
	for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++) {
		Coord minx, miny, maxx, maxy;

		/* next one if the index isn't used or symbol is empty (SPACE) */
		if (!symbol->Valid || !symbol->LineN)
			continue;

		minx = miny = MAX_COORD;
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
			MOVE_LINE_LOWLEVEL(line, -minx, 0);

		/* set symbol bounding box with a minimum cell size of (1,1) */
		symbol->Width = maxx - minx + 1;
		symbol->Height = maxy + 1;

		/* check total min/max  */
		Ptr->MaxWidth = MAX(Ptr->MaxWidth, symbol->Width);
		Ptr->MaxHeight = MAX(Ptr->MaxHeight, symbol->Height);
		totalminy = MIN(totalminy, miny);
	}

	/* move coordinate system to the upper edge (lowest y on screen) */
	for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
		if (symbol->Valid) {
			symbol->Height -= totalminy;
			for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
				MOVE_LINE_LOWLEVEL(line, 0, -totalminy);
		}

	/* setup the box for the default symbol */
	Ptr->DefaultSymbol.X1 = Ptr->DefaultSymbol.Y1 = 0;
	Ptr->DefaultSymbol.X2 = Ptr->DefaultSymbol.X1 + Ptr->MaxWidth;
	Ptr->DefaultSymbol.Y2 = Ptr->DefaultSymbol.Y1 + Ptr->MaxHeight;
}

/* creates a new line in a symbol */
LineTypePtr CreateNewLineInSymbol(SymbolTypePtr Symbol, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness)
{
	LineTypePtr line = Symbol->Line;

	/* realloc new memory if necessary and clear it */
	if (Symbol->LineN >= Symbol->LineMax) {
		Symbol->LineMax += STEP_SYMBOLLINE;
		line = (LineTypePtr) realloc(line, Symbol->LineMax * sizeof(LineType));
		Symbol->Line = line;
		memset(line + Symbol->LineN, 0, STEP_SYMBOLLINE * sizeof(LineType));
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


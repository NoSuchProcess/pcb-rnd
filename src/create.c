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

/* functions used to create vias, pins ... */

#include "config.h"

#include <setjmp.h>

#include "conf_core.h"

#include "board.h"
#include "math_helper.h"
#include "create.h"
#include "data.h"
#include "misc.h"
#include "rtree.h"
#include "search.h"
#include "undo.h"
#include "plug_io.h"
#include "stub_vendor.h"
#include "hid_actions.h"
#include "paths.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_all.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */

/* current object ID; incremented after each creation of an object */
long int ID = 1;

pcb_bool pcb_create_be_lenient = pcb_false;

/* ---------------------------------------------------------------------------
 *  Set the lenience mode.
 */
void CreateBeLenient(pcb_bool v)
{
	pcb_create_be_lenient = v;
}

/* ---------------------------------------------------------------------------
 * creates a new line in a symbol
 */
#define STEP_SYMBOLLINE 10
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

/* ---------------------------------------------------------------------------
 * parses a file with font information and installs it into the provided PCB
 * checks directories given as colon separated list by resource fontPath
 * if the fonts filename doesn't contain a directory component
 */
void CreateDefaultFont(PCBTypePtr pcb)
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

void CreateIDBump(int min_id)
{
	if (ID < min_id)
		ID = min_id;
}

void CreateIDReset(void)
{
	ID = 1;
}

long int CreateIDGet(void)
{
	return ID++;
}

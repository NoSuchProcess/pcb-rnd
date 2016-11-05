/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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

/* misc functions used by several modules */

#include "config.h"
#include "conf_core.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <signal.h>

#include "board.h"
#include "box.h"
#include "crosshair.h"
#include "data.h"
#include "plug_io.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "rtree.h"
#include "rotate.h"
#include "rubberband.h"
#include "set.h"
#include "undo.h"
#include "compat_misc.h"
#include "obj_all.h"

/* ---------------------------------------------------------------------------
 * creates a filename from a template
 * %f is replaced by the filename
 * %p by the searchpath
 */
#warning TODO: kill this in favor of pcb_strdup_subst
char *EvaluateFilename(const char *Template, const char *Path, const char *Filename, const char *Parameter)
{
	gds_t command;
	const char *p;

	if (conf_core.rc.verbose) {
		printf("EvaluateFilename:\n");
		printf("\tTemplate: \033[33m%s\033[0m\n", Template);
		printf("\tPath: \033[33m%s\033[0m\n", Path);
		printf("\tFilename: \033[33m%s\033[0m\n", Filename);
		printf("\tParameter: \033[33m%s\033[0m\n", Parameter);
	}

	gds_init(&command);

	for (p = Template; p && *p; p++) {
		/* copy character or add string to command */
		if (*p == '%' && (*(p + 1) == 'f' || *(p + 1) == 'p' || *(p + 1) == 'a'))
			switch (*(++p)) {
			case 'a':
				gds_append_str(&command, Parameter);
				break;
			case 'f':
				gds_append_str(&command, Filename);
				break;
			case 'p':
				gds_append_str(&command, Path);
				break;
			}
		else
			gds_append(&command, *p);
	}

	if (conf_core.rc.verbose)
		printf("EvaluateFilename: \033[32m%s\033[0m\n", command.array);

	return command.array;
}


/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  MUCS unixplot import HID
 *  Copyright (C) 2017 Erich Heinzle 
 *  Based on up2pcb.cc, a simple unixplot file to pcb syntax converter
 *  Copyright (C) 2001 Luis Claudio Gamb�a Lopes
 *  And loosely based on dsn.c
 *  Copyright (C) 2008, 2011 Josh Jordan, Dan McMahill, and Jared Casper
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
 */

/*
This program imports unixplot format line and via data into a
geda .pcb
*/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "rats.h"
#include "buffer.h"
#include "change.h"
#include "draw.h"
#include "undo.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_pinvia.h"
#include "obj_rat.h"

#include "action_helper.h"
#include "hid.h"
#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "hid_actions.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "plugins.h"

static const char *mucs_cookie = "mucs importer";

/* mucs name */
#define FREE(x) if((x) != NULL) { free (x); (x) = NULL; }

static const char load_mucs_syntax[] = "LoadMucsFrom(filename)";

static const char load_mucs_help[] = "Loads the specified mucs routing file.";

int pcb_act_LoadMucsFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;
	char str[200];
	FILE *fi;
	char c;

	int ret;
	pcb_coord_t dim1, dim2, x0 = 0, y0 = 0, x1, y1, x2, y2, r;
	pcb_coord_t linethick = 0, lineclear, viadiam, viadrill;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect(_("Load mucs routing session Resource File..."),
														_("Picks a mucs session resource file to load.\n"
															"This file could be generated by mucs-pcb\n"),
														default_file, ".pl", "unixplot", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_AFAIL(load_mucs);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}

		if (fname && *fname)
			default_file = pcb_strdup(fname);
	}

	lineclear = PCB->RouteStyle.array[0].Clearance * 2;
	fi = fopen(fname, "r");
	if (!fi) {
		pcb_message(PCB_MSG_ERROR, "Can't load mucs unixplot file %s for read\n", fname);
		return 1;
	}

	while ((c = getc(fi)) != EOF)
	{ 
	pcb_printf("Char: %d \n", c); 
	switch(c)
	    {
	    case 's':
	      x1 = 100*(getc (fi) + (getc (fi) * 256));
	      y1 = 100*(getc (fi) + (getc (fi) * 256));
	      x2 = 100*(getc (fi) + (getc (fi) * 256));
	      y2 = 100*(getc (fi) + (getc (fi) * 256));

	      pcb_printf ("s--%i %i %i %i ???\n", x1, y1, x2, y2);
	      break;
	    case 'l':
	      x1 = 100*(getc (fi) + (getc (fi) * 256));
	      y1 = 100*(getc (fi) + (getc (fi) * 256));
	      x2 = 100*(getc (fi) + (getc (fi) * 256));
	      y2 = 100*(getc (fi) + (getc (fi) * 256));
	      pcb_printf ("Line[%d %d %d %d 2000 ""]\n",x1,y1,x2,y2);
	      break;
	    case 'c':
	      x1 = 100*(getc (fi) + (getc (fi) * 256));
	      y1 = 100*(getc (fi) + (getc (fi) * 256));
	      r = 100*(getc (fi) + (getc (fi) * 256));
	      pcb_printf ("Via[%d %d 6000 2500 \"\" ""]\n",x1, y1);
	      break;
	    case 'n':
	      x1 = 100*(getc (fi) + (getc (fi) * 256));
	      y1 = 100*(getc (fi) + (getc (fi) * 256));
	      pcb_printf ("Line[%d %d %d %d 2000 ""]\n",x1,y1,x2,y2);
	      x2=x1;
	      y2=y1;
	      break;
	    case 'a':
	      x1 = 100*((getc (fi) * 256) + getc (fi));
	      y1 = 100*((getc (fi) * 256) + getc (fi));
	      x2 = 100*((getc (fi) * 256) + getc (fi));
	      y2 = 100*((getc (fi) * 256) + getc (fi));
	      r = 100*((getc (fi) * 256) + getc (fi));
	      pcb_printf
		("a--stroke newpath\n%d %d %d %d %d arc\n",
		 x1, y1, x2, y2, r);
	      break;
	    case 'e':
	      break;
	    case 't':
	      while (r != '\0' && r != EOF)
		r = getc (fi);
	      break;
	}
	}
	fclose(fi);
	return 0;
}

pcb_hid_action_t mucs_action_list[] = {
	{"LoadMucsFrom", 0, pcb_act_LoadMucsFrom, load_mucs_help, load_mucs_syntax}
};

PCB_REGISTER_ACTIONS(mucs_action_list, mucs_cookie)

static void hid_mucs_uninit()
{
	pcb_hid_remove_actions_by_cookie(mucs_cookie);
	conf_unreg_fields("plugins/import_mucs/");
}

#include "dolists.h"
pcb_uninit_t hid_import_mucs_init()
{
	PCB_REGISTER_ACTIONS(mucs_action_list, mucs_cookie)
	return hid_mucs_uninit;
}


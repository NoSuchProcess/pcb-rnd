/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"
#include "global.h"
#include "autoroute.h"
#include "plugins.h"
#include "set.h"

/* action routines for the autorouter
 */

static const char autoroute_syntax[] = "AutoRoute(AllRats|SelectedRats)";

static const char autoroute_help[] = "Auto-route some or all rat lines.";

/* %start-doc actions AutoRoute

@table @code

@item AllRats
Attempt to autoroute all rats.

@item SelectedRats
Attempt to autoroute the selected rats.

@end table

Before autorouting, it's important to set up a few things.  First,
make sure any layers you aren't using are disabled, else the
autorouter may use them.  Next, make sure the current line and via
styles are set accordingly.  Last, make sure "new lines clear
polygons" is set, in case you eventually want to add a copper pour.

Autorouting takes a while.  During this time, the program may not be
responsive.

%end-doc */

#warning TODO: make this central
#define ARG(n) (argc > (n) ? argv[n] : NULL)

static int ActionAutoRoute(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	hid_action("Busy");
	if (function) {								/* one parameter */
		if (strcmp(function, "AllRats") == 0) {
			if (AutoRoute(false))
				SetChangedFlag(true);
		}
		else if ((strcmp(function, "SelectedRats") == 0) || (strcmp(function, "Selected") == 0)) {
			if (AutoRoute(true))
				SetChangedFlag(true);
		}
	}
	return 0;
}

static const char *autoroute_cookie = "autoroute plugin";

HID_Action autoroute_action_list[] = {
	{"AutoRoute", 0, ActionAutoRoute,
	 autoroute_help, autoroute_syntax}
	,
};

REGISTER_ACTIONS(autoroute_action_list, autoroute_cookie)

#include "dolists.h"
pcb_uninit_t hid_autoroute_init(void)
{
	REGISTER_ACTIONS(autoroute_action_list, autoroute_cookie)
	return NULL;
}

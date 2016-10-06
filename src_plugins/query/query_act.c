/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* Query language - actions */

#include "global.h"
#include "query.h"
#include "query_y.h"

static const char query_action_syntax[] =
	"query(dump, expr) - dry run: compile and dump an expression\n"
	;
static const char query_action_help[] = "Perform various queries on PCB data.";

static int query_action(int argc, const char **argv, Coord x, Coord y)
{
	const char *cmd = argc > 0 ? argv[0] : 0;

	if (cmd == NULL) {
		return -1;
	}

	if (strcmp(cmd, "dump") == 0) {
		printf("Script: '%s'\n", argv[1]);
		pcb_qry_set_input(argv[1]);
		qry_parse();
		return 0;
	}

	return -1;
}

HID_Action query_action_list[] = {
	{"query", NULL, query_action,
	 query_action_help, query_action_syntax}
};

REGISTER_ACTIONS(query_action_list, NULL)

#include "dolists.h"
void query_action_reg(const char *cookie)
{
	REGISTER_ACTIONS(query_action_list, cookie)
}

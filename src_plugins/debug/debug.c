/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include "config.h"
#include "global.h"
#include "data.h"
#include "debug_conf.h"
#include "action_helper.h"
#include "plugins.h"
#include "conf.h"
#include "error.h"

static const char conf_syntax[] =
	"dumpconf(native, [verbose], [prefix]) - dump the native (binary) config tree to stdout\n"
	"dumpconf(lihata, role, [prefix]) - dump in-memory lihata representation of a config tree\n"
	;

static const char conf_help[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_root[];
static int ActionDumpConf(int argc, char **argv, Coord x, Coord y)
{
	char *cmd = argc > 0 ? argv[0] : 0;

	if (NSTRCMP(cmd, "native") == 0) {
		int verbose;
		const char *prefix = "";
		if (argc > 1)
			verbose = atoi(argv[1]);
		if (argc > 2)
			prefix = argv[2];
		conf_dump(stdout, prefix, verbose);
	}

	else if (NSTRCMP(cmd, "lihata") == 0) {
		conf_role_t role;
		const char *prefix = "";
		if (argc <= 1) {
			Message("conf(dumplht) needs a role");
			return 1;
		}
		role = conf_role_parse(argv[1]);
		if (role == CFR_invalid) {
			Message("Invalid role: '%s'", argv[3]);
			return 1;
		}
		if (argc > 2)
			prefix = argv[2];
		if (conf_root[role] != NULL)
			lht_dom_export(conf_root[role]->root, stdout, prefix);
		else
			printf("%s <empty>\n", prefix);
	}

	else {
		Message("Invalid conf command '%s'\n", argv[0]);
		return 1;
	}
	return 0;
}


HID_Action debug_action_list[] = {
	{"dumpconf", 0, ActionDumpConf,
	 conf_help, conf_syntax}
};

static const char *debug_cookie = "debug plugin";

REGISTER_ACTIONS(debug_action_list, debug_cookie)

static void hid_debug_uninit(void)
{
	hid_remove_actions_by_cookie(debug_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_debug_init(void)
{
	REGISTER_ACTIONS(debug_action_list, debug_cookie)
	return hid_debug_uninit;
}


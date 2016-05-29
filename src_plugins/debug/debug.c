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
#include "data.h"
#include "debug_conf.h"
#include "action_helper.h"
#include "plugins.h"
#include "conf.h"
#include "error.h"

static const char conf_syntax[] =
	"conf(dump, [verbose], [prefix]) - dump the current config tree to stdout\n"
	"conf(dumplht, role, [prefix]) - dump in-memory lihata representation of a config tree\n"
	"conf(set,  path, value, [role], [policy]) - change a config setting\n"
	;

static const char conf_help[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_root[];
static int ActionConf(int argc, char **argv, Coord x, Coord y)
{
	char *cmd = argc > 0 ? argv[0] : 0;

	if (NSTRCMP(cmd, "dump") == 0) {
		int verbose;
		const char *prefix = "";
		if (argc > 1)
			verbose = atoi(argv[1]);
		if (argc > 2)
			prefix = argv[2];
		conf_dump(stdout, prefix, verbose);
	}

	else if (NSTRCMP(cmd, "dumplht") == 0) {
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

	else if (NSTRCMP(cmd, "set") == 0) {
		char *path, *val;
		conf_policy_t pol = POL_OVERWRITE;
		conf_role_t role = CFR_PROJECT;

		if (argc < 2) {
			Message("conf(set) needs at least two arguments");
			return 1;
		}
		if (argc > 2) {
			role = conf_role_parse(argv[3]);
			if (role == CFR_invalid) {
				Message("Invalid role: '%s'", argv[3]);
				return 1;
			}
		}
		if (argc > 3) {
			pol = conf_policy_parse(argv[4]);
			if (pol == POL_invalid) {
				Message("Invalid policy: '%s'", argv[4]);
				return 1;
			}
		}
		path = argv[1];
		val = argv[2];
		if (conf_set(role, path, -1, val, pol) != 0) {
			Message("conf(set) failed.\n");
			return 1;
		}
		conf_update();
	}

	else {
		Message("Invalid conf command '%s'\n", argv[0]);
		return 1;
	}
	return 0;
}


HID_Action debug_action_list[] = {
	{"conf", 0, ActionConf,
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


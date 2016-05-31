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
	"conf(dump, [verbose], [prefix]) - dump the current config tree to stdout\n"
	"conf(dumplht, role, [prefix]) - dump in-memory lihata representation of a config tree\n"
	"conf(set, path, value, [role], [policy]) - change a config setting\n"
	"conf(toggle, path, [role]) - invert boolean value of a flag; if no role given, overwrite the highest prio config\n"
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
		conf_role_t role = CFR_invalid;
		int res;

		if (argc < 3) {
			Message("conf(set) needs at least two arguments");
			return 1;
		}
		if (argc > 3) {
			role = conf_role_parse(argv[3]);
			if (role == CFR_invalid) {
				Message("Invalid role: '%s'", argv[3]);
				return 1;
			}
		}
		if (argc > 4) {
			pol = conf_policy_parse(argv[4]);
			if (pol == POL_invalid) {
				Message("Invalid policy: '%s'", argv[4]);
				return 1;
			}
		}
		path = argv[1];
		val = argv[2];


		if (role == CFR_invalid) {
			conf_native_t *n = conf_get_field(argv[1]);
			if (n == NULL) {
				Message("Invalid conf field '%s': no such path\n", argv[1]);
				return 1;
			}
			res = conf_set_native(n, 0, val);
		}
		else
			res = conf_set(role, path, -1, val, pol);
		if (res != 0) {
			Message("conf(set) failed.\n");
			return 1;
		}
		conf_update();
	}

	else if (NSTRCMP(cmd, "toggle") == 0) {
		conf_native_t *n = conf_get_field(argv[1]);
		char *new_value;
		conf_role_t role = CFR_invalid;
		int res;

		if (n == NULL) {
			Message("Invalid conf field '%s': no such path\n", argv[1]);
			return 1;
		}
		if (n->type != CFN_BOOLEAN) {
			Message("Can not toggle '%s': not a boolean\n", argv[1]);
			return 1;
		}
		if (n->used != 1) {
			Message("Can not toggle '%s': array size should be 1, not %d\n", argv[1], n->used);
			return 1;
		}
		if (argc > 2) {
			role = conf_role_parse(argv[2]);
			if (role == CFR_invalid) {
				Message("Invalid role: '%s'", argv[2]);
				return 1;
			}
		}
		if (n->val.boolean[0])
			new_value = "false";
		else
			new_value = "true";
		if (role == CFR_invalid)
			res = conf_set_native(n, 0, new_value);
		else
			res = conf_set(role, argv[1], -1, new_value, POL_OVERWRITE);
		
		if (res != 0) {
			Message("Can not toggle '%s': failed to set new value\n", argv[1]);
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


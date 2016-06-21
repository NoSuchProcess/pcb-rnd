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

#include "global.h"
#include "data.h"
#include "config.h"
#include "conf.h"
#include "conf_core.h"
#include "error.h"
#include "misc.h"
#include "misc_util.h"

static const char conf_syntax[] =
	"conf(set, path, value, [role], [policy]) - change a config setting\n"
	"conf(toggle, path, [role]) - invert boolean value of a flag; if no role given, overwrite the highest prio config\n"
	"conf(reset, role) - reset the in-memory lihata of a role\n"
	;

static const char conf_help[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_root[];
static int ActionConf(int argc, char **argv, Coord x, Coord y)
{
	char *cmd = argc > 0 ? argv[0] : 0;

	if (NSTRCMP(cmd, "set") == 0) {
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
		conf_update(argv[1]);
	}

	else if (NSTRCMP(cmd, "reset") == 0) {
		conf_role_t role;
		int res;
		role = conf_role_parse(argv[1]);
		if (role == CFR_invalid) {
			Message("Invalid role: '%s'", argv[1]);
			return 1;
		}
		conf_reset(role, "<action>");
		conf_update(argv[1]);
	}

	else {
		Message("Invalid conf command '%s'\n", argv[0]);
		return 1;
	}
	return 0;
}

/*------------ get/chk (check flag actions for menus) ------------------*/
static const char GetStyle_syntax[] = "GetStyle()" ;
static const char GetStyle_help[] = "Return integer index (>=1) of the currently active style or 0 if no style is selected";
static int ActionGetStyle(int argc, char **argv, Coord x, Coord y)
{
	STYLE_LOOP(PCB);
	{
		if (style->Thick == conf_core.design.line_thickness &&
				style->Diameter == conf_core.design.via_thickness &&
				style->Hole == conf_core.design.via_drilling_hole && style->Clearance == conf_core.design.clearance)
			return n + 1;
	}
	END_LOOP;
	return 0;
}

static const char ChkMode_syntax[] = "ChkMode(expected_mode)" ;
static const char ChkMode_help[] = "Return 1 if the currently selected mode is the expected_mode";
static int ActionChkMode(int argc, char **argv, Coord x, Coord y)
{
#warning TODO: convert this to a compile-time hash
	struct {
		char *name;
		int mode;
	} *m, modes[] = {
		{"none", NO_MODE},
		{"arc", ARC_MODE},
		{"arrow", ARROW_MODE},
		{"copy", COPY_MODE},
		{"insertpoint", INSERTPOINT_MODE},
		{"line", LINE_MODE},
		{"lock", LOCK_MODE},
		{"move", MOVE_MODE},
		{"pastebuffer", PASTEBUFFER_MODE},
		{"polygon", POLYGON_MODE},
		{"polygonhole", POLYGONHOLE_MODE},
		{"rectangle", RECTANGLE_MODE},
		{"remove", REMOVE_MODE},
		{"rotate", ROTATE_MODE},
		{"rubberbandmove", RUBBERBANDMOVE_MODE},
		{"text", TEXT_MODE},
		{"thermal", THERMAL_MODE},
		{"via", VIA_MODE},
		{NULL, 0}
	};
	assert(argc == 1);
	for(m = modes; m->name != NULL; m++) {
		if (strcmp(m->name, argv[0]) == 0)
			return conf_core.editor.mode == m->mode;
	}
	Message("Unknown mode in ChkMode(): %s\n", argv[1]);
	abort();
	return -1;
}


static const char ChkGridSize_syntax[] = 
	"ChkGridSize(expected_size)\n"
	"ChkGridSize(none)\n"
	;
static const char ChkGridSize_help[] = "Return 1 if the currently selected grid matches the expected_size. If argument is \"none\" return 1 if there is no grid.";
static int ActionChkGridSize(int argc, char **argv, Coord x, Coord y)
{
	assert(argc == 1);
	if (strcmp(argv[0], "none") == 0)
		return PCB->Grid <= 300;

	return (PCB->Grid == GetValueEx(argv[0], NULL, NULL, NULL, NULL, NULL));
}

static const char ChkElementName_syntax[] = 
	"ChkElementName(1) - expect description\n"
	"ChkElementName(2) - expect refdes\n"
	"ChkElementName(3) - expect value\n"
	;
static const char ChkElementName_help[] = "Return 1 if currently shown element label (name) type matches the expected";
static int ActionChkElementName(int argc, char **argv, Coord x, Coord y)
{
	int have, expected = argv[0][0] - '0';

	assert(argc == 1);
	if (conf_core.editor.description) have = 1;
	else if (conf_core.editor.name_on_pcb) have = 2;
	else have = 3;

	return expected == have;
}

static const char ChkGridUnits_syntax[] = "ChkGridUnits(expected)";
static const char ChkGridUnits_help[] = "Return 1 if currently selected grid unit matches the expected (normally mm or mil)";
static int ActionChkGridUnits(int argc, char **argv, Coord x, Coord y)
{
	assert(argc == 1);
	return strcmp(conf_core.editor.grid_unit->suffix, argv[0]) == 0;
}

static const char ChkBuffer_syntax[] = "ChkBuffer(idx)";
static const char ChkBuffer_help[] = "Return 1 if currently selected buffer's index matches idx";
static int ActionChkBuffer(int argc, char **argv, Coord x, Coord y)
{
	int expected = argv[0][0] - '0';
	assert(argc == 1);

	return (conf_core.editor.buffer_number + 1) == expected;
}

HID_Action conf_action_list[] = {
	{"conf", 0, ActionConf,
	 conf_help, conf_syntax}
	,
	{"GetStyle", 0, ActionGetStyle,
	 GetStyle_help, GetStyle_syntax}
	,
	{"ChkMode", 0, ActionChkMode,
	 ChkMode_help, ChkMode_syntax}
	,
	{"ChkGridSize", 0, ActionChkGridSize,
	 ChkGridSize_help, ChkGridSize_syntax}
	,
	{"ChkElementName", 0, ActionChkElementName,
	 ChkElementName_help, ChkElementName_syntax}
	,
	{"ChkGridUnits", 0, ActionChkGridUnits,
	 ChkGridUnits_help, ChkGridUnits_syntax}
	,
	{"ChkBuffer", 0, ActionChkBuffer,
	 ChkBuffer_help, ChkBuffer_syntax}
};

REGISTER_ACTIONS(conf_action_list, NULL)

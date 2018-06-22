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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "board.h"
#include "actions.h"
#include "conf_core.h"
#include "route_style.h"
#include "error.h"
#include "tool.h"
#include "macro.h"

static const char pcb_acts_Conf[] =
	"conf(set, path, value, [role], [policy]) - change a config setting to an absolute value\n"
	"conf(delta, path, value, [role], [policy]) - change a config setting by a delta value (numerics-only)\n"
	"conf(toggle, path, [role]) - invert boolean value of a flag; if no role given, overwrite the highest prio config\n"
	"conf(reset, role) - reset the in-memory lihata of a role\n"
	"conf(iseq, path, value) - returns whether the value of a conf item matches value (for menu checked's)\n"
	;
static const char pcb_acth_Conf[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_root[];
static inline int conf_iseq_pf(void *ctx, const char *fmt, ...)
{
	int res;
	va_list ap;
	va_start(ap, fmt);
	res = pcb_append_vprintf((gds_t *)ctx, fmt, ap);
	va_end(ap);
	return res;
}

static fgw_error_t pcb_act_Conf(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *cmd = argc > 0 ? argv[0] : 0;

	if ((PCB_NSTRCMP(cmd, "set") == 0) || (PCB_NSTRCMP(cmd, "delta") == 0)) {
		const char *path, *val;
		char valbuff[128];
		conf_policy_t pol = POL_OVERWRITE;
		conf_role_t role = CFR_invalid;
		int res;

		if (argc < 3) {
			pcb_message(PCB_MSG_ERROR, "conf(set) needs at least two arguments");
			return 1;
		}
		if (argc > 3) {
			role = conf_role_parse(argv[3]);
			if (role == CFR_invalid) {
				pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", argv[3]);
				return 1;
			}
		}
		if (argc > 4) {
			pol = conf_policy_parse(argv[4]);
			if (pol == POL_invalid) {
				pcb_message(PCB_MSG_ERROR, "Invalid policy: '%s'", argv[4]);
				return 1;
			}
		}
		path = argv[1];
		val = argv[2];

		if (cmd[0] == 'd') {
			double d;
			char *end;
			conf_native_t *n = conf_get_field(argv[1]);

			if (n == 0) {
				pcb_message(PCB_MSG_ERROR, "Can't delta-set '%s': no such path\n", argv[1]);
				return 1;
			}

			switch(n->type) {
				case CFN_REAL:
					d = strtod(val, &end);
					if (*end != '\0') {
						pcb_message(PCB_MSG_ERROR, "Can't delta-set '%s': invalid delta value\n", argv[1]);
						return 1;
					}
					d += *n->val.real;
					sprintf(valbuff, "%f", d);
					val = valbuff;
					break;
				case CFN_COORD:
				case CFN_INTEGER:
				default:
					pcb_message(PCB_MSG_ERROR, "Can't delta-set '%s': not a numeric item\n", argv[1]);
					return 1;
			}
		}

		if (role == CFR_invalid) {
			conf_native_t *n = conf_get_field(argv[1]);
			if (n == NULL) {
				pcb_message(PCB_MSG_ERROR, "Invalid conf field '%s': no such path\n", argv[1]);
				return 1;
			}
			res = conf_set_native(n, 0, val);
			conf_update(argv[1], 0);
		}
		else
			res = conf_set(role, path, -1, val, pol);

		if (res != 0) {
			pcb_message(PCB_MSG_ERROR, "conf(set) failed.\n");
			return 1;
		}
	}

	else if (PCB_NSTRCMP(cmd, "iseq") == 0) {
		const char *path, *val;
		int res;
		gds_t nval;
		conf_native_t *n;

		if (argc != 3) {
			pcb_message(PCB_MSG_ERROR, "conf(iseq) needs two arguments");
			return -1;
		}
		path = argv[1];
		val = argv[2];

		n = conf_get_field(argv[1]);
		if (n == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid conf field '%s' in iseq: no such path\n", path);
			return -1;
		}

		gds_init(&nval);
		conf_print_native_field(conf_iseq_pf, &nval, 0, &n->val, n->type, NULL, 0);
		res = !strcmp(nval.array, val);
/*		printf("iseq: %s %s==%s %d\n", path, nval.array, val, res);*/
		gds_uninit(&nval);

		return res;
	}

	else if (PCB_NSTRCMP(cmd, "toggle") == 0) {
		conf_native_t *n = conf_get_field(argv[1]);
		const char *new_value;
		conf_role_t role = CFR_invalid;
		int res;

		if (n == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid conf field '%s': no such path\n", argv[1]);
			return 1;
		}
		if (n->type != CFN_BOOLEAN) {
			pcb_message(PCB_MSG_ERROR, "Can not toggle '%s': not a boolean\n", argv[1]);
			return 1;
		}
		if (n->used != 1) {
			pcb_message(PCB_MSG_ERROR, "Can not toggle '%s': array size should be 1, not %d\n", argv[1], n->used);
			return 1;
		}
		if (argc > 2) {
			role = conf_role_parse(argv[2]);
			if (role == CFR_invalid) {
				pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", argv[2]);
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
			pcb_message(PCB_MSG_ERROR, "Can not toggle '%s': failed to set new value\n", argv[1]);
			return 1;
		}
		conf_update(argv[1], -1);
	}

	else if (PCB_NSTRCMP(cmd, "reset") == 0) {
		conf_role_t role;
		role = conf_role_parse(argv[1]);
		if (role == CFR_invalid) {
			pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", argv[1]);
			return 1;
		}
		conf_reset(role, "<action>");
		conf_update(argv[1], -1);
	}

	else {
		pcb_message(PCB_MSG_ERROR, "Invalid conf command '%s'\n", argv[0]);
		return 1;
	}
	return 0;
	PCB_OLD_ACT_END;
}

/*------------ get/chk (check flag actions for menus) ------------------*/
static const char GetStyle_syntax[] = "GetStyle()" ;
static const char GetStyle_help[] = "Return integer index (>=0) of the currently active style or -1 if no style is selected (== custom style)";
static fgw_error_t pcb_act_GetStyle(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	return pcb_route_style_lookup(&PCB->RouteStyle, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL);
	PCB_OLD_ACT_END;
}

static const char ChkMode_syntax[] = "ChkMode(expected_mode)" ;
static const char ChkMode_help[] = "Return 1 if the currently selected mode is the expected_mode";
static fgw_error_t pcb_act_ChkMode(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
#warning cleanup TODO: convert this to a compile-time hash; or make the toolbar configurable from the menu file
	struct {
		const char *name;
		int mode;
	} *m, modes[] = {
		{"none", PCB_MODE_NO},
		{"arc", PCB_MODE_ARC},
		{"arrow", PCB_MODE_ARROW},
		{"copy", PCB_MODE_COPY},
		{"insertpoint", PCB_MODE_INSERT_POINT},
		{"line", PCB_MODE_LINE},
		{"lock", PCB_MODE_LOCK},
		{"move", PCB_MODE_MOVE},
		{"pastebuffer", PCB_MODE_PASTE_BUFFER},
		{"polygon", PCB_MODE_POLYGON},
		{"polygonhole", PCB_MODE_POLYGON_HOLE},
		{"rectangle", PCB_MODE_RECTANGLE},
		{"remove", PCB_MODE_REMOVE},
		{"rotate", PCB_MODE_ROTATE},
		{"rubberbandmove", PCB_MODE_RUBBERBAND_MOVE},
		{"text", PCB_MODE_TEXT},
		{"thermal", PCB_MODE_THERMAL},
		{"via", PCB_MODE_VIA},
		{NULL, 0}
	};
	assert(argc == 1);
	for(m = modes; m->name != NULL; m++) {
		if (strcmp(m->name, argv[0]) == 0)
			return conf_core.editor.mode == m->mode;
	}
	pcb_message(PCB_MSG_ERROR, "Unknown mode in ChkMode(): %s\n", argv[1]);
	abort();
	return -1;
	PCB_OLD_ACT_END;
}


static const char ChkGridSize_syntax[] =
	"ChkGridSize(expected_size)\n"
	"ChkGridSize(none)\n"
	;
static const char ChkGridSize_help[] = "Return 1 if the currently selected grid matches the expected_size. If argument is \"none\" return 1 if there is no grid.";
static fgw_error_t pcb_act_ChkGridSize(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	assert(argc == 1);
	if (strcmp(argv[0], "none") == 0)
		return PCB->Grid <= 300;

	return (PCB->Grid == pcb_get_value_ex(argv[0], NULL, NULL, NULL, NULL, NULL));
	PCB_OLD_ACT_END;
}

static const char ChkSubcID_syntax[] = "ChkSubcID(pattern)\n";
static const char ChkSubcID_help[] = "Return 1 if currently shown subc ID matches the requested pattern";
static fgw_error_t pcb_act_ChkSubcID(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *have = conf_core.editor.subc_id, *expected;

	if (have == NULL) have = "";
	if (argc > 0) expected = argv[0];
	else expected = "";

	return strcmp(expected, have) == 0;
	PCB_OLD_ACT_END;
}

static const char ChkGridUnits_syntax[] = "ChkGridUnits(expected)";
static const char ChkGridUnits_help[] = "Return 1 if currently selected grid unit matches the expected (normally mm or mil)";
static fgw_error_t pcb_act_ChkGridUnits(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	assert(argc == 1);
	return strcmp(conf_core.editor.grid_unit->suffix, argv[0]) == 0;
	PCB_OLD_ACT_END;
}

static const char ChkBuffer_syntax[] = "ChkBuffer(idx)";
static const char ChkBuffer_help[] = "Return 1 if currently selected buffer's index matches idx";
static fgw_error_t pcb_act_ChkBuffer(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	int expected = argv[0][0] - '0';
	assert(argc == 1);

	return (conf_core.editor.buffer_number + 1) == expected;
	PCB_OLD_ACT_END;
}

pcb_action_t conf_action_list[] = {
	{"conf", pcb_act_Conf, pcb_acth_Conf, pcb_acts_Conf},
	{"GetStyle", pcb_act_GetStyle, GetStyle_help, GetStyle_syntax},
	{"ChkMode", pcb_act_ChkMode, ChkMode_help, ChkMode_syntax},
	{"ChkGridSize", pcb_act_ChkGridSize, ChkGridSize_help, ChkGridSize_syntax},
	{"ChkSubcID", pcb_act_ChkSubcID, ChkSubcID_help, ChkSubcID_syntax},
	{"ChkGridUnits", pcb_act_ChkGridUnits, ChkGridUnits_help, ChkGridUnits_syntax},
	{"ChkBuffer", pcb_act_ChkBuffer, ChkBuffer_help, ChkBuffer_syntax}
};

PCB_REGISTER_ACTIONS(conf_action_list, NULL)

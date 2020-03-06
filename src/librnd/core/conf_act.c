/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include <librnd/core/actions.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/funchash_core.h>
#include <librnd/core/error.h>
#include <librnd/core/tool.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/grid.h>
#include <librnd/core/compat_misc.h>

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
	res = pcb_safe_append_vprintf((gds_t *)ctx, 0, fmt, ap);
	va_end(ap);
	return res;
}

static fgw_error_t pcb_act_Conf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	const char *a1, *a2, *a3, *a4;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, Conf, op = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_STR, Conf, a1 = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, Conf, a2 = argv[3].val.str);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, Conf, a3 = argv[4].val.str);
	PCB_ACT_MAY_CONVARG(5, FGW_STR, Conf, a4 = argv[5].val.str);

	if ((op == F_Set) || (op == F_Delta)) {
		const char *path, *val;
		char valbuff[128];
		conf_policy_t pol = POL_OVERWRITE;
		conf_role_t role = CFR_invalid;
		int rs;

		if (argc < 4) {
			pcb_message(PCB_MSG_ERROR, "conf(set) needs at least two arguments");
			return FGW_ERR_ARGC;
		}
		if (argc > 4) {
			role = pcb_conf_role_parse(a3);
			if (role == CFR_invalid) {
				pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", a3);
				return FGW_ERR_ARG_CONV;
			}
		}
		if (argc > 5) {
			pol = pcb_conf_policy_parse(a4);
			if (pol == POL_invalid) {
				pcb_message(PCB_MSG_ERROR, "Invalid policy: '%s'", a4);
				return FGW_ERR_ARG_CONV;
			}
		}
		path = a1;
		val = a2;

		if (op == F_Delta) {
			double d;
			char *end;
			conf_native_t *n = pcb_conf_get_field(a1);

			if (n == 0) {
				pcb_message(PCB_MSG_ERROR, "Can't delta-set '%s': no such path\n", argv[1]);
				return FGW_ERR_ARG_CONV;
			}

			switch(n->type) {
				case CFN_REAL:
					d = strtod(val, &end);
					if (*end != '\0') {
						pcb_message(PCB_MSG_ERROR, "Can't delta-set '%s': invalid delta value\n", a1);
						return FGW_ERR_ARG_CONV;
					}
					d += *n->val.real;
					sprintf(valbuff, "%f", d);
					val = valbuff;
					break;
				case CFN_COORD:
				case CFN_INTEGER:
				default:
					pcb_message(PCB_MSG_ERROR, "Can't delta-set '%s': not a numeric item\n", a1);
					return FGW_ERR_ARG_CONV;
			}
		}

		if (role == CFR_invalid) {
			conf_native_t *n = pcb_conf_get_field(a1);
			if (n == NULL) {
				pcb_message(PCB_MSG_ERROR, "Invalid conf field '%s': no such path\n", a1);
				return FGW_ERR_ARG_CONV;
			}
			rs = pcb_conf_set_native(n, 0, val);
			pcb_conf_update(a1, 0);
		}
		else
			rs = pcb_conf_set(role, path, -1, val, pol);

		if (rs != 0) {
			pcb_message(PCB_MSG_ERROR, "conf(set) failed.\n");
			return FGW_ERR_UNKNOWN;
		}
	}

	else if (op == F_Iseq) {
		const char *path, *val;
		int rs;
		gds_t nval;
		conf_native_t *n;

		if (argc != 4) {
			pcb_message(PCB_MSG_ERROR, "conf(iseq) needs two arguments");
			return FGW_ERR_ARGC;
		}
		path = a1;
		val = a2;

		n = pcb_conf_get_field(a1);
		if (n == NULL) {
			if (pcbhl_conf.rc.verbose)
				pcb_message(PCB_MSG_ERROR, "Invalid conf field '%s' in iseq: no such path\n", path);
			return FGW_ERR_ARG_CONV;
		}

		gds_init(&nval);
		pcb_conf_print_native_field(conf_iseq_pf, &nval, 0, &n->val, n->type, NULL, 0);
		rs = !strcmp(nval.array, val);
/*		printf("iseq: %s %s==%s %d\n", path, nval.array, val, rs);*/
		gds_uninit(&nval);

		PCB_ACT_IRES(rs);
		return 0;
	}

	else if (op == F_Toggle) {
		conf_native_t *n = pcb_conf_get_field(a1);
		const char *new_value;
		conf_role_t role = CFR_invalid;
		int res;

		if (n == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid conf field '%s': no such path\n", a1);
			return FGW_ERR_UNKNOWN;
		}
		if (n->type != CFN_BOOLEAN) {
			pcb_message(PCB_MSG_ERROR, "Can not toggle '%s': not a boolean\n", a1);
			return FGW_ERR_UNKNOWN;
		}
		if (n->used != 1) {
			pcb_message(PCB_MSG_ERROR, "Can not toggle '%s': array size should be 1, not %d\n", a1, n->used);
			return FGW_ERR_UNKNOWN;
		}
		if (argc > 3) {
			role = pcb_conf_role_parse(a2);
			if (role == CFR_invalid) {
				pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", a2);
				return FGW_ERR_ARG_CONV;
			}
		}
		if (n->val.boolean[0])
			new_value = "false";
		else
			new_value = "true";
		if (role == CFR_invalid)
			res = pcb_conf_set_native(n, 0, new_value);
		else
			res = pcb_conf_set(role, a1, -1, new_value, POL_OVERWRITE);

		if (res != 0) {
			pcb_message(PCB_MSG_ERROR, "Can not toggle '%s': failed to set new value\n", a1);
			return FGW_ERR_UNKNOWN;
		}
		pcb_conf_update(a1, -1);
	}

	else if (op == F_Reset) {
		conf_role_t role;
		role = pcb_conf_role_parse(a1);
		if (role == CFR_invalid) {
			pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", a1);
			return FGW_ERR_ARG_CONV;
		}
		pcb_conf_reset(role, "<action>");
		pcb_conf_update(a1, -1);
	}

	else {
		pcb_message(PCB_MSG_ERROR, "Invalid conf command\n");
		return FGW_ERR_ARG_CONV;
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ChkMode[] = "ChkMode(expected_mode)" ;
static const char pcb_acth_ChkMode[] = "Return 1 if the currently selected mode is the expected_mode";
static fgw_error_t pcb_act_ChkMode(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dst;
	pcb_toolid_t id;

	PCB_ACT_CONVARG(1, FGW_STR, ChkMode, dst = argv[1].val.str);

	id = pcb_tool_lookup(dst);
	if (id >= 0) {
		PCB_ACT_IRES(pcbhl_conf.editor.mode == id);
		return 0;
	}
	PCB_ACT_IRES(-1);
	return 0;
}


static const char pcb_acts_ChkGridSize[] =
	"ChkGridSize(expected_size)\n"
	"ChkGridSize(none)\n"
	;
static const char pcb_acth_ChkGridSize[] = "Return 1 if the currently selected grid matches the expected_size. If argument is \"none\" return 1 if there is no grid.";
static fgw_error_t pcb_act_ChkGridSize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dst;

	PCB_ACT_CONVARG(1, FGW_STR, ChkGridSize, dst = argv[1].val.str);

	if (strcmp(dst, "none") == 0) {
		PCB_ACT_IRES(PCB_ACT_HIDLIB->grid <= 300);
		return 0;
	}

	PCB_ACT_IRES(PCB_ACT_HIDLIB->grid == pcb_get_value_ex(dst, NULL, NULL, NULL, NULL, NULL));
	return 0;
}


static const char pcb_acts_ChkGridUnits[] = "ChkGridUnits(expected)";
static const char pcb_acth_ChkGridUnits[] = "Return 1 if currently selected grid unit matches the expected (normally mm or mil)";
static fgw_error_t pcb_act_ChkGridUnits(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *expected;
	PCB_ACT_CONVARG(1, FGW_STR, ChkGridUnits, expected = argv[1].val.str);
	PCB_ACT_IRES(strcmp(pcbhl_conf.editor.grid_unit->suffix, expected) == 0);
	return 0;
}

static const char pcb_acts_SetGrid[] = "SetGrid(delta|*mult|/div, [unit])";
static const char pcb_acth_SetGrid[] = "Change various board-wide values and sizes.";
/* for doc: copy from SetValue(grid,...) */
static fgw_error_t pcb_act_SetGrid(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *val, *units = NULL;
	pcb_bool absolute;
	double value;

	PCB_ACT_CONVARG(1, FGW_STR, SetGrid, val = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, SetGrid, units = argv[2].val.str);

	PCB_ACT_IRES(0);

	/* special case: can't convert with pcb_get_value() */
	if ((val[0] == '*') || (val[0] == '/')) {
		double d;
		char *end;

		d = strtod(val+1, &end);
		if ((*end != '\0') || (d <= 0)) {
			pcb_message(PCB_MSG_ERROR, "SetGrid: Invalid multiplier/divider for grid set: needs to be a positive number\n");
			return 1;
		}
		pcb_grid_inval();
		if (val[0] == '*')
			pcb_hidlib_set_grid(PCB_ACT_HIDLIB, pcb_round(PCB_ACT_HIDLIB->grid * d), pcb_false, 0, 0);
		else
			pcb_hidlib_set_grid(PCB_ACT_HIDLIB, pcb_round(PCB_ACT_HIDLIB->grid / d), pcb_false, 0, 0);
		return 0;
	}

	value = pcb_get_value(val, units, &absolute, NULL);

	pcb_grid_inval();
	if (absolute)
		pcb_hidlib_set_grid(PCB_ACT_HIDLIB, value, pcb_false, 0, 0);
	else {
		/* On the way down, until the minimum unit (1) */
		if ((value + PCB_ACT_HIDLIB->grid) < 1)
			pcb_hidlib_set_grid(PCB_ACT_HIDLIB, 1, pcb_false, 0, 0);
		else if (PCB_ACT_HIDLIB->grid == 1)
			pcb_hidlib_set_grid(PCB_ACT_HIDLIB, value, pcb_false, 0, 0);
		else
			pcb_hidlib_set_grid(PCB_ACT_HIDLIB, value + PCB_ACT_HIDLIB->grid, pcb_false, 0, 0);
	}
	return 0;
}


static const char pcb_acts_setunits[] = "SetUnits(mm|mil)";
static const char pcb_acth_setunits[] = "Set the default measurement units.";
/* DOC: setunits.html */
static fgw_error_t pcb_act_SetUnits(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const pcb_unit_t *new_unit;
	const char *name;

	PCB_ACT_CONVARG(1, FGW_STR, setunits, name = argv[1].val.str);
	PCB_ACT_IRES(0);

	new_unit = get_unit_struct(name);
	pcb_hidlib_set_unit(PCB_ACT_HIDLIB, new_unit);

	return 0;
}


static pcb_action_t rnd_conf_action_list[] = {
	{"conf", pcb_act_Conf, pcb_acth_Conf, pcb_acts_Conf},
	{"ChkMode", pcb_act_ChkMode, pcb_acth_ChkMode, pcb_acts_ChkMode},
	{"ChkGridSize", pcb_act_ChkGridSize, pcb_acth_ChkGridSize, pcb_acts_ChkGridSize},
	{"ChkGridUnits", pcb_act_ChkGridUnits, pcb_acth_ChkGridUnits, pcb_acts_ChkGridUnits},
	{"SetGrid", pcb_act_SetGrid, pcb_acth_SetGrid, pcb_acts_SetGrid},
	{"SetUnits", pcb_act_SetUnits, pcb_acth_setunits, pcb_acts_setunits}
};

void rnd_conf_act_init2(void)
{
	PCB_REGISTER_ACTIONS(rnd_conf_action_list, NULL);
}




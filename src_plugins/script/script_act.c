/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

static const char pcb_acth_LoadScript[] = "Load a fungw script";
static const char pcb_acts_LoadScript[] = "LoadScript(id, filename, [language])";
static fgw_error_t pcb_act_LoadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id, *fn, *lang = NULL;
	PCB_ACT_CONVARG(1, FGW_STR, LoadScript, id = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, LoadScript, fn = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, LoadScript, lang = argv[3].val.str);

	PCB_ACT_IRES(script_load(id, fn, lang));
	return 0;
}

static const char pcb_acth_UnloadScript[] = "Unload a fungw script";
static const char pcb_acts_UnloadScript[] = "UnloadScript(id)";
static fgw_error_t pcb_act_UnloadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id = NULL;
	PCB_ACT_CONVARG(1, FGW_STR, UnloadScript, id = argv[1].val.str);

	PCB_ACT_IRES(script_unload(id));
	return 0;
}

static const char pcb_acth_ListScripts[] = "List a fungw script";
static const char pcb_acts_ListScripts[] = "ListScripts([pat])";
static fgw_error_t pcb_act_ListScripts(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *pat = NULL;
	PCB_ACT_MAY_CONVARG(1, FGW_STR, ListScripts, pat = argv[1].val.str);

	script_list(pat);

	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t script_action_list[] = {
	{"LoadScript", pcb_act_LoadScript, pcb_acth_LoadScript, pcb_acts_LoadScript},
	{"UnloadScript", pcb_act_UnloadScript, pcb_acth_UnloadScript, pcb_acts_UnloadScript},
	{"ListScripts", pcb_act_ListScripts, pcb_acth_ListScripts, pcb_acts_ListScripts}
};

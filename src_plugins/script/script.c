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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include "actions.h"
#include "plugins.h"
#include "error.h"
#include "compat_misc.h"

typedef struct {
	char *id, *fn, *lang;
} script_t;

htsp_t scripts; /* ID->script_t */

static void script_unload_entry(htsp_entry_t *e)
{
	script_t *s = (script_t *)e->value;
	free(s->id);
	free(s->fn);
	free(s);
	e->key = NULL;
	htsp_delentry(&scripts, e);
}

static int script_unload(const char *id)
{
	htsp_entry_t *e = htsp_getentry(&scripts, id);
	if (e == NULL)
		return -1;
	script_unload_entry(e);
	return 0;
}

static int script_load(const char *id, const char *fn, const char *lang)
{
	script_t *s;
	if (htsp_has(&scripts, id)) {
		pcb_message(PCB_MSG_ERROR, "Can not load script %s from file %s: ID already in use\n", id, fn);
		return -1;
	}

	if (lang == NULL) {
#warning TODO: guess
		pcb_message(PCB_MSG_ERROR, "Can not load script %s from file %s: failed to guess language from file name\n", id, fn);
		return -1;
	}

	s = calloc(1, sizeof(s));
	s->id = pcb_strdup(id);
	s->fn = pcb_strdup(fn);
	s->lang = pcb_strdup(lang);
	htsp_set(&scripts, s->id, s);
	return 0;
}

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

static pcb_action_t script_action_list[] = {
	{"LoadScript", pcb_act_LoadScript, pcb_acth_LoadScript, pcb_acts_LoadScript}
};

char *script_cookie = "script plugin";

PCB_REGISTER_ACTIONS(script_action_list, script_cookie)

int pplg_check_ver_script(int ver_needed) { return 0; }

void pplg_uninit_script(void)
{
	htsp_entry_t *e;
	pcb_remove_actions_by_cookie(script_cookie);
	for(e = htsp_first(&scripts); e; e = htsp_next(&scripts, e))
		script_unload_entry(e);

	htsp_uninit(&scripts);
}

#include "dolists.h"
int pplg_init_script(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(script_action_list, script_cookie);
	htsp_init(&scripts, strhash, strkeyeq);
	return 0;
}

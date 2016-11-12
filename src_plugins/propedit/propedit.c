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

#include <stdlib.h>
#include "plugins.h"
#include "config.h"
#include "props.h"
#include "propsel.h"
#include "hid_actions.h"
#include "pcb-printf.h"
#include "error.h"

/* ************************************************************ */

#warning TODO
static const char propedit_syntax[] = "propedit()";
static const char propedit_help[] = "Run the property editor";
int propedit_action(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pe_ctx_t ctx;
	htsp_entry_t *pe;

	if ((gui == NULL) || (gui->propedit_start == NULL)) {
		Message(PCB_MSG_DEFAULT, "Error: there's no GUI or the active GUI can't edit properties.\n");
		return 1;
	}

	ctx.core_props = pcb_props_init();
	pcb_propsel_map_core(ctx.core_props);

	gui->propedit_start(&ctx, ctx.core_props->fill, propedit_query);
	for (pe = htsp_first(ctx.core_props); pe; pe = htsp_next(ctx.core_props, pe))
		propedit_ins_prop(&ctx, pe);

	gui->propedit_end(&ctx);
	pcb_props_uninit(ctx.core_props);
	return 0;
}

static const char propset_syntax[] = "propset(name, value)";
static const char propset_help[] = "Change the named property of all selected objects to/by value";
int propset_action(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int res;
/*
	if (argc != 2)
*/
	printf("argc=%d '%s'\n", argc, argv[0]);
	res = pcb_propsel_set(argv[0], argv[1]);
	printf("res=%d\n", res);
	return 0;
}

static const char *propedit_cookie = "propedit";

pcb_hid_action_t propedit_action_list[] = {
	{"propedit", 0, propedit_action,
	 propedit_help, propedit_syntax},
	{"propset", 0, propset_action,
	 propset_help, propset_syntax},
};

REGISTER_ACTIONS(propedit_action_list, propedit_cookie)

static void hid_propedit_uninit(void)
{
	hid_remove_actions_by_cookie(propedit_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_propedit_init(void)
{
	REGISTER_ACTIONS(propedit_action_list, propedit_cookie)
	return hid_propedit_uninit;
}

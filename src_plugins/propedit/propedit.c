/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design - pcb-rnd
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
#include "props.h"
#include "propsel.h"
#include "hid_actions.h"
#include "pcb-printf.h"

/* ************************************************************ */

#warning TODO
static const char propedit_syntax[] = "propedit()";

static const char propedit_help[] = "Run the property editor";

/* %start-doc actions propedit

TODO
%end-doc */

typedef struct {
	htsp_t *core_props;
} pe_ctx_t;


static char buff[8][128];
static int buff_idx = 0;
static const char *sprint_val(pcb_prop_type_t type, pcb_propval_t val)
{
	char *b;
	buff_idx++;
	if (buff_idx > 7)
		buff_idx = 0;
	b = buff[buff_idx];
	switch(type) {
		case PCB_PROPT_STRING: return val.string; break;
		case PCB_PROPT_COORD:  pcb_snprintf(b, 128, "%$mm (%$ml)", val.coord, val.coord); break;
		case PCB_PROPT_ANGLE:  sprintf(b, "%f", val.angle); break;
		case PCB_PROPT_INT:    sprintf(b, "%d", val.i); break;
		default: strcpy(b, "<unknown type>");
	}
	return b;
}




int propedit_action(int argc, char **argv, Coord x, Coord y)
{
	pe_ctx_t ctx;
	ctx.core_props = pcb_props_init();
	htsp_entry_t *pe;

	if ((gui == NULL) || (gui->propedit_start == NULL))
		return 1;

	pcb_propsel_map_core(ctx.core_props);

	gui->propedit_start(&ctx, ctx.core_props->fill);
	for (pe = htsp_first(ctx.core_props); pe; pe = htsp_next(ctx.core_props, pe)) {
		htprop_entry_t *e;
		void *rowid;
		pcb_props_t *p = pe->value;
		pcb_propval_t common, min, max, avg;

		rowid = gui->propedit_add_prop(&ctx, pe->key, 1, p->values.fill);

		if (p->type == PCB_PROPT_STRING) {
			pcb_props_stat(ctx.core_props, pe->key, &common, NULL, NULL, NULL);
			gui->propedit_add_stat(&ctx, pe->key, rowid, sprint_val(p->type, common), NULL, NULL, NULL);
		}
		else {
			pcb_props_stat(ctx.core_props, pe->key, &common, &min, &max, &avg);
			gui->propedit_add_stat(&ctx, pe->key, rowid, sprint_val(p->type, common), sprint_val(p->type, min), sprint_val(p->type, max), sprint_val(p->type, avg));
		}

		for (e = htprop_first(&p->values); e; e = htprop_next(&p->values, e))
			gui->propedit_add_value(&ctx, pe->key, rowid, sprint_val(p->type, e->key), e->value);
	}

	gui->propedit_end(&ctx);
	pcb_props_uninit(ctx.core_props);
	return 0;
}


static const char *propedit_cookie = "propedit";

HID_Action propedit_action_list[] = {
	{"propedit", 0, propedit_action,
	 propedit_help, propedit_syntax},
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


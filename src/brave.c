/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, diag, was written and is Copyright (C) 2016 by Tibor Palinkas
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

#include <genvector/gds_char.h>

#include "brave.h"
#include "compat_misc.h"
#include "error.h"
#include "hid_dad.h"

pcb_brave_t pcb_brave = 0;

typedef struct {
	pcb_brave_t bit;
	const char *name, *shrt, *lng;
	int widget;
} desc_t;

static desc_t desc[] = {
	{PCB_BRAVE_PSTK_VIA, "pstk_via", "use padstack for vias",
		"placing new vias will place padstacks instsad of old via objects", 0 },
	{0, NULL, NULL, NULL}
};

static desc_t *find_by_name(const char *name)
{
	desc_t *d;
	for(d = desc; d->name != NULL; d++)
		if (pcb_strcasecmp(name, d->name) == 0)
			return d;
	return NULL;
}

static void set_conf(pcb_brave_t br)
{
	gds_t tmp;
	desc_t *d;

	gds_init(&tmp);

	for(d = desc; d->name != NULL; d++) {
		if (br & d->bit) {
			gds_append_str(&tmp, d->name);
			gds_append(&tmp, ',');
		}
	}

	/* truncate last comma */
	gds_truncate(&tmp, gds_len(&tmp)-1);

#warning TODO: set the conf

	gds_uninit(&tmp);
}

static void brave_set(pcb_brave_t bit, int on)
{
	int state = pcb_brave & bit;
	pcb_brave_t nb;

	if (state == on)
		return;

	if (on)
		nb = pcb_brave | bit;
	else
		nb = pcb_brave & ~bit;

	set_conf(nb);
}


static const char pcb_acts_Brave[] =
	"Brave()\n"
	"Brave(setting, on|off)\n";
static const char pcb_acth_Brave[] = "Changes brave settings.";
static int pcb_act_Brave(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	desc_t *d;
	if (argc == 0) {
		
	}

	/* look up */
	if (argc > 0) {
		d = find_by_name(argv[0]);
		if (d == NULL) {
			pcb_message(PCB_MSG_ERROR, "Unknown brave setting: %s\n", argv[0]);
			return -1;
		}
	}

	if (argc > 1)
		brave_set(d->bit, (pcb_strcasecmp(argv[1], "on") == 0));

	pcb_message(PCB_MSG_INFO, "Brave setting: %s in %s\n", argv[0], (pcb_brave & d->bit) ? "on" : "off");

	return 0;
}

pcb_hid_action_t brave_action_list[] = {
	{"Brave", 0, pcb_act_Brave,
	 pcb_acth_Brave, pcb_acts_Brave}
};

PCB_REGISTER_ACTIONS(brave_action_list, NULL)


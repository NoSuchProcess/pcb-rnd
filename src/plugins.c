/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
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

#include <stdlib.h>
#include <string.h>
#include "plugins.h"

/* for the action */
#include "config.h"
#include "genvector/gds_char.h"
#include "compat_misc.h"
#include "hid.h"

pup_context_t pcb_pup;
char **pcb_pup_paths = NULL;
static int paths_used = 0, paths_alloced = 0;

void pcb_plugin_add_dir(const char *dir)
{
	if (paths_used+1 >= paths_alloced) {
		paths_alloced += 16;
		pcb_pup_paths = realloc(pcb_pup_paths, sizeof(char *) * paths_alloced);
	}
	pcb_pup_paths[paths_used] = pcb_strdup(dir);
	paths_used++;
	pcb_pup_paths[paths_used] = NULL;
}

/* ---------------------------------------------------------------- */
static const char pcb_acts_ManagePlugins[] = "ManagePlugins()\n";

static const char pcb_acth_ManagePlugins[] = "Manage plugins dialog.";

static int pcb_act_ManagePlugins(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
#warning puplug TODO: rewrite for puplug
#if 0
	pcb_plugin_info_t *i;
	int nump = 0, numb = 0;
	gds_t str;

	gds_init(&str);

	for (i = plugins; i != NULL; i = i->next)
		if (i->dynamic_loaded)
			nump++;
		else
			numb++;

	gds_append_str(&str, "Plugins loaded:\n");
	if (nump > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (i->dynamic_loaded) {
				gds_append(&str, ' ');
				gds_append_str(&str, i->name);
				gds_append(&str, ' ');
				gds_append_str(&str, i->path);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nBuildins:\n");
	if (numb > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (!i->dynamic_loaded) {
				gds_append(&str, ' ');
				gds_append_str(&str, i->name);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nNOTE: this is the alpha version, can only list plugins/buildins\n");
	pcb_gui->report_dialog("Manage plugins", str.array);
	gds_uninit(&str);
#endif
	return 0;
}


pcb_hid_action_t plugins_action_list[] = {
	{"ManagePlugins", 0, pcb_act_ManagePlugins,
	 pcb_acth_ManagePlugins, pcb_acts_ManagePlugins}
};

PCB_REGISTER_ACTIONS(plugins_action_list, NULL)

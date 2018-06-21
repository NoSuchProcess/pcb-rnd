/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <string.h>
#include "plugins.h"

/* for the action */
#include "config.h"
#include "genvector/gds_char.h"
#include "compat_misc.h"
#include "actions.h"

unsigned long pcb_api_ver = PCB_API_VER;

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

void pcb_plugin_uninit(void)
{
	int n;
	for(n = 0; n < paths_used; n++)
		free(pcb_pup_paths[n]);
	free(pcb_pup_paths);
	pcb_pup_paths = NULL;
}


/* ---------------------------------------------------------------- */
static const char pcb_acts_ManagePlugins[] = "ManagePlugins()\n";

static const char pcb_acth_ManagePlugins[] = "Manage plugins dialog.";

static int pcb_act_ManagePlugins(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pup_plugin_t *p;
	int nump = 0, numb = 0;
	gds_t str;

	gds_init(&str);

	for (p = pcb_pup.plugins; p != NULL; p = p->next)
		if (p->flags & PUP_FLG_STATIC)
			numb++;
		else
			nump++;

	gds_append_str(&str, "Plugins loaded:\n");
	if (nump > 0) {
		for (p = pcb_pup.plugins; p != NULL; p = p->next) {
			if (!(p->flags & PUP_FLG_STATIC)) {
				gds_append(&str, ' ');
				gds_append_str(&str, p->name);
				gds_append(&str, ' ');
				gds_append_str(&str, p->path);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nBuildins:\n");
	if (numb > 0) {
		for (p = pcb_pup.plugins; p != NULL; p = p->next) {
			if (p->flags & PUP_FLG_STATIC) {
				gds_append(&str, ' ');
				gds_append_str(&str, p->name);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nNOTE: this is the alpha version, can only list plugins/buildins\n");
	pcb_gui->report_dialog("Manage plugins", str.array);
	gds_uninit(&str);
	return 0;
	PCB_OLD_ACT_END;
}


pcb_hid_action_t plugins_action_list[] = {
	{"ManagePlugins", pcb_act_ManagePlugins, pcb_acth_ManagePlugins, pcb_acts_ManagePlugins}
};

PCB_REGISTER_ACTIONS(plugins_action_list, NULL)

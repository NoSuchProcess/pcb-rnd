/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"

#include <stdio.h>
#include <librnd/config.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/plugins.h>
#include <librnd/core/file_loaded.h>
#include <librnd/core/safe_fs.h>

static const char pcb_acts_PrintActions[] = "PrintActions()";
static const char pcb_acth_PrintActions[] = "Print all actions available.";
fgw_error_t pcb_act_PrintActions(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_print_actions();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpActions[] = "DumpActions()";
static const char pcb_acth_DumpActions[] = "Dump all actions available.";
fgw_error_t pcb_act_DumpActions(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dump_actions();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_PrintFiles[] = "PrintFiles()";
static const char pcb_acth_PrintFiles[] = "Print files currently loaded.";
static void print_cat(pcb_file_loaded_t *cat)
{
	htsp_entry_t *e;
	printf("%s\n", cat->name);
	for (e = htsp_first(&cat->data.category.children); e; e = htsp_next(&cat->data.category.children, e)) {
		pcb_file_loaded_t *file = e->value;
		printf(" %s\t%s\t%s\n", file->name, file->data.file.path, file->data.file.desc);
	}
}
fgw_error_t pcb_act_PrintFiles(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	htsp_entry_t *e;
	printf("# Data files loaded\n");
	for (e = htsp_first(&pcb_file_loaded); e; e = htsp_next(&pcb_file_loaded, e))
		print_cat(e->value);
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpPlugins[] = "DumpPlugins()";
static const char pcb_acth_DumpPlugins[] = "Print plugins loaded in a format digestable by scripts.";
fgw_error_t pcb_act_DumpPlugins(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pup_plugin_t *p;
	const pup_buildin_t **bu;
	int n;

	printf("#state\tname\tbuildin\tautoload\trefco\tloaded_from\n");

	for(p = pcb_pup.plugins; p != NULL; p = p->next)
		printf("loaded\t%s\t%d\t%d\t%d\t%s\n",
			p->name,
			!!(p->flags & PUP_FLG_STATIC), !!(p->flags & PUP_FLG_AUTOLOAD), p->references,
			(p->path == NULL ? "<builtin>" : p->path));

	for(n = 0, bu = pcb_pup.bu; n < pcb_pup.bu_used; n++, bu++)
		if (pup_lookup(&pcb_pup, (*bu)->name) == NULL)
			printf("unloaded buildin\t%s\t1\t0\t0\t<builtin>\n", (*bu)->name);

	PCB_ACT_IRES(0);
	return 0;
}


static const char pcb_acts_DumpPluginDirs[] = "DumpPluginDirs()";
static const char pcb_acth_DumpPluginDirs[] = "Print plugins directories in a format digestable by scripts.";
fgw_error_t pcb_act_DumpPluginDirs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char **p;
	for(p = pcb_pup_paths; *p != NULL; p++)
		printf("%s\n", *p);

	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t rnd_main_action_list[] = {
	{"PrintActions", pcb_act_PrintActions, pcb_acth_PrintActions, pcb_acts_PrintActions},
	{"DumpActions", pcb_act_DumpActions, pcb_acth_DumpActions, pcb_acts_DumpActions},
	{"PrintFiles", pcb_act_PrintFiles, pcb_acth_PrintFiles, pcb_acts_PrintFiles},
	{"DumpPlugins", pcb_act_DumpPlugins, pcb_acth_DumpPlugins, pcb_acts_DumpPlugins},
	{"DumpPluginDirs", pcb_act_DumpPluginDirs, pcb_acth_DumpPluginDirs, pcb_acts_DumpPluginDirs},
};

void rnd_main_act_init2(void)
{
	PCB_REGISTER_ACTIONS(rnd_main_action_list, NULL);
}

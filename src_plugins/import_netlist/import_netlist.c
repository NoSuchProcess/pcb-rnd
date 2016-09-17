/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015,2016 Tibor 'Igor2' Palinkas
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

/* for popen() */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "config.h"
#include "global.h"
#include "plugins.h"
#include "plug_io.h"
#include "plug_import.h"
#include "conf_core.h"
#include "error.h"
#include "misc.h"
#include "data.h"
#include "rats_patch.h"
#include "compat_misc.h"

static plug_import_t import_netlist;


#define BLANK(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' \
		|| (x) == '\0')

/* ---------------------------------------------------------------------------
 * Read in a netlist and store it in the netlist menu 
 */
static int ReadNetlist(const char *filename)
{
	char *command = NULL;
	char inputline[MAX_NETLIST_LINE_LENGTH + 1];
	char temp[MAX_NETLIST_LINE_LENGTH + 1];
	FILE *fp;
	LibraryMenuTypePtr menu = NULL;
	LibraryEntryTypePtr entry;
	int i, j, lines, kind;
	pcb_bool continued;
	int used_popen = 0;

	if (!filename)
		return (1);									/* nothing to do */

	Message(PCB_MSG_DEFAULT, _("Importing PCB netlist %s\n"), filename);

	if (EMPTY_STRING_P(conf_core.rc.rat_command)) {
		fp = fopen(filename, "r");
		if (!fp) {
			Message(PCB_MSG_DEFAULT, "Cannot open %s for reading", filename);
			return 1;
		}
	}
	else {
		used_popen = 1;
		command = EvaluateFilename(conf_core.rc.rat_command, conf_core.rc.rat_path, filename, NULL);

		/* open pipe to stdout of command */
		if (*command == '\0' || (fp = popen(command, "r")) == NULL) {
			PopenErrorMessage(command);
			free(command);
			return (1);
		}
		free(command);
	}
	lines = 0;
	/* kind = 0  is net name
	 * kind = 1  is route style name
	 * kind = 2  is connection
	 */
	kind = 0;
	while (fgets(inputline, MAX_NETLIST_LINE_LENGTH, fp)) {
		size_t len = strlen(inputline);
		/* check for maximum length line */
		if (len) {
			if (inputline[--len] != '\n')
				Message(PCB_MSG_DEFAULT, _("Line length (%i) exceeded in netlist file.\n"
									"additional characters will be ignored.\n"), MAX_NETLIST_LINE_LENGTH);
			else
				inputline[len] = '\0';
		}
		continued = (inputline[len - 1] == '\\') ? pcb_true : pcb_false;
		if (continued)
			inputline[len - 1] = '\0';
		lines++;
		i = 0;
		while (inputline[i] != '\0') {
			j = 0;
			/* skip leading blanks */
			while (inputline[i] != '\0' && BLANK(inputline[i]))
				i++;
			if (kind == 0) {
				/* add two spaces for included/unincluded */
				temp[j++] = ' ';
				temp[j++] = ' ';
			}
			while (!BLANK(inputline[i]))
				temp[j++] = inputline[i++];
			temp[j] = '\0';
			while (inputline[i] != '\0' && BLANK(inputline[i]))
				i++;
			if (kind == 0) {
				menu = GetLibraryMenuMemory(&PCB->NetlistLib[NETLIST_INPUT], NULL);
				menu->Name = pcb_strdup(temp);
				menu->flag = 1;
				kind++;
			}
			else {
				if (kind == 1 && strchr(temp, '-') == NULL) {
					kind++;
					menu->Style = pcb_strdup(temp);
				}
				else {
					entry = GetLibraryEntryMemory(menu);
					entry->ListEntry = pcb_strdup(temp);
					entry->ListEntry_dontfree = 0;
				}
			}
		}
		if (!continued)
			kind = 0;
	}
	if (!lines) {
		Message(PCB_MSG_DEFAULT, _("Empty netlist file!\n"));
		pclose(fp);
		return (1);
	}
	if (used_popen)
		pclose(fp);
	else
		fclose(fp);
	sort_netlist();
	rats_patch_make_edited(PCB);
	return (0);
}

int netlist_support_prio(plug_import_t *ctx, unsigned int aspects, FILE *fp, const char *filename)
{
	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */

	/* we are sort of a low prio fallback without any chance to check the file format in advance */
	return 10;
}


static int netlist_import(plug_import_t *ctx, unsigned int aspects, const char *fn)
{
	return ReadNetlist(fn);
}

static void hid_import_netlist_uninit(void)
{
}

pcb_uninit_t hid_import_netlist_init(void)
{

	/* register the IO hook */
	import_netlist.plugin_data = NULL;

	import_netlist.fmt_support_prio = netlist_support_prio;
	import_netlist.import           = netlist_import;

	HOOK_REGISTER(plug_import_t, plug_import_chain, &import_netlist);

	return hid_import_netlist_uninit;
}


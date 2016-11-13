/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#warning TODO: replace this with genvect

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "library.h"
#include "macro.h"
#include "compat_misc.h"

#define	STEP_LIBRARYMENU	10
#define	STEP_LIBRARYENTRY	20

/* ---------------------------------------------------------------------------
 * get next slot for a library menu, allocates memory if necessary
 */
pcb_lib_menu_t *pcb_lib_menu_new(pcb_lib_t *lib, int *idx)
{
	pcb_lib_menu_t *menu = lib->Menu;

	/* realloc new memory if necessary and clear it */
	if (lib->MenuN >= lib->MenuMax) {
		lib->MenuMax += STEP_LIBRARYMENU;
		menu = (pcb_lib_menu_t *) realloc(menu, lib->MenuMax * sizeof(pcb_lib_menu_t));
		lib->Menu = menu;
		memset(menu + lib->MenuN, 0, STEP_LIBRARYMENU * sizeof(pcb_lib_menu_t));
	}
	if (idx != NULL)
		*idx = lib->MenuN;
	return (menu + lib->MenuN++);
}

void pcb_lib_menu_free(pcb_lib_t *lib, int menuidx)
{
	pcb_lib_menu_t *menu = lib->Menu;

	if (menu[menuidx].Name != NULL)
		free(menu[menuidx].Name);
	if (menu[menuidx].directory != NULL)
		free(menu[menuidx].directory);

	lib->MenuN--;
	memmove(menu + menuidx, menu + menuidx + 1, sizeof(pcb_lib_menu_t) * (lib->MenuN - menuidx));
}

/* ---------------------------------------------------------------------------
 * get next slot for a library entry, allocates memory if necessary
 */
pcb_lib_entry_t *pcb_lib_entry_new(pcb_lib_menu_t *Menu)
{
	pcb_lib_entry_t *entry = Menu->Entry;

	/* realloc new memory if necessary and clear it */
	if (Menu->EntryN >= Menu->EntryMax) {
		Menu->EntryMax += STEP_LIBRARYENTRY;
		entry = (pcb_lib_entry_t *) realloc(entry, Menu->EntryMax * sizeof(pcb_lib_entry_t));
		Menu->Entry = entry;
		memset(entry + Menu->EntryN, 0, STEP_LIBRARYENTRY * sizeof(pcb_lib_entry_t));
	}
	return (entry + Menu->EntryN++);
}


/* ---------------------------------------------------------------------------
 * releases the memory that's allocated by the library
 */
void pcb_lib_free(pcb_lib_t *lib)
{
	MENU_LOOP(lib);
	{
		ENTRY_LOOP(menu);
		{
			if (!entry->ListEntry_dontfree)
				free((char*)entry->ListEntry);
		}
		END_LOOP;
		free(menu->Entry);
		free(menu->Name);
		free(menu->directory);
	}
	END_LOOP;
	free(lib->Menu);

	/* clear struct */
	memset(lib, 0, sizeof(pcb_lib_t));
}

/* ---------------------------------------------------------------------------
 * Add a new net to the netlist menu
 */
pcb_lib_menu_t *pcb_lib_net_new(pcb_lib_t *lib, char *name, const char *style)
{
	pcb_lib_menu_t *menu;
	char temp[64];

	sprintf(temp, "  %s", name);
	menu = pcb_lib_menu_new(lib, NULL);
	menu->Name = pcb_strdup(temp);
	menu->flag = 1;								/* net is enabled by default */
	if (style == NULL || NSTRCMP("(unknown)", style) == 0)
		menu->Style = NULL;
	else
		menu->Style = pcb_strdup(style);
	return (menu);
}

/* ---------------------------------------------------------------------------
 * Add a connection to the net
 */
pcb_lib_entry_t *pcb_lib_conn_new(pcb_lib_menu_t *net, char *conn)
{
	pcb_lib_entry_t *entry = pcb_lib_entry_new(net);

	entry->ListEntry = pcb_strdup_null(conn);
	entry->ListEntry_dontfree = 0;

	return (entry);
}

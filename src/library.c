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
#include "config.h"
#include "library.h"
#include "macro.h"

#define	STEP_LIBRARYMENU	10
#define	STEP_LIBRARYENTRY	20

/* ---------------------------------------------------------------------------
 * get next slot for a library menu, allocates memory if necessary
 */
LibraryMenuTypePtr GetLibraryMenuMemory(LibraryTypePtr lib, int *idx)
{
	LibraryMenuTypePtr menu = lib->Menu;

	/* realloc new memory if necessary and clear it */
	if (lib->MenuN >= lib->MenuMax) {
		lib->MenuMax += STEP_LIBRARYMENU;
		menu = (LibraryMenuTypePtr) realloc(menu, lib->MenuMax * sizeof(LibraryMenuType));
		lib->Menu = menu;
		memset(menu + lib->MenuN, 0, STEP_LIBRARYMENU * sizeof(LibraryMenuType));
	}
	if (idx != NULL)
		*idx = lib->MenuN;
	return (menu + lib->MenuN++);
}

void DeleteLibraryMenuMemory(LibraryTypePtr lib, int menuidx)
{
	LibraryMenuTypePtr menu = lib->Menu;

	if (menu[menuidx].Name != NULL)
		free(menu[menuidx].Name);
	if (menu[menuidx].directory != NULL)
		free(menu[menuidx].directory);

	lib->MenuN--;
	memmove(menu + menuidx, menu + menuidx + 1, sizeof(LibraryMenuType) * (lib->MenuN - menuidx));
}

/* ---------------------------------------------------------------------------
 * get next slot for a library entry, allocates memory if necessary
 */
LibraryEntryTypePtr GetLibraryEntryMemory(LibraryMenuTypePtr Menu)
{
	LibraryEntryTypePtr entry = Menu->Entry;

	/* realloc new memory if necessary and clear it */
	if (Menu->EntryN >= Menu->EntryMax) {
		Menu->EntryMax += STEP_LIBRARYENTRY;
		entry = (LibraryEntryTypePtr) realloc(entry, Menu->EntryMax * sizeof(LibraryEntryType));
		Menu->Entry = entry;
		memset(entry + Menu->EntryN, 0, STEP_LIBRARYENTRY * sizeof(LibraryEntryType));
	}
	return (entry + Menu->EntryN++);
}


/* ---------------------------------------------------------------------------
 * releases the memory that's allocated by the library
 */
void FreeLibraryMemory(LibraryTypePtr lib)
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
	memset(lib, 0, sizeof(LibraryType));
}

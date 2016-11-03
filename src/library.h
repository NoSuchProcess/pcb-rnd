/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_LIBRARY_H
#define PCB_LIBRARY_H

#include "global_typedefs.h"

typedef struct LibraryEntryTpye_s  LibraryEntryType, *LibraryEntryTypePtr;
typedef struct LibraryMenuType_s   LibraryMenuType, *LibraryMenuTypePtr;

/* ---------------------------------------------------------------------------
 * structure used by library routines
 */
struct LibraryEntryTpye_s {
	const char *ListEntry;				/* the string for the selection box */
	int ListEntry_dontfree;       /* do not free(ListEntry) if non-zero */
	/* This used to contain some char *AllocatedMemory, possibly with
	 * the intention of the following fields pointing into it.
	 * It was never used that way, so removing for now.
	 * TODO: re-introduce and actually use it for the following fields?
	 */
	const char *Package;	 				/* package */
	const char *Value;						/* the value field */
	const char *Description;			/* some descriptive text */
#if 0
	fp_type_t Type;
	void **Tags;									/* an array of void * tag IDs; last tag ID is NULL */
#endif
};

/* If the internal flag is set, the only field that is valid is Name,
   and the struct is allocated with malloc instead of
   CreateLibraryEntry.  These "internal" entries are used for
   electrical paths that aren't yet assigned to a real net.  */

struct LibraryMenuType_s {
	char *Name,										/* name of the menu entry */
	 *directory,									/* Directory name library elements are from */
	 *Style;											/* routing style */
	pcb_cardinal_t EntryN,							/* number of objects */
	  EntryMax;										/* number of reserved memory locations */
	LibraryEntryTypePtr Entry;		/* the entries */
	char flag;										/* used by the netlist window to enable/disable nets */
	char internal;								/* if set, this is an internal-only entry, not
																   part of the global netlist. */
};

typedef struct {
	pcb_cardinal_t MenuN;								/* number of objects */
	pcb_cardinal_t MenuMax;							/* number of reserved memory locations */
	LibraryMenuTypePtr Menu;			/* the entries */
} LibraryType, *LibraryTypePtr;

#endif

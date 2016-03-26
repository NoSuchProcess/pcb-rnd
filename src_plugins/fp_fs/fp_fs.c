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

#include "config.h"
#include "global.h"

#include <stdlib.h>
#include <string.h>

#include "mymem.h"
#include "data.h"
#include "paths.h"
#include "plugins.h"
#include "plug_footprint.h"

/* ---------------------------------------------------------------------------
 * Parse the directory tree where newlib footprints are found
 */

/* Helper function for ParseLibraryTree */
static const char *pcb_basename(const char *p)
{
	char *rv = strrchr(p, '/');
	if (rv)
		return rv + 1;
	return p;
}

typedef struct list_dir_s list_dir_t;

struct list_dir_s {
	char *parent;
	char *subdir;
	list_dir_t *next;
};

typedef struct {
	LibraryMenuTypePtr menu;
	list_dir_t *subdirs;
	int children;
} list_st_t;


static int list_cb(void *cookie, const char *subdir, const char *name, pcb_fp_type_t type, void *tags[])
{
	list_st_t *l = (list_st_t *) cookie;
	LibraryEntryTypePtr entry;		/* Pointer to individual menu entry */
	size_t len;

	if (type == PCB_FP_DIR) {
		list_dir_t *d;
		/* can not recurse directly from here because that would ruin the menu
		   pointer: GetLibraryMenuMemory (&Library) calls realloc()! 
		   Build a list of directories to be visited later, instead. */
		d = malloc(sizeof(list_dir_t));
		d->subdir = strdup(name);
		d->parent = strdup(subdir);
		d->next = l->subdirs;
		l->subdirs = d;
		return 0;
	}

	l->children++;
	entry = GetLibraryEntryMemory(l->menu);

	/* 
	 * entry->AllocatedMemory points to abs path to the footprint.
	 * entry->ListEntry points to fp name itself.
	 */
	len = strlen(subdir) + strlen("/") + strlen(name) + 8;
	entry->AllocatedMemory = (char *) calloc(1, len);
	strcat(entry->AllocatedMemory, subdir);
	strcat(entry->AllocatedMemory, PCB_DIR_SEPARATOR_S);

	/* store pointer to start of footprint name */
	entry->ListEntry = entry->AllocatedMemory + strlen(entry->AllocatedMemory);
	entry->ListEntry_dontfree = 1;

	/* Now place footprint name into AllocatedMemory */
	strcat(entry->AllocatedMemory, name);

	if (type == PCB_FP_PARAMETRIC)
		strcat(entry->AllocatedMemory, "()");

	entry->Type = type;

	entry->Tags = tags;

	return 0;
}

static int fp_fs_load_dir_(const char *subdir, const char *toppath, int is_root)
{
	LibraryMenuTypePtr menu = NULL;	/* Pointer to PCB's library menu structure */
	list_st_t l;
	list_dir_t *d, *nextd;
	char working_[MAXPATHLEN + 1];
	char *working;								/* String holding abs path to working dir */
	int menuidx;

	sprintf(working_, "%s%c%s", toppath, PCB_DIR_SEPARATOR_C, subdir);
	resolve_path(working_, &working);

	/* Get pointer to memory holding menu */
	menu = GetLibraryMenuMemory(&Library, &menuidx);


	/* Populate menuname and path vars */
	menu->Name = strdup(pcb_basename(subdir));
	menu->directory = strdup(pcb_basename(toppath));

	l.menu = menu;
	l.subdirs = NULL;
	l.children = 0;

	pcb_fp_list(working, 0, list_cb, &l, is_root, 1);

	/* now recurse to each subdirectory mapped in the previous call;
	   by now we don't care if menu is ruined by the realloc() in GetLibraryMenuMemory() */
	for (d = l.subdirs; d != NULL; d = nextd) {
		l.children += fp_fs_load_dir_(d->subdir, d->parent, 0);
		nextd = d->next;
		free(d->subdir);
		free(d->parent);
		free(d);
	}
	if (l.children == 0) {
		DeleteLibraryMenuMemory(&Library, menuidx);
	}
	free(working);
	return l.children;
}


static int fp_fs_load_dir(plug_fp_t *ctx, const char *path)
{
	return fp_fs_load_dir_(".", path, 1);
}


static plug_fp_t fp_fs;

pcb_uninit_t hid_fp_fs_init(void)
{
	fp_fs.plugin_data = NULL;
	fp_fs.load_dir = fp_fs_load_dir;
	HOOK_REGISTER(plug_fp_t, plug_fp_chain, &fp_fs);

	return NULL;
}

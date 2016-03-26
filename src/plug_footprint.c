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

//#include <locale.h>
#include "global.h"
#include <dirent.h>
#include <sys/stat.h>

#include "data.h"
#include "file.h"
#include "mymem.h"
#include "libpcb_fp.h"
#include "paths.h"


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

int LoadNewlibFootprintsFromDir(const char *subdir, const char *toppath, int is_root)
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
		l.children += LoadNewlibFootprintsFromDir(d->subdir, d->parent, 0);
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



/* This function loads the newlib footprints into the Library.
 * It examines all directories pointed to by Settings.LibraryTree.
 * In each directory specified there, it looks both in that directory,
 * as well as *one* level down.  It calls the subfunction 
 * LoadNewlibFootprintsFromDir to put the footprints into PCB's internal
 * datastructures.
 */
static int ParseLibraryTree(void)
{
	char toppath[MAXPATHLEN + 1];	/* String holding abs path to top level library dir */
	char *libpaths;								/* String holding list of library paths to search */
	char *p;											/* Helper string used in iteration */
	DIR *dirobj;									/* Iterable directory object */
	struct dirent *direntry = NULL;	/* Object holding individual directory entries */
	struct stat buffer;						/* buffer used in stat */
	int n_footprints = 0;					/* Running count of footprints found */

	/* Initialize path, working by writing 0 into every byte. */
	memset(toppath, 0, sizeof toppath);

	/* Additional loop to allow for multiple 'newlib' style library directories 
	 * called out in Settings.LibraryTree
	 */
	libpaths = strdup(Settings.LibrarySearchPaths);
	for (p = strtok(libpaths, PCB_PATH_DELIMETER); p && *p; p = strtok(NULL, PCB_PATH_DELIMETER)) {
		/* remove trailing path delimeter */
		strncpy(toppath, p, sizeof(toppath) - 1);

#ifdef DEBUG
		printf("In ParseLibraryTree, looking for newlib footprints inside top level directory %s ... \n", toppath);
#endif

		/* Next read in any footprints in the top level dir */
		n_footprints += LoadNewlibFootprintsFromDir(".", toppath, 1);
	}

#ifdef DEBUG
	printf("Leaving ParseLibraryTree, found %d footprints.\n", n_footprints);
#endif

	free(libpaths);
	return n_footprints;
}

int ReadLibraryContents(void)
{
	FILE *resultFP = NULL;

	/* List all footprint libraries.  Then sort the whole
	 * library.
	 */
	if (ParseLibraryTree() > 0 || resultFP != NULL) {
		sort_library(&Library);
		return 0;
	}

	return (1);
}

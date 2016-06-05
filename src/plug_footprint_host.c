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


/* Glue between plug_footrpint and pcb-rnd; this is a separate file so a
   different glue can be used with gsch2pcb */

#include <dirent.h>
#include <sys/stat.h>

#include "config.h"
#include "conf_core.h"

#include "plug_footprint.h"
#include "plugins.h"
#include "error.h"

const char *fp_get_library_shell(void)
{
	return conf_core.rc.library_shell;
}

/* This function loads the newlib footprints into the Library.
 * It examines all directories pointed to by the search paths
 * (normally Settings.LibraryTree).
 * In each directory specified there, it looks both in that directory,
 * as well as *one* level down.  It calls the subfunction 
 * fp_fs_load_dir to put the footprints into PCB's internal
 * datastructures.
 */
static int fp_read_lib_all_(const char *searchpath)
{
	char toppath[MAXPATHLEN + 1];	/* String holding abs path to top level library dir */
	char *libpaths;								/* String holding list of library paths to search */
	char *p;											/* Helper string used in iteration */
	DIR *dirobj;									/* Iterable directory object */
	struct dirent *direntry = NULL;	/* Object holding individual directory entries */
	struct stat buffer;						/* buffer used in stat */
	int n_footprints = 0;					/* Running count of footprints found */
	int res;

	/* Initialize path, working by writing 0 into every byte. */
	memset(toppath, 0, sizeof toppath);

	/* Additional loop to allow for multiple 'newlib' style library directories 
	 * called out in Settings.LibraryTree
	 */
	libpaths = strdup(searchpath);
	for (p = strtok(libpaths, PCB_PATH_DELIMETER); p && *p; p = strtok(NULL, PCB_PATH_DELIMETER)) {
		/* remove trailing path delimeter */
		strncpy(toppath, p, sizeof(toppath) - 1);

#ifdef DEBUG
		printf("In ParseLibraryTree, looking for newlib footprints inside top level directory %s ... \n", toppath);
#endif

		/* Next read in any footprints in the top level dir */
		res = -1;
		HOOK_CALL(plug_fp_t, plug_fp_chain, load_dir, res, >= 0, toppath);
		if (res >= 0)
			n_footprints += res;
		else
			Message("Warning: footprint library list error on %s\n", toppath);
	}

#ifdef DEBUG
	printf("Leaving ParseLibraryTree, found %d footprints.\n", n_footprints);
#endif

	free(libpaths);
	return n_footprints;
}

int fp_read_lib_all(void)
{
	FILE *resultFP = NULL;

	/* List all footprint libraries.  Then sort the whole
	 * library.
	 */
	if (fp_read_lib_all_(conf_core.rc.library_search_paths) > 0 || resultFP != NULL) {
		fp_sort_children(&library);
		return 0;
	}

	return (1);
}

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
#include "error.h"
#include "plug_footprint.h"
#include "plugins.h"

#include "../src_3rd/genht/htsp.h"
#include "../src_3rd/genht/hash.h"

plug_fp_t *plug_fp_chain = NULL;

/* This function loads the newlib footprints into the Library.
 * It examines all directories pointed to by Settings.LibraryTree.
 * In each directory specified there, it looks both in that directory,
 * as well as *one* level down.  It calls the subfunction 
 * fp_fs_load_dir to put the footprints into PCB's internal
 * datastructures.
 */
static int fp_read_lib_all_(void)
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
	libpaths = strdup(Settings.LibrarySearchPaths);
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
	if (fp_read_lib_all_() > 0 || resultFP != NULL) {
		sort_library(&Library);
		return 0;
	}

	return (1);
}

int fp_dupname(const char *name, char **basename, char **params)
{
	char *newname, *s;

	*basename = newname = strdup(name);
	s = strchr(newname, '(');
	if (s == NULL) {
		*params = NULL;
		return 0;
	}

	/* split at '(' */
	*s = '\0';
	*params = s + 1;
	s = *params + strlen(*params) - 1;

	/* strip ')' */
	if (*s == ')')
		*s = '\0';

	return 1;
}

static htsp_t *fp_tags = NULL;

static int keyeq(char *a, char *b)
{
	return !strcmp(a, b);
}

const void *fp_tag(const char *tag, int alloc)
{
	htsp_entry_t *e;
	static int counter = 0;

	if (fp_tags == NULL)
		fp_tags = htsp_alloc(strhash, keyeq);
	e = htsp_getentry(fp_tags, (char *) tag);
	if ((e == NULL) && alloc) {
		htsp_set(fp_tags, strdup(tag), (void *) counter);
		counter++;
		e = htsp_getentry(fp_tags, (char *) tag);
	}
	return e == NULL ? NULL : e->key;
}

void fp_uninit()
{
	htsp_entry_t *e;
	for (e = htsp_first(fp_tags); e; e = htsp_next(fp_tags, e))
		free(e->key);
	htsp_free(fp_tags);
	fp_tags = NULL;
}

const char *fp_tagname(const void *tagid)
{
	return (char *) tagid;
}

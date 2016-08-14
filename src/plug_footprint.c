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

#include "plug_footprint.h"
#include "plugins.h"

#include <genht/htsp.h>
#include <genht/hash.h>
#include "conf_core.h"
#include "plugins.h"
#include "error.h"
#include "compat_misc.h"

plug_fp_t *plug_fp_chain = NULL;
library_t library;

int fp_dupname(const char *name, char **basename, char **params)
{
	char *newname, *s;

	*basename = newname = pcb_strdup(name);
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

const void *fp_tag(const char *tag, int alloc)
{
	htsp_entry_t *e;
	static char *counter = NULL;

	if (fp_tags == NULL)
		fp_tags = htsp_alloc(strhash, strkeyeq);
	e = htsp_getentry(fp_tags, (char *) tag);
	if ((e == NULL) && alloc) {
		htsp_set(fp_tags, pcb_strdup(tag), (void *) counter);
		counter++;
		e = htsp_getentry(fp_tags, (char *) tag);
	}
	return e == NULL ? NULL : e->key;
}

void fp_init()
{
	library.type = LIB_DIR;
	library.name = "/";
}

void fp_uninit()
{
	htsp_entry_t *e;
	fp_free_children(&library);
	if (fp_tags != NULL) {
		for (e = htsp_first(fp_tags); e; e = htsp_next(fp_tags, e))
			free(e->key);
		htsp_free(fp_tags);
		fp_tags = NULL;
	}
}

const char *fp_tagname(const void *tagid)
{
	return (char *) tagid;
}

FILE *fp_fopen(const char *path, const char *name, fp_fopen_ctx_t *fctx)
{
	FILE *res = NULL;
	HOOK_CALL(plug_fp_t, plug_fp_chain, fopen, res, != NULL, path, name, fctx);
	return res;
}

void fp_fclose(FILE * f, fp_fopen_ctx_t *fctx)
{
	if (fctx->backend->fclose != NULL)
		fctx->backend->fclose(fctx->backend, f, fctx);
}

library_t *fp_append_entry(library_t *parent, const char *name, fp_type_t type, void *tags[])
{
	library_t *entry;   /* Pointer to individual menu entry */

	assert(parent->type == LIB_DIR);
	entry = get_library_memory(parent);
	if (entry == NULL)
		return NULL;

	if (type == PCB_FP_PARAMETRIC) {
		/* concat name and () */
		/* do not use pcb_strdup_printf or Concat here, do not increase gsch2pcb-rnd deps */
		int nl = strlen(name);
		entry->name = malloc(nl+3);
		memcpy(entry->name, name, nl);
		strcpy(entry->name+nl, "()");
	}
	else
		entry->name = pcb_strdup(name);

	entry->type = LIB_FOOTPRINT;
	entry->data.fp.type = type;
	entry->data.fp.tags = tags;
	entry->data.fp.loc_info = NULL;
	entry->data.fp.backend_data = NULL;
	return entry;
}

library_t *fp_lib_search_len(library_t *dir, const char *name, int name_len)
{
	library_t *l;
	int n;

	if (dir->type != LIB_DIR)
		return NULL;

	for(n = 0, l = dir->data.dir.children.array; n < dir->data.dir.children.used; n++, l++)
		if (strncmp(l->name, name, name_len) == 0)
			return l;

	return NULL;
}

library_t *fp_lib_search(library_t *dir, const char *name)
{
	library_t *l;
	int n;

	if (dir->type != LIB_DIR)
		return NULL;

	for(n = 0, l = dir->data.dir.children.array; n < dir->data.dir.children.used; n++, l++)
		if (strcmp(l->name, name) == 0)
			return l;

	return NULL;
}


library_t *fp_mkdir_len(library_t *parent, const char *name, int name_len)
{
	library_t *l = get_library_memory(parent);

	if (name_len > 0)
		l->name = pcb_strndup(name, name_len);
	else
		l->name = pcb_strdup(name);
	l->parent = parent;
	l->type = LIB_DIR;
	vtlib_init(&l->data.dir.children);
	return l;
}

library_t *fp_mkdir_p(const char *path)
{
	library_t *l, *parent = NULL;
	const char *next;

	/* search for the last existing dir in the path */
	
	while(*path == '/') path++;
	for(parent = l = &library; l != NULL; parent = l,path = next) {
		next = strchr(path, '/');
		if (next == NULL)
			l = fp_lib_search(l, path);
		else
			l = fp_lib_search_len(l, path, next-path);

		/* skip path sep */
		if (next != NULL) {
			while(*next == '/') next++;
			if (*next == '\0')
				next = NULL;
		}

		/* last elem of the path */
		if (next == NULL) {
			if (l != NULL)
				return l;
			break; /* found a non-existing node */
		}
		
		if (l == NULL)
			break;
	}

	/* by now path points to the first non-existing dir, under parent */
	for(;path != NULL; path = next) {
		next = strchr(path, '/');
		parent = fp_mkdir_len(parent, path, next-path);
		if (next != NULL) {
			while(*next == '/') next++;
			if (*next == '\0')
				next = NULL;
		}
	}

	return parent;
}

void fp_sort_children(library_t *parent)
{
/*	int i;
	qsort(lib->Menu, lib->MenuN, sizeof(lib->Menu[0]), netlist_sort);
	for (i = 0; i < lib->MenuN; i++)
		qsort(lib->Menu[i].Entry, lib->Menu[i].EntryN, sizeof(lib->Menu[i].Entry[0]), netnode_sort);*/
}

void fp_free_entry(library_t *l)
{
	switch(l->type) {
		case LIB_DIR:
			fp_free_children(l);
			vtlib_uninit(&(l->data.dir.children));
			break;
		case LIB_FOOTPRINT:
			if (l->data.fp.loc_info != NULL)
				free(l->data.fp.loc_info);
			if (l->data.fp.tags != NULL)
				free(l->data.fp.tags);
			break;
		case LIB_INVALID: break; /* suppress compiler warning */
	}
	if (l->name != NULL) {
		free(l->name);
		l->name = NULL;
	}
	l->type = LIB_INVALID;
}

void fp_free_children(library_t *parent)
{
	int n;
	library_t *l;

	assert(parent->type == LIB_DIR);

	for(n = 0, l = parent->data.dir.children.array; n < parent->data.dir.children.used; n++, l++)
		fp_free_entry(l);

	vtlib_truncate(&(parent->data.dir.children), 0);
}


void fp_rmdir(library_t *dir)
{
	library_t *l, *parent = dir->parent;
	int n;
	fp_free_entry(dir);
	if (parent != NULL) {
		for(n = 0, l = parent->data.dir.children.array; n < parent->data.dir.children.used; n++,l++) {
			if (l == dir) {
				vtlib_remove(&(parent->data.dir.children), n, 1);
				break;
			}
		}
	}
}

/* Debug functions */
void fp_dump_dir(library_t *dir, int level)
{
	library_t *l;
	int n, p;

	for(n = 0, l = dir->data.dir.children.array; n < dir->data.dir.children.used; n++, l++) {
		for(p = 0; p < level; p++)
			putchar(' ');
		if (l->type == LIB_DIR) {
			printf("%s/\n", l->name);
			fp_dump_dir(l, level+1);
		}
		else if (l->type == LIB_FOOTPRINT)
			printf("%s\n", l->name);
		else
			printf("*INVALID*\n");
	}
}

void fp_dump()
{
	fp_dump_dir(&library, 0);
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
	int n_footprints = 0;					/* Running count of footprints found */
	int res;

	/* Initialize path, working by writing 0 into every byte. */
	memset(toppath, 0, sizeof toppath);

	/* Additional loop to allow for multiple 'newlib' style library directories 
	 * called out in Settings.LibraryTree
	 */
	libpaths = pcb_strdup(searchpath);
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

static gds_t fpds_paths;
static int fpds_inited = 0;

const char *fp_default_search_path(void)
{
	return conf_concat_strlist(&conf_core.rc.library_search_paths, &fpds_paths, &fpds_inited, ':');
}

int fp_host_uninit(void)
{
	if (fpds_inited)
		gds_uninit(&fpds_paths);
	return 0;
}

int fp_read_lib_all(void)
{
	FILE *resultFP = NULL;

	/* List all footprint libraries.  Then sort the whole
	 * library.
	 */
	if (fp_read_lib_all_(fp_default_search_path()) > 0 || resultFP != NULL) {
		fp_sort_children(&library);
		return 0;
	}

	return (1);
}

int fp_rehash(void)
{
	fp_free_children(&library);
	return fp_read_lib_all();
}

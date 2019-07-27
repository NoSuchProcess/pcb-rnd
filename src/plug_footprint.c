/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include "plug_footprint.h"
#include "plugins.h"

#include <genht/htsp.h>
#include <genht/hash.h>
#include "conf_core.h"
#include "error.h"
#include "compat_misc.h"
#include "event.h"

FILE PCB_FP_FOPEN_IN_DST_, *PCB_FP_FOPEN_IN_DST = &PCB_FP_FOPEN_IN_DST_;


pcb_plug_fp_t *pcb_plug_fp_chain = NULL;
pcb_fplibrary_t pcb_library;

int pcb_fp_dupname(const char *name, char **basename, char **params)
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

const void *pcb_fp_tag(const char *tag, int alloc)
{
	htsp_entry_t *e;
	static char *counter = NULL;

	if (fp_tags == NULL)
		fp_tags = htsp_alloc(strhash, strkeyeq);
	e = htsp_getentry(fp_tags, tag);
	if ((e == NULL) && alloc) {
		htsp_set(fp_tags, pcb_strdup(tag), (void *) counter);
		counter++;
		e = htsp_getentry(fp_tags, tag);
	}
	return e == NULL ? NULL : e->key;
}

void pcb_fp_init()
{
	pcb_library.type = LIB_DIR;
	pcb_library.name = pcb_strdup("/");  /* All names are eventually free()'d */
}

void pcb_fp_uninit()
{
	htsp_entry_t *e;

	if (pcb_plug_fp_chain != NULL)
		pcb_message(PCB_MSG_ERROR, "pcb_plug_fp_chain is not empty; a plugin did not remove itself from the chain. Fix your plugins!\n");

	pcb_fp_free_children(&pcb_library);
	if (fp_tags != NULL) {
		for (e = htsp_first(fp_tags); e; e = htsp_next(fp_tags, e))
			free(e->key);
		htsp_free(fp_tags);
		fp_tags = NULL;
	}
	free(pcb_library.name);
	pcb_library.name = NULL;
}

const char *pcb_fp_tagname(const void *tagid)
{
	return (char *) tagid;
}

FILE *pcb_fp_fopen(const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst)
{
	FILE *res = NULL;
	if (strchr(path, ':') != NULL)  {
		char *tmp, *next, *curr;

		curr = tmp = pcb_strdup(path);
		while((res == NULL) && (curr != NULL)) {
			next = strchr(curr, ':');
			if (next != NULL) {
				*next= '\0';
				next++;
			}

			PCB_HOOK_CALL(pcb_plug_fp_t, pcb_plug_fp_chain, fp_fopen, res, != NULL, (self, curr, name, fctx, dst));
			curr = next;
		}
		free(tmp);
	}
	else
		PCB_HOOK_CALL(pcb_plug_fp_t, pcb_plug_fp_chain, fp_fopen, res, != NULL, (self, path, name, fctx, dst));
	return res;
}

void pcb_fp_fclose(FILE * f, pcb_fp_fopen_ctx_t *fctx)
{
	if ((fctx->backend != NULL) && (fctx->backend->fp_fclose != NULL))
		fctx->backend->fp_fclose(fctx->backend, f, fctx);
}

pcb_fplibrary_t *pcb_fp_append_entry(pcb_fplibrary_t *parent, const char *name, pcb_fptype_t type, void *tags[])
{
	pcb_fplibrary_t *entry;   /* Pointer to individual menu entry */

	assert(parent->type == LIB_DIR);
	entry = pcb_get_library_memory(parent);
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

pcb_fplibrary_t *fp_lib_search_len(pcb_fplibrary_t *dir, const char *name, int name_len)
{
	pcb_fplibrary_t *l;
	int n;

	if (dir->type != LIB_DIR)
		return NULL;

	for(n = 0, l = dir->data.dir.children.array; n < dir->data.dir.children.used; n++, l++)
		if (strncmp(l->name, name, name_len) == 0)
			return l;

	return NULL;
}

pcb_fplibrary_t *pcb_fp_lib_search(pcb_fplibrary_t *dir, const char *name)
{
	pcb_fplibrary_t *l;
	int n;

	if (dir->type != LIB_DIR)
		return NULL;

	for(n = 0, l = dir->data.dir.children.array; n < dir->data.dir.children.used; n++, l++)
		if (strcmp(l->name, name) == 0)
			return l;

	return NULL;
}


pcb_fplibrary_t *pcb_fp_mkdir_len(pcb_fplibrary_t *parent, const char *name, int name_len)
{
	pcb_fplibrary_t *l = pcb_get_library_memory(parent);

	if (name_len > 0)
		l->name = pcb_strndup(name, name_len);
	else
		l->name = pcb_strdup(name);
	l->parent = parent;
	l->type = LIB_DIR;
	l->data.dir.backend = NULL;
	vtlib_init(&l->data.dir.children);
	return l;
}

pcb_fplibrary_t *pcb_fp_mkdir_p(const char *path)
{
	pcb_fplibrary_t *l, *parent = NULL;
	const char *next;

	/* search for the last existing dir in the path */

	while(*path == '/') path++;
	for(parent = l = &pcb_library; l != NULL; parent = l,path = next) {
		next = strchr(path, '/');
		if (next == NULL)
			l = pcb_fp_lib_search(l, path);
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
		parent = pcb_fp_mkdir_len(parent, path, next-path);
		if (next != NULL) {
			while(*next == '/') next++;
			if (*next == '\0')
				next = NULL;
		}
	}

	return parent;
}

static int fp_sort_cb(const void *a, const void *b)
{
	const pcb_fplibrary_t *fa = a, *fb = b;
	int res = strcmp(fa->name, fb->name);
	return res == 0 ? 1 : res;
}

void pcb_fp_sort_children(pcb_fplibrary_t *parent)
{
	vtlib_t *v;
	int n;

	if (parent->type != LIB_DIR)
		return;

	v = &parent->data.dir.children;
	qsort(v->array, vtlib_len(v), sizeof(pcb_fplibrary_t), fp_sort_cb);

	for (n = 0; n < vtlib_len(v); n++)
		pcb_fp_sort_children(&v->array[n]);
}

void fp_free_entry(pcb_fplibrary_t *l)
{
	switch(l->type) {
		case LIB_DIR:
			pcb_fp_free_children(l);
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

void pcb_fp_free_children(pcb_fplibrary_t *parent)
{
	int n;
	pcb_fplibrary_t *l;

	assert(parent->type == LIB_DIR);

	for(n = 0, l = parent->data.dir.children.array; n < parent->data.dir.children.used; n++, l++)
		fp_free_entry(l);

	vtlib_truncate(&(parent->data.dir.children), 0);
}


void pcb_fp_rmdir(pcb_fplibrary_t *dir)
{
	pcb_fplibrary_t *l, *parent = dir->parent;
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
void fp_dump_dir(pcb_fplibrary_t *dir, int level)
{
	pcb_fplibrary_t *l;
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
	fp_dump_dir(&pcb_library, 0);
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
	char *toppath, toppath_[PCB_PATH_MAX + 1];  /* String holding abs path to top level library dir */
	char *libpaths;        /* String holding list of library paths to search */
	char *p;               /* Helper string used in iteration */
	int n_footprints = 0;  /* Running count of footprints found */
	int res;

	/* Additional loop to allow for multiple 'newlib' style library directories
	 * called out in Settings.LibraryTree
	 */
	libpaths = pcb_strdup(searchpath);
	for (p = strtok(libpaths, PCB_PATH_DELIMETER); p && *p; p = strtok(NULL, PCB_PATH_DELIMETER)) {
		int silent_fail = 0;

		/* remove trailing path delimiter */
		strncpy(toppath_, p, sizeof(toppath_) - 1);
		toppath_[sizeof(toppath_) - 1] = '\0';

		/* make paths starting with '?' optional (silently fail) */
		toppath = toppath_;
		if (toppath[0] == '?') {
			toppath++;
			silent_fail = 1;
		}

#ifdef DEBUG
		printf("In ParseLibraryTree, looking for newlib footprints inside top level directory %s ... \n", toppath);
#endif

		/* Next read in any footprints in the top level dir */
		res = -1;
		PCB_HOOK_CALL(pcb_plug_fp_t, pcb_plug_fp_chain, load_dir, res, >= 0, (self, toppath, 0));
		if (res >= 0)
			n_footprints += res;
		else if (!silent_fail)
			pcb_message(PCB_MSG_WARNING, "Warning: footprint library list error on %s\n", toppath);
	}

#ifdef DEBUG
	printf("Leaving ParseLibraryTree, found %d footprints.\n", n_footprints);
#endif

	free(libpaths);
	return n_footprints;
}

static gds_t fpds_paths;
static int fpds_inited = 0;

const char *pcb_fp_default_search_path(void)
{
	return pcb_conf_concat_strlist(&conf_core.rc.library_search_paths, &fpds_paths, &fpds_inited, ':');
}

int pcb_fp_host_uninit(void)
{
	if (fpds_inited)
		gds_uninit(&fpds_paths);
	return 0;
}

int pcb_fp_read_lib_all(void)
{
	FILE *resultFP = NULL;

	/* List all footprint libraries.  Then sort the whole
	 * library.
	 */
	if (fp_read_lib_all_(pcb_fp_default_search_path()) > 0 || resultFP != NULL) {
		pcb_fp_sort_children(&pcb_library);
		return 0;
	}

	return 1;
}

int pcb_fp_rehash(pcb_hidlib_t *hidlib, pcb_fplibrary_t *l)
{
	pcb_plug_fp_t *be;
	char *path;
	int res;

	if (l == NULL) {
		pcb_fp_free_children(&pcb_library);
		res = pcb_fp_read_lib_all();
		pcb_event(hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);
		return res;
	}
	if (l->type != LIB_DIR)
		return -1;

	be = l->data.dir.backend;
	if ((be == NULL) || (be->load_dir == NULL))
		return -1;

	path = pcb_strdup(l->name);
	pcb_fp_rmdir(l);
	res = be->load_dir(be, path, 1);
	pcb_fp_sort_children(&pcb_library);
	free(path);

	if (res >= 0) {
		pcb_event(hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);
		return 0;
	}
	return -1;
}

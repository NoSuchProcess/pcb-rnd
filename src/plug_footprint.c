/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <ctype.h>

#include "plug_footprint.h"
#include <librnd/core/plugins.h>

#include <genht/htsp.h>
#include <genht/hash.h>
#include "conf_core.h"
#include "actions_pcb.h"
#include "plug_io.h"
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include "event.h"

static const int PCB_FP_FOPEN_IN_DST_;

/* This is safe because PCB_FP_FOPEN_IN_DST is never dereferenced */
FILE *PCB_FP_FOPEN_IN_DST = (FILE *)&PCB_FP_FOPEN_IN_DST_;


pcb_plug_fp_t *pcb_plug_fp_chain = NULL;
pcb_fplibrary_t pcb_library;

static pcb_fplibrary_t *pcb_get_library_memory(pcb_fplibrary_t *parent)
{
	pcb_fplibrary_t *res;
	vtp0_t *vt = ((parent) == NULL ? &pcb_library.data.dir.children : &(parent)->data.dir.children);

	res = calloc(sizeof(pcb_fplibrary_t), 1);
	res->parent = parent;
	vtp0_append(vt, res);

	return res;
}


int pcb_fp_dupname(const char *name, char **basename, char **params)
{
	char *newname, *s;

	*basename = newname = rnd_strdup(name);
	s = strchr(newname, '(');
	if (s == NULL) {
		*params = NULL;
		return 0;
	}

	/* split at '(' */
	*s = '\0';
	*params = s + 1;

	/* strip ')' */
	s = *params + strlen(*params) - 1;
	while(isspace(*s) && (s > *params)) s--;
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
		htsp_set(fp_tags, rnd_strdup(tag), (void *) counter);
		counter++;
		e = htsp_getentry(fp_tags, tag);
	}
	return e == NULL ? NULL : e->key;
}

void pcb_fp_init()
{
	pcb_library.type = PCB_LIB_DIR;
	pcb_library.name = rnd_strdup("/");  /* All names are eventually free()'d */
}

void pcb_fp_uninit()
{
	htsp_entry_t *e;

	if (pcb_plug_fp_chain != NULL)
		rnd_message(RND_MSG_ERROR, "pcb_plug_fp_chain is not empty; a plugin did not remove itself from the chain. Fix your plugins!\n");

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

FILE *pcb_fp_fopen(const rnd_conflist_t *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst)
{
	rnd_conf_listitem_t *ci;
	FILE *res = NULL;
	char *sep = strstr(name, "::");

	if (sep != NULL) {
		long offs = sep - name;
		fctx->filename = rnd_strdup(name);
		sep = (char *)fctx->filename + offs;
		*sep = '\0';
		fctx->free_filename = 0;
		fctx->subfpname = sep+2;
		fctx->free_filename = 1;
	}
	else {
		fctx->filename = name;
		fctx->subfpname = NULL;
		fctx->free_filename = 0;
	}

	for(ci = rnd_conflist_first((rnd_conflist_t *)path); ci != NULL; ci = rnd_conflist_next(ci)) {
		const char *curr = ci->val.string[0];
		RND_HOOK_CALL(pcb_plug_fp_t, pcb_plug_fp_chain, fp_fopen, res, != NULL, (self, curr, fctx->filename, fctx, dst));
		if (res != NULL)
			return res;
	}

	fctx->backend = NULL;
	return NULL;
}

void pcb_fp_fclose(FILE * f, pcb_fp_fopen_ctx_t *fctx)
{
	if ((fctx->backend != NULL) && (fctx->backend->fp_fclose != NULL))
		fctx->backend->fp_fclose(fctx->backend, f, fctx);
	if (fctx->free_filename)
		free((char *)fctx->filename);
	fctx->filename = NULL;
	fctx->free_filename = 0;
}

static void **pcb_fp_dup_tags(void *tags[])
{
	long len = 0;
	void **src, **dst, **res;

	if (tags == NULL)
		return NULL;

	for(src = tags; *src != NULL; src++) len++;

	res = malloc(sizeof(void *) * (len+1));

	for(src = tags, dst = res; *src != NULL; src++, dst++)
		*dst = *src; /* these are coming from the central hash, it is safe to keep them */

	*dst = NULL;
	return res;
}

pcb_fplibrary_t *pcb_fp_append_entry(pcb_fplibrary_t *parent, const char *name, pcb_fptype_t type, void *tags[], rnd_bool dup_tags)
{
	pcb_fplibrary_t *entry;   /* Pointer to individual menu entry */

	assert(parent->type == PCB_LIB_DIR);
	entry = pcb_get_library_memory(parent);
	if (entry == NULL)
		return NULL;

	if (type == PCB_FP_PARAMETRIC) {
		/* concat name and () */
		/* do not use rnd_strdup_printf or Concat here, do not increase gsch2pcb-rnd deps */
		int nl = strlen(name);
		entry->name = malloc(nl+3);
		memcpy(entry->name, name, nl);
		strcpy(entry->name+nl, "()");
	}
	else
		entry->name = rnd_strdup(name);

	entry->type = PCB_LIB_FOOTPRINT;
	entry->data.fp.type = type;
	entry->data.fp.tags = dup_tags ? pcb_fp_dup_tags(tags) : tags;
	entry->data.fp.loc_info = NULL;
	entry->data.fp.backend_data = NULL;
	return entry;
}

pcb_fplibrary_t *fp_lib_search_len(pcb_fplibrary_t *dir, const char *name, int name_len)
{
	int n;

	if (dir->type != PCB_LIB_DIR)
		return NULL;

	for(n = 0; n < dir->data.dir.children.used; n++) {
		pcb_fplibrary_t *l = dir->data.dir.children.array[n];
		if (strncmp(l->name, name, name_len) == 0)
			return l;
	}

	return NULL;
}

pcb_fplibrary_t *pcb_fp_lib_search(pcb_fplibrary_t *dir, const char *name)
{
	int n;

	if (dir->type != PCB_LIB_DIR)
		return NULL;

	for(n = 0; n < dir->data.dir.children.used; n++) {
	pcb_fplibrary_t *l = dir->data.dir.children.array[n];
		if (strcmp(l->name, name) == 0)
			return l;
	}

	return NULL;
}


pcb_fplibrary_t *pcb_fp_mkdir_len(pcb_fplibrary_t *parent, const char *name, int name_len)
{
	pcb_fplibrary_t *l = pcb_get_library_memory(parent);

	if (name_len > 0)
		l->name = rnd_strndup(name, name_len);
	else
		l->name = rnd_strdup(name);
	l->type = PCB_LIB_DIR;
	l->data.dir.backend = NULL;
	vtp0_init(&l->data.dir.children);
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
	pcb_fplibrary_t * const *fa = a, * const *fb = b;
	int res = strcmp((*fa)->name, (*fb)->name);
	return res == 0 ? 1 : res;
}

void pcb_fp_sort_children(pcb_fplibrary_t *parent)
{
	vtp0_t *v;
	int n;

	if (parent->type != PCB_LIB_DIR)
		return;

	v = &parent->data.dir.children;
	qsort(v->array, vtp0_len(v), sizeof(pcb_fplibrary_t *), fp_sort_cb);

	for (n = 0; n < vtp0_len(v); n++)
		pcb_fp_sort_children(v->array[n]);
}

void fp_free_entry(pcb_fplibrary_t *l)
{
	switch(l->type) {
		case PCB_LIB_DIR:
			pcb_fp_free_children(l);
			vtp0_uninit(&(l->data.dir.children));
			break;
		case PCB_LIB_FOOTPRINT:
			if (l->data.fp.loc_info != NULL)
				free(l->data.fp.loc_info);
			if (l->data.fp.tags != NULL)
				free(l->data.fp.tags);
			break;
		case PCB_LIB_INVALID: break; /* suppress compiler warning */
	}
	if (l->name != NULL)
		free(l->name);
	free(l);
}

void pcb_fp_free_children(pcb_fplibrary_t *parent)
{
	int n;

	assert(parent->type == PCB_LIB_DIR);

	for(n = 0; n < parent->data.dir.children.used; n++) {
		pcb_fplibrary_t *l = parent->data.dir.children.array[n];
		fp_free_entry(l);
	}

	vtp0_truncate(&(parent->data.dir.children), 0);
}


void pcb_fp_rmdir(pcb_fplibrary_t *dir)
{
	pcb_fplibrary_t *l, *parent = dir->parent;
	int n;
	fp_free_entry(dir);
	if (parent != NULL) {
		for(n = 0; n < parent->data.dir.children.used; n++) {
			l = parent->data.dir.children.array[n];
			if (l == dir) {
				vtp0_remove(&(parent->data.dir.children), n, 1);
				break;
			}
		}
	}
}

/* Debug functions */
void fp_dump_dir(pcb_fplibrary_t *dir, int level)
{
	int n, p;

	for(n = 0; n < dir->data.dir.children.used; n++) {
		pcb_fplibrary_t *l = dir->data.dir.children.array[0];

		for(p = 0; p < level; p++)
			putchar(' ');
		if (l->type == PCB_LIB_DIR) {
			printf("%s/\n", l->name);
			fp_dump_dir(l, level+1);
		}
		else if (l->type == PCB_LIB_FOOTPRINT)
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
 * (usually from conf node rc/library_search_paths)
 * In each directory specified there, it looks both in that directory,
 * as well as *one* level down.  It calls the subfunction
 * fp_fs_load_dir to put the footprints into PCB's internal
 * datastructures.
 */
static int fp_read_lib_all_(const rnd_conflist_t *searchpath)
{
	rnd_conf_listitem_t *ci;
	char *toppath, toppath_[RND_PATH_MAX + 1];  /* String holding abs path to top level library dir */
	int n_footprints = 0;  /* Running count of footprints found */
	int res;

	for(ci = rnd_conflist_first((rnd_conflist_t *)searchpath); ci != NULL; ci = rnd_conflist_next(ci)) {
		const char *p = ci->val.string[0];
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
		RND_HOOK_CALL(pcb_plug_fp_t, pcb_plug_fp_chain, load_dir, res, >= 0, (self, toppath, 0));
		if (res >= 0)
			n_footprints += res;
		else if (!silent_fail)
			rnd_message(RND_MSG_WARNING, "Warning: footprint library list error on %s\n", toppath);
	}

#ifdef DEBUG
	printf("Leaving ParseLibraryTree, found %d footprints.\n", n_footprints);
#endif

	return n_footprints;
}

static gds_t fpds_paths;
static int fpds_inited = 0;

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
	if (fp_read_lib_all_(&conf_core.rc.library_search_paths) > 0 || resultFP != NULL) {
		pcb_fp_sort_children(&pcb_library);
		return 0;
	}

	return 1;
}

int pcb_fp_rehash(rnd_hidlib_t *hidlib, pcb_fplibrary_t *l)
{
	pcb_plug_fp_t *be;
	char *path;
	int res;

	if (l == NULL) {
		pcb_fp_free_children(&pcb_library);
		res = pcb_fp_read_lib_all();
		rnd_event(hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);
		return res;
	}
	if (l->type != PCB_LIB_DIR)
		return -1;

	be = l->data.dir.backend;
	if ((be == NULL) || (be->load_dir == NULL))
		return -1;

	path = rnd_strdup(l->name);
	pcb_fp_rmdir(l);
	res = be->load_dir(be, path, 1);
	pcb_fp_sort_children(&pcb_library);
	free(path);

	if (res >= 0) {
		rnd_event(hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);
		return 0;
	}
	return -1;
}

const char *PCB_PTR_DOMAIN_FPMAP = "pcb_fgw_ptr_domain_fpmap";

const char *pcb_fp_map_choose(rnd_hidlib_t *hidlib, const pcb_plug_fp_map_t *map)
{
	int numfp;
	const pcb_plug_fp_map_t *bestm = NULL, *m;

	/* if there's only one choice, don't present the dialog */
	for(numfp = 0, m = map; m != NULL; m = m->next) {
		if (m->type == PCB_FP_FILE) {
			numfp++;
			bestm = m;
		}
	}
	if (numfp == 1)
		return bestm->name;


	if (RND_HAVE_GUI_ATTR_DLG) {
		fgw_arg_t res;
		fgw_arg_t args[2];

		fgw_ptr_reg(&rnd_fgw, &args[1], PCB_PTR_DOMAIN_FPMAP, FGW_PTR | FGW_STRUCT, (void *)map);
		rnd_actionv_bin(hidlib, "gui_fpmap_choose", &res, 2, args);
		fgw_ptr_unreg(&rnd_fgw, &args[1], PCB_PTR_DOMAIN_FPMAP);

		if (res.type & FGW_STR)
			return res.val.str;
	}
	else {
		rnd_message(RND_MSG_ERROR, "No gui available, automatically choosing the first footprint from the available footprints.\n");
		return map->name;
	}

	return NULL;
}

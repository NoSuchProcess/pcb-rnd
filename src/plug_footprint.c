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

#include "data.h"
#include "file.h"
#include "mymem.h"
#include "paths.h"
#include "plug_footprint.h"
#include "plugins.h"

#include "../src_3rd/genht/htsp.h"
#include "../src_3rd/genht/hash.h"

plug_fp_t *plug_fp_chain = NULL;

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

FILE *fp_fopen(const char *path, const char *name, fp_fopen_ctx_t *fctx)
{
	FILE *res = NULL;
	HOOK_CALL(plug_fp_t, plug_fp_chain, fopen, res, != NULL, path, name, fctx);
}

void fp_fclose(FILE * f, fp_fopen_ctx_t *fctx)
{
	if (fctx->backend->fclose != NULL)
		fctx->backend->fclose(fctx->backend, f, fctx);
}

LibraryEntryType *fp_append_entry(LibraryMenuType *parent, const char *dirname, const char *name, fp_type_t type, void *tags[])
{
	LibraryEntryType *entry;   /* Pointer to individual menu entry */
	size_t len;

	entry = GetLibraryEntryMemory(parent);

	/* 
	 * entry->AllocatedMemory points to abs path to the footprint.
	 * entry->ListEntry points to fp name itself.
	 */
	len = strlen(dirname) + strlen("/") + strlen(name) + 8;
	entry->AllocatedMemory = (char *) calloc(1, len);
	strcat(entry->AllocatedMemory, dirname);
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
}

LibraryMenuType *fp_append_topdir(const char *parent_dir, const char *dir_name, int *menuidx)
{
	LibraryMenuType *menu;

	/* Get pointer to memory holding menu */
	menu = GetLibraryMenuMemory(&Library, menuidx);

	/* Populate menuname and path vars */
	menu->Name = strdup(dir_name);
	menu->directory = strdup(parent_dir);

	return menu;
}


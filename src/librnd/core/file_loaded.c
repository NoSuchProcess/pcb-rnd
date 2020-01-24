/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include <assert.h>
#include <genht/hash.h>
#include "file_loaded.h"
#include "compat_misc.h"

htsp_t pcb_file_loaded;

pcb_file_loaded_t *pcb_file_loaded_category(const char *name, int alloc)
{
	pcb_file_loaded_t *cat = htsp_get(&pcb_file_loaded, name);

	if ((cat == NULL) && (alloc)) {
		cat = calloc(sizeof(pcb_file_loaded_t), 1);
		cat->type = PCB_FLT_CATEGORY;
		cat->name = pcb_strdup(name);
		htsp_init(&cat->data.category.children, strhash, strkeyeq);
		htsp_set(&pcb_file_loaded, cat->name, cat);
	}
	return cat;
}

int pcb_file_loaded_file_free(pcb_file_loaded_t *file)
{
	free(file->data.file.path);
	free(file->data.file.desc);
	free(file->name);
	free(file);
	return 0;
}

int pcb_file_loaded_clear(pcb_file_loaded_t *cat)
{
	htsp_entry_t *e;

	assert(cat->type == PCB_FLT_CATEGORY);

	for (e = htsp_first(&cat->data.category.children); e; e = htsp_next(&cat->data.category.children, e)) {
		pcb_file_loaded_file_free(e->value);
		htsp_delentry(&cat->data.category.children, e);
	}
	return 0;
}

int pcb_file_loaded_clear_at(const char *catname)
{
	pcb_file_loaded_t *cat = pcb_file_loaded_category(catname, 0);
	if (cat != NULL)
		return pcb_file_loaded_clear(cat);
	return 0;
}

int pcb_file_loaded_set(pcb_file_loaded_t *cat, const char *name, const char *path, const char *desc)
{
	pcb_file_loaded_t *file;

	assert(cat->type == PCB_FLT_CATEGORY);
	file = htsp_get(&cat->data.category.children, name);
	if (file != NULL) {
		free(file->data.file.path);
		free(file->data.file.desc);
	}
	else {
		file = malloc(sizeof(pcb_file_loaded_t));
		file->type = PCB_FLT_FILE;
		file->name = pcb_strdup(name);
		htsp_set(&cat->data.category.children, file->name, file);
	}
	if (path != NULL)
		file->data.file.path = pcb_strdup(path);
	else
		file->data.file.path = NULL;

	if (desc != NULL)
		file->data.file.desc = pcb_strdup(desc);
	else
		file->data.file.desc = NULL;
	return 0;
}

int pcb_file_loaded_set_at(const char *catnam, const char *name, const char *path, const char *desc)
{
	pcb_file_loaded_t *cat = pcb_file_loaded_category(catnam, 1);
	return pcb_file_loaded_set(cat, name, path, desc);
}

int pcb_file_loaded_del(pcb_file_loaded_t *cat, const char *name)
{
	pcb_file_loaded_t *file;
	assert(cat->type == PCB_FLT_CATEGORY);
	file = htsp_pop(&cat->data.category.children, name);
	if (file != NULL) {
		if (file->type != PCB_FLT_FILE)
			return -1;
		pcb_file_loaded_file_free(file);
	}
	return 0;
}

int pcb_file_loaded_del_at(const char *catname, const char *name)
{
	pcb_file_loaded_t *cat = pcb_file_loaded_category(catname, 1);
	return pcb_file_loaded_del(cat, name);
}

void pcb_file_loaded_init(void)
{
	htsp_init(&pcb_file_loaded, strhash, strkeyeq);
}

void pcb_file_loaded_uninit(void)
{
	htsp_entry_t *e;

	for (e = htsp_first(&pcb_file_loaded); e; e = htsp_next(&pcb_file_loaded, e)) {
		pcb_file_loaded_t *cat = e->value;
		pcb_file_loaded_clear(cat);
		free(cat->name);
		htsp_uninit(&cat->data.category.children);
		free(cat);
		htsp_delentry(&pcb_file_loaded, e);
	}

	htsp_uninit(&pcb_file_loaded);
}



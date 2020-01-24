/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* info on where files are loaded from */

#ifndef PCB_FILE_LOADED_H
#define PCB_FILE_LOADED_H

#include <genht/htsp.h>

/* Hash-in-hash tree with the top tree being categories. */

typedef enum pcb_file_loaded_type_e {
	PCB_FLT_CATEGORY,
	PCB_FLT_FILE
} pcb_file_loaded_type_t;

typedef struct pcb_file_loaded_s pcb_file_loaded_t;

struct pcb_file_loaded_s {
	char *name; /* same as the hash key; strdup'd */
	pcb_file_loaded_type_t type;
	union {
		struct {
			htsp_t children;
		} category;
		struct {
			char *path;
			char *desc;
		} file;
	} data;
};

extern htsp_t pcb_file_loaded;

/* Return the category called name; if doesn't exist and alloc is 1,
   allocate it it (else return NULL) */
pcb_file_loaded_t *pcb_file_loaded_category(const char *name, int alloc);

/* clear the subtree of a category, keeping the category; return 0 on success */
int pcb_file_loaded_clear(pcb_file_loaded_t *cat);
int pcb_file_loaded_clear_at(const char *catname);

/* clear the subtree of a category, keeping the category; return 0 on success */
int pcb_file_loaded_set(pcb_file_loaded_t *cat, const char *name, const char *path, const char *desc);
int pcb_file_loaded_set_at(const char *catname, const char *name, const char *path, const char *desc);

/* remove an entry */
int pcb_file_loaded_del(pcb_file_loaded_t *cat, const char *name);
int pcb_file_loaded_del_at(const char *catname, const char *name);


/* called once, from main */
void pcb_file_loaded_init(void);
void pcb_file_loaded_uninit(void);

#endif

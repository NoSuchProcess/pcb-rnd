/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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

/* attribute lists */

#ifndef PCB_ATTRIB_H
#define PCB_ATTRIB_H

typedef struct pcb_attribute_list_s pcb_attribute_list_t;

typedef struct pcb_attribute_s {
	char *name;
	char *value;
	unsigned cpb_written:1; /* copyback: written */
} pcb_attribute_t;

struct pcb_attribute_list_s {
	int Number, Max;
	pcb_attribute_t *List;
	void (*post_change)(pcb_attribute_list_t *list, const char *name, const char *value); /* called after put or remove, but not after free(); value is NULL if removed */
};

/* Returns NULL if the name isn't found, else the value for that named
   attribute.  */
char *pcb_attribute_get(const pcb_attribute_list_t *list, const char *name);

/* Adds an attribute to the list.  If the attribute already exists, the value
   is replaced. Returns non-zero if an existing attribute was replaced.  */
int pcb_attribute_put(pcb_attribute_list_t * list, const char *name, const char *value);
/* Simplistic version: Takes a pointer to an object, looks up attributes in it.  */
#define pcb_attrib_get(OBJ,name) pcb_attribute_get(&(OBJ->Attributes), name)
/* Simplistic version: Takes a pointer to an object, sets attributes in it.  */
#define pcb_attrib_put(OBJ,name,value) pcb_attribute_put(&(OBJ->Attributes), name, value)
/* Remove an attribute by name; returns number of items removed  */
int pcb_attribute_remove(pcb_attribute_list_t * list, const char *name);
/* Simplistic version of Remove.  */
#define pcb_attrib_remove(OBJ, name) pcb_attribute_remove(&(OBJ->Attributes), name)

/* remove item by index - WARNING: no checks are made, idx has to be valid! */
int pcb_attribute_remove_idx(pcb_attribute_list_t * list, int idx);

/* Frees memory used by an attribute list */
void pcb_attribute_free(pcb_attribute_list_t *list);

/* Copy each attribute from src to dest */
void pcb_attribute_copy_all(pcb_attribute_list_t *dest, const pcb_attribute_list_t *src);

/* Copy back a mirrored attribute list, minimizing the changes */
void pcb_attribute_copyback_begin(pcb_attribute_list_t *dst);
void pcb_attribute_copyback(pcb_attribute_list_t *dst, const char *name, const char *value);
void pcb_attribute_copyback_end(pcb_attribute_list_t *dst);

/* Set the intconn attribute - hack for compatibility parsing */
void pcb_attrib_compat_set_intconn(pcb_attribute_list_t *dst, int intconn);

#endif

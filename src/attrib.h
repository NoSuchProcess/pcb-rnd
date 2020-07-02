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

#ifndef RND_ATTRIB_H
#define RND_ATTRIB_H

typedef struct rnd_attribute_list_s rnd_attribute_list_t;

typedef struct rnd_attribute_s {
	char *name;
	char *value;
	unsigned cpb_written:1; /* copyback: written */
} rnd_attribute_t;

struct rnd_attribute_list_s {
	int Number, Max;
	rnd_attribute_t *List;
	void (*post_change)(rnd_attribute_list_t *list, const char *name, const char *value); /* called any time an attribute changes (including removes); value is NULL if removed; old value is free'd only after the call so cached old values are valid */
};

/* Returns NULL if the name isn't found, else the value for that named
   attribute. The ptr version returns the address of the value str in the slot */
char *rnd_attribute_get(const rnd_attribute_list_t *list, const char *name);
char **rnd_attribute_get_ptr(const rnd_attribute_list_t *list, const char *name);

/* Same as rnd_attribute_get, but also look for plugin::key which overrides
   plain key hits */
char *rnd_attribute_get_namespace(const rnd_attribute_list_t *list, const char *plugin, const char *key);
char **rnd_attribute_get_namespace_ptr(const rnd_attribute_list_t *list, const char *plugin, const char *key);


/* Adds an attribute to the list.  If the attribute already exists, the value
   is replaced. Returns non-zero if an existing attribute was replaced.  */
int rnd_attribute_put(rnd_attribute_list_t * list, const char *name, const char *value);
/* Simplistic version: Takes a pointer to an object, looks up attributes in it.  */
#define rnd_attrib_get(OBJ,name) rnd_attribute_get(&(OBJ->Attributes), name)
/* Simplistic version: Takes a pointer to an object, sets attributes in it.  */
#define rnd_attrib_put(OBJ,name,value) rnd_attribute_put(&(OBJ->Attributes), name, value)
/* Remove an attribute by name; returns number of items removed  */
int rnd_attribute_remove(rnd_attribute_list_t * list, const char *name);
/* Simplistic version of Remove.  */
#define rnd_attrib_remove(OBJ, name) rnd_attribute_remove(&(OBJ->Attributes), name)

/* remove item by index - WARNING: no checks are made, idx has to be valid! */
int rnd_attribute_remove_idx(rnd_attribute_list_t * list, int idx);

/* Frees memory used by an attribute list */
void rnd_attribute_free(rnd_attribute_list_t *list);

/* Copy each attribute from src to dest */
void rnd_attribute_copy_all(rnd_attribute_list_t *dest, const rnd_attribute_list_t *src);

/* Copy back a mirrored attribute list, minimizing the changes */
void rnd_attribute_copyback_begin(rnd_attribute_list_t *dst);
void rnd_attribute_copyback(rnd_attribute_list_t *dst, const char *name, const char *value);
void rnd_attribute_copyback_end(rnd_attribute_list_t *dst);

/* Set the intconn attribute - hack for compatibility parsing */
void rnd_attrib_compat_set_intconn(rnd_attribute_list_t *dst, int intconn);

#endif

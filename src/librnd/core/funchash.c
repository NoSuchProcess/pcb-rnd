/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <genht/htpi.h>
#include <genht/hash.h>
#include <librnd/config.h>
#include <librnd/core/funchash.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/misc_util.h>

typedef struct {
	const char *cookie;
	const char *key;
	char key_buff[1];
} fh_key_t;

static htpi_t *funchash;

static int keyeq(const void *a_, const void *b_)
{
	const fh_key_t *a = a_, *b = b_;
	if (a->cookie != b->cookie)
		return 1;
	return !pcb_strcasecmp(a->key, b->key);
}

static unsigned fh_hash(const void *key)
{
	const fh_key_t *k = key;
	return strhash_case((char *)k->key) ^ ptrhash((void *)k->cookie);
}

#include <librnd/core/funchash_core.h>
#define action_entry(x) { #x, F_ ## x},
static pcb_funchash_table_t rnd_functions[] = {
#include <librnd/core/funchash_core_list.h>
	{"F_RND_END", F_RND_END}
};


void pcb_funchash_init(void)
{
	funchash = htpi_alloc(fh_hash, keyeq);
	pcb_funchash_set_table(rnd_functions, PCB_ENTRIES(rnd_functions), NULL);
}

void pcb_funchash_uninit(void)
{
	htpi_entry_t *e;

	for (e = htpi_first(funchash); e; e = htpi_next(funchash, e)) {
		fh_key_t *k = e->key;
		if (k->cookie != NULL)
			fprintf(stderr, "WARNING: function not removed: %s::%s\n", k->cookie, k->key);
		htpi_pop(funchash, e->key);
		free(e->key);
	}
	htpi_free(funchash);
	funchash = NULL;
}

void pcb_funchash_remove_cookie(const char *cookie)
{
	htpi_entry_t *e;

	/* Slow linear search - probably OK, this will run only on uninit */
	for (e = htpi_first(funchash); e; e = htpi_next(funchash, e)) {
		fh_key_t *k = e->key;
		if (k->cookie == cookie) {
			htpi_pop(funchash, e->key);
			free(e->key);
		}
	}
}

int pcb_funchash_get(const char *key, const char *cookie)
{
	fh_key_t new_key;
	htpi_entry_t *e;

	if (key == NULL)
		return -1;

	new_key.cookie = cookie;
	new_key.key = key;

	e = htpi_getentry(funchash, &new_key);
	if (e == NULL)
		return -1;
	return e->value;
}

int pcb_funchash_set(const char *key, int val, const char *cookie)
{
	fh_key_t *new_key;
	int kl;

	if (pcb_funchash_get(key, cookie) >= 0)
		return -1;

	kl = strlen(key);
	new_key = malloc(sizeof(fh_key_t) + kl);
	new_key->cookie = cookie;
	new_key->key = new_key->key_buff;
	strcpy(new_key->key_buff, key);
	htpi_set(funchash, new_key, val);
	return 0;
}

int pcb_funchash_set_table(pcb_funchash_table_t *table, int numelem, const char *cookie)
{
	int i, rv = 0;

	for (i = 0; i < numelem; i++)
		rv |= pcb_funchash_set(table[i].key, table[i].val, cookie);

	return rv;
}

const char *pcb_funchash_reverse(int id)
{
	htpi_entry_t *e;

	for (e = htpi_first(funchash); e; e = htpi_next(funchash, e)) {
		if (e->value == id) {
			fh_key_t *k = e->key;
			return k->key;
		}
	}
	return NULL;
}

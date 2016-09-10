/*
 *                            COPYRIGHT
 *
 *  PCB-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <genht/htpi.h>
#include <genht/hash.h>
#include "funchash_core.h"
#include "macro.h"

#define action_entry(x) { #x, F_ ## x},
static funchash_table_t Functions[] = {
#include "funchash_core_list.h"
	{"F_END", F_END}
};

typedef struct {
	const char *cookie;
	const char *key;
	char key_buff[1];
} fh_key_t;

static htpi_t *funchash;

static int keyeq(void *a_, void *b_)
{
	fh_key_t *a = a_, *b = b_;
	if (a->cookie != b->cookie)
		return 1;
	return !strcasecmp(a->key, b->key);
}

static unsigned fh_hash(void *key)
{
	fh_key_t *k = key;
	return strhash_case((char *)k->key) ^ ptrhash((void *)k->cookie);
}

void funchash_init(void)
{
	funchash = htpi_alloc(fh_hash, keyeq);
	funchash_set_table(Functions, ENTRIES(Functions), NULL);
}

void funchash_uninit(void)
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

void funchash_remove_cookie(const char *cookie)
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

int funchash_get(const char *key, const char *cookie)
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

int funchash_set(const char *key, int val, const char *cookie)
{
	fh_key_t *new_key;
	int kl;

	if (funchash_get(key, cookie) >= 0)
		return -1;

	kl = strlen(key);
	new_key = malloc(sizeof(fh_key_t) + kl);
	new_key->cookie = cookie;
	new_key->key = new_key->key_buff;
	strcpy(new_key->key_buff, key);
	htpi_set(funchash, new_key, val);
	return 0;
}

int funchash_set_table(funchash_table_t *table, int numelem, const char *cookie)
{
	int i, rv = 0;

	for (i = 0; i < numelem; i++)
		rv |= funchash_set(table[i].key, table[i].val, cookie);

	return rv;
}

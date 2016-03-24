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
#include <genht/htsi.h>
#include <genht/hash.h>
#include "funchash_core.h"
#include "macro.h"

#define MAXKEYLEN 256

#define action_entry(x) { #x, F_ ## x},
static funchash_table_t Functions[] = {
#include "funchash_core_list.h"
	{"F_END", F_END}
};

static htsi_t *funchash;

static int keyeq(char *a, char *b)
{
	return !strcmp(a, b);
}

void funchash_init(void)
{
	funchash = htsi_alloc(strhash, keyeq);
	funchash_set_table(Functions, ENTRIES(Functions), NULL);
}

void funchash_uninit(void)
{

}

/* A key is either a plain string (cookie == NULL) or an aggregate of
   cookie and the key - this guarantees each cookie has its own namespace */
#define asm_key(dest, buff, key, cookie, badretval) \
do { \
	if (cookie != NULL) { \
		int __lk__; \
		__lk__ = strlen(key); \
		if (strlen(key)+sizeof(int)*2+6 > sizeof(buff)) \
			return badretval; \
		sprintf(buff, "%p::%s", cookie, key); \
		new_key = buff; \
	} \
	else \
		new_key = (char *)key; \
} while(0)


int funchash_get(const char *key, const char *cookie)
{
	char buff[MAXKEYLEN];
	char *new_key;
	htsi_entry_t *e;

	if (key == NULL)
		return -1;

	asm_key(new_key, buff, key, cookie, -1);
	e = htsi_getentry(funchash, new_key);
	if (e == NULL)
		return -1;
	return e->value;
}

int funchash_set(const char *key, int val, const char *cookie)
{
	char buff[MAXKEYLEN];
	char *new_key;
	htsi_entry_t *e;

	asm_key(new_key, buff, key, cookie, -1);
	e = htsi_getentry(funchash, new_key);
	if (e != NULL)
		return -1;

	htsi_set(funchash, strdup(new_key), val);
	return 0;
}

int funchash_set_table(funchash_table_t *table, int numelem, const char *cookie)
{
	int i, rv = 0;

	for (i = 0; i < numelem; i++)
		rv |= funchash_set(table[i].key, table[i].val, cookie);

	return rv;
}

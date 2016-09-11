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

/* Table entry format for funchash_set_table() */
typedef struct {
	char *key;
	int val;
} funchash_table_t;

/* Cookie is the namespace so that different modules can use the same
   function names with different integer IDs without interference. Core
   should use cookie==NULL. */

/* Resolve a key string into an integer ID */
int funchash_get(const char *key, const char *cookie);

/* Store key string - integer ID pair */
int funchash_set(const char *key, int val, const char *cookie);

/* Store multiple key strings - integer ID pairs using a table */
int funchash_set_table(funchash_table_t *table, int numelem, const char *cookie);

/* Remove all keys inserted for a cookie */
void funchash_remove_cookie(const char *cookie);

/* Init-ununit the hash */
void funchash_init(void);
void funchash_uninit(void);

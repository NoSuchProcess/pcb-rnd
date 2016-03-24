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

int funchash_get(const char *key, const char *cookie);
int funchash_set(const char *key, int val, const char *cookie);
int funchash_set_table(funchash_table_t *table, int numelem, const char *cookie);

void funchash_remove_cookie(const char *cookie);

void funchash_init(void);
void funchash_uninit(void);

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Register custom (plugin) config file names, reference counted. These files
   are loaded from system and user config paths */

#include <genht/htsi.h>

static htsi_t conf_files;
static int conf_files_inited = 0;

static void conf_files_init(void)
{
	htsi_init(&conf_files, strhash, strkeyeq);
	conf_files_inited = 1;
}

void conf_reg_file(const char *path, const char *intern)
{
	htsi_entry_t *e;
	if (!conf_files_inited) conf_files_init();
	e = htsi_getentry(&conf_files, path);
	if (e == NULL)
		htsi_set(&conf_files, pcb_strdup(path), 1);
	else
		e->value++;
}

void conf_unreg_file(const char *path)
{
	htsi_entry_t *e;
	assert(conf_files_inited);
	if (!conf_files_inited) return;

	e = htsi_getentry(&conf_files, path);
	assert(e != NULL);
	if (e == NULL) return;

	e->value--;
	if (e->value == 0) {
		htsi_delentry(&conf_files, e);
		free(e->key);
	}
}


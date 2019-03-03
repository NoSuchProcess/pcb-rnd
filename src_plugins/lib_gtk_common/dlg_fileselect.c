/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <gtk/gtk.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include "hid.h"
#include "compat_misc.h"

static htsp_t history;
static int inited = 0;

#define MAX_HIST 8
typedef struct file_history_s {
	char *fn[MAX_HIST];
} file_history_t;

char *pcb_gtk_fileselect2(GtkWidget *top_window, const char *title, const char *descr, const char *default_file, const char *default_ext, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub)
{
	file_history_t *hi;

	if (!inited) {
		htsp_init(&history, strhash, strkeyeq);
		inited = 1;
	}

	if ((history_tag != NULL) && (*history_tag != '\0')) {
		hi = htsp_get(&history, history_tag);
		if (hi == NULL) {
			hi = calloc(sizeof(file_history_t), 1);
			htsp_set(&history, pcb_strdup(history_tag), hi);
		}
	}

	return NULL;
}

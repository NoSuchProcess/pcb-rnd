/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

/* GUI for parametric footprints parameter exploration */

#include "config.h"

#define _BSD_SOURCE
#include <stdio.h>

#include "dlg_library_param.h"

char *pcb_gtk_library_param_ui(pcb_gtk_library_t *library_window, pcb_fplibrary_t *entry)
{
	FILE *f;
	char *cmd, line[1024];

	printf("Not yet\n");

	cmd = pcb_strdup_printf("%s --help", entry->data.fp.loc_info);
	f = popen(cmd, "r");
	free(cmd);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not execute parametric footprint %s\n", entry->data.fp.loc_info);
		return NULL;
	}
	while(fgets(line, sizeof(line), f) > 0) {
		printf("line=%s\n", line);
	}
	pclose(f);
	return NULL;
}


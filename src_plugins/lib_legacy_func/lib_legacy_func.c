/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include <genvector/gds_char.h>
#include <stdio.h>
#include "config.h"
#include <errno.h>
#include "data.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "safe_fs.h"

char *ExpandFilename(char *Dirname, char *Filename)
{
	gds_t answer;
	char *command;
	FILE *pipe;
	int c;

	/* allocate memory for commandline and build it */
	gds_init(&answer);
	if (Dirname) {
		command = (char *) calloc(strlen(Filename) + strlen(Dirname) + 7, sizeof(char));
		sprintf(command, "echo %s/%s", Dirname, Filename);
	}
	else {
		command = (char *) calloc(strlen(Filename) + 6, sizeof(char));
		sprintf(command, "echo %s", Filename);
	}

	/* execute it with shell */
	if ((pipe = pcb_popen(&PCB->hidlib, command, "r")) != NULL) {
		/* discard all but the first returned line */
		for (;;) {
			if ((c = fgetc(pipe)) == EOF || c == '\n' || c == '\r')
				break;
			else
				gds_append(&answer, c);
		}

		free(command);
		if (pcb_pclose(pipe)) {
			gds_uninit(&answer);
			return NULL;
		}
		else
			return answer.array;
	}

	/* couldn't be expanded by the shell */
	pcb_popen_error_message(command);
	free(command);
	gds_uninit(&answer);
	return NULL;
}

void CreateQuotedString(gds_t *DS, char *S)
{
	gds_truncate(DS, 0);
	gds_append(DS, '"');
	while (*S) {
		if (*S == '"' || *S == '\\')
			gds_append(DS, '\\');
		gds_append(DS, *S++);
	}
	gds_append(DS, '"');
}

/* ---------------------------------------------------------------------------
 * Convenience for plugins using the old {Hide,Restore}pcb_crosshair API.
 * This links up to notify the GUI of the expected changes using the new APIs.
 *
 * Use of this old API is deprecated, as the names don't necessarily reflect
 * what all GUIs may do in response to the notifications. Keeping these APIs
 * is aimed at easing transition to the newer API, they will emit a harmless
 * warning at the time of their first use.
 *
 */
void pcb_crosshair_hide(void)
{
	static pcb_bool warned_old_api = pcb_false;
	if (!warned_old_api) {
		pcb_message(PCB_MSG_WARNING, "WARNING: A plugin is using the deprecated API pcb_crosshair_hide().\n"
							"         This API may be removed in a future release of PCB.\n");
		warned_old_api = pcb_true;
	}

	pcb_notify_crosshair_change(pcb_false);
	pcb_notify_mark_change(pcb_false);
}

void pcb_crosshair_restore(void)
{
	static pcb_bool warned_old_api = pcb_false;
	if (!warned_old_api) {
		pcb_message(PCB_MSG_WARNING, "WARNING: A plugin is using the deprecated API pcb_crosshair_restore().\n"
							"         This API may be removed in a future release of PCB.\n");
		warned_old_api = pcb_true;
	}

	pcb_notify_crosshair_change(pcb_true);
	pcb_notify_mark_change(pcb_true);
}


int pplg_check_ver_lib_legacy_func(int ver_needed) { return 0; }

void pplg_uninit_lib_legacy_func(void)
{
}

int pplg_init_lib_legacy_func(void)
{
	PCB_API_CHK_VER;
	return 0;
}


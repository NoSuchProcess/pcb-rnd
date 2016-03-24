/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include <genvector/gds_char.h>
#include "config.h"
#include "global.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"

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
	if ((pipe = popen(command, "r")) != NULL) {
		/* discard all but the first returned line */
		for (;;) {
			if ((c = fgetc(pipe)) == EOF || c == '\n' || c == '\r')
				break;
			else
				gds_append(&answer, c);
		}

		free(command);
		if (pclose(pipe)) {
			gds_uninit(&answer);
			return NULL;
		}
		else
			return answer.array;
	}

	/* couldn't be expanded by the shell */
	PopenErrorMessage(command);
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

int FileExists(const char *name)
{
	FILE *f;
	f = fopen(name, "r");
	if (f) {
		fclose(f);
		return 1;
	}
	return 0;
}


pcb_uninit_t hid_legacy_func_init(void)
{
	return NULL;
}


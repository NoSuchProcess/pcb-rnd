/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "safe_fs.h"
#include "error.h"

typedef struct stl_facet_s stl_facet_t;

struct stl_facet_s {
	double nx, ny, nz;
	double vx[3], vy[3], vz[3];
	stl_facet_t *next;
};

static char *stl_getline(char *line, int linelen, FILE *f)
{
	char *cmd;
	while((cmd = fgets(line, linelen, f)) != NULL) {
		while(isspace(*cmd)) cmd++;
		if (*cmd == '\0') continue;
		return cmd;
	}
	return NULL;
}

void stl_solid_free(stl_facet_t *head)
{
	stl_facet_t *next;
	for(; head != NULL; head = next) {
		next = head->next;
		free(head);
	}
}

stl_facet_t *stl_solid_load(rnd_hidlib_t *hl, const char *fn)
{
	FILE *f;
	stl_facet_t *head = NULL, *tail = NULL, *t;
	char *cmd, line[512];

	f = rnd_fopen(hl, fn, "r");
	if (f == NULL)
		return NULL;

	/* find the 'solid' header */
	cmd = stl_getline(line, sizeof(line), f);
	if ((cmd == NULL) || (strncmp(cmd, "solid", 5) != 0)) {
		rnd_message(RND_MSG_ERROR, "Invalid stl file: first line is not a 'solid'\n");
		goto err0;
	}

	for(;;) {
		int n;

		cmd = stl_getline(line, sizeof(line), f);
		if (cmd == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: premature end of file in solid\n");
			goto err1;
		}
		if (strncmp(cmd, "endsolid", 8) == 0)
			break; /* normal end of file */

		if (strncmp(cmd, "facet normal", 12) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected facet, got %s\n", cmd);
			goto err1;
		}

		t = malloc(sizeof(stl_facet_t));
		t->next = NULL;
		if (tail != NULL) {
			tail->next = t;
			tail = t;
		}
		else
			head = tail = t;

		cmd += 12;
		if (sscanf(cmd, "%lf %lf %lf", &t->nx, &t->ny, &t->nz) != 3) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: wrong facet normals '%s'\n", cmd);
			goto err1;
		}

		cmd = stl_getline(line, sizeof(line), f);
		if (strncmp(cmd, "outer loop", 10) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected outer loop, got %s\n", cmd);
			goto err1;
		}

		for(n = 0; n < 3; n++) {
			cmd = stl_getline(line, sizeof(line), f);
			if (strncmp(cmd, "vertex", 6) != 0) {
				rnd_message(RND_MSG_ERROR, "Invalid stl file: expected vertex, got %s\n", cmd);
				goto err1;
			}
			cmd+=6;
			if (sscanf(cmd, "%lf %lf %lf", &t->vx[n], &t->vy[n], &t->vz[n]) != 3) {
				rnd_message(RND_MSG_ERROR, "Invalid stl file: wrong facet vertex '%s'\n", cmd);
				goto err1;
			}
		}

		cmd = stl_getline(line, sizeof(line), f);
		if (strncmp(cmd, "endloop", 7) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected endloop, got %s\n", cmd);
			goto err1;
		}

		cmd = stl_getline(line, sizeof(line), f);
		if (strncmp(cmd, "endfacet", 8) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected endfacet, got %s\n", cmd);
			goto err1;
		}
	}


	fclose(f);
	err0:;
	return head;

	err1:;
	stl_solid_free(head);
	fclose(f);
	return NULL;
}

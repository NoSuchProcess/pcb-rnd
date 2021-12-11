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

stl_facet_t *stl_solid_fload(rnd_hidlib_t *hl, FILE *f)
{
	stl_facet_t *head = NULL, *tail = NULL, *t;
	char *cmd, line[512];

	/* find the 'solid' header */
	cmd = stl_getline(line, sizeof(line), f);
	if ((cmd == NULL) || (strncmp(cmd, "solid", 5) != 0)) {
		rnd_message(RND_MSG_ERROR, "Invalid stl file: first line is not a 'solid'\n");
		return NULL;
	}

	for(;;) {
		int n;

		cmd = stl_getline(line, sizeof(line), f);
		if (cmd == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: premature end of file in solid\n");
			goto error;
		}
		if (strncmp(cmd, "endsolid", 8) == 0)
			break; /* normal end of file */

		if (strncmp(cmd, "facet normal", 12) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected facet, got %s\n", cmd);
			goto error;
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
		if (sscanf(cmd, "%lf %lf %lf", &t->n[0], &t->n[1], &t->n[2]) != 3) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: wrong facet normals '%s'\n", cmd);
			goto error;
		}

		cmd = stl_getline(line, sizeof(line), f);
		if (strncmp(cmd, "outer loop", 10) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected outer loop, got %s\n", cmd);
			goto error;
		}

		for(n = 0; n < 3; n++) {
			cmd = stl_getline(line, sizeof(line), f);
			if (strncmp(cmd, "vertex", 6) != 0) {
				rnd_message(RND_MSG_ERROR, "Invalid stl file: expected vertex, got %s\n", cmd);
				goto error;
			}
			cmd+=6;
			if (sscanf(cmd, "%lf %lf %lf", &t->vx[n], &t->vy[n], &t->vz[n]) != 3) {
				rnd_message(RND_MSG_ERROR, "Invalid stl file: wrong facet vertex '%s'\n", cmd);
				goto error;
			}
		}

		cmd = stl_getline(line, sizeof(line), f);
		if (strncmp(cmd, "endloop", 7) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected endloop, got %s\n", cmd);
			goto error;
		}

		cmd = stl_getline(line, sizeof(line), f);
		if (strncmp(cmd, "endfacet", 8) != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid stl file: expected endfacet, got %s\n", cmd);
			goto error;
		}
	}


	return head;

	error:;
	stl_solid_free(head);
	fclose(f);
	return NULL;
}


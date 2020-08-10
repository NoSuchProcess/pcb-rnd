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
#include <math.h>

typedef struct stl_facet_s stl_facet_t;

struct stl_facet_s {
	double n[3];
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

RND_INLINE void v_transform(double dst[3], double src[3], double mx[16])
{
	dst[0] = src[0]*mx[0] + src[1]*mx[4] + src[2]*mx[8]  + mx[12];
	dst[1] = src[0]*mx[1] + src[1]*mx[5] + src[2]*mx[9]  + mx[13];
	dst[2] = src[0]*mx[2] + src[1]*mx[6] + src[2]*mx[10] + mx[14];
}

static const double mx_ident[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1 };

RND_INLINE void mx_rot_x(double mx_dst[16], double a)
{
	double s = sin(a), c = cos(a);
	mx_dst[ 0] = 1; mx_dst[ 1] =  0; mx_dst[ 2] = 0; mx_dst[ 3] = 0;
	mx_dst[ 4] = 0; mx_dst[ 5] =  c; mx_dst[ 6] = s; mx_dst[ 7] = 0;
	mx_dst[ 8] = 0; mx_dst[ 9] = -s; mx_dst[10] = c; mx_dst[11] = 0;
	mx_dst[12] = 0; mx_dst[13] =  0; mx_dst[14] = 0; mx_dst[15] = 1;
}


RND_INLINE void mx_rot_y(double mx_dst[16], double a)
{
	double s = sin(a), c = cos(a);

	mx_dst[ 0] = c;  mx_dst[ 1] = 0; mx_dst[ 2] = -s; mx_dst[ 3] = 0;
	mx_dst[ 4] = 0;  mx_dst[ 5] = 1; mx_dst[ 6] =  0; mx_dst[ 7] = 0;
	mx_dst[ 8] = s;  mx_dst[ 9] = 0; mx_dst[10] =  c; mx_dst[11] = 0;
	mx_dst[12] = 0;  mx_dst[13] = 0; mx_dst[14] =  0; mx_dst[15] = 1;
}

RND_INLINE void mx_rot_z(double mx_dst[16], double a)
{
	double s = sin(a), c = cos(a);
	mx_dst[ 0] =  c; mx_dst[ 1] = s; mx_dst[ 2] = 0; mx_dst[ 3] = 0;
	mx_dst[ 4] = -s; mx_dst[ 5] = c; mx_dst[ 6] = 0; mx_dst[ 7] = 0;
	mx_dst[ 8] =  0; mx_dst[ 9] = 0; mx_dst[10] = 1; mx_dst[11] = 0;
	mx_dst[12] =  0; mx_dst[13] = 0; mx_dst[14] = 0; mx_dst[15] = 1;
}

RND_INLINE void mx_xlate(double mx_dst[16], double x, double y, double z)
{
	mx_dst[ 0] = 1; mx_dst[ 1] = 0; mx_dst[ 2] = 0; mx_dst[ 3] = 0;
	mx_dst[ 4] = 0; mx_dst[ 5] = 1; mx_dst[ 6] = 0; mx_dst[ 7] = 0;
	mx_dst[ 8] = 0; mx_dst[ 9] = 0; mx_dst[10] = 1; mx_dst[11] = 0;
	mx_dst[12] = x; mx_dst[13] = y; mx_dst[14] = z; mx_dst[15] = 1;
}

RND_INLINE void mx_mult(double dst[16], double a[16], double b[16])
{
	int n, m, k, l;
	for(n = 0; n<16; n++) {
		dst[n] = 0;
		for(m = 0, k = n & 3, l = n & 12; m < 4; m++, k += 4, l++)
			dst[n] = dst[n] + b[k] * a[l];
	}
}

void stl_solid_print_facets(FILE *f, stl_facet_t *head, double rotx, double roty, double rotz, double xlatex, double xlatey, double xlatez)
{
	double mxn[16], mx[16], tmp[16], tmp2[16];

	memcpy(mx, mx_ident, sizeof(mx_ident));
	if (rotx != 0) { mx_rot_x(tmp, rotx); mx_mult(tmp2, mx, tmp); memcpy(mx, tmp2, sizeof(tmp2)); }
	if (roty != 0) { mx_rot_y(tmp, roty); mx_mult(tmp2, mx, tmp); memcpy(mx, tmp2, sizeof(tmp2)); }
	if (rotz != 0) { mx_rot_z(tmp, rotz); mx_mult(tmp2, mx, tmp); memcpy(mx, tmp2, sizeof(tmp2)); }
	memcpy(mxn, mx, sizeof(mx));
	if ((xlatex != 0) || (xlatey != 0) || (xlatez != 0)) {
		mx_xlate(tmp, xlatex, xlatey, xlatez);
		mx_mult(tmp2, mx, tmp);
		memcpy(mx, tmp2, sizeof(tmp2));
	}

	for(; head != NULL; head = head->next) {
		double v[3], p[3];
		int n;
		v_transform(v, head->n, mxn);
		fprintf(f, " facet normal %f %f %f\n", v[0], -v[1], v[2]);
		fprintf(f, "  outer loop\n");
		for(n = 0; n < 3; n++) {
			p[0] = head->vx[n]; p[1] = head->vy[n]; p[2] = head->vz[n];
			v_transform(v, p, mx);
			fprintf(f, "   vertex %f %f %f\n", v[0], v[1], v[2]);
		}
		fprintf(f, "  endloop\n");
		fprintf(f, " endfacet\n");
	}
}

#ifndef STL_TESTER

static void stl_model_place(rnd_hidlib_t *hl, FILE *outf, htsp_t *models, const char *name, rnd_coord_t ox, rnd_coord_t oy, double rotdeg, int on_bottom, const char *user_xlate, const char *user_rot, double maxy, rnd_coord_t z0, rnd_coord_t z1)
{
	stl_facet_t *head = NULL;
	double xlate[3], rot[3];

	if (!htsp_has(models, name)) {
		char *full_path;
		FILE *f = rnd_fopen_first(&PCB->hidlib, &conf_core.rc.library_search_paths, name, "r", &full_path, rnd_true);
		if (f != NULL) {
			head = stl_solid_fload(hl, f);
			if (head == NULL)
				rnd_message(RND_MSG_ERROR, "STL model failed to load: %s\n", full_path);
		}
		else
			rnd_message(RND_MSG_ERROR, "STL model not found: %s\n", name);
		free(full_path);
		fclose(f);
		htsp_set(models, rnd_strdup(name), head);
	}
	else
		head = htsp_get(models, name);

	if (head == NULL)
		return;

	xlate[0] = RND_COORD_TO_MM(ox);
	xlate[1] = RND_COORD_TO_MM(maxy - oy);
	xlate[2] = on_bottom ? RND_COORD_TO_MM(z0) : RND_COORD_TO_MM(z1);

	rot[0] = 0;
	rot[1] = on_bottom ? M_PI : 0;
	rot[2] = rotdeg / RND_RAD_TO_DEG;

	stl_solid_print_facets(outf, head, rot[0], rot[1], rot[2], xlate[0], xlate[1], xlate[2]);
}


void stl_models_print(pcb_board_t *pcb, FILE *outf, double maxy, rnd_coord_t z0, rnd_coord_t z1)
{
	htsp_t models;
	const char *mod;
	htsp_entry_t *e;

	htsp_init(&models, strhash, strkeyeq);

	PCB_SUBC_LOOP(PCB->Data); {
		mod = pcb_attribute_get(&subc->Attributes, "stl");
		if (mod != NULL) {
			rnd_coord_t ox, oy;
			double rot = 0;
			int on_bottom = 0;
			const char *srot, *sxlate;
			
			if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)subc, "subc-place", "Failed to get origin of subcircuit", "fix the missing subc-aux layer");
				continue;
			}
			pcb_subc_get_rotation(subc, &rot);
			pcb_subc_get_side(subc, &on_bottom);

			sxlate = pcb_attribute_get(&subc->Attributes, "stl::translate");
			if (sxlate == NULL)
				sxlate = pcb_attribute_get(&subc->Attributes, "stl-translate");
			srot = pcb_attribute_get(&subc->Attributes, "stl::rotate");
			if (srot == NULL)
				srot = pcb_attribute_get(&subc->Attributes, "stl-rotate");

			stl_model_place(&pcb->hidlib, outf, &models, mod, ox, oy, rot, on_bottom, sxlate, srot, maxy, z0, z1);
		}
	} PCB_END_LOOP;

	for (e = htsp_first(&models); e; e = htsp_next(&models, e)) {
		free(e->key);
		stl_solid_free((stl_facet_t *)e->value);
	}

	htsp_uninit(&models);

}

#endif

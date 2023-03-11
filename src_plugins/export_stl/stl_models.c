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

void stl_solid_print_facets(FILE *f, stl_facet_t *head, double rotx, double roty, double rotz, double xlatex, double xlatey, double xlatez, const stl_fmt_t *fmt)
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

	for(; head != NULL; head = head->next)
		fmt->print_facet(f, head, mx, mxn);
}

#ifndef STL_TESTER

static void parse_utrans(double dst[3], const char *src)
{
	double tmp[3];
	int n;
	const char *s = src;
	char *end;

	if (src == NULL)
		return;

	for(n = 0; n < 3; n++) {
		if (s == NULL)
			break;
		tmp[n] = strtod(s, &end);
		if ((!isspace(*end)) && (*end != ',') && (*end != ';') && (*end != '\0')) {
			rnd_message(RND_MSG_ERROR, "stl: Invalis user coords in footprint transformation attribute: %s\n", src);
			return;
		}
		s = end+1;
	}
	memcpy(dst, tmp, sizeof(tmp));
}

static stl_facet_t stl_format_not_supported;

static void stl_model_place(rnd_design_t *hl, FILE *outf, htsp_t *models, const char *name, rnd_coord_t ox, rnd_coord_t oy, double rotdeg, int on_bottom, const char *user_xlate, const char *user_rot, double maxy, rnd_coord_t z0, rnd_coord_t z1, const stl_fmt_t *ifmt, const stl_fmt_t *ofmt)
{
	stl_facet_t *head = NULL;
	double uxlate[3] = {0,0,0}, xlate[3], urot[3] = {0,0,0}, rot[3];

	if (!htsp_has(models, name)) {
		char *full_path;
		FILE *f = rnd_fopen_first(&PCB->hidlib, &conf_core.rc.library_search_paths, name, "r", &full_path, rnd_true);
		if (f != NULL) {
			head = ifmt->model_load(hl, f, full_path);
			if (head == NULL)
				rnd_message(RND_MSG_ERROR, "export_stl model failed to load: %s\n", full_path);
			else if (head == &stl_format_not_supported)
				head = NULL;
		}
		else
			rnd_message(RND_MSG_ERROR, "export_stl model not found: %s\n", name);
		free(full_path);
		if (f != NULL)
			fclose(f);
		htsp_set(models, rnd_strdup(name), head);
	}
	else
		head = htsp_get(models, name);

	if (head == NULL)
		return;

	parse_utrans(uxlate, user_xlate);
	xlate[0] = RND_COORD_TO_MM(ox) + uxlate[0];
	xlate[1] = RND_COORD_TO_MM(maxy - (oy)) + uxlate[1];
	xlate[2] = RND_COORD_TO_MM((on_bottom ? z0 : z1)) + uxlate[2];

	if (on_bottom)
		rotdeg = -rotdeg;

	parse_utrans(urot, user_rot);
	rot[0] = 0 + urot[0] / RND_RAD_TO_DEG;
	rot[1] = (on_bottom ? M_PI : 0) + urot[1] / RND_RAD_TO_DEG;
	rot[2] = rotdeg / RND_RAD_TO_DEG + urot[2] / RND_RAD_TO_DEG;

	stl_solid_print_facets(outf, head, rot[0], rot[1], rot[2], xlate[0], xlate[1], xlate[2], ofmt);
}

static int stl_model_print(pcb_board_t *pcb, FILE *outf, double maxy, rnd_coord_t z0, rnd_coord_t z1, htsp_t *models, pcb_subc_t *subc, int *first, const stl_fmt_t *ifmt, const stl_fmt_t *ofmt)
{
	const char *mod;
	rnd_coord_t ox, oy;
	double rot = 0;
	int on_bottom = 0;
	const char *srot, *sxlate;

	mod = pcb_attribute_get(&subc->Attributes, ifmt->attr_model_name);
	if (mod == NULL)
		return -1;

	if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
		pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)subc, "subc-place", "Failed to get origin of subcircuit", "fix the missing subc-aux layer");
		return -1;
	}
	pcb_subc_get_rotation(subc, &rot);
	pcb_subc_get_side(subc, &on_bottom);

	sxlate = pcb_attribute_get(&subc->Attributes, ifmt->attr_xlate);
	if ((sxlate == NULL) && (ifmt->attr_xlate_old != NULL))
		sxlate = pcb_attribute_get(&subc->Attributes, ifmt->attr_xlate_old);
	srot = pcb_attribute_get(&subc->Attributes, ifmt->attr_rotate);
	if ((srot == NULL)  && (ifmt->attr_rotate_old != NULL))
		srot = pcb_attribute_get(&subc->Attributes, ifmt->attr_rotate_old);

	if (*first) {
		ofmt->new_obj(0.5, 0.5, 0.5);
		*first = 0;
	}

	stl_model_place(&pcb->hidlib, outf, models, mod, ox, oy, rot, on_bottom, sxlate, srot, maxy, z0, z1, ifmt, ofmt);
	return 0;
}

void stl_models_print(pcb_board_t *pcb, FILE *outf, double maxy, rnd_coord_t z0, rnd_coord_t z1, const stl_fmt_t *fmt)
{
	htsp_t models;
	htsp_entry_t *e;
	int first = 1;

	htsp_init(&models, strhash, strkeyeq);

	PCB_SUBC_LOOP(PCB->Data); {
		int good = 0;

		if (subc->extobj != NULL) continue;

		if (stl_model_print(pcb, outf, maxy, z0, z1, &models, subc, &first, fmt, fmt) != 0) {
			const stl_fmt_t **n; /* fallback: try all other formats */
			for(n = fmt_all; *n != NULL; n++) {
				if ((*n == fmt) || ((*n)->model_load == NULL)) continue; /* already tried or can't load model */
				if (stl_model_print(pcb, outf, maxy, z0, z1, &models, subc, &first, *n, fmt) == 0) {
					good = 1;
					break;
				}
			}
		}
		else
			good = 1;

		if (!good)
			pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)subc, "subc-place", "Missing 3d model", "Could not load 3d surface model - component missing from the output");
	} PCB_END_LOOP;

	for (e = htsp_first(&models); e; e = htsp_next(&models, e)) {
		free(e->key);
		stl_solid_free((stl_facet_t *)e->value);
	}

	htsp_uninit(&models);

}

#endif

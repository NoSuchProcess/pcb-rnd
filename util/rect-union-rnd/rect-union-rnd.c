/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  rect-union-rnd - utility to compute union of rectangles
 *  pcb-rnd Copyright (C) 2025 Tibor 'Igor2' Palinkas
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <librnd/core/unit.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/misc_util.h>
#include <librnd/poly/rtree.h>
#include <librnd/poly/polyarea.h>
#include <librnd/poly/polygon1_gen.h>

rnd_polyarea_t *acc = NULL;

static void rect_xy(char *spec)
{
	rnd_coord_t c[4];
	rnd_polyarea_t *pa, *tmp;
	int n;
	char *curr, *sep, osep;
	rnd_bool succ;

	for(n = 0, curr = spec; n < 4; n++) {
		if (curr == NULL) {
			fprintf(stderr, "Invalid rxy spec: need 4 coordinates: '%s'\n", spec);
			exit(1);
		}
		while(isspace(*curr) || (*curr == ';') || (*curr == ','))
			curr++;

		sep = strpbrk(curr, ";, \r\n");
		if (sep != NULL) {
			osep = *sep;
			*sep = '\0';
		}

		c[n] = rnd_get_value(curr, "mm", NULL, &succ);
		if (!succ) {
			fprintf(stderr, "Invalid rxy coord: '%s'\n", curr);
			exit(1);
		}

		if (sep != NULL)
			*sep = osep;

		curr = sep+1;
	}



	pa = rnd_poly_from_rect(c[0], c[2], c[1], c[3]);
	if (pa == NULL) {
		fprintf(stderr, "invalid rxy spec: failed to create poly: '%s'\n", spec);
		exit(1);
	}

	if (acc == NULL) {
		acc = pa;
		return;
	}

	if (rnd_polyarea_boolean_free(acc, pa, &tmp, RND_PBO_UNITE) != 0) {
		fprintf(stderr, "failed to add rxy: '%s'\n", spec);
		exit(1);
	}

	acc = tmp;
}

static void help(const char *topic)
{
	printf("rect-union-rnd - compute union of rectangles and print resulting polygon\n");
	printf("Usage:\n");
	printf("rect-union-rnd options... \n");
	printf("Options:\n");
	printf("  -ind str            change output indentation\n");
	printf("  -clr str            change polygon clearance\n");
	printf("  -rxy x1,y1,x2,y2    add a rectangle specified with corners (*)\n");
	printf("\n");
	printf("Options marked with (*) can be specified multipe times");
	printf("\n");
}

static const char *ind = "          ";
static long ID = 1000000;
static const char *clr = "0";
static const char *flg = "";

static void print_island(rnd_polyarea_t *pa)
{
	rnd_vnode_t *v;

	printf("%sha:polygon.%ld {\n", ind, ID++);
	printf("%s clearance=%s\n", ind, clr);
	printf("%s li:geometry {\n", ind);
	printf("%s   ta:contour {\n", ind);

	v = pa->contours->head;
	do {
		rnd_printf("%s    { %.09$mm; %.09$mm }\n", ind, v->point[0], v->point[1]);
	} while((v = v->next) != pa->contours->head);

	printf("%s   }\n", ind);
	printf("%s }\n", ind);
	printf("%s ha:attributes {}\n", ind);
	printf("%s ha:flags {%s}\n", ind, flg);
	printf("%s}\n", ind);
}

int main(int argc, char *argv[])
{
	int n;

	rnd_units_init();

	for(n = 1; n < argc; n++) {
		char *cmd = argv[n], *arg = argv[n+1];
		while(*cmd == '-') cmd++;
		if (strcmp(cmd, "help") == 0)
			help(arg);
		else if (strcmp(cmd, "ind") == 0) {
			ind = arg;
			n++;
		}
		else if (strcmp(cmd, "clr") == 0) {
			clr = arg;
			n++;
		}
		else if (strcmp(cmd, "rxy") == 0) {
			rect_xy(arg);
			n++;
		}
		else {
			fprintf(stderr, "Invalid argument: %s\n", argv[n]);
			return 1;
		}
	}

	if (acc != NULL) {
		rnd_polyarea_t *pa = NULL;
		pa = acc;
		do {
			print_island(pa);
		} while((pa = pa->f) != acc);
	}

	return 0;
}

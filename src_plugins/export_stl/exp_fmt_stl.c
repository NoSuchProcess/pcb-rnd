/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

static void stl_print_horiz_tri(FILE *f, fp2t_triangle_t *t, int up, rnd_coord_t z)
{
	fprintf(f, "	facet normal 0 0 %d\n", up ? 1 : -1);
	fprintf(f, "		outer loop\n");

	if (up) {
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[0]->X, (rnd_coord_t)t->Points[0]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[1]->X, (rnd_coord_t)t->Points[1]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[2]->X, (rnd_coord_t)t->Points[2]->Y, z);
	}
	else {
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[2]->X, (rnd_coord_t)t->Points[2]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[1]->X, (rnd_coord_t)t->Points[1]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[0]->X, (rnd_coord_t)t->Points[0]->Y, z);
	}

	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");
}

static void stl_print_vert_tri(FILE *f, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t z0, rnd_coord_t z1)
{
	double vx, vy, nx, ny, len;

	vx = x2 - x1; vy = y2 - y1;
	len = sqrt(vx*vx + vy*vy);
	if (len == 0) return;
	vx /= len; vy /= len;
	nx = -vy; ny = vx;

	fprintf(f, "	facet normal %f %f 0\n", nx, ny);
	fprintf(f, "		outer loop\n");
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z1);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z1);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z0);
	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");

	fprintf(f, "	facet normal %f %f 0\n", nx, ny);
	fprintf(f, "		outer loop\n");
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z1);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z0);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z0);
	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");
}

static void stl_print_facet(FILE *f, stl_facet_t *head, double mx[16], double mxn[16])
{
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

static void stl_new_obj(float r, float g, float b)
{
}

static void stl_print_header(FILE *f)
{
	fprintf(f, "solid pcb\n");
}

static void stl_print_footer(FILE *f)
{
	fprintf(f, "endsolid\n");
}

static stl_facet_t *stl_solid_fload(rnd_hidlib_t *hl, FILE *f, const char *fn);

static const stl_fmt_t fmt_stl = {
	/* output */
	".stl",
	stl_print_horiz_tri,
	stl_print_vert_tri,
	stl_print_facet,
	stl_new_obj,
	stl_print_header,
	stl_print_footer,

	/* model load */
	stl_solid_fload
};

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

static void proj_print_header(FILE *f)
{
	fprintf(f, "obj \"board\"\n");
	fprintf(f, "	realempty\n");

	verthash_init(&verthash);
}

static void proj_print_footer(FILE *f)
{
	long n, *vx;
	rnd_coord_t *c;


	fprintf(f, "	verts\n");
	for(n = 0, c = verthash.vxcoords.array; n < verthash.next_id; n++, c += 3)
		rnd_fprintf(f, "		%.09mm %.09mm %.09mm\n", c[0], c[1], c[2]);

	for(n = 0, vx = verthash.triangles.array; n < verthash.triangles.used; n += 3, vx += 3) {
		if (vx[0] < 0) {
			vx++;
			n++;
			fprintf(f, "	color %.6f %.6f %.6f\n", (double)vx[0]/1000000, (double)vx[1]/1000000, (double)vx[2]/1000000);
			continue; /* shift colors */
		}
		rnd_fprintf(f, "		tri :%ld :%ld :%ld\n", vx[0], vx[1], vx[2]);
	}
	fprintf(f, "	normals\n");

	verthash_uninit(&verthash);
}

static const stl_fmt_t fmt_proj = {
	/* output */
	".pro",
	amf_print_horiz_tri,
	amf_print_vert_tri,
	amf_print_facet,
	amf_new_obj,
	proj_print_header,
	proj_print_footer,

	/* model load */
	"projector",
	"projector::translate", NULL,
	"projector::rotate", NULL,
	NULL /* no loader yet */
};

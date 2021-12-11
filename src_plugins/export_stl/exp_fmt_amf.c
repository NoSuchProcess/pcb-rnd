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

static void amf_print_horiz_tri(FILE *f, fp2t_triangle_t *t, int up, rnd_coord_t z)
{
	if (up) {
		verthash_add_triangle_coord(&verthash,
			t->Points[0]->X, t->Points[0]->Y, z,
			t->Points[1]->X, t->Points[1]->Y, z,
			t->Points[2]->X, t->Points[2]->Y, z
			);
	}
	else {
		verthash_add_triangle_coord(&verthash,
			t->Points[2]->X, (rnd_coord_t)t->Points[2]->Y, z,
			t->Points[1]->X, (rnd_coord_t)t->Points[1]->Y, z,
			t->Points[0]->X, (rnd_coord_t)t->Points[0]->Y, z
			);
	}
}

static void amf_print_vert_tri(FILE *f, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t z0, rnd_coord_t z1)
{
	verthash_add_triangle_coord(&verthash,
		x2, y2, z1,
		x1, y1, z1,
		x1, y1, z0
		);

	verthash_add_triangle_coord(&verthash,
		x2, y2, z1,
		x1, y1, z0,
		x2, y2, z0
		);
}


static void amf_print_facet(FILE *f, stl_facet_t *head, double mx[16], double mxn[16])
{
	double v[3], p[3];
	long vert[3];
	int n;

	for(n = 0; n < 3; n++) {
		p[0] = head->vx[n]; p[1] = head->vy[n]; p[2] = head->vz[n];
		v_transform(v, p, mx);
		vert[n] = verthash_add_vertex(&verthash,
			(rnd_coord_t)RND_MM_TO_COORD(v[0]),
			(rnd_coord_t)RND_MM_TO_COORD(v[1]),
			(rnd_coord_t)RND_MM_TO_COORD(v[2])
			);
	}

	verthash_add_triangle(&verthash, vert[0], vert[1], vert[2]);
}

static void amf_new_obj(float r, float g, float b)
{
	verthash_new_obj(&verthash, r, g, b);
}

static void amf_print_header(FILE *f)
{
	fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(f, "<amf unit=\"millimeter\">\n");
	fprintf(f, " <metadata type=\"producer\">pcb-rnd export_stl</metadata>\n");
	fprintf(f, " <object id=\"0\">\n");
	fprintf(f, "  <mesh>\n");

	verthash_init(&verthash);
}

static void amf_print_footer(FILE *f)
{
	long n, *vx;
	rnd_coord_t *c;

	fprintf(f, "   <vertices>\n");
	for(n = 0, c = verthash.vxcoords.array; n < verthash.next_id; n++, c += 3)
		rnd_fprintf(f, "    <vertex><coordinates> <x>%.09mm</x>\t<y>%.09mm</y>\t<z>%.09mm</z> </coordinates></vertex>\n", c[0], c[1], c[2]);
	fprintf(f, "   </vertices>\n");

	for(n = 0, vx = verthash.triangles.array; n < verthash.triangles.used; n += 3, vx += 3) {
		if (vx[0] < 0) {
			if (n != 0)
				fprintf(f, "   </volume>\n");
			fprintf(f, "   <volume materialid=\"%ld\">\n", (long)-vx[0]);
			vx++;
			n++;
			continue; /* shift colors */
		}
		rnd_fprintf(f, "    <triangle> <v1>%ld</v1>\t<v2>%ld</v2>\t<v3>%ld</v3> </triangle>\n", vx[0], vx[1], vx[2]);
	}
	fprintf(f, "   </volume>\n");
	fprintf(f, "  </mesh>\n");
	fprintf(f, " </object>\n");

	for(n = 0, vx = verthash.triangles.array; n < verthash.triangles.used; n += 3, vx += 3) {
		if (vx[0] < 0) {
			fprintf(f, " <material id=\"%ld\">\n", (long)-vx[0]);
			vx++;
			n++;
			fprintf(f, "   <color><r>%.6f</r><g>%.6f</g><b>%.6f</b></color>\n", (double)vx[0]/1000000, (double)vx[1]/1000000, (double)vx[2]/1000000);
			fprintf(f, " </material>\n");
		}
	}

	fprintf(f, "</amf>\n");

	verthash_uninit(&verthash);
}

static const stl_fmt_t fmt_amf = {
	".amf",
	amf_print_horiz_tri,
	amf_print_vert_tri,
	amf_print_facet,
	amf_new_obj,
	amf_print_header,
	amf_print_footer
};

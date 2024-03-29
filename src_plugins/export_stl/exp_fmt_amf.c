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

static stl_facet_t *amf_solid_fload(rnd_design_t *hl, FILE *f, const char *fn);

static const stl_fmt_t fmt_amf = {
	/* output */
	".amf",
	vhs_print_horiz_tri,
	vhs_print_vert_tri,
	vhs_print_facet,
	vhs_new_obj,
	amf_print_header,
	amf_print_footer,

	/* model load */
	"amf",
	"amf::translate", NULL,
	"amf::rotate", NULL,
	amf_solid_fload
};

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "../lib_polyhelp/topoly.h"
#include "plug_io.h"
#include <librnd/core/hid_inlines.h>


#define TRX_(x) (x)
#define TRY_(y) (PCB->hidlib.size_y - (y))

#define TRX(x)
#define TRY(y) y = TRY_(y)

static void scad_draw_primitives(void)
{
	fprintf(f, "// Round cap line\n");
	fprintf(f, "module pcb_line_rc(x1, y1, length, angle, width, thick) {\n");
	fprintf(f, "	translate([x1,y1,0]) {\n");
	fprintf(f, "		rotate([0,0,angle]) {\n");
	fprintf(f, "			translate([length/2, 0, 0])\n");
	fprintf(f, "				cube([length, width, thick], center=true);\n");
	fprintf(f, "			cylinder(r=width/2, h=thick, center=true, $fn=30);\n");
	fprintf(f, "			translate([length, 0, 0])\n");
	fprintf(f, "				cylinder(r=width/2, h=thick, center=true, $fn=30);\n");
	fprintf(f, "		}\n");
	fprintf(f, "	}\n");
	fprintf(f, "}\n");

	fprintf(f, "// Square cap line\n");
	fprintf(f, "module pcb_line_sc(x1, y1, length, angle, width, thick) {\n");
	fprintf(f, "	translate([x1,y1,0]) {\n");
	fprintf(f, "		rotate([0,0,angle]) {\n");
	fprintf(f, "			translate([length/2, 0, 0])\n");
	fprintf(f, "				cube([length + width, width, thick], center=true);\n");
	fprintf(f, "		}\n");
	fprintf(f, "	}\n");
	fprintf(f, "}\n");

	fprintf(f, "// filled rectangle\n");
	fprintf(f, "module pcb_fill_rect(x1, y1, x2, y2, angle, thick) {\n");
	fprintf(f, "	translate([(x1+x2)/2,(y1+y2)/2,0])\n");
	fprintf(f, "		rotate([0,0,angle])\n");
	fprintf(f, "			cube([x2-x1, y2-y1, thick], center=true);\n");
	fprintf(f, "}\n");

	fprintf(f, "// filled polygon\n");
	fprintf(f, "module pcb_fill_poly(coords, thick) {\n");
	fprintf(f, "	linear_extrude(height=thick)\n");
	fprintf(f, "		polygon(coords);\n");
	fprintf(f, "}\n");

	fprintf(f, "// filled circle\n");
	fprintf(f, "module pcb_fcirc(x1, y1, radius, thick) {\n");
	fprintf(f, "	translate([x1,y1,0])\n");
	fprintf(f, "		cylinder(r=radius, h=thick, center=true, $fn=30);\n");
	fprintf(f, "}\n");
}

static int scad_draw_outline(void)
{
	pcb_poly_t *poly = pcb_topoly_1st_outline(PCB, PCB_TOPOLY_FLOATING);
	int n;

	if (poly == NULL)
		return -1;

	fprintf(f, "module pcb_outline() {\n");
	fprintf(f, "	polygon([\n\t\t");
	/* we need the as-drawn polygon and we know there are no holes */
	for(n = 0; n < poly->PointN; n++)
		rnd_fprintf(f, "[%mm,%mm]%s", TRX_(poly->Points[n].X), TRY_(poly->Points[n].Y), ((n < (poly->PointN-1)) ? "," : "\n"));
	fprintf(f, "	]);\n");
	fprintf(f, "}\n");

	pcb_poly_free(poly);
	return 0;
}

static void scad_draw_pstk(pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	pcb_pstk_shape_t *mech;
	int n;

	if (proto == NULL) {
		pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padstack-proto", "failed to retrieve padstack prototype", "internal pcb-rnd error, please file a bugreport");
		return;
	}

TODO("padstack: these ignore bbvias")
	if (proto->hdia > 0) {
		rnd_fprintf(f, "	translate([%mm,%mm,0])\n", TRX_(ps->x), TRY_(ps->y));
		rnd_fprintf(f, "		cylinder(r=%mm, h=4, center=true, $fn=30);\n", proto->hdia/2);
	}


	mech = pcb_pstk_shape_mech_gid(PCB, ps, pcb_layergrp_get_top_copper());
	if (mech != NULL) {
		rnd_hid_gc_s gc = {0};

		effective_layer_thickness = 4;
		switch(mech->shape) {
			case PCB_PSSH_POLY:
				fprintf(f, "	translate([0,0,%f])\n", -effective_layer_thickness/2);
				fprintf(f, "	pcb_fill_poly([");
				for(n = 0; n < mech->data.poly.len; n++)
					rnd_fprintf(f, "%s[%mm,%mm]", (n > 0 ? ", " : ""),
						TRX_(ps->x + mech->data.poly.x[n]),
						TRY_(ps->y + mech->data.poly.y[n]));
				rnd_fprintf(f, "], %f);\n", effective_layer_thickness);
				break;
			case PCB_PSSH_LINE:
				rnd_hid_set_line_cap(&gc, mech->data.line.square ? rnd_cap_square : rnd_cap_round);
				rnd_hid_set_line_width(&gc, MAX(mech->data.line.thickness, 1));
				rnd_render->draw_line(&gc, ps->x + mech->data.line.x1, ps->y + mech->data.line.y1, ps->x + mech->data.line.x2, ps->y + mech->data.line.y2);
				break;
			case PCB_PSSH_CIRC:
				rnd_fprintf(f, "	translate([%mm,%mm,0])\n", TRX_(ps->x + mech->data.circ.x), TRY_(ps->y + mech->data.circ.y));
				rnd_fprintf(f, "		cylinder(r=%mm, h=4, center=true, $fn=30);\n", MAX(mech->data.circ.dia/2, 1));
				break;
			case PCB_PSSH_HSHADOW:
				break;
		}
	}
}

static void scad_draw_drills(void)
{
	rnd_rtree_it_t it;
	rnd_box_t *obj;

	fprintf(f, "module pcb_drill() {\n");

	for(obj = rnd_r_first(PCB->Data->padstack_tree, &it); obj != NULL; obj = rnd_r_next(&it))
		scad_draw_pstk((pcb_pstk_t *)obj);

	fprintf(f, "}\n");
}

static void scad_draw_finish(rnd_hid_attr_val_t *options)
{
	fprintf(f, "module pcb_board_main() {\n");
	fprintf(f, "	translate ([0, 0, -0.8])\n");
	fprintf(f, "		linear_extrude(height=1.6)\n");
	fprintf(f, "			pcb_outline();\n");
	fprintf(f, "%s", layer_group_calls.array);
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "module pcb_board() {\n");
	fprintf(f, "	intersection() {\n");
	fprintf(f, "		translate ([0, 0, -4])\n");
	fprintf(f, "			linear_extrude(height=8)\n");
	fprintf(f, "				pcb_outline();\n");
	fprintf(f, "		union() {\n");
	fprintf(f, "			difference() {\n");
	fprintf(f, "				pcb_board_main();\n");
	if (options[HA_drill].lng)
		fprintf(f, "				pcb_drill();\n");
	fprintf(f, "			}\n");
	fprintf(f, "		}\n");
	fprintf(f, "	}\n");
	fprintf(f, "%s", model_calls.array);
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "pcb_board();\n");
}

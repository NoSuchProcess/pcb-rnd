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
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
  */

#include "../lib_polyhelp/topoly.h"

#define TRX_(x) (x)
#define TRY_(y) (PCB->MaxHeight - (y))

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
	pcb_any_obj_t *start = pcb_topoly_find_1st_outline(PCB);
	pcb_polygon_t *poly;
	int n;

	if (start == NULL) {
		poly = pcb_poly_alloc(PCB->Data->Layer);
		pcb_poly_point_new(poly, 0, 0);
		pcb_poly_point_new(poly, PCB->MaxWidth, 0);
		pcb_poly_point_new(poly, PCB->MaxWidth, PCB->MaxHeight);
		pcb_poly_point_new(poly, 0, PCB->MaxHeight);
	}
	else {
		poly = pcb_topoly_conn(PCB, start, 0);
		if (poly == NULL)
			return -1;
	}

	fprintf(f, "module pcb_outline() {\n");
	fprintf(f, "	polygon([\n\t\t");
	/* we need the as-drawn polygon and we know there are no holes */
	for(n = 0; n < poly->PointN; n++)
		pcb_fprintf(f, "[%mm,%mm]%s", TRX_(poly->Points[n].X), TRY_(poly->Points[n].Y), ((n < (poly->PointN-1)) ? "," : "\n"));
	fprintf(f, "	]);\n");
	fprintf(f, "}\n");

	pcb_poly_free(poly);
	return 0;
}

static void scad_draw_drill(const pcb_pin_t *pin)
{
	pcb_fprintf(f, "	translate([%mm,%mm,0])\n", TRX_(pin->X), TRY_(pin->Y));
	pcb_fprintf(f, "		cylinder(r=%mm, h=4, center=true, $fn=30);\n", pin->DrillingHole/2);
}

static void scad_draw_drills(void)
{
	pcb_rtree_it_t it;
	pcb_box_t *obj;

	fprintf(f, "module pcb_drill() {\n");

	for(obj = pcb_r_first(PCB->Data->via_tree, &it); obj != NULL; obj = pcb_r_next(&it))
		scad_draw_drill((pcb_pin_t *)obj);
	pcb_r_end(&it);

	PCB_PIN_ALL_LOOP(PCB->Data); {
		scad_draw_drill(pin);
	} PCB_ENDALL_LOOP;


	fprintf(f, "}\n");
}

static void scad_draw_finish()
{
	fprintf(f, "module pcb_board_main() {\n");
	fprintf(f, "	translate ([0, 0, -0.8])\n");
	fprintf(f, "		linear_extrude(height=1.6)\n");
	fprintf(f, "			pcb_outline();\n");
	fprintf(f, "%s", layer_calls.array);
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "module pcb_board() {\n");
	fprintf(f, "	difference() {\n");
	fprintf(f, "		pcb_board_main();\n");
	if (openscad_attribute_list[HA_drill].default_val.int_value)
		fprintf(f, "		pcb_drill();\n");
	fprintf(f, "	}\n");
	fprintf(f, "%s", model_calls.array);
	fprintf(f, "}\n");
	fprintf(f, "\n");


	fprintf(f, "pcb_board();\n");
}

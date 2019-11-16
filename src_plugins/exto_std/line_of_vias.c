/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Extended object for a line-of-vias; edit: line; places a row of vias over the line
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

typedef struct {
	pcb_line_t edit;
} line_of_vias;

static void line_of_vias_unpack(pcb_subc_t *obj)
{
	line_of_vias *lov;
	const char *s;
	double v;
	pcb_bool succ;

	if (obj->extobj_data == NULL)
		obj->extobj_data = calloc(sizeof(line_of_vias), 1);

	lov = obj->extobj_data;
	lov->edit.Thickness = 1;
	pcb_subc_get_origin(obj, &lov->edit.Point1.X, &lov->edit.Point1.Y);

	lov->edit.Point2.X = lov->edit.Point1.X; lov->edit.Point2.Y = lov->edit.Point1.Y;

	s = pcb_attribute_get(&obj->Attributes, "extobj::x2");
	if (s != NULL) {
		v = pcb_get_value(s, NULL, NULL, &succ);
		if (succ) lov->edit.Point2.X = v;
	}

	s = pcb_attribute_get(&obj->Attributes, "extobj::y2");
	if (s != NULL) {
		v = pcb_get_value(s, NULL, NULL, &succ);
		if (succ) lov->edit.Point2.Y = v;
	}
	
}

static void pcb_line_of_vias_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	int selected = PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc);
	line_of_vias *lov;

	if (subc->extobj_data == NULL)
		line_of_vias_unpack(subc);
	lov = subc->extobj_data;


	pcb_render->set_color(pcb_draw_out.fgGC, selected ? &conf_core.appearance.color.selected : &conf_core.appearance.color.extobj);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, PCB_MM_TO_COORD(0.15));
	pcb_render->draw_line(pcb_draw_out.fgGC, lov->edit.Point1.X, lov->edit.Point1.Y, lov->edit.Point2.X, lov->edit.Point2.Y);
}


static pcb_extobj_t pcb_line_of_vias = {
	"line-of-vias",
	pcb_line_of_vias_draw_mark
};

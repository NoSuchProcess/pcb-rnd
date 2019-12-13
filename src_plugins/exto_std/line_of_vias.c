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
	pcb_coord_t pitch;
	pcb_coord_t clearance;
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

	s = pcb_attribute_get(&obj->Attributes, "extobj::pitch");
	if (s != NULL) {
		v = pcb_get_value(s, NULL, NULL, &succ);
		if (succ) lov->pitch = v;
	}

	s = pcb_attribute_get(&obj->Attributes, "extobj::clearance");
	if (s != NULL) {
		v = pcb_get_value(s, NULL, NULL, &succ);
		if (succ) lov->clearance = v;
	}
}

/* remove all existing padstacks from the subc */
static void line_of_vias_clear(pcb_subc_t *subc)
{
	pcb_pstk_t *ps;
	for(ps = padstacklist_first(&subc->data->padstack); ps != NULL; ps = padstacklist_first(&subc->data->padstack)) {
		pcb_poly_restore_to_poly(ps->parent.data, PCB_OBJ_PSTK, NULL, ps);
		pcb_pstk_free(ps);
	}
}

/* create all new padstacks */
static int line_of_vias_gen(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	double offs, len, dx, dy, x, y, pitch;
	line_of_vias *lov;
	pcb_line_t *line = (pcb_line_t *)edit_obj;
	pcb_data_t *data = subc->parent.data;

	if (edit_obj->type != PCB_OBJ_LINE)
		return -1;

	if (subc->extobj_data == NULL)
		line_of_vias_unpack(subc);
	lov = subc->extobj_data;

	if (data->subc_tree != NULL)
		pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);
	pcb_undo_freeze_add();

	len = pcb_distance(line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
	dx = (double)(line->Point2.X - line->Point1.X) / len;
	dy = (double)(line->Point2.Y - line->Point1.Y) / len;
	x = line->Point1.X;
	y = line->Point1.Y;
	pitch = lov->pitch;
	for(offs = 0; offs <= len; offs += pitch) {
		pcb_pstk_new(subc->data, -1, 0, pcb_round(x), pcb_round(y), lov->clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
		x += dx * pitch;
		y += dy * pitch;
	}

	pcb_undo_unfreeze_add();
	pcb_subc_bbox(subc);
	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc);

	return 0;
}


pcb_any_obj_t *pcb_line_of_vias_get_edit_obj(pcb_subc_t *obj)
{
	pcb_any_obj_t *edit = pcb_extobj_get_editobj_by_attr(obj);

	if (edit->type != PCB_OBJ_LINE)
		return NULL;

	return edit;
}



static void pcb_line_of_vias_edit_pre(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("LoV: edit pre %ld %ld\n", subc->ID, edit_obj->ID);
	line_of_vias_clear(subc);
}

static void pcb_line_of_vias_edit_geo(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("LoV: edit geo %ld %ld\n", subc->ID, edit_obj->ID);
	line_of_vias_gen(subc, edit_obj);
}


static pcb_extobj_t pcb_line_of_vias = {
	"line-of-vias",
	NULL,
	pcb_line_of_vias_get_edit_obj,
	pcb_line_of_vias_edit_pre,
	pcb_line_of_vias_edit_geo
};

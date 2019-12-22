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

#define LID_EDIT 0

#include "../src_plugins/lib_compat_help/pstk_compat.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int gui_active;
	pcb_coord_t pitch;
	pcb_coord_t clearance;
} line_of_vias;

static void pcb_line_of_vias_del_pre(pcb_subc_t *subc)
{
	line_of_vias *lov = subc->extobj_data;
	pcb_trace("LoV del_pre\n");

	if ((lov != NULL) && (lov->gui_active))
		PCB_DAD_FREE(lov->dlg);

	free(lov);
	subc->extobj_data = NULL;
}

static void line_of_vias_unpack(pcb_subc_t *obj)
{
	line_of_vias *lov;

	if (obj->extobj_data == NULL)
		obj->extobj_data = calloc(sizeof(line_of_vias), 1);
	lov = obj->extobj_data;

	pcb_extobj_unpack_coord(obj, &lov->pitch, "extobj::pitch");
	pcb_extobj_unpack_coord(obj, &lov->clearance, "extobj::clearance");
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

#define line_geo_def \
	double len, dx, dy \

#define line_geo_calc(line) \
	do { \
		len = pcb_distance(line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y); \
		dx = (double)(line->Point2.X - line->Point1.X) / len; \
		dy = (double)(line->Point2.Y - line->Point1.Y) / len; \
	} while(0)


/* create all new padstacks */
static void line_of_vias_gen_line(pcb_board_t *pcb, pcb_subc_t *subc, pcb_line_t *line)
{
	line_geo_def;
	double offs, x, y, pitch, too_close, qbox_bloat;
	line_of_vias *lov = subc->extobj_data;

	line_geo_calc(line);
	x = line->Point1.X;
	y = line->Point1.Y;
	pitch = lov->pitch;
	too_close = pitch/2.0;
	qbox_bloat = pitch/4.0;
	for(offs = 0; offs <= len; offs += pitch) {
		pcb_rtree_it_t it;
		pcb_rtree_box_t qbox;
		pcb_pstk_t *cl;
		int skip = 0;
		pcb_coord_t rx = pcb_round(x), ry = pcb_round(y);

		/* skip if there's a via too close */
		qbox.x1 = pcb_round(rx - qbox_bloat); qbox.y1 = pcb_round(ry - qbox_bloat);
		qbox.x2 = pcb_round(rx + qbox_bloat); qbox.y2 = pcb_round(ry + qbox_bloat);

		if ((pcb != NULL) && (pcb->Data->padstack_tree != NULL)) {
			for(cl = pcb_rtree_first(&it, pcb->Data->padstack_tree, &qbox); cl != NULL; cl = pcb_rtree_next(&it)) {
				if (pcb_distance(rx, ry, cl->x, cl->y) < too_close) {
					skip = 1;
					break;
				}
			}
		}

		if (!skip)
			pcb_pstk_new(subc->data, -1, 0, rx, ry, lov->clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));

		x += dx * pitch;
		y += dy * pitch;
	}
}

static int line_of_vias_gen(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_line_t *line;
	pcb_board_t *pcb = pcb_data_get_top(subc->data);
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];
	line_of_vias *lov;

	if (subc->extobj_data == NULL)
		line_of_vias_unpack(subc);

	lov = subc->extobj_data;
	if (lov->pitch < PCB_MM_TO_COORD(0.001)) {
		pcb_message(PCB_MSG_ERROR, "line_of_vias_gen(): can not generate line-of-vias, pitch value is too small\n");
		return -1;
	}


	pcb_exto_regen_begin(subc);
	for(line = linelist_first(&ly->Line); line != NULL; line = linelist_next(line))
		line_of_vias_gen_line(pcb, subc, line);


	{
		line_geo_def;
		line = linelist_first(&ly->Line);
		line_geo_calc(line);
		pcb_subc_move_origin_to(subc, line->Point1.X - dy * PCB_SUBC_AUX_UNIT, line->Point1.Y + dx * PCB_SUBC_AUX_UNIT, 0);
	}

	return pcb_exto_regen_end(subc);
}

static void draw_mark_line(pcb_draw_info_t *info, pcb_subc_t *subc, pcb_line_t *line)
{
	int selected;
	double disp = PCB_MM_TO_COORD(0.05);
	double arrow = PCB_MM_TO_COORD(0.2);
	double ax, ay, ax1, ay1, ax2, ay2;
	line_geo_def;
	pcb_coord_t x1 = line->Point1.X, y1 = line->Point1.Y, x2 = line->Point2.X, y2 = line->Point2.Y;
/*	line_of_vias *lov = subc->extobj_data;*/

	line_geo_calc(line);

	selected = PCB_FLAG_TEST(PCB_FLAG_SELECTED, line);
	pcb_render->set_color(pcb_draw_out.fgGC, selected ? &conf_core.appearance.color.selected : &conf_core.appearance.color.extobj);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, -1);
	pcb_render->draw_line(pcb_draw_out.fgGC, x1 - dy * +disp, y1 + dx * +disp, x2 - dy * +disp, y2 + dx * +disp);
	pcb_render->draw_line(pcb_draw_out.fgGC, x1 - dy * -disp, y1 + dx * -disp, x2 - dy * -disp, y2 + dx * -disp);

	pcb_hid_set_line_width(pcb_draw_out.fgGC, -2);
	ax = x1 + dx * arrow;
	ay = y1 + dy * arrow;
	ax1 = x1 - dy * +arrow;
	ay1 = y1 + dx * +arrow;
	ax2 = x1 - dy * -arrow;
	ay2 = y1 + dx * -arrow;
	pcb_render->draw_line(pcb_draw_out.fgGC, ax1, ay1, ax2, ay2);
	pcb_render->draw_line(pcb_draw_out.fgGC, ax1, ay1, ax, ay);
	pcb_render->draw_line(pcb_draw_out.fgGC, ax2, ay2, ax, ay);
}

static void pcb_line_of_vias_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	pcb_line_t *line;
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];

	if (subc->extobj_data == NULL)
		line_of_vias_unpack(subc);

	for(line = linelist_first(&ly->Line); line != NULL; line = linelist_next(line))
		draw_mark_line(info, subc, line);

	pcb_exto_draw_makr(info, subc);
}


static void pcb_line_of_vias_float_pre(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("LoV: edit pre %ld %ld\n", subc->ID, edit_obj->ID);
	line_of_vias_clear(subc);
}

static void pcb_line_of_vias_float_geo(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("LoV: edit geo %ld %ld\n", subc->ID, edit_obj == NULL ? -1 : edit_obj->ID);
	line_of_vias_gen(subc, edit_obj);
}

static pcb_extobj_new_t pcb_line_of_vias_float_new(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_trace("LoV: float new %ld %ld\n", subc->ID, floater->ID);
	return PCB_EXTONEW_FLOATER;
}

static pcb_extobj_del_t pcb_line_of_vias_float_del(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];
	long len = linelist_length(&ly->Line);

	pcb_trace("LoV: float del %ld %ld edit-objs=%ld\n", subc->ID, floater->ID, len);

	return len == 1 ? PCB_EXTODEL_SUBC : PCB_EXTODEL_FLOATER; /* removing the last floater should remove the subc */
}


static void pcb_line_of_vias_chg_attr(pcb_subc_t *subc, const char *key, const char *value)
{
	pcb_trace("LoV chg_attr\n");
	if (strncmp(key, "extobj::", 8) == 0) {
		line_of_vias_clear(subc);
		line_of_vias_unpack(subc);
		line_of_vias_gen(subc, NULL);
	}
}


static pcb_subc_t *pcb_line_of_vias_conv_objs(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from)
{
	long n;
	pcb_subc_t *subc;
	pcb_line_t *l;
	pcb_layer_t *ly;
	pcb_dflgmap_t layers[] = {
		{"edit", PCB_LYT_DOC, "extobj", 0, 0},
		{NULL, 0, NULL, 0, 0}
	};

	pcb_trace("LoV: conv_objs\n");

	/* refuse anything that's not a line */
	for(n = 0; n < objs->used; n++) {
		l = objs->array[n];
		if (l->type != PCB_OBJ_LINE)
			return NULL;
	}

	/* use the layer of the first object */
	l = objs->array[0];
	layers[0].lyt = pcb_layer_flags_(l->parent.layer);
	pcb_layer_purpose_(l->parent.layer, &layers[0].purpose);

	subc = pcb_exto_create(dst, "line-of-vias", layers, l->Point1.X, l->Point1.Y, 0, copy_from);
	if (copy_from == NULL)
		pcb_attribute_put(&subc->Attributes, "extobj::pitch", "4mm");

	/* create edit-objects */
	ly = &subc->data->Layer[LID_EDIT];
	for(n = 0; n < objs->used; n++) {
		l = pcb_line_dup(ly, objs->array[n]);
		PCB_FLAG_SET(PCB_FLAG_FLOATER, l);
		PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, l);
		pcb_attribute_put(&l->Attributes, "extobj::role", "edit");
	}

	/* create the padstack prototype */
TODO("pstk #21: do not work in comp mode, use a pstk proto + remove the plugin dependency when done")
	pcb_pstk_new_compat_via(subc->data, -1, l->Point1.X, l->Point1.Y,
		conf_core.design.via_drilling_hole, conf_core.design.via_thickness, conf_core.design.clearance,
		0, PCB_PSTK_COMPAT_ROUND, pcb_true);

	line_of_vias_unpack(subc);
	line_of_vias_gen(subc, NULL);
	return subc;
}


static void pcb_line_of_vias_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pcb_subc_t *subc = caller_data;
	line_of_vias *lov = subc->extobj_data;

	PCB_DAD_FREE(lov->dlg);
	lov->gui_active = 0;
}

static void pcb_line_of_vias_gui_propedit(pcb_subc_t *subc)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	line_of_vias *lov;

	pcb_trace("LoV: gui propedit\n");

	if (subc->extobj_data == NULL)
		line_of_vias_unpack(subc);
	lov = subc->extobj_data;

	pcb_trace("LoV: active=%d\n", lov->gui_active);
	if (lov->gui_active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(lov->dlg);
		PCB_DAD_COMPFLAG(lov->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(lov->dlg, 2);
			pcb_exto_dlg_coord(lov->dlg, subc, "pitch", "extobj::pitch", "target distance between center of vias");
			pcb_exto_dlg_coord(lov->dlg, subc, "clearance", "extobj::clearance", "global clarance value on vias");
		PCB_DAD_END(lov->dlg);
		PCB_DAD_BUTTON_CLOSES(lov->dlg, clbtn);
	PCB_DAD_END(lov->dlg);

	/* set up the context */
	lov->gui_active = 1;

	PCB_DAD_NEW("line_of_vias", lov->dlg, "Line of vias", subc, pcb_false, pcb_line_of_vias_close_cb);
}

static pcb_extobj_t pcb_line_of_vias = {
	"line-of-vias",
	pcb_line_of_vias_draw_mark,
	pcb_line_of_vias_float_pre,
	pcb_line_of_vias_float_geo,
	pcb_line_of_vias_float_new,
	pcb_line_of_vias_float_del,
	pcb_line_of_vias_chg_attr,
	pcb_line_of_vias_del_pre,
	pcb_line_of_vias_conv_objs,
	pcb_line_of_vias_gui_propedit
};

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Extended object for a bus; edit: thick line; places parallel thin lines
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
#define LID_TARGET 1

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int gui_active;
	int width;
	pcb_coord_t pitch;
	pcb_coord_t thickness;
	pcb_coord_t clearance;

	/* cache: */
	pcb_coord_t vthickness; /* virtual thickness (edit object's) */
} bus_t;

static void pcb_bus_del_pre(pcb_subc_t *subc)
{
	bus_t *bus = subc->extobj_data;
	pcb_trace("bus del_pre\n");

	if ((bus != NULL) && (bus->gui_active))
		PCB_DAD_FREE(bus->dlg);

	free(bus);
	subc->extobj_data = NULL;
}

static void bus_unpack(pcb_subc_t *obj)
{
	bus_t *bus;

	if (obj->extobj_data == NULL)
		obj->extobj_data = calloc(sizeof(bus_t), 1);
	bus = obj->extobj_data;

	pcb_extobj_unpack_int(obj, &bus->width, "extobj::width");
	pcb_extobj_unpack_coord(obj, &bus->thickness, "extobj::thickness");
	pcb_extobj_unpack_coord(obj, &bus->clearance, "extobj::clearance");
	pcb_extobj_unpack_coord(obj, &bus->pitch, "extobj::pitch");
	bus->vthickness = (bus->width - 1) * bus->pitch + bus->thickness;
}

/* remove all bus traces from the subc */
static void bus_clear(pcb_subc_t *subc)
{
	pcb_line_t *l, *next;
	pcb_layer_t *ly = &subc->data->Layer[LID_TARGET];

	for(l = linelist_first(&ly->Line); l != NULL; l = next) {
		next = linelist_next(l);
		if (PCB_FLAG_TEST(PCB_FLAG_FLOATER, l)) continue; /* do not free the floater */
		pcb_line_free(l);
	}
}

#define bus_seg_def \
	double vx, vy, nx, ny, len

#define bus_seg_calc(l) \
	do { \
		vx = l->Point2.X - l->Point1.X; \
		vy = l->Point2.Y - l->Point1.Y; \
		len = sqrt(vx*vx + vy *vy); \
		vx /= len; \
		vy /= len; \
		nx = -vy; \
		ny = vx; \
	} while(0)

static int close_enough(pcb_point_t a, pcb_point_t b)
{
	pcb_coord_t dx = a.X - b.X, dy = a.Y - b.Y;

	if (dx < -1) return 0;
	if (dx > +1) return 0;
	if (dy < -1) return 0;
	if (dy > +1) return 0;
	return 1;
}

/* create all new padstacks */
static int bus_gen(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	bus_t *bus = subc->extobj_data;
	pcb_line_t *l1 = NULL, *l, *tr, *ltmp;
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];
	pcb_layer_t *tly = &subc->data->Layer[LID_TARGET];
	double o0;

	if (subc->extobj_data == NULL) {
		bus_unpack(subc);
		bus = subc->extobj_data;
	}


	o0 = ((bus->width - 1) * bus->pitch)/2;
	for(l = linelist_first(&ly->Line); l != NULL; l = linelist_next(l)) {
		pcb_rtree_box_t sb;
		pcb_rtree_it_t it;
		bus_seg_def;
		double o = o0, a, a1 = 0, a2 = 0, tune1, tune2;
		int n, c1 = 0, c2 = 0;

		if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, l)) continue; /* gen only for floater */

		l->Thickness = bus->vthickness;
		if (l1 == NULL) l1 = l;

		pcb_trace("line\n");
		sb.x1 = l->Point1.X-1; sb.y1 = l->Point1.Y-1;
		sb.x2 = l->Point1.X+1; sb.y2 = l->Point1.Y+1;
		for(ltmp = pcb_rtree_first(&it, ly->line_tree, &sb); ltmp != NULL; ltmp = pcb_rtree_next(&it))
		{
			if (ltmp == l) continue;
			if (close_enough(l->Point1, ltmp->Point1)) {
				a1 = atan2(ltmp->Point2.Y - ltmp->Point1.Y, ltmp->Point2.X - ltmp->Point1.X);
				c1 = 1;
/*				pcb_trace(" conn1 2-1 %f\n", a1);*/
				break;
			}
			else if (close_enough(l->Point1, ltmp->Point2)) {
				a1 = atan2(ltmp->Point1.Y - ltmp->Point2.Y, ltmp->Point1.X - ltmp->Point2.X);
				c1 = 1;
/*				pcb_trace(" conn1 1-2 %f\n", a1);*/
				break;
			}
		}
		sb.x1 = l->Point2.X-1; sb.y1 = l->Point2.Y-1;
		sb.x2 = l->Point2.X+1; sb.y2 = l->Point2.Y+1;
		for(ltmp = pcb_rtree_first(&it, ly->line_tree, &sb); ltmp != NULL; ltmp = pcb_rtree_next(&it))
		{
			if (ltmp == l) continue;
			if (close_enough(l->Point2, ltmp->Point1)) {
				a2 = atan2(ltmp->Point2.Y - ltmp->Point1.Y, ltmp->Point2.X - ltmp->Point1.X);
				c2 = 1;
/*				pcb_trace(" conn2 2-1 %f\n", a1);*/
				break;
			}
			if (close_enough(l->Point2, ltmp->Point2)) {
				a2 = atan2(ltmp->Point1.Y - ltmp->Point2.Y, ltmp->Point1.X - ltmp->Point2.X);
				c2 = 1;
/*				pcb_trace(" conn2 2-1 %f\n", a1);*/
				break;
			}
		}

		bus_seg_calc(l);

		if (c1) {
			double a0 = atan2(l->Point2.Y - l->Point1.Y, l->Point2.X - l->Point1.X);
pcb_trace("c1 a0=%f a1=%f\n", a0 * PCB_RAD_TO_DEG, a1 * PCB_RAD_TO_DEG);
			a1 = M_PI - a1 - a0;
			a1 = -a1;
			tune1 = tan(a1/2.0);
		}
		else
			tune1 = 0;
	
		if (c2) {
			double a0 = atan2(l->Point1.Y - l->Point2.Y, l->Point1.X - l->Point2.X);
pcb_trace("c1 a0=%f a2=%f\n", a0 * PCB_RAD_TO_DEG, a2 * PCB_RAD_TO_DEG);
			a2 = M_PI - a2 - a0;
			tune2 = tan(a2/2.0);
		}
		else
			tune2 = 0;

		pcb_trace("tune: %f:%f  %f:%f\n", a1 * PCB_RAD_TO_DEG, tune1, a2 * PCB_RAD_TO_DEG, tune2);

		for(n = 0; n < bus->width; n++,o-=bus->pitch) {
			pcb_trace(" off1: %f %f %ml = %ml\n", vx, tune1, (pcb_coord_t)o, (pcb_coord_t)(vx * tune1 * o));
			pcb_trace(" off2: %f %f %ml = %ml\n", vx, tune2, (pcb_coord_t)o, (pcb_coord_t)(vx * tune2 * o));
			tr = pcb_line_new(tly,
				l->Point1.X + nx * o + vx * tune1 * o, l->Point1.Y + ny * o + vy * tune1 * o,
				l->Point2.X + nx * o + vx * tune2 * o, l->Point2.Y + ny * o + vy * tune2 * o,
				bus->thickness, bus->clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
		}

	}

	if (l1 != NULL) {
pcb_trace("bus origin at %mm %mm\n", l1->Point1.X, l1->Point1.Y);
		pcb_subc_move_origin_to(subc, l1->Point1.X, l1->Point1.Y, 0);
	}
	return pcb_exto_regen_end(subc);
}

static void pcb_bus_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	bus_t *bus = subc->extobj_data;
	pcb_line_t *l;
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];

	if (subc->extobj_data == NULL) {
		bus_unpack(subc);
		bus = subc->extobj_data;
	}

	pcb_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.extobj);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, -1);

	for(l = linelist_first(&ly->Line); l != NULL; l = linelist_next(l)) {
		pcb_coord_t x, y;
		double o;
		bus_seg_def;

		if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, l)) continue;

		bus_seg_calc(l);

		x = l->Point1.X; y = l->Point1.Y;
		for(o = 0; o < len; o += PCB_MM_TO_COORD(5), x += vx*PCB_MM_TO_COORD(5), y += vy*PCB_MM_TO_COORD(5))
			pcb_render->draw_line(pcb_draw_out.fgGC,
				x + nx * bus->vthickness/2, y + ny * bus->vthickness/2,
				x - nx * bus->vthickness/2, y - ny * bus->vthickness/2);
	}

	pcb_exto_draw_makr(info, subc);
}


static void pcb_bus_float_pre(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("bus: edit pre %ld %ld\n", subc->ID, edit_obj->ID);
	bus_clear(subc);
}

static void pcb_bus_float_geo(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("bus: edit geo %ld %ld\n", subc->ID, edit_obj == NULL ? -1 : edit_obj->ID);
	bus_gen(subc, edit_obj);
}

static pcb_extobj_new_t pcb_bus_float_new(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_trace("bus: float new %ld %ld\n", subc->ID, floater->ID);
	return PCB_EXTONEW_FLOATER;
}

static pcb_extobj_del_t pcb_bus_float_del(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];
	long len = linelist_length(&ly->Line);

	pcb_trace("bus: float del %ld %ld edit-objs=%ld\n", subc->ID, floater->ID, len);

	return len == 1 ? PCB_EXTODEL_SUBC : PCB_EXTODEL_FLOATER; /* removing the last floater should remove the subc */
}


static void pcb_bus_chg_attr(pcb_subc_t *subc, const char *key, const char *value)
{
	pcb_trace("bus chg_attr\n");
	if (strncmp(key, "extobj::", 8) == 0) {
		bus_clear(subc);
		bus_unpack(subc);
		bus_gen(subc, NULL);
	}
}


static pcb_subc_t *pcb_bus_conv_objs(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from)
{
	long n;
	pcb_subc_t *subc;
	pcb_line_t *l;
	pcb_layer_t *ly;
	pcb_dflgmap_t layers[] = {
		{"edit", PCB_LYT_DOC, "extobj", 0, 0},
		{"target", PCB_LYT_COPPER, NULL, 0, 0},
		{NULL, 0, NULL, 0, 0}
	};

	pcb_trace("bus: conv_objs\n");

	/* refuse anything that's not a line */
	for(n = 0; n < objs->used; n++) {
		l = objs->array[n];
		if (l->type != PCB_OBJ_LINE)
			return NULL;
	}

	/* use the layer of the first object for edit */
	l = objs->array[0];
	layers[0].lyt = pcb_layer_flags_(l->parent.layer);
	pcb_layer_purpose_(l->parent.layer, &layers[0].purpose);

	/* use current layer for target */
	ly = PCB_CURRLAYER(PCB);
	layers[1].lyt = pcb_layer_flags_(ly);
	pcb_layer_purpose_(ly, &layers[1].purpose);


	subc = pcb_exto_create(dst, "bus", layers, l->Point1.X, l->Point1.Y, 0, copy_from);
	if (copy_from == NULL) {
		pcb_attribute_put(&subc->Attributes, "extobj::width", "2");
		pcb_attribute_put(&subc->Attributes, "extobj::pitch", "1mm");
		pcb_attribute_put(&subc->Attributes, "extobj::thickness", "0.25mm");
		pcb_attribute_put(&subc->Attributes, "extobj::clearance", "0.2mm");
	}

	if (layers[1].lyt & PCB_LYT_INTERN) {
		TODO("Set offset and update binding");
	}

	bus_unpack(subc);

TODO("set vthickness");
	/* create edit-objects */
	ly = &subc->data->Layer[LID_EDIT];
	for(n = 0; n < objs->used; n++) {
		l = pcb_line_dup(ly, objs->array[n]);
		PCB_FLAG_SET(PCB_FLAG_FLOATER, l);
		PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, l);
		pcb_attribute_put(&l->Attributes, "extobj::role", "edit");
pcb_trace(" subc=%p l=%p\n", subc, ly);
	}

	bus_gen(subc, NULL);
	return subc;
}


static void pcb_bus_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pcb_subc_t *subc = caller_data;
	bus_t *bus = subc->extobj_data;

	PCB_DAD_FREE(bus->dlg);
	bus->gui_active = 0;
}

static void pcb_bus_gui_propedit(pcb_subc_t *subc)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	bus_t *bus;

	pcb_trace("bus: gui propedit\n");

	if (subc->extobj_data == NULL)
		bus_unpack(subc);
	bus = subc->extobj_data;

	pcb_trace("bus: active=%d\n", bus->gui_active);
	if (bus->gui_active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(bus->dlg);
		PCB_DAD_COMPFLAG(bus->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(bus->dlg, 2);
			pcb_exto_dlg_int(bus->dlg, subc, "width", "extobj::width", "number of traces in the bus");
			pcb_exto_dlg_coord(bus->dlg, subc, "thickness", "extobj::thickness", "thickness of each trace");
			pcb_exto_dlg_coord(bus->dlg, subc, "clearance", "extobj::clearance", "clearance on each trace");
			pcb_exto_dlg_coord(bus->dlg, subc, "pitch", "extobj::pitch", "distance between trace centerlines");
		PCB_DAD_END(bus->dlg);
		PCB_DAD_BUTTON_CLOSES(bus->dlg, clbtn);
	PCB_DAD_END(bus->dlg);

	/* set up the context */
	bus->gui_active = 1;

	PCB_DAD_NEW("bus", bus->dlg, "Bus", subc, pcb_false, pcb_bus_close_cb);
}

static pcb_extobj_t pcb_bus = {
	"bus",
	pcb_bus_draw_mark,
	pcb_bus_float_pre,
	pcb_bus_float_geo,
	pcb_bus_float_new,
	pcb_bus_float_del,
	pcb_bus_chg_attr,
	pcb_bus_del_pre,
	pcb_bus_conv_objs,
	pcb_bus_gui_propedit
};

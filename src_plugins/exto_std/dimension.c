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
	int style;
	pcb_coord_t displace;
	const char *fmt;

	unsigned int valid:1;
	double x1, y1, x2, y2, len, dx, dy;
} dimension;

static int dimension_update_src(dimension *dim, pcb_any_obj_t *edit)
{
	dim->valid = 0;
	switch(edit->type) {
		case PCB_OBJ_LINE:
			{
				pcb_line_t *line = (pcb_line_t *)edit;
				dim->x1 = line->Point1.X; dim->y1 = line->Point1.Y;
				dim->x2 = line->Point2.X; dim->y2 = line->Point2.Y;
				dim->valid = 1;
			}
			break;
		default:
			/* can not dimension the given object type */
			break;
	}


	if (dim->valid) {
		int swap;
		if (dim->x1 == dim->x2)
			swap = dim->y1 < dim->y2;
		else
			swap = dim->x1 < dim->x2;

		if (swap) {
			double tmp;
			tmp = dim->x1; dim->x1 = dim->x2; dim->x2 = tmp;
			tmp = dim->y1; dim->y1 = dim->y2; dim->y2 = tmp;
		}

		dim->dx = dim->x2 - dim->x1;
		dim->dy = dim->y2 - dim->y1;
		dim->len = sqrt(dim->dx*dim->dx + dim->dy*dim->dy);
		if (dim->len != 0) {
			dim->dx /= dim->len;
			dim->dy /= dim->len;
		}
	}

	return !dim->valid;
}

static void dimension_unpack(pcb_subc_t *obj)
{
	dimension *dim;
	pcb_any_obj_t *edit;

	if (obj->extobj_data == NULL)
		obj->extobj_data = calloc(sizeof(dimension), 1);
	dim = obj->extobj_data;

	pcb_extobj_unpack_coord(obj, &dim->displace, "extobj::displace");
	dim->fmt = pcb_attribute_get(&obj->Attributes, "extobj::format");
	if (dim->fmt == NULL)
		dim->fmt = "%.03$mm";

	edit = pcb_extobj_get_editobj_by_attr(obj);
	if (edit != NULL)
		dimension_update_src(dim, edit);
}

/* remove all existing graphics from the subc */
static void dimension_clear(pcb_subc_t *subc)
{
	pcb_line_t *l;
	pcb_poly_t *p;
	pcb_text_t *t;

	for(l = linelist_first(&subc->data->Layer[0].Line); l != NULL; l = linelist_first(&subc->data->Layer[0].Line)) {
		pcb_poly_restore_to_poly(l->parent.data, PCB_OBJ_LINE, NULL, l);
		pcb_line_free(l);
	}

	for(p = polylist_first(&subc->data->Layer[0].Polygon); p != NULL; p = polylist_first(&subc->data->Layer[0].Polygon)) {
		pcb_poly_restore_to_poly(p->parent.data, PCB_OBJ_POLY, NULL, p);
		pcb_poly_free(p);
	}

	for(t = textlist_first(&subc->data->Layer[0].Text); t != NULL; t = textlist_first(&subc->data->Layer[0].Text)) {
		pcb_poly_restore_to_poly(t->parent.data, PCB_OBJ_TEXT, NULL, t);
		pcb_text_free(t);
	}
}

static void draw_arrow(dimension *dim, pcb_data_t *data, pcb_layer_t *ly, pcb_coord_t x1, pcb_coord_t y1, double arrx, double arry)
{
	pcb_poly_t *p = pcb_poly_new(ly, 0, pcb_flag_make(0));
	pcb_poly_point_new(p, x1, y1);
	pcb_poly_point_new(p, x1 + dim->dx * arrx - dim->dy * arry, y1 + dim->dy * arrx + dim->dx * arry);
	pcb_poly_point_new(p, x1 + dim->dx * arrx + dim->dy * arry, y1 + dim->dy * arrx - dim->dx * arry);
	pcb_add_poly_on_layer(ly, p);
	pcb_poly_init_clip(data, ly, p);
}

/* create the graphics */
static int dimension_gen(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	dimension *dim;
	pcb_board_t *pcb;
	pcb_layer_t *ly;
	double ang, dispe, arrx = PCB_MM_TO_COORD(2), arry = PCB_MM_TO_COORD(0.5);
	pcb_coord_t x1, y1, x2, y2, x1e, y1e, x2e, y2e, tx, ty, x, y;
	pcb_text_t *t;
	char ttmp[128];

	pcb = pcb_data_get_top(subc->data);

	if (subc->extobj_data == NULL)
		dimension_unpack(subc);
	dim = subc->extobj_data;
	if (dimension_update_src(dim, edit_obj) != 0)
		return -1;

	pcb_exto_regen_begin(subc);

	ly = &subc->data->Layer[0];
	
	/* endpoints of the displaced baseline */
	x1 = pcb_round(dim->x1 + dim->displace * -dim->dy);
	y1 = pcb_round(dim->y1 + dim->displace * +dim->dx);
	x2 = pcb_round(dim->x2 + dim->displace * -dim->dy);
	y2 = pcb_round(dim->y2 + dim->displace * +dim->dx);

	/* endpoints of the extended-displaced baseline */
	dispe = dim->displace + PCB_MM_TO_COORD(0.5);
	x1e = pcb_round(dim->x1 + dispe * -dim->dy);
	y1e = pcb_round(dim->y1 + dispe * +dim->dx);
	x2e = pcb_round(dim->x2 + dispe * -dim->dy);
	y2e = pcb_round(dim->y2 + dispe * +dim->dx);

	/* main dim line */
	pcb_line_new(ly,
		x1 + arrx * dim->dx, y1 + arrx * dim->dy,
		x2 - arrx * dim->dx, y2 - arrx * dim->dy,
		PCB_MM_TO_COORD(0.25), 0, pcb_flag_make(PCB_FLAG_FLOATER));

	/* guide lines */
	pcb_line_new(ly, dim->x1, dim->y1, x1e, y1e, PCB_MM_TO_COORD(0.25), 0, pcb_flag_make(0));
	pcb_line_new(ly, dim->x2, dim->y2, x2e, y2e, PCB_MM_TO_COORD(0.25), 0, pcb_flag_make(0));

	/* arrows */
	draw_arrow(dim, subc->data, ly, x1, y1, arrx, arry);
	draw_arrow(dim, subc->data, ly, x2, y2, -arrx, arry);

	/* text */
	pcb_snprintf(ttmp, sizeof(ttmp), dim->fmt, (pcb_coord_t)dim->len);
	t = pcb_text_new(ly, pcb_font(PCB, 0, 0), 0, 0, 0, 100, 0, ttmp, pcb_flag_make(0));
	tx = t->BoundingBox.X2 - t->BoundingBox.X1;
	ty = t->BoundingBox.Y2 - t->BoundingBox.Y1;

	x = (x1+x2)/2; y = (y1+y2)/2;
	x += tx/2 * dim->dx; y += tx/2 * dim->dy;
	x += ty * -dim->dy; y += ty * dim->dx;
	ang = atan2(-dim->dy, dim->dx) + M_PI;
	if ((ang > 0.001) || (ang < -0.001))
		pcb_text_rotate(t, 0, 0, cos(ang), sin(ang), ang * PCB_RAD_TO_DEG);
	pcb_text_move(t,  x, y);
	return pcb_exto_regen_end(subc);
}


static void pcb_dimension_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
}

pcb_any_obj_t *pcb_dimension_get_edit_obj(pcb_subc_t *obj)
{
	pcb_any_obj_t *edit = pcb_extobj_get_editobj_by_attr(obj);

	if (edit->type != PCB_OBJ_LINE)
		return NULL;

	return edit;
}



static void pcb_dimension_edit_pre(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("dim: edit pre %ld %ld\n", subc->ID, edit_obj->ID);
	dimension_clear(subc);
}

static void pcb_dimension_edit_geo(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_trace("dim: edit geo %ld %ld\n", subc->ID, edit_obj->ID);
	dimension_gen(subc, edit_obj);
}

static void pcb_dimension_float_pre(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_trace("dim: float pre %ld %ld\n", subc->ID, floater->ID);
}

static void pcb_dimension_float_geo(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_trace("dim: float geo %ld %ld\n", subc->ID, floater->ID);
}


static pcb_extobj_t pcb_dimension = {
	"dimension",
	pcb_dimension_draw_mark,
	pcb_dimension_get_edit_obj,
	pcb_dimension_edit_pre,
	pcb_dimension_edit_geo,
	pcb_dimension_float_pre,
	pcb_dimension_float_geo
};

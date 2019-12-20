/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Extended object for a dimension; draws a dimension line over the size of the edit-object
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

#include "search.h"

typedef struct {
	int style;
	pcb_coord_t displace;
	const char *fmt;

	unsigned int valid:1;
	double x1, y1, x2, y2, len, dx, dy;
} dimension;

static void pcb_dimension_del_pre(pcb_subc_t *subc)
{
	pcb_trace("dim del_pre\n");
	free(subc->extobj_data);
	subc->extobj_data = NULL;
}

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
	pcb_line_t *l, *next;
	pcb_poly_t *p;
	pcb_text_t *t;

	for(l = linelist_first(&subc->data->Layer[0].Line); l != NULL; l = next) {
		next = linelist_next(l);
		if (PCB_FLAG_TEST(PCB_FLAG_FLOATER, l)) continue; /* do not free the floater */
		pcb_line_free(l);
	}

	for(p = polylist_first(&subc->data->Layer[0].Polygon); p != NULL; p = polylist_first(&subc->data->Layer[0].Polygon)) {
		pcb_poly_free(p);
	}

	for(t = textlist_first(&subc->data->Layer[0].Text); t != NULL; t = textlist_first(&subc->data->Layer[0].Text)) {
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
	pcb_line_t *flt;
	double ang, deg, dispe, rotsign;
	double arrx = PCB_MM_TO_COORD(2), arry = PCB_MM_TO_COORD(0.5);
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
	dispe = PCB_MM_TO_COORD(0.5);
	if (dim->displace < 0)
		dispe = -dispe;
	dispe = dim->displace + dispe;

	x1e = pcb_round(dim->x1 + dispe * -dim->dy);
	y1e = pcb_round(dim->y1 + dispe * +dim->dx);
	x2e = pcb_round(dim->x2 + dispe * -dim->dy);
	y2e = pcb_round(dim->y2 + dispe * +dim->dx);

	/* main dim line */
	flt = linelist_first(&subc->data->Layer[0].Line);
	if (flt == NULL) { /* create the floater if it doesn't exist */
		flt = (pcb_any_obj_t *)pcb_line_new(ly,
			x1 + arrx * dim->dx, y1 + arrx * dim->dy,
			x2 - arrx * dim->dx, y2 - arrx * dim->dy,
			PCB_MM_TO_COORD(0.25), 0, pcb_flag_make(PCB_FLAG_FLOATER));
	}
	else { /* modify the floater if it exists */
		if (ly->line_tree != NULL)
			pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)flt);

		flt->Point1.X = x1 + arrx * dim->dx; flt->Point1.Y = y1 + arrx * dim->dy;
		flt->Point2.X = x2 - arrx * dim->dx; flt->Point2.Y = y2 - arrx * dim->dy;

		pcb_line_bbox(flt);
		if (ly->line_tree != NULL)
			pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)flt);
	}

	/* guide lines */
	pcb_line_new(ly, dim->x1, dim->y1, x1e, y1e, PCB_MM_TO_COORD(0.25), 0, pcb_flag_make(0));
	pcb_line_new(ly, dim->x2, dim->y2, x2e, y2e, PCB_MM_TO_COORD(0.25), 0, pcb_flag_make(0));

	/* arrows */
	draw_arrow(dim, subc->data, ly, x1, y1, arrx, arry);
	draw_arrow(dim, subc->data, ly, x2, y2, -arrx, arry);

	/* text */
	if (pcb_safe_snprintf(ttmp, sizeof(ttmp), PCB_SAFEPRINT_COORD_ONLY | 1, dim->fmt, (pcb_coord_t)dim->len) < 0)
		strcpy(ttmp, "<invalid format>");
	t = pcb_text_new(ly, pcb_font(PCB, 0, 0), 0, 0, 0, 100, 0, ttmp, pcb_flag_make(0));
	tx = t->BoundingBox.X2 - t->BoundingBox.X1;
	ty = t->BoundingBox.Y2 - t->BoundingBox.Y1;

	ang = atan2(-dim->dy, dim->dx) + M_PI;
	deg = ang * PCB_RAD_TO_DEG;
	if ((deg > 135) && (deg < 315)) {
		ang = ang-M_PI;
		deg = ang * PCB_RAD_TO_DEG;
		rotsign = -1;
	}
	else
		rotsign = +1;

	x = (x1+x2)/2; y = (y1+y2)/2;
	x += rotsign * tx/2 * dim->dx; y += rotsign * tx/2 * dim->dy;
	x += rotsign * ty * -dim->dy;  y += rotsign * ty * dim->dx;
	if ((ang > 0.001) || (ang < -0.001))
		pcb_text_rotate(t, 0, 0, cos(ang), sin(ang), deg);
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

static void pcb_dimension_float_pre(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	dimension_clear(subc);
}


static void pcb_dimension_dimline_geo(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	dimension *dim;
	pcb_any_obj_t *edit_obj;
	pcb_line_t *fline = floater, bline;
	pcb_coord_t fx, fy, cpx, cpy;
	double d;

	pcb_trace("dim: float geo %ld %ld\n", subc->ID, floater->ID);
	if (floater->type != PCB_OBJ_LINE)
		return;

	if (subc->extobj_data == NULL)
		dimension_unpack(subc);
	dim = subc->extobj_data;

	fx = pcb_round((double)(fline->Point1.X + fline->Point2.X)/2.0);
	fy = pcb_round((double)(fline->Point1.Y + fline->Point2.Y)/2.0);

	memset(&bline, 0, sizeof(bline));
	bline.Point1.X = dim->x1; bline.Point1.Y = dim->y1;
	bline.Point2.X = dim->x2; bline.Point2.Y = dim->y2;
	d = pcb_point_line_dist2(fx, fy, &bline);
	if (d != 0) {
		double side = ((dim->x2 - dim->x1)*(fy - dim->y1) - (dim->y2 - dim->y1)*(fx - dim->x1)); /* which side fx;fy is on */
		d = sqrt(d);
		if (side < 0)
			d = -d;
	}

pcb_trace("new disp: %mm f=%mm;%mm\n", (pcb_coord_t)d, fx, fy);

	if ((d > -PCB_MM_TO_COORD(0.1)) && (d < PCB_MM_TO_COORD(0.1)))
		return;

pcb_trace("let's do it!\n");

	edit_obj = pcb_extobj_get_editobj_by_attr(subc);
	if (edit_obj == NULL)
		return;
	dimension_clear(subc);
	dim->displace = d;
	dimension_gen(subc, edit_obj);
}

static void pcb_dimension_float_geo(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
/* TODO: call this when role is not "edit" */
/*	pcb_dimension_dimline_geo(subc, edit_obj)*/
	dimension_gen(subc, edit_obj);
}

static void pcb_dimension_float_new(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_trace("dim: float new %ld %ld\n", subc->ID, floater->ID);
}

static void pcb_dimension_chg_attr(pcb_subc_t *subc, const char *key, const char *value)
{
	pcb_trace("dim chg_attr\n");
	if (strncmp(key, "extobj::", 8) == 0) {
		pcb_any_obj_t *edit_obj = pcb_extobj_get_editobj_by_attr(subc);
		dimension_clear(subc);
		dimension_unpack(subc);
		dimension_gen(subc, edit_obj);
	}
}

static pcb_extobj_t pcb_dimension = {
	"dimension",
	pcb_dimension_draw_mark,
	pcb_dimension_get_edit_obj,
	pcb_dimension_float_pre,
	pcb_dimension_float_geo,
	pcb_dimension_float_new,
	pcb_dimension_chg_attr,
	pcb_dimension_del_pre
};

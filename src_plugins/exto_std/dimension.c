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

#define LID_EDIT 0
#define LID_TARGET 1

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int gui_active;

	int style;
	rnd_coord_t displace;
	const char *fmt;

	unsigned int valid:1;
	double x1, y1, x2, y2, len, dx, dy;
} dimension;

static pcb_any_obj_t *dimension_edit_obj(pcb_subc_t *subc)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];
	return (pcb_any_obj_t *)linelist_first(&ly->Line);
}

static void pcb_dimension_del_pre(pcb_subc_t *subc)
{
	rnd_trace("dim del_pre\n");
	free(subc->extobj_data);
	subc->extobj_data = NULL;
}

static int dimension_update_src(dimension *dim, pcb_any_obj_t *edit)
{
	dim->valid = 0;

	if (edit == NULL)
		return 0;

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

	edit = dimension_edit_obj(obj);
	dimension_update_src(dim, edit);
}

/* remove all existing graphics from the subc */
static void dimension_clear(pcb_subc_t *subc)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_TARGET];
	pcb_line_t *l, *next;
	pcb_poly_t *p;
	pcb_text_t *t;

	for(l = linelist_first(&ly->Line); l != NULL; l = next) {
		next = linelist_next(l);
		if (PCB_FLAG_TEST(PCB_FLAG_FLOATER, l)) continue; /* do not free the floater */
		pcb_line_free(l);
	}

	for(p = polylist_first(&ly->Polygon); p != NULL; p = polylist_first(&ly->Polygon)) {
		pcb_poly_free(p);
	}

	for(t = textlist_first(&ly->Text); t != NULL; t = textlist_first(&ly->Text)) {
		pcb_text_free(t);
	}
}

static void draw_arrow(dimension *dim, pcb_data_t *data, pcb_layer_t *ly, rnd_coord_t x1, rnd_coord_t y1, double arrx, double arry)
{
	pcb_poly_t *p = pcb_poly_new(ly, 0, pcb_flag_make(0));
	pcb_poly_point_new(p, x1, y1);
	pcb_poly_point_new(p, x1 + dim->dx * arrx - dim->dy * arry, y1 + dim->dy * arrx + dim->dx * arry);
	pcb_poly_point_new(p, x1 + dim->dx * arrx + dim->dy * arry, y1 + dim->dy * arrx - dim->dx * arry);
	pcb_add_poly_on_layer(ly, p);
	pcb_poly_init_clip(data, ly, p);
}

/* create the graphics */
static int dimension_gen(pcb_subc_t *subc)
{
	dimension *dim;
	pcb_layer_t *ly;
	pcb_line_t *flt;
	double ang, deg, dispe, rotsign;
	double arrx = RND_MM_TO_COORD(2), arry = RND_MM_TO_COORD(0.5);
	rnd_coord_t x1, y1, x2, y2, x1e, y1e, x2e, y2e, tx, ty, x, y, ex, ey;
	pcb_text_t *t;
	char ttmp[128];
	pcb_any_obj_t *edit_obj;

	if (subc->extobj_data == NULL)
		dimension_unpack(subc);
	dim = subc->extobj_data;
	edit_obj = dimension_edit_obj(subc);
	if (dimension_update_src(dim, edit_obj) != 0)
		return -1;

	pcb_exto_regen_begin(subc);

	ly = &subc->data->Layer[LID_TARGET];

	/* endpoints of the displaced baseline */
	x1 = rnd_round(dim->x1 + dim->displace * -dim->dy);
	y1 = rnd_round(dim->y1 + dim->displace * +dim->dx);
	x2 = rnd_round(dim->x2 + dim->displace * -dim->dy);
	y2 = rnd_round(dim->y2 + dim->displace * +dim->dx);

	/* endpoints of the extended-displaced baseline */
	dispe = RND_MM_TO_COORD(0.5);
	if (dim->displace < 0)
		dispe = -dispe;
	dispe = dim->displace + dispe;

	x1e = rnd_round(dim->x1 + dispe * -dim->dy);
	y1e = rnd_round(dim->y1 + dispe * +dim->dx);
	x2e = rnd_round(dim->x2 + dispe * -dim->dy);
	y2e = rnd_round(dim->y2 + dispe * +dim->dx);

	/* main dim line */
	flt = linelist_first(&subc->data->Layer[LID_TARGET].Line);
	if (flt == NULL) { /* create the floater if it doesn't exist */
		flt = pcb_line_new(ly,
			x1 + arrx * dim->dx, y1 + arrx * dim->dy,
			x2 - arrx * dim->dx, y2 - arrx * dim->dy,
			RND_MM_TO_COORD(0.25), 0, pcb_flag_make(PCB_FLAG_FLOATER));
		pcb_attribute_put(&flt->Attributes, "extobj::role", "dimline");
	}
	else { /* modify the floater if it exists */
		if (ly->line_tree != NULL)
			rnd_r_delete_entry(ly->line_tree, (rnd_box_t *)flt);

		flt->Point1.X = x1 + arrx * dim->dx; flt->Point1.Y = y1 + arrx * dim->dy;
		flt->Point2.X = x2 - arrx * dim->dx; flt->Point2.Y = y2 - arrx * dim->dy;

		pcb_line_bbox(flt);
		if (ly->line_tree != NULL)
			rnd_r_insert_entry(ly->line_tree, (rnd_box_t *)flt);
	}

	/* guide lines */
	pcb_line_new(ly, dim->x1, dim->y1, x1e, y1e, RND_MM_TO_COORD(0.25), 0, pcb_flag_make(0));
	pcb_line_new(ly, dim->x2, dim->y2, x2e, y2e, RND_MM_TO_COORD(0.25), 0, pcb_flag_make(0));

	/* arrows */
	draw_arrow(dim, subc->data, ly, x1, y1, arrx, arry);
	draw_arrow(dim, subc->data, ly, x2, y2, -arrx, arry);

	/* text */
	if (rnd_safe_snprintf(ttmp, sizeof(ttmp), RND_SAFEPRINT_COORD_ONLY | 1, dim->fmt, (rnd_coord_t)dim->len) < 0)
		strcpy(ttmp, "<invalid format>");
	t = pcb_text_new(ly, pcb_font(PCB, 0, 0), 0, 0, 0, 100, 0, ttmp, pcb_flag_make(0));
	tx = t->BoundingBox.X2 - t->BoundingBox.X1;
	ty = t->BoundingBox.Y2 - t->BoundingBox.Y1;

	ang = atan2(-dim->dy, dim->dx) + M_PI;
	deg = ang * RND_RAD_TO_DEG;
	if ((deg > 135) && (deg < 315)) {
		ang = ang-M_PI;
		deg = ang * RND_RAD_TO_DEG;
		rotsign = -1;
	}
	else
		rotsign = +1;

	x = ex = (x1+x2)/2; y = ey = (y1+y2)/2;
	x += rotsign * tx/2 * dim->dx; y += rotsign * tx/2 * dim->dy;
	x += rotsign * ty * -dim->dy;  y += rotsign * ty * dim->dx;
	if ((ang > 0.001) || (ang < -0.001))
		pcb_text_rotate(t, 0, 0, cos(ang), sin(ang), deg);
	pcb_text_move(t,  x, y);

	x = ex + dim->dy * PCB_SUBC_AUX_UNIT * 0.5 * rotsign;
	y = ey - dim->dx * PCB_SUBC_AUX_UNIT * 0.5 * rotsign;
	pcb_subc_move_origin_to(subc, x, y, 0);

	return pcb_exto_regen_end(subc);
}


static void pcb_dimension_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	pcb_exto_draw_mark(info, subc);
}

static void pcb_dimension_float_pre(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	rnd_trace("dim: float pre %ld %ld role=%s\n", subc->ID, floater->ID, floater->extobj_role);

	dimension_clear(subc);
}


static void pcb_dimension_dimline_geo(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	dimension *dim;
	pcb_line_t *fline = (pcb_line_t *)floater, bline;
	rnd_coord_t fx, fy;
	double d;
	char tmp[128];


	if (floater->type != PCB_OBJ_LINE)
		return;

	if (subc->extobj_data == NULL)
		dimension_unpack(subc);
	dim = subc->extobj_data;

	fx = rnd_round((double)(fline->Point1.X + fline->Point2.X)/2.0);
	fy = rnd_round((double)(fline->Point1.Y + fline->Point2.Y)/2.0);

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

rnd_trace("new disp: %mm f=%mm;%mm\n", (rnd_coord_t)d, fx, fy);

	if ((d > -RND_MM_TO_COORD(0.1)) && (d < RND_MM_TO_COORD(0.1)))
		return;

rnd_trace("let's do it!\n");

	dimension_clear(subc);
	dim->displace = d;
	rnd_snprintf(tmp, sizeof(tmp), "%.08$mH", (rnd_coord_t)d);
	pcb_attribute_put(&subc->Attributes, "extobj::displace", tmp);
	dimension_gen(subc);
}

static void pcb_dimension_float_geo(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	rnd_trace("dim: float geo %ld %ld role=%s\n", subc->ID, floater->ID, floater->extobj_role);

	if (floater->extobj_role == NULL)
		return;

	if (strcmp(floater->extobj_role, "edit") == 0)
		dimension_gen(subc);
	else if (strcmp(floater->extobj_role, "dimline") == 0)
		pcb_dimension_dimline_geo(subc, floater);

}

static pcb_extobj_new_t pcb_dimension_float_new(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	rnd_trace("dim: float new %ld %ld\n", subc->ID, floater->ID);
	return PCB_EXTONEW_SPAWN;
}

static pcb_extobj_del_t pcb_dimension_float_del(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	rnd_trace("dim: float del %ld %ld\n", subc->ID, floater->ID);
	return PCB_EXTODEL_SUBC;
}

static void pcb_dimension_chg_attr(pcb_subc_t *subc, const char *key, const char *value)
{
	rnd_trace("dim chg_attr\n");
	if (strncmp(key, "extobj::", 8) == 0) {
		dimension_clear(subc);
		dimension_unpack(subc);
		dimension_gen(subc);
	}
}

static pcb_subc_t *pcb_dimension_conv_objs(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from)
{
	pcb_subc_t *subc;
	pcb_line_t *l;
	pcb_layer_t *ly, *targetly = NULL;
	pcb_dflgmap_t layers[] = {
		{"edit", PCB_LYT_DOC, "extobj", 0, 0},
		{"target", PCB_LYT_DOC, "fab", 0, 0},
		{NULL, 0, NULL, 0, 0}
	};

	rnd_trace("dim: conv_objs\n");

	if (objs->used != 1)
		return NULL; /* there must be a single line */

	/* use the layer of the first object */
	l = objs->array[0];
	if (l->type != PCB_OBJ_LINE)
		return NULL;

	layers[0].lyt = pcb_layer_flags_(l->parent.layer);
	pcb_layer_purpose_(l->parent.layer, &layers[0].purpose);
	if ((copy_from != NULL) && (copy_from->data->LayerN > LID_TARGET) && (copy_from->data->Layer[LID_TARGET].meta.bound.real != NULL)) {
		targetly = copy_from->data->Layer[LID_TARGET].meta.bound.real;
	}
	else if (dst->parent_type == PCB_PARENT_BOARD) {
		pcb_board_t *pcb = dst->parent.board;
		targetly = PCB_CURRLAYER(pcb);
	}

	if (targetly != NULL) {
		layers[1].lyt = pcb_layer_flags_(targetly);
		pcb_layer_purpose_(targetly, &layers[1].purpose);
	}

	subc = pcb_exto_create(dst, "dimension", layers, l->Point1.X, l->Point1.Y, 0, copy_from);
	if (copy_from == NULL)
		pcb_attribute_put(&subc->Attributes, "extobj::displace", "4mm");

	/* create edit-objects */
	ly = &subc->data->Layer[LID_EDIT];
	l = pcb_line_dup(ly, objs->array[0]);
	PCB_FLAG_SET(PCB_FLAG_FLOATER, l);
	PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, l);
	pcb_attribute_put(&l->Attributes, "extobj::role", "edit");

	dimension_unpack(subc);
	dimension_gen(subc);
	return subc;
}

static void pcb_dimension_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	pcb_subc_t *subc = caller_data;
	dimension *dim = subc->extobj_data;

	RND_DAD_FREE(dim->dlg);
	dim->gui_active = 0;
}

static void pcb_dimension_gui_propedit(pcb_subc_t *subc)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	dimension *dim;

	rnd_trace("dim: gui propedit\n");

	if (subc->extobj_data == NULL)
		dimension_unpack(subc);
	dim = subc->extobj_data;

	rnd_trace("dim: active=%d\n", dim->gui_active);
	if (dim->gui_active)
		return; /* do not open another */

	RND_DAD_BEGIN_VBOX(dim->dlg);
		RND_DAD_COMPFLAG(dim->dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABLE(dim->dlg, 2);
			pcb_exto_dlg_coord(dim->dlg, subc, "displacement", "extobj::displace", "distance between base line and dimension line");
			pcb_exto_dlg_str(dim->dlg, subc, "format", "extobj::format", "printf coord format string of the dimension value");
		RND_DAD_END(dim->dlg);
		RND_DAD_BUTTON_CLOSES(dim->dlg, clbtn);
	RND_DAD_END(dim->dlg);

	/* set up the context */
	dim->gui_active = 1;

	RND_DAD_NEW("dimension", dim->dlg, "Dimension line", subc, rnd_false, pcb_dimension_close_cb);
}

static pcb_extobj_t pcb_dimension = {
	"dimension",
	pcb_dimension_draw_mark,
	pcb_dimension_float_pre,
	pcb_dimension_float_geo,
	pcb_dimension_float_new,
	pcb_dimension_float_del,
	pcb_dimension_chg_attr,
	pcb_dimension_del_pre,
	pcb_dimension_conv_objs,
	pcb_dimension_gui_propedit
};

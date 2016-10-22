 /*
  *                            COPYRIGHT
  *
  *  PCB, interactive printed circuit board design
  *  Copyright (C) 2006 Dan McMahill
  *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/*
 *  Based on the png exporter by Dan McMahill
 */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "layer.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "plugins.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_helper.h"
#include "hid_flags.h"


static HID svg_hid;

const char *svg_cookie = "svg HID";


typedef struct hid_gc_struct {
	HID *me_pointer;
	EndCapStyle cap;
	int width;
	char *color;
} hid_gc_struct;


static FILE *f = 0;


HID_Attribute svg_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 svg Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_svgfile 0

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-mode
@item --photo-mode
Export a photo realistic image of the layout.
@end ftable
%end-doc
*/
	{"photo-mode", "Photo-realistic export mode",
	 HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_mode 1

};

#define NUM_OPTIONS (sizeof(svg_attribute_list)/sizeof(svg_attribute_list[0]))

REGISTER_ATTRIBUTES(svg_attribute_list, svg_cookie)

static HID_Attr_Val svg_values[NUM_OPTIONS];

static HID_Attribute *svg_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".svg";

	if (PCB)
		derive_default_filename(PCB->Filename, &svg_attribute_list[HA_svgfile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return svg_attribute_list;
}

void svg_hid_export_to_file(FILE * the_file, HID_Attr_Val * options)
{
	static int saved_layer_stack[MAX_LAYER];
	BoxType region;

	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->MaxWidth;
	region.Y2 = PCB->MaxHeight;

	f = the_file;

	memcpy(saved_layer_stack, LayerStack, sizeof(LayerStack));

	{
		conf_force_set_bool(conf_core.editor.thin_draw, 0);
		conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
/*		conf_force_set_bool(conf_core.editor.check_planes, 0);*/
		conf_force_set_bool(conf_core.editor.show_solder_side, 0);
		conf_force_set_bool(conf_core.editor.show_mask, 0);

		if (options[HA_photo_mode].int_value) {
			conf_force_set_bool(conf_core.editor.show_mask, 1);
		}
	}

	hid_expose_callback(&svg_hid, &region, 0);

	conf_update(NULL); /* restore forced sets */
}


static void svg_do_export(HID_Attr_Val * options)
{
	const char *filename;
	int save_ons[MAX_LAYER + 2];
	int i;

	if (!options) {
		svg_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			svg_values[i] = svg_attribute_list[i].default_val;
		options = svg_values;
	}

/*
	if (options[HA_photo_mode].int_value) {
	}
*/

	filename = options[HA_svgfile].str_value;
	if (!filename)
		filename = "pcb.svg";

	f = fopen(filename, "wb");
	if (!f) {
		perror(filename);
		return;
	}

	hid_save_and_show_layer_ons(save_ons);

	svg_hid_export_to_file(f, options);

	hid_restore_layer_ons(save_ons);

	fclose(f);
}

static void svg_parse_arguments(int *argc, char ***argv)
{
	hid_register_attributes(svg_attribute_list, sizeof(svg_attribute_list) / sizeof(svg_attribute_list[0]), svg_cookie, 0);
	hid_parse_command_line(argc, argv);
}


static int svg_set_layer(const char *name, int group, int empty)
{
	return 0;
}


static hidGC svg_make_gc(void)
{
	hidGC rv = (hidGC) malloc(sizeof(hid_gc_struct));
	rv->me_pointer = &svg_hid;
	rv->cap = Trace_Cap;
	rv->width = 1;
	rv->color = NULL;
	return rv;
}

static void svg_destroy_gc(hidGC gc)
{
	free(gc);
}

static void svg_use_mask(int use_it)
{
	if (use_it == HID_MASK_CLEAR) {
		return;
	}
	if (use_it) {
	}
}

static void svg_set_color(hidGC gc, const char *name)
{
	if (name == NULL)
		name = "#ff0000";
	if (strcmp(gc->color, name) == 0)
		return;
	free(gc->color);
	gc->color = pcb_strdup(name);
}

static void svg_set_line_cap(hidGC gc, EndCapStyle style)
{
	gc->cap = style;
}

static void svg_set_line_width(hidGC gc, Coord width)
{
	gc->width = width;
}

static void svg_set_draw_xor(hidGC gc, int xor_)
{
	;
}

static void svg_draw_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
/*	gdImageRectangle(im, SCALE_X(x1), SCALE_Y(y1), SCALE_X(x2), SCALE_Y(y2), gc->color->c);*/
}

static void svg_fill_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	if (x1 > x2) {
		Coord t = x1;
		x2 = x2;
		x2 = t;
	}
	if (y1 > y2) {
		Coord t = y1;
		y2 = y2;
		y2 = t;
	}
}

static void svg_draw_line(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{

/*	if (x1 == x2 && y1 == y2) {
		Coord w = gc->width / 2;
		if (gc->cap != Square_Cap)
			svg_fill_circle(gc, x1, y1, w);
		else
			svg_fill_rect(gc, x1 - w, y1 - w, x1 + w, y1 + w);
		return;
	}*/

/*		gdImageLine(im, SCALE_X(x1), SCALE_Y(y1), SCALE_X(x2), SCALE_Y(y2), gdBrushed);*/
}

static void svg_draw_arc(hidGC gc, Coord cx, Coord cy, Coord width, Coord height, Angle start_angle, Angle delta_angle)
{
#if 0
	Angle sa, ea;

	/*
	 * zero angle arcs need special handling as gd will output either
	 * nothing at all or a full circle when passed delta angle of 0 or 360.
	 */
	if (delta_angle == 0) {
		Coord x = (width * cos(start_angle * M_PI / 180));
		Coord y = (width * sin(start_angle * M_PI / 180));
		x = cx - x;
		y = cy + y;
		svg_fill_circle(gc, x, y, gc->width / 2);
		return;
	}

	/* 
	 * in gdImageArc, 0 degrees is to the right and +90 degrees is down
	 * in pcb, 0 degrees is to the left and +90 degrees is down
	 */
	start_angle = 180 - start_angle;
	delta_angle = -delta_angle;
	if (show_solder_side) {
		start_angle = -start_angle;
		delta_angle = -delta_angle;
	}
	if (delta_angle > 0) {
		sa = start_angle;
		ea = start_angle + delta_angle;
	}
	else {
		sa = start_angle + delta_angle;
		ea = start_angle;
	}

	/* 
	 * make sure we start between 0 and 360 otherwise gd does
	 * strange things
	 */
	sa = NormalizeAngle(sa);
	ea = NormalizeAngle(ea);

	have_outline |= doing_outline;

#if 0
	printf("draw_arc %d,%d %dx%d %d..%d %d..%d\n", cx, cy, width, height, start_angle, delta_angle, sa, ea);
	printf("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
				 (void *)im, SCALE_X(cx), SCALE_Y(cy), SCALE(width), SCALE(height), sa, ea, gc->color->c);
#endif
	gdImageSetThickness(im, 0);
	linewidth = 0;
	gdImageArc(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * width), SCALE(2 * height), sa, ea, gdBrushed);
#endif
}

static void svg_fill_circle(hidGC gc, Coord cx, Coord cy, Coord radius)
{
/*
	gdImageSetThickness(im, 0);
	linewidth = 0;
	gdImageFilledEllipse(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * radius + my_bloat), SCALE(2 * radius + my_bloat), gc->color->c);
*/
}

static void svg_fill_polygon(hidGC gc, int n_coords, Coord * x, Coord * y)
{
#if 0
	int i;
	for (i = 0; i < n_coords; i++) {
		if (NOT_EDGE(x[i], y[i]))
			have_outline |= doing_outline;
		points[i].x = SCALE_X(x[i]);
		points[i].y = SCALE_Y(y[i]);
	}
	gdImageSetThickness(im, 0);
	linewidth = 0;
	gdImageFilledPolygon(im, points, n_coords, gc->color->c);
	free(points);
#endif
}

static void svg_calibrate(double xval, double yval)
{
	Message(PCB_MSG_ERROR, "svg_calibrate() not implemented");
	return;
}

static void svg_set_crosshair(int x, int y, int a)
{
}

static int svg_usage(const char *topic)
{
	fprintf(stderr, "\nsvg exporter command line arguments:\n\n");
	hid_usage(svg_attribute_list, sizeof(svg_attribute_list) / sizeof(svg_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x svg foo.pcb [svg options]\n\n");
	return 0;
}

#include "dolists.h"

pcb_uninit_t hid_export_svg_init()
{
	memset(&svg_hid, 0, sizeof(HID));

	common_nogui_init(&svg_hid);
	common_draw_helpers_init(&svg_hid);

	svg_hid.struct_size = sizeof(HID);
	svg_hid.name = "svg";
	svg_hid.description = "Scalable Vector Graphics export";
	svg_hid.exporter = 1;
	svg_hid.poly_before = 1;

	svg_hid.get_export_options = svg_get_export_options;
	svg_hid.do_export = svg_do_export;
	svg_hid.parse_arguments = svg_parse_arguments;
	svg_hid.set_layer = svg_set_layer;
	svg_hid.make_gc = svg_make_gc;
	svg_hid.destroy_gc = svg_destroy_gc;
	svg_hid.use_mask = svg_use_mask;
	svg_hid.set_color = svg_set_color;
	svg_hid.set_line_cap = svg_set_line_cap;
	svg_hid.set_line_width = svg_set_line_width;
	svg_hid.set_draw_xor = svg_set_draw_xor;
	svg_hid.draw_line = svg_draw_line;
	svg_hid.draw_arc = svg_draw_arc;
	svg_hid.draw_rect = svg_draw_rect;
	svg_hid.fill_circle = svg_fill_circle;
	svg_hid.fill_polygon = svg_fill_polygon;
	svg_hid.fill_rect = svg_fill_rect;
	svg_hid.calibrate = svg_calibrate;
	svg_hid.set_crosshair = svg_set_crosshair;

	svg_hid.usage = svg_usage;

#ifdef HAVE_SOME_FORMAT
	hid_register_hid(&svg_hid);

#endif
	return NULL;
}

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
#include <genvector/gds_char.h>

#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "error.h"
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


static pcb_hid_t svg_hid;

const char *svg_cookie = "svg HID";


typedef struct hid_gc_s {
	pcb_hid_t *me_pointer;
	pcb_cap_style_t cap;
	int width;
	char *color;
	int erase, drill;
} hid_gc_s;

static const char *CAPS(pcb_cap_style_t cap)
{
	switch (cap) {
		case Trace_Cap:
		case Round_Cap:
			return "round";
		case Square_Cap:
			return "square";
		case Beveled_Cap:
			return "butt";
	}
	return "";
}

static FILE *f = NULL;
static int group_open = 0;
static int opacity = 100, drawing_mask, drawing_hole, photo_mode, flip;

gds_t sbright, sdark, snormal;

/* Photo mode colors and hacks */
const char *board_color = "#464646";
const char *mask_color = "#00ff00";
float mask_opacity_factor = 0.5;

enum {
	PHOTO_MASK,
	PHOTO_SILK,
	PHOTO_COPPER,
	PHOTO_INNER
} photo_color;

struct {
	const char *bright;
	const char *normal;
	const char *dark;
	pcb_coord_t offs;
} photo_palette[] = {
	/* MASK */   { "#00ff00", "#00ff00", "#00ff00", PCB_MM_TO_COORD(0) },
	/* SILK */   { "#ffffff", "#eeeeee", "#aaaaaa", PCB_MM_TO_COORD(0) },
	/* COPPER */ { "#bbbbbb", "#707090", "#555555", PCB_MM_TO_COORD(0.05) },
	/* INNER */  { "#222222", "#111111", "#000000", PCB_MM_TO_COORD(0.05) }
};

pcb_hid_attribute_t svg_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 SVG Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_svgfile 0

/* %start-doc options "93 SVG Options"
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

/* %start-doc options "93 SVG Options"
@ftable @code
@cindex opacity
@item --opacity
Layer opacity
@end ftable
%end-doc
*/
	{"opacity", "Layer opacity",
	 HID_Integer, 0, 100, {100, 0, 0}, 0, 0},
#define HA_opacity 2

/* %start-doc options "93 SVG Options"
@ftable @code
@cindex flip
@item --flip
Flip board, look at it from the bottom side
@end ftable
%end-doc
*/
	{"flip", "Flip board",
	 HID_Boolean, 0, 0, {0, 0, 0}, 0, 0}
#define HA_flip 3
};

#define NUM_OPTIONS (sizeof(svg_attribute_list)/sizeof(svg_attribute_list[0]))

#define TRX(x)
#define TRY(y) \
do { \
	if (flip) \
		y = PCB->MaxHeight - y; \
} while(0)


REGISTER_ATTRIBUTES(svg_attribute_list, svg_cookie)

static pcb_hid_attr_val_t svg_values[NUM_OPTIONS];

static pcb_hid_attribute_t *svg_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".svg";

	if (PCB)
		derive_default_filename(PCB->Filename, &svg_attribute_list[HA_svgfile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return svg_attribute_list;
}

void svg_hid_export_to_file(FILE * the_file, pcb_hid_attr_val_t * options)
{
	static int saved_layer_stack[MAX_LAYER];
	pcb_box_t region;

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
			photo_mode = 1;
			conf_force_set_bool(conf_core.editor.show_mask, 1);
		}
		else
			photo_mode = 0;

		if (options[HA_flip].int_value) {
			flip = 1;
			conf_force_set_bool(conf_core.editor.show_solder_side, 1);
		}
		else
			flip = 0;
	}

	if (photo_mode) {
		pcb_fprintf(f, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			0, 0, PCB->MaxWidth, PCB->MaxHeight, board_color);
	}

	opacity = options[HA_opacity].int_value;

	gds_init(&sbright);
	gds_init(&sdark);
	gds_init(&snormal);
	hid_expose_callback(&svg_hid, &region, 0);

	conf_update(NULL); /* restore forced sets */
}

static void group_close()
{
	if (group_open == 1) {
		if (gds_len(&sdark) > 0) {
			fprintf(f, "<!--dark-->\n");
			fprintf(f, "%s", sdark.array);
			gds_truncate(&sdark, 0);
		}

		if (gds_len(&sbright) > 0) {
			fprintf(f, "<!--bright-->\n");
			fprintf(f, "%s", sbright.array);
			gds_truncate(&sbright, 0);
		}

		if (gds_len(&snormal) > 0) {
			fprintf(f, "<!--normal-->\n");
			fprintf(f, "%s", snormal.array);
			gds_truncate(&snormal, 0);
		}

	}
	fprintf(f, "</g>\n");
}

static void svg_do_export(pcb_hid_attr_val_t * options)
{
	const char *filename;
	int save_ons[MAX_LAYER + 2];
	int i;
	pcb_coord_t w, h, x1, y1, x2, y2;

	if (!options) {
		svg_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			svg_values[i] = svg_attribute_list[i].default_val;
		options = svg_values;
	}

	filename = options[HA_svgfile].str_value;
	if (!filename)
		filename = "pcb.svg";

	f = fopen(filename, "wb");
	if (!f) {
		perror(filename);
		return;
	}

	fprintf(f, "<?xml version=\"1.0\"?>\n");
	w = PCB->MaxWidth;
	h = PCB->MaxHeight;
	while((w < PCB_MM_TO_COORD(1024)) && (h < PCB_MM_TO_COORD(1024))) {
		w *= 2;
		h *= 2;
	}

	x1 = PCB_MM_TO_COORD(2);
	y1 = PCB_MM_TO_COORD(2);
	x2 = PCB->MaxWidth;
	y2 = PCB->MaxHeight;
	x2 += PCB_MM_TO_COORD(5);
	y2 += PCB_MM_TO_COORD(5);
	pcb_fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.0\" width=\"%mm\" height=\"%mm\" viewBox=\"-%mm -%mm %mm %mm\">\n", w, h, x1, y1, x2, y2);

	hid_save_and_show_layer_ons(save_ons);

	svg_hid_export_to_file(f, options);

	hid_restore_layer_ons(save_ons);

	while(group_open) {
		group_close();
		group_open--;
	}

	fprintf(f, "</svg>\n");
	fclose(f);
	f = NULL;
}

static void svg_parse_arguments(int *argc, char ***argv)
{
	hid_register_attributes(svg_attribute_list, sizeof(svg_attribute_list) / sizeof(svg_attribute_list[0]), svg_cookie, 0);
	hid_parse_command_line(argc, argv);
}

static int svg_set_layer(const char *name, int group, int empty)
{
	int opa, our_mask, our_silk;
	unsigned int our_side;

	if (flip) {
		our_mask = SL(MASK, BOTTOM);
		our_silk = SL(SILK, BOTTOM);
		our_side = PCB_LYT_BOTTOM;
	}
	else {
		our_mask = SL(MASK, TOP);
		our_silk = SL(SILK, TOP);
		our_side = PCB_LYT_TOP;
	}

	/* don't draw the mask if we are not in the photo mode */
	if (!photo_mode && ((group == SL(MASK, TOP)) || (group == SL(MASK, BOTTOM))))
		return 0;

	if ((group < 0) && (group != our_silk) && (group != our_mask) && (group != SL(UDRILL, 0)) && (group != SL(PDRILL, 0)))
		return 0;
	while(group_open) {
		group_close();
		group_open--;
	}
	if (name == NULL)
		name = "copper";
	fprintf(f, "<g id=\"layer_%d_%s\"", group, name);
	drawing_mask = (group == our_mask);
	opa = opacity;
	if (drawing_mask)
		opa *= mask_opacity_factor;
	if (opa != 100)
		fprintf(f, " opacity=\"%.2f\"", ((float)opa) / 100.0);
	fprintf(f, ">\n");
	group_open = 1;

	if (photo_mode) {
		if (group == our_silk)
			photo_color = PHOTO_SILK;
		if (group == our_mask)
			photo_color = PHOTO_MASK;
		else if (group >= 0) {
			int ly = PCB->LayerGroups.Entries[group][0];
			unsigned int fl;
			fl = pcb_layer_flags(ly) & PCB_LYT_ANYWHERE;
			if (fl == our_side)
				photo_color = PHOTO_COPPER;
			else
				photo_color = PHOTO_INNER;
		}
	}

	drawing_hole = (group == SL(UDRILL, 0)) || (group == SL(PDRILL, 0));

	return 1;
}


static pcb_hid_gc_t svg_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) malloc(sizeof(hid_gc_s));
	rv->me_pointer = &svg_hid;
	rv->cap = Trace_Cap;
	rv->width = 1;
	rv->color = NULL;
	return rv;
}

static void svg_destroy_gc(pcb_hid_gc_t gc)
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

static void svg_set_color(pcb_hid_gc_t gc, const char *name)
{
	gc->drill = gc->erase = 0;
	if (name == NULL)
		name = "#ff0000";
	if (strcmp(name, "drill") == 0) {
		name = "#ffffff";
		gc->drill = 1;
	}
	else if (strcmp(name, "erase") == 0) {
		name = "#ffffff";
		gc->erase = 1;
	}
	else if (drawing_mask)
		name = mask_color;
	if ((gc->color != NULL) && (strcmp(gc->color, name) == 0))
		return;
	free(gc->color);
	gc->color = pcb_strdup(name);
}

static void svg_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void svg_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}

static void indent(gds_t *s)
{
	static char ind[] = "                                                                              ";
	if (group_open < sizeof(ind)-1) {
		ind[group_open] = '\0';
		if (s == NULL)
			pcb_fprintf(f, ind);
		else
			pcb_append_printf(s, ind);
		ind[group_open] = ' ';
		return;
	}

	if (s == NULL)
		pcb_fprintf(f, ind);
	else
		pcb_append_printf(s, ind);
}

static void svg_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

#define fix_rect_coords() \
	if (x1 > x2) {\
		pcb_coord_t t = x1; \
		x1 = x2; \
		x2 = t; \
	} \
	if (y1 > y2) { \
		pcb_coord_t t = y1; \
		y1 = y2; \
		y2 = t; \
	}

static void draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t w, pcb_coord_t h, pcb_coord_t stroke)
{
	indent(&snormal);
	pcb_append_printf(&snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
		x1, y1, w, h, stroke, gc->color, CAPS(gc->cap));
}

static void svg_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	fix_rect_coords();
	draw_rect(gc, x1, y1, x2-x1, y2-y1, gc->width);
}

static void draw_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t w, pcb_coord_t h)
{
	if ((photo_mode) && (!gc->erase)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(&sdark);
			pcb_append_printf(&sdark, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, w, h, photo_palette[photo_color].dark);
			indent(&sbright);
			pcb_append_printf(&sbright, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, w, h, photo_palette[photo_color].bright);
		}
		indent(&snormal);
		pcb_append_printf(&snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, photo_palette[photo_color].normal);
	}
	else {
		indent(&snormal);
		pcb_append_printf(&snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, gc->color);
	}
}

static void svg_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	fix_rect_coords();
	draw_fill_rect(gc, x1, y1, x2-x1, y2-y1);
}

static void draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	if ((photo_mode) && (!gc->erase)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(&sbright);
			pcb_append_printf(&sbright, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1-photo_offs, y1-photo_offs, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[photo_color].bright, CAPS(gc->cap));
			indent(&sdark);
			pcb_append_printf(&sdark, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1+photo_offs, y1+photo_offs, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[photo_color].dark, CAPS(gc->cap));
		}
		indent(&snormal);
		pcb_append_printf(&snormal, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, photo_palette[photo_color].normal, CAPS(gc->cap));
	}
	else {
		indent(&snormal);
		pcb_append_printf(&snormal, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, gc->color, CAPS(gc->cap));
	}
}

static void svg_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	draw_line(gc, x1, y1, x2, y2);
}

static void draw_arc(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t r, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t stroke)
{
	if ((photo_mode) && (!gc->erase)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(&sbright);
			pcb_append_printf(&sbright, "<path d=\"M %mm %mm A %mm %mm 0 0 0 %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, r, r, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[photo_color].bright, CAPS(gc->cap));
			indent(&sdark);
			pcb_append_printf(&sdark, "<path d=\"M %mm %mm A %mm %mm 0 0 0 %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, r, r, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[photo_color].dark, CAPS(gc->cap));
		}
		indent(&snormal);
		pcb_append_printf(&snormal, "<path d=\"M %mm %mm A %mm %mm 0 0 0 %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, x2, y2, gc->width, photo_palette[photo_color].normal, CAPS(gc->cap));
	}
	else {
		indent(&snormal);
		pcb_append_printf(&snormal, "<path d=\"M %mm %mm A %mm %mm 0 0 0 %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, x2, y2, gc->width, gc->color, CAPS(gc->cap));
	}
}

static void svg_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	pcb_coord_t x1, y1, x2, y2;
	pcb_angle_t sa, ea;

	TRX(cx); TRY(cy);

	/* calculate start and end angles considering flip */
	start_angle = 180 - start_angle;
	delta_angle = -delta_angle;
	if (flip) {
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

	/* calculate the endpoints */
	x2 = cx + (width * cos(sa * M_PI / 180));
	y2 = cy + (width * sin(sa * M_PI / 180));
	x1 = cx + (width * cos(ea * M_PI / 180));
	y1 = cy + (width * sin(ea * M_PI / 180));

	draw_arc(gc, x1, y1, width, x2, y2, gc->width);
}

static void draw_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r, pcb_coord_t stroke)
{
	if ((photo_mode) && (!gc->erase)) {
		if (!drawing_hole) {
			pcb_coord_t photo_offs = photo_palette[photo_color].offs;
			if ((!gc->drill) && (photo_offs != 0)) {
				indent(&sbright);
				pcb_append_printf(&sbright, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx-photo_offs, cy-photo_offs, r, stroke, photo_palette[photo_color].bright);

				indent(&sdark);
				pcb_append_printf(&sdark, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx+photo_offs, cy+photo_offs, r, stroke, photo_palette[photo_color].dark);
			}
			indent(&snormal);
			pcb_append_printf(&snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, photo_palette[photo_color].normal);
		}
		else {
			indent(&snormal);
			pcb_append_printf(&snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, "#000000");
		}
	}
	else{
		indent(&snormal);
		pcb_append_printf(&snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			cx, cy, r, stroke, gc->color);
	}
}

static void svg_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	TRX(cx); TRY(cy);
	draw_fill_circle(gc, cx, cy, radius, gc->width);
}

static void draw_poly(gds_t *s, pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y, pcb_coord_t offs, const char *clr)
{
	int i;
	float poly_bloat = 0.075;

	indent(s);
	gds_append_str(s, "<polygon points=\"");
	for (i = 0; i < n_coords; i++) {
		pcb_coord_t px = x[i], py = y[i];
		TRX(px); TRY(py);
		pcb_append_printf(s, "%mm,%mm ", px+offs, py+offs);
	}
	pcb_append_printf(s, "\" stroke-width=\"%.3f\" stroke=\"%s\" fill=\"%s\"/>\n", poly_bloat, clr, clr);
}

static void svg_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	if ((photo_mode) && (!gc->erase)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			draw_poly(&sbright, gc, n_coords, x, y, -photo_offs, photo_palette[photo_color].bright);
			draw_poly(&sdark, gc, n_coords, x, y, +photo_offs, photo_palette[photo_color].dark);
		}
		draw_poly(&snormal, gc, n_coords, x, y, 0, photo_palette[photo_color].normal);
	}
	else
		draw_poly(&snormal, gc, n_coords, x, y, 0, gc->color);
}

static void svg_calibrate(double xval, double yval)
{
	pcb_message(PCB_MSG_ERROR, "svg_calibrate() not implemented");
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
	memset(&svg_hid, 0, sizeof(pcb_hid_t));

	common_nogui_init(&svg_hid);
	common_draw_helpers_init(&svg_hid);

	svg_hid.struct_size = sizeof(pcb_hid_t);
	svg_hid.name = "svg";
	svg_hid.description = "Scalable Vector Graphics export";
	svg_hid.exporter = 1;
	svg_hid.poly_before = 1;
	svg_hid.holes_after = 1;

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

	hid_register_hid(&svg_hid);

	return NULL;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *
 *  Breadboard export HID
 *  This code is based on the GERBER and OpenSCAD export HID
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math_helper.h"
#include "board.h"
#include "config.h"
#include "data.h"
#include "error.h"
#include "buffer.h"
#include "layer.h"
#include "layer_grp.h"
#include "layer_vis.h"
#include "plugins.h"
#include "compat_misc.h"
#include "compat_fs.h"
#include "misc_util.h"
#include "funchash_core.h"
#include "rtree.h"

#include "hid.h"
#include "hid_attrib.h"
#include "hid_nogui.h"
#include "hid_init.h"
#include "hid_cam.h"

#include <cairo.h>

static const char *bboard_cookie = "bboard exporter";

static cairo_surface_t *bboard_cairo_sfc;
static cairo_t *bboard_cairo_ctx;
#define ASSERT_CAIRO  if ((bboard_cairo_ctx == NULL) || (bboard_cairo_sfc == NULL)) \
                         return;

#define DPI_SCALE	150

#define BBEXT		".png"
#define MODELBASE	"models"
#define BBOARDBASE	"bboard"

/****************************************************************************************************
* Breadboard export filter parameters and options
****************************************************************************************************/

static pcb_hid_t bboard_hid;

static struct {
	int draw;
	int exp;
	float z_offset;
	int solder;
	int component;
} group_data[PCB_MAX_LAYERGRP];


#define HA_bboardfile  0
#define HA_bgcolor     1
#define HA_skipsolder  2
#define HA_antialias   3

static pcb_export_opt_t bboard_options[] = {
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --bbfile <string>
Name of breadboard image output file.
@end ftable
%end-doc
*/
	{
	 "bbfile", "Braedboard file name", PCB_HATT_STRING, 0, 0,
	 {
		0, 0, 0}, 0, 0},
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --bgcolor <string>
Background color. It can be HTML color code (RRGGBB); if left empty or set to 'transparent', transparent background is used.
@end ftable
%end-doc
*/
	{
	 "bgcolor", "Background color (rrggbb)", PCB_HATT_STRING, 0, 0,
	 {
		0, 0, 0}, 0, 0},
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --skipsolder <bool>
The solder layer will be ommited (if set to true) from output.
@end ftable
%end-doc
*/
	{
	 "skipsolder", "Ignore solder layer", PCB_HATT_BOOL, 0, 0,
	 {
		1, 0, 0}, 0, 0},
/*
%start-doc options "Breadboard Export"
@ftable @code
@item --antialias <bool>
Connections are antialiased. Antialiasing applies only to wires, models are not antialiased..
@end ftable
%end-doc
*/
	{
	 "antialias", "Antialiasing", PCB_HATT_BOOL, 0, 0,
	 {
		1, 0, 0}, 0, 0},
};

#define NUM_OPTIONS (sizeof(bboard_options)/sizeof(bboard_options[0]))

static pcb_hid_attr_val_t bboard_values[NUM_OPTIONS];

/****************************************************************************************************/
static const char *bboard_filename = 0;
static const char *bboard_bgcolor = 0;

pcb_coord_t bboard_scale_coord(pcb_coord_t x)
{
	return ((x * DPI_SCALE / 254 / 10000) + 0) / 10;
}


/*******************************************
* Export filter implementation starts here
********************************************/

static pcb_export_opt_t *bboard_get_export_options(pcb_hid_t *hid, int *n)
{
	if ((PCB != NULL)  && (bboard_options[HA_bboardfile].default_val.str == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &bboard_options[HA_bboardfile], ".png");

	bboard_options[HA_bgcolor].default_val.str = pcb_strdup("#FFFFFF");

	if (n)
		*n = NUM_OPTIONS;
	return bboard_options;
}

static int bboard_validate_layer(unsigned long flags, int purpi, int group, int skipsolder)
{
	if ((flags & PCB_LYT_INVIS) || PCB_LAYER_IS_ASSY(flags, purpi) || PCB_LAYER_IS_ROUTE(flags, purpi))
		return 0;

	if (group_data[group].solder && skipsolder)
		return 0;

	if (group >= 0 && group < pcb_max_group(PCB)) {
		if (!group_data[group].draw)
			return 0;
		group_data[group].exp = 1;
		return 1;
	}
	else
		return 0;
}

static void bboard_get_layer_color(pcb_layer_t * layer, int *clr_r, int *clr_g, int *clr_b)
{
	char *clr;
	unsigned int r, g, b;

	if ((clr = pcb_attribute_get(&(layer->Attributes), "BBoard::LayerColor")) != NULL) {
		if (clr[0] == '#') {
			if (sscanf(&(clr[1]), "%02x%02x%02x", &r, &g, &b) == 3)
				goto ok;
		}
	}

	r = layer->meta.real.color.r;
	g = layer->meta.real.color.g;
	b = layer->meta.real.color.b;

	/* default color */
	*clr_r = 0xff;
	*clr_g = 0xff;
	*clr_b = 0xff;
	return;

ok:
	*clr_r = r;
	*clr_g = g;
	*clr_b = b;
	return;
}


static void bboard_set_color_cairo(int r, int g, int b)
{
	ASSERT_CAIRO;

	cairo_set_source_rgb(bboard_cairo_ctx, ((float) r) / 255., ((float) g) / 255., ((float) b) / 255.);
}


static void bboard_draw_line_cairo(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thickness)
{
	ASSERT_CAIRO;

	cairo_set_line_cap(bboard_cairo_ctx, CAIRO_LINE_CAP_ROUND);

	cairo_move_to(bboard_cairo_ctx, bboard_scale_coord(x1), bboard_scale_coord(y1));
	cairo_line_to(bboard_cairo_ctx, bboard_scale_coord(x2), bboard_scale_coord(y2));

	cairo_set_line_width(bboard_cairo_ctx, bboard_scale_coord(thickness));
	cairo_stroke(bboard_cairo_ctx);
}

static void bboard_draw_arc_cairo(pcb_coord_t x, pcb_coord_t y, pcb_coord_t w, pcb_coord_t h, pcb_angle_t sa, pcb_angle_t a, pcb_coord_t thickness)
{
	ASSERT_CAIRO;

	cairo_set_line_cap(bboard_cairo_ctx, CAIRO_LINE_CAP_ROUND);

	cairo_save(bboard_cairo_ctx);
	cairo_translate(bboard_cairo_ctx, bboard_scale_coord(x), bboard_scale_coord(y));
	cairo_scale(bboard_cairo_ctx, bboard_scale_coord(w), -bboard_scale_coord(h));
	if (a < 0) {
		cairo_arc_negative(bboard_cairo_ctx, 0., 0., 1., (sa + 180.) * M_PI / 180., (a + sa + 180.) * M_PI / 180.);
	}
	else {
		cairo_arc(bboard_cairo_ctx, 0., 0., 1., (sa + 180.) * M_PI / 180., (a + sa + 180.) * M_PI / 180.);
	}
	cairo_restore(bboard_cairo_ctx);

	cairo_set_line_width(bboard_cairo_ctx, bboard_scale_coord(thickness));
	cairo_stroke(bboard_cairo_ctx);
}

static pcb_bool bboard_init_board_cairo(pcb_coord_t x1, pcb_coord_t y1, const char *color, int antialias)
{
	unsigned int r, g, b;
	float tr = 1.;								/* background transparency */

	if (color) {
		if (strlen(color) == 0 || !strcmp(color, "transparent")) {
			tr = 0.;
			r = g = b = 0xff;
		}
		else {
			if ((color[0] != '#')
					|| sscanf(&(color[1]), "%02x%02x%02x", &r, &g, &b) != 3) {
				pcb_message(PCB_MSG_ERROR, "BBExport: Invalid background color \"%s\"", color);
				r = g = b = 0xff;
			}

		}
	}

	bboard_cairo_sfc = NULL;
	bboard_cairo_ctx = NULL;
	bboard_cairo_sfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bboard_scale_coord(x1), bboard_scale_coord(y1));
	if (bboard_cairo_sfc) {
		bboard_cairo_ctx = cairo_create(bboard_cairo_sfc);
		if (!bboard_cairo_ctx) {
			cairo_surface_destroy(bboard_cairo_sfc);
			bboard_cairo_sfc = NULL;
		}

	}

	if ((bboard_cairo_ctx != NULL) && (bboard_cairo_sfc != NULL)) {
		cairo_set_antialias(bboard_cairo_ctx, (antialias) ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
		cairo_rectangle(bboard_cairo_ctx, 0, 0, bboard_scale_coord(x1), bboard_scale_coord(y1));
		cairo_set_source_rgba(bboard_cairo_ctx, ((float) r) / 255., ((float) g) / 255., ((float) b) / 255., tr);
		cairo_fill(bboard_cairo_ctx);
		return 1;
	}

	return 0;
}

static void bboard_finish_board_cairo(void)
{
	ASSERT_CAIRO;

	cairo_surface_write_to_png(bboard_cairo_sfc, bboard_filename);

	cairo_destroy(bboard_cairo_ctx);
	cairo_surface_destroy(bboard_cairo_sfc);
}

static char *bboard_get_model_filename(char *basename, char *value, pcb_bool nested)
{
	char *s;

/*
	s = pcb_concat(pcblibdir, PCB_DIR_SEPARATOR_S, MODELBASE, PCB_DIR_SEPARATOR_S,
					 BBOARDBASE, PCB_DIR_SEPARATOR_S, basename, (value
																											 && nested) ?
					 PCB_DIR_SEPARATOR_S : "", (value && nested) ? basename : "", (value) ? "-" : "", (value) ? value : "", BBEXT, NULL);
*/
	s = pcb_strdup("TODO_fn1");
	if (s != NULL) {
		if (!pcb_file_readable(s)) {
			free(s);
			s = NULL;
		}
	}
	return s;
}


static int bboard_parse_offset(char *s, pcb_coord_t * ox, pcb_coord_t * oy)
{
	pcb_coord_t xx = 0, yy = 0;
	int n = 0, ln = 0;
	char val[32];

	while (sscanf(s, "%30s%n", val, &ln) >= 1) {
		switch (n) {
		case 0:
			xx = pcb_get_value_ex(val, NULL, NULL, NULL, "mm", NULL);
			break;
		case 1:
			yy = pcb_get_value_ex(val, NULL, NULL, NULL, "mm", NULL);
			break;
		}
		s = s + ln;
		n++;
	}
	if (n == 2) {
		*ox = xx;
		*oy = yy;
		return pcb_true;
	}
	else {
		return pcb_false;
	}

}


static void bboard_export_element_cairo(pcb_subc_t *subc, pcb_bool onsolder)
{
	cairo_surface_t *sfc;
	pcb_coord_t ex, ey;
	pcb_coord_t ox = 0, oy = 0;
	int w, h;
	pcb_angle_t tmp_angle = 0.0;
	char *model_angle, *s = 0, *s1, *s2, *fname = NULL;
	pcb_bool offset_in_model = pcb_false;

	ASSERT_CAIRO;

	s1 = pcb_attribute_get(&(subc->Attributes), "BBoard::Model");
	if (s1) {
		s = pcb_strdup(s1);
		if (!s)
			return;

		if ((s2 = strchr(s, ' ')) != NULL) {
			*s2 = 0;
			offset_in_model = bboard_parse_offset(s2 + 1, &ox, &oy);
		}
TODO("subc: rewrite")
#if 0
		if (!PCB_EMPTY_STRING_P(PCB_ELEM_NAME_VALUE(element))) {
			fname = bboard_get_model_filename(s, PCB_ELEM_NAME_VALUE(element), pcb_true);
			if (!fname)
				fname = bboard_get_model_filename(s, PCB_ELEM_NAME_VALUE(element), pcb_false);
		}
#endif
		if (!fname)
			fname = bboard_get_model_filename(s, NULL, pcb_false);

		if (s)
			free(s);
	}
	if (!fname) {
		/* invalidate offset from BBoard::Model, if such model does not exist */
		offset_in_model = pcb_false;

		s = pcb_attribute_get(&(subc->Attributes), "Footprint::File");
		if (s) {
TODO("subc: rewrite")
#if 0
			if (!PCB_EMPTY_STRING_P(PCB_ELEM_NAME_VALUE(element))) {
				fname = bboard_get_model_filename(s, PCB_ELEM_NAME_VALUE(element), pcb_true);
				if (!fname)
					fname = bboard_get_model_filename(s, PCB_ELEM_NAME_VALUE(element), pcb_false);
			}
#endif
			if (!fname)
				fname = bboard_get_model_filename(s, NULL, pcb_false);
		}
	}
	if (!fname) {
TODO("subc: rewrite")
#if 0
		s = PCB_ELEM_NAME_DESCRIPTION(element);
		if (!PCB_EMPTY_STRING_P(PCB_ELEM_NAME_DESCRIPTION(element))) {
			if (!PCB_EMPTY_STRING_P(PCB_ELEM_NAME_VALUE(element))) {
				fname = bboard_get_model_filename(PCB_ELEM_NAME_DESCRIPTION(element), PCB_ELEM_NAME_VALUE(element), pcb_true);
				if (!fname)
					fname = bboard_get_model_filename(PCB_ELEM_NAME_DESCRIPTION(element), PCB_ELEM_NAME_VALUE(element), pcb_false);

			}
			if (!fname)
				fname = bboard_get_model_filename(PCB_ELEM_NAME_DESCRIPTION(element), NULL, pcb_false);
		}
#endif
	}

	if (!fname)
		return;

	sfc = cairo_image_surface_create_from_png(fname);

	free(fname);

	if (sfc) {
		w = cairo_image_surface_get_width(sfc);
		h = cairo_image_surface_get_height(sfc);

		/* read offest from attribute */
		if (!offset_in_model) {
			s = pcb_attribute_get(&(subc->Attributes), "BBoard::Offset");

			/* Parse values with units... */
			if (s) {
				bboard_parse_offset(s, &ox, &oy);
			}
		}

		{
			pcb_coord_t ox = 0, oy = 0;
			pcb_subc_get_origin(subc, &ox, &oy);
			ex = bboard_scale_coord(ox);
			ey = bboard_scale_coord(oy);
		}

		cairo_save(bboard_cairo_ctx);

		if ((model_angle = pcb_attribute_get(&(subc->Attributes), "Footprint::RotationTracking")) != NULL) {
			sscanf(model_angle, "%lf", &tmp_angle);
		}


		cairo_translate(bboard_cairo_ctx, ex, ey);
		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (subc))) {
			cairo_scale(bboard_cairo_ctx, 1, -1);
		}
		cairo_rotate(bboard_cairo_ctx, -tmp_angle * M_PI / 180.);
		cairo_set_source_surface(bboard_cairo_ctx, sfc, bboard_scale_coord(ox) - w / 2, bboard_scale_coord(oy) - h / 2);
		cairo_paint(bboard_cairo_ctx);

		cairo_restore(bboard_cairo_ctx);

		cairo_surface_destroy(sfc);
	}
}


static void bboard_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	int i;
	int clr_r, clr_g, clr_b;
	pcb_layer_t *layer;
	pcb_layergrp_id_t gtop, gbottom;

	if (!options) {
		bboard_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			bboard_values[i] = bboard_options[i].default_val;
		options = bboard_values;
	}
	bboard_filename = options[HA_bboardfile].str;
	if (!bboard_filename)
		bboard_filename = "unknown.png";

	bboard_bgcolor = options[HA_bgcolor].str;
	if (!bboard_bgcolor)
		bboard_bgcolor = "FFFFFF";

	memset(group_data, 0, sizeof(group_data));

	if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &gbottom, 1) > 0)
		group_data[gbottom].solder = 1;

	if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_TOP, &gtop, 1) > 0)
		group_data[gtop].component = 1;

	for (i = 0; i < pcb_max_layer; i++) {
		layer = PCB->Data->Layer + i;
		if (!PCB_RTREE_EMPTY(layer->line_tree))
			group_data[pcb_layer_get_group(PCB, i)].draw = 1;
	}

	bboard_init_board_cairo(PCB->hidlib.size_x, PCB->hidlib.size_y, bboard_bgcolor, options[HA_antialias].lng);

TODO("subc: rewrite")
#if 0
	/* write out components on solder side */
	PCB_ELEMENT_LOOP(PCB->Data);
	if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (element))) {
		bboard_export_element_cairo(element, 1);
	}
	PCB_END_LOOP;

	/* write out components on component side */
	PCB_ELEMENT_LOOP(PCB->Data);
	if (!PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (element))) {
		bboard_export_element_cairo(element, 0);
	}
	PCB_END_LOOP;
#endif

	/* draw all wires from all valid layers */
	for (i = pcb_max_layer; i >= 0; i--) {
		unsigned int flg = pcb_layer_flags(PCB, i);
		pcb_layergrp_id_t gid;
		pcb_layergrp_t *grp;
		int purpi;

		if (flg & PCB_LYT_SILK)
			continue;

		gid = pcb_layer_get_group(PCB, i);
		grp = pcb_get_layergrp(PCB, gid);
		if (grp == NULL)
			purpi = -1;
		else
			purpi = grp->purpi;

		if (bboard_validate_layer(flg, purpi, gid, options[HA_skipsolder].lng)) {
			bboard_get_layer_color(&(PCB->Data->Layer[i]), &clr_r, &clr_g, &clr_b);
			bboard_set_color_cairo(clr_r, clr_g, clr_b);
			PCB_LINE_LOOP(&(PCB->Data->Layer[i]));
			{
				bboard_draw_line_cairo(line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness);
			}
			PCB_END_LOOP;
			PCB_ARC_LOOP(&(PCB->Data->Layer[i]));
			{
TODO(": remove x1;y1;x2;y2")
				bboard_draw_arc_cairo(/*arc->Point1.X, arc->Point1.Y,
															arc->Point2.X, arc->Point2.Y,*/
															arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
			}
			PCB_END_LOOP;
		}
	}

	bboard_finish_board_cairo();

}

static int bboard_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	return pcb_hid_parse_command_line(argc, argv);
}


static void bboard_calibrate(pcb_hid_t *hid, double xval, double yval)
{
	fprintf(stderr, "HID error: pcb called unimplemented breaboard function bboard_calibrate.\n");
	abort();
}

static void bboard_set_crosshair(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, int action)
{
}

static pcb_hid_t bboard_hid;

int pplg_check_ver_export_bboard(int ver_needed) { return 0; }

void pplg_uninit_export_bboard(void)
{
	pcb_hid_remove_attributes_by_cookie(bboard_cookie);
}


int pplg_init_export_bboard(void)
{
	PCB_API_CHK_VER;
	memset(&bboard_hid, 0, sizeof(bboard_hid));

	pcb_hid_nogui_init(&bboard_hid);

	bboard_hid.struct_size = sizeof(bboard_hid);
	bboard_hid.name = "bboard";
	bboard_hid.description = "Breadboard export";
	bboard_hid.exporter = 1;

	bboard_hid.get_export_options = bboard_get_export_options;
	bboard_hid.do_export = bboard_do_export;
	bboard_hid.parse_arguments = bboard_parse_arguments;
	bboard_hid.calibrate = bboard_calibrate;
	bboard_hid.set_crosshair = bboard_set_crosshair;
	pcb_hid_register_hid(&bboard_hid);

	pcb_hid_register_attributes(bboard_options, sizeof(bboard_options) / sizeof(bboard_options[0]), bboard_cookie, 0);
	return 0;
}


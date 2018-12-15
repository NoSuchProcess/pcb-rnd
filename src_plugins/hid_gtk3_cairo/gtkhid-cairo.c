/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017,2018 Alain Vigne
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>

#include "crosshair.h"
#include "data.h"
#include "layer.h"
#include "grid.h"
#include "hid_draw_helpers.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "funchash_core.h"

#include "../src_plugins/lib_gtk_hid/gui.h"
#include "../src_plugins/lib_gtk_hid/coord_conv.h"
#include "../src_plugins/lib_gtk_hid/preview_helper.h"

#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"

#include "../src_plugins/lib_hid_common/clip.h"

extern pcb_hid_t gtk3_cairo_hid;
static void ghid_cairo_screen_update(void);

/* Sets priv->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

//static int cur_mask = -1;
//static int mask_seq = 0;
static pcb_composite_op_t curr_drawing_mode;

typedef struct render_priv_s {
	GdkRGBA bg_color;											/* cached back-ground color           */
	GdkRGBA offlimits_color;							/* cached external board color        */
	GdkRGBA grid_color;										/* cached grid color                  */
	GdkRGBA crosshair_color;							/* cached crosshair color             */

	cairo_t *cr;													/* pointer to current drawing context             */
	cairo_t *cr_target;										/* pointer to destination widget drawing context  */

	cairo_surface_t *surf_da;							/* cairo surface connected to gport->drawing_area */
	cairo_t *cr_drawing_area;							/* cairo context created from surf_da             */

	cairo_surface_t *surf_layer;					/* cairo surface for temporary layer composition  */
	cairo_t *cr_layer;										/* cairo context created from surf_layer          */

	//GdkPixmap *pixmap, *mask;

	//GdkGC *bg_gc;
	//GdkGC *offlimits_gc;
	//GdkGC *mask_gc;
	//GdkGC *u_gc;
	//GdkGC *grid_gc;
	pcb_bool clip;
	GdkRectangle clip_rect;
	int xor_mode;													/* 1 if drawn in XOR mode, 0 otherwise*/
	int attached_invalidate_depth;
	int mark_invalidate_depth;
} render_priv_t;

typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	pcb_hid_t *me_pointer;

	const char *colorname;								/* current color name for this GC.    */
	pcb_color_t pcolor;
	GdkRGBA color;												/* current color      for this GC.    */

	pcb_coord_t width;
	cairo_line_cap_t cap;
	cairo_line_join_t join;
	gchar xor_mask;
	//gint mask_seq;
} hid_gc_s;

static void draw_lead_user(render_priv_t * priv_);

static void copy_color(GdkRGBA * dest, GdkRGBA * source)
{
	dest->red = source->red;
	dest->green = source->green;
	dest->blue = source->blue;
	dest->alpha = source->alpha;
}

static const gchar *get_color_name(pcb_gtk_color_t * color)
{
	static char tmp[16];

	if (!color)
		return "#000000";

	sprintf(tmp, "#%2.2x%2.2x%2.2x",
					(int) (color->red * 255) & 0xff, (int) (color->green * 255) & 0xff, (int) (color->blue * 255) & 0xff);
	return tmp;
}

/* Returns TRUE if color_string has been successfully parsed to color. */
static pcb_bool map_color_string(const char *color_string, pcb_gtk_color_t * color)
{
	pcb_bool parsed;

	if (!color)
		return FALSE;

	parsed = gdk_rgba_parse(color, color_string);

	return parsed;
}

static void cr_draw_line(cairo_t * cr, int fill, double x1, double y1, double x2, double y2)
{
	if (cr == NULL)
		return;

	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y2);
TODO("gtk3: What means 'fill a line'? cairo_fill is not appropriate if path is only a line.")
	if (fill)
		cairo_fill(cr);
	else
		cairo_stroke(cr);
}

static void cr_destroy_surf_and_context(cairo_surface_t ** psurf, cairo_t ** pcr)
{
	if (*psurf)
		cairo_surface_destroy(*psurf);
	if (*pcr)
		cairo_destroy(*pcr);

	*psurf = NULL;
	*pcr = NULL;
}

/* First, frees previous surface and context, then creates new ones, similar to drawing_area. */
static void cr_create_similar_surface_and_context(cairo_surface_t ** psurf, cairo_t ** pcr, void *vport)
{
	GHidPort *port = vport;
	cairo_surface_t *surface;
	cairo_t *cr;

	cr_destroy_surf_and_context(psurf, pcr);

	surface = gdk_window_create_similar_surface(gtk_widget_get_window(port->drawing_area),
																							CAIRO_CONTENT_COLOR_ALPHA,
																							gtk_widget_get_allocated_width(port->drawing_area),
																							gtk_widget_get_allocated_height(port->drawing_area));
	cr = cairo_create(surface);
	*psurf = surface;
	*pcr = cr;
}

/* Creates or reuses a context to render a single layer group with "union" type of shape rendering. */
static void start_subcomposite(void)
{
	render_priv_t *priv = gport->render_priv;
	cairo_t *cr;

	if (priv->surf_layer == NULL)
		cr_create_similar_surface_and_context(&priv->surf_layer, &priv->cr_layer, gport);
	cr = priv->cr_layer;
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_set_operator(priv->cr, CAIRO_OPERATOR_SOURCE);
	priv->cr = priv->cr_layer;
}

/* Applies the layer composited so far, to the target, using translucency. */
static void end_subcomposite(void)
{
	render_priv_t *priv = gport->render_priv;

	cairo_set_operator(priv->cr_target, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(priv->cr_target, priv->surf_layer, 0, 0);
	cairo_paint_with_alpha(priv->cr_target, conf_core.appearance.layer_alpha);
	priv->cr = priv->cr_target;
}

static int ghid_cairo_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	int idx = group;
	if (idx >= 0 && idx < pcb_max_group(PCB)) {
		int n = PCB->LayerGroups.grp[group].len;
		for (idx = 0; idx < n - 1; idx++) {
			int ni = PCB->LayerGroups.grp[group].lid[idx];
			if (ni >= 0 && ni < pcb_max_layer && PCB->Data->Layer[ni].meta.real.vis)
				break;
		}
		idx = PCB->LayerGroups.grp[group].lid[idx];
	}

	start_subcomposite();

	/* non-virtual layers with group visibility */
	switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_MASK:
		case PCB_LYT_PASTE:
			return (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags) && PCB->LayerGroups.grp[group].vis);
	}

	/* normal layers */
	if (idx >= 0 && idx < pcb_max_layer && ((flags & PCB_LYT_ANYTHING) != PCB_LYT_SILK))
		return PCB->Data->Layer[idx].meta.real.vis;

	if (PCB_LAYER_IS_ASSY(flags, purpi))
		return 0;

	/* virtual layers */
	{
		if (PCB_LAYER_IS_DRILL(flags, purpi))
			return 1;

		switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_INVIS:
			return PCB->InvisibleObjectsOn;
		case PCB_LYT_SILK:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				return pcb_silk_on(PCB);
			return 0;
		case PCB_LYT_UI:
			return 1;
		case PCB_LYT_RAT:
			return PCB->RatOn;
		}
	}
	return 0;
}

static void ghid_cairo_end_layer_group(void)
{
	end_subcomposite();
}

TODO(": misleading comment or broken code")
/* Do not clean up internal structures, as they will be used probably elsewhere. */
static void ghid_cairo_destroy_gc(pcb_hid_gc_t gc)
{
	g_free(gc);
}

/* called from pcb_hid_t->make_gc() . Use only this to hold pointers, do not
   own references, avoid costly memory allocation that needs to be destroyed with
   ghid_cairo_destroy_gc(). */
static pcb_hid_gc_t ghid_cairo_make_gc(void)
{
	pcb_hid_gc_t rv;

	rv = g_new0(hid_gc_s, 1);
	rv->me_pointer = &gtk3_cairo_hid;
	rv->pcolor = conf_core.appearance.color.background;
	return rv;
}

TODO("gtk3: check if clipping is not necessary")
//static void set_clip(render_priv_t * priv, cairo_t * cr)
//{
//	if (cr == NULL)
//		return;
//
//	if (priv->clip) {
//		gdk_cairo_rectangle(cr, &priv->clip_rect);
//		cairo_clip(cr);
//	}
//	else {
//		/*FIXME: do nothing if no clipping ? */
//		//cairo_mask(cr, NULL);
//		//gdk_gc_set_clip_mask(gc, NULL);
//	}
//}

static inline void ghid_cairo_draw_grid_global(cairo_t *cr)
{
	//render_priv_t *priv = gport->render_priv;
	pcb_coord_t x, y, x1, y1, x2, y2, grd;
	int n, i;
	static GdkPoint *points = NULL;
	static int npoints = 0;

	x1 = pcb_grid_fit(MAX(0, SIDE_X(gport->view.x0)), PCB->Grid, PCB->GridOffsetX);
	y1 = pcb_grid_fit(MAX(0, SIDE_Y(gport->view.y0)), PCB->Grid, PCB->GridOffsetY);
	x2 = pcb_grid_fit(MIN(PCB->MaxWidth, SIDE_X(gport->view.x0 + gport->view.width - 1)), PCB->Grid, PCB->GridOffsetX);
	y2 = pcb_grid_fit(MIN(PCB->MaxHeight, SIDE_Y(gport->view.y0 + gport->view.height - 1)), PCB->Grid, PCB->GridOffsetY);

	grd = PCB->Grid;

	if (Vz(grd) < conf_hid_gtk.plugins.hid_gtk.global_grid.min_dist_px) {
		if (!conf_hid_gtk.plugins.hid_gtk.global_grid.sparse)
			return;
		grd *= (conf_hid_gtk.plugins.hid_gtk.global_grid.min_dist_px / Vz(grd));
	}

	if (x1 > x2) {
		pcb_coord_t tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	if (y1 > y2) {
		pcb_coord_t tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	if (Vx(x1) < 0)
		x1 += grd;
	if (Vy(y1) < 0)
		y1 += grd;
	if (Vx(x2) >= gport->view.canvas_width)
		x2 -= grd;
	if (Vy(y2) >= gport->view.canvas_height)
		y2 -= grd;


	n = (x2 - x1) / grd + 1;
	if (n > npoints) {
		npoints = n + 10;
		points = (GdkPoint *) realloc(points, npoints * sizeof(GdkPoint));
	}
	n = 0;
	for (x = x1; x <= x2; x += grd) {
		points[n].x = Vx(x);
		n++;
	}
	if (n == 0)
		return;

	/* Draw N points... is this the most efficient ? At least, it works.
	   From cairo development :
	   "    cairo_move_to (cr, x, y);
	   .    cairo_line_to (cr, x, y);
	   .    .. repeat for each point ..
	   .    
	   .    cairo_stroke (cr);
	   .    
	   .  Within the implementation (and test suite) we call these "degenerate"
	   .  paths and we explicitly support drawing round caps for such degenerate
	   .  paths. So this should work perfectly for the case of
	   .  CAIRO_LINE_CAP_ROUND and you'll get the diameter controlled by
	   .  cairo_set_line_width just like you want.
	   "
	 */
	for (y = y1; y <= y2; y += grd) {
		for (i = 0; i < n; i++) {
			points[i].y = Vy(y);
			cairo_move_to(cr, points[i].x, points[i].y);
			cairo_line_to(cr, points[i].x, points[i].y);
		}
		cairo_stroke(cr);
	}
}

static void ghid_cairo_draw_grid_local_(pcb_coord_t cx, pcb_coord_t cy, int radius)
{
	render_priv_t *priv = gport->render_priv;
	cairo_t *cr = priv->cr_target;
	static GdkPoint *points_base = NULL;
	static GdkPoint *points_abs = NULL;
	static int apoints = 0, npoints = 0, old_radius = 0;
	static pcb_coord_t last_grid = 0;
	int recalc = 0, n, r2;
	pcb_coord_t x, y;

	/* PI is approximated with 3.25 here - allows a minimal overallocation, speeds up calculations */
	r2 = radius * radius;
	n = r2 * 3 + r2 / 4 + 1;
	if (n > apoints) {
		apoints = n;
		points_base = (GdkPoint *) realloc(points_base, apoints * sizeof(GdkPoint));
		points_abs = (GdkPoint *) realloc(points_abs, apoints * sizeof(GdkPoint));
	}

	if (radius != old_radius) {
		old_radius = radius;
		recalc = 1;
	}

	if (last_grid != PCB->Grid) {
		last_grid = PCB->Grid;
		recalc = 1;
	}

	/* recaculate the 'filled circle' mask (base relative coords) if grid or radius changed */
	if (recalc) {

		npoints = 0;
		for (y = -radius; y <= radius; y++) {
			int y2 = y * y;
			for (x = -radius; x <= radius; x++) {
				if (x * x + y2 < r2) {
					points_base[npoints].x = x * PCB->Grid;
					points_base[npoints].y = y * PCB->Grid;
					npoints++;
				}
			}
		}
	}

	/* calculate absolute positions */
	for (n = 0; n < npoints; n++) {
		points_abs[n].x = Vx(points_base[n].x + cx);
		points_abs[n].y = Vy(points_base[n].y + cy);
		cairo_move_to(cr, points_abs[n].x, points_abs[n].y);
		cairo_line_to(cr, points_abs[n].x, points_abs[n].y);
	}
	cairo_stroke(cr);
	//gdk_draw_points(gport->drawable, priv->grid_gc, points_abs, npoints);
}

static int grid_local_have_old = 0, grid_local_old_r = 0;
static pcb_coord_t grid_local_old_x, grid_local_old_y;

static void ghid_cairo_draw_grid_local(pcb_coord_t cx, pcb_coord_t cy)
{
	if (grid_local_have_old) {
		ghid_cairo_draw_grid_local_(grid_local_old_x, grid_local_old_y, grid_local_old_r);
		grid_local_have_old = 0;
	}

	if (!conf_hid_gtk.plugins.hid_gtk.local_grid.enable)
		return;

	if ((Vz(PCB->Grid) < PCB_MIN_GRID_DISTANCE) || (!conf_core.editor.draw_grid))
		return;

	/* cx and cy are the actual cursor snapped to wherever - round them to the nearest real grid point */
	cx = (cx / PCB->Grid) * PCB->Grid + PCB->GridOffsetX;
	cy = (cy / PCB->Grid) * PCB->Grid + PCB->GridOffsetY;

	grid_local_have_old = 1;
	ghid_cairo_draw_grid_local_(cx, cy, conf_hid_gtk.plugins.hid_gtk.local_grid.radius);
	grid_local_old_x = cx;
	grid_local_old_y = cy;
	grid_local_old_r = conf_hid_gtk.plugins.hid_gtk.local_grid.radius;
}

static void ghid_cairo_draw_grid(void)
{
	render_priv_t *priv = gport->render_priv;
	cairo_t *cr = priv->cr_target;

	if (cr == NULL)
		return;

	grid_local_have_old = 0;

	if (!conf_core.editor.draw_grid)
		return;
	//if (!priv->grid_gc) {
	//  if (gdk_color_parse(conf_core.appearance.color.grid, &gport->grid_color)) {
	//    gport->grid_color.red ^= gport->bg_color.red;
	//    gport->grid_color.green ^= gport->bg_color.green;
	//    gport->grid_color.blue ^= gport->bg_color.blue;
	//    gdk_color_alloc(gport->colormap, &gport->grid_color);
	//  }
	//  priv->grid_gc = gdk_gc_new(gport->drawable);
	//  gdk_gc_set_function(priv->grid_gc, GDK_XOR);
	//  gdk_gc_set_foreground(priv->grid_gc, &gport->grid_color);
	//  gdk_gc_set_clip_origin(priv->grid_gc, 0, 0);
	//  set_clip(priv, priv->grid_gc);
	//}

	cairo_save(cr);
TODO("gtk3: deal with gc")
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(cr, 1.0);
	gdk_cairo_set_source_rgba(cr, &priv->grid_color);

	if (conf_hid_gtk.plugins.hid_gtk.local_grid.enable) {
		ghid_cairo_draw_grid_local(grid_local_old_x, grid_local_old_y);
		cairo_restore(cr);
		return;
	}

	ghid_cairo_draw_grid_global(cr);
	cairo_restore(cr);
}

/* ------------------------------------------------------------ */
static void ghid_cairo_draw_bg_image(void)
{
	static GdkPixbuf *pixbuf;
	GdkInterpType interp_type;
	gint src_x, src_y, dst_x, dst_y, w, h, w_src, h_src;
	static gint w_scaled, h_scaled;
	render_priv_t *priv = gport->render_priv;

	if (!ghidgui->bg_pixbuf)
		return;

	src_x = gport->view.x0;
	src_y = gport->view.y0;
	dst_x = 0;
	dst_y = 0;

	if (src_x < 0) {
		dst_x = -src_x;
		src_x = 0;
	}
	if (src_y < 0) {
		dst_y = -src_y;
		src_y = 0;
	}

	w = PCB->MaxWidth / gport->view.coord_per_px;
	h = PCB->MaxHeight / gport->view.coord_per_px;
	src_x = src_x / gport->view.coord_per_px;
	src_y = src_y / gport->view.coord_per_px;
	dst_x = dst_x / gport->view.coord_per_px;
	dst_y = dst_y / gport->view.coord_per_px;

	if (w_scaled != w || h_scaled != h) {
		if (pixbuf)
			g_object_unref(G_OBJECT(pixbuf));

		w_src = gdk_pixbuf_get_width(ghidgui->bg_pixbuf);
		h_src = gdk_pixbuf_get_height(ghidgui->bg_pixbuf);
		if (w > w_src && h > h_src)
			interp_type = GDK_INTERP_NEAREST;
		else
			interp_type = GDK_INTERP_BILINEAR;

		pixbuf = gdk_pixbuf_scale_simple(ghidgui->bg_pixbuf, w, h, interp_type);
		w_scaled = w;
		h_scaled = h;
	}

	if (pixbuf) {
		gdk_cairo_set_source_pixbuf(priv->cr, pixbuf, src_x, src_y);
		cairo_rectangle(priv->cr, dst_x, dst_y, gport->view.canvas_width, gport->view.canvas_height);
		cairo_fill(priv->cr);
		//gdk_pixbuf_render_to_drawable(pixbuf, gport->drawable, priv->bg_gc,
		//                              src_x, src_y, dst_x, dst_y, w - src_x, h - src_y, GDK_RGB_DITHER_NORMAL, 0, 0);
	}
}

static void ghid_cairo_render_burst(pcb_burst_op_t op, const pcb_box_t *screen)
{
	pcb_gui->coord_per_pix = gport->view.coord_per_px;
}

/* Drawing modes usually cycle from RESET to (POSITIVE | NEGATIVE) to FLUSH. screen is not used in this HID. */
static void ghid_cairo_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	render_priv_t *priv = gport->render_priv;

	if (!priv->cr) {
		//abort();
		return;
	}

	switch(op) {
		case PCB_HID_COMP_RESET:
			//ghid_sketch_setup(priv);
			start_subcomposite();

		/* clear the canvas */
			//priv->clip_color.pixel = 0;
			//if (priv->clear_gc == NULL)
			//	priv->clear_gc = gdk_gc_new(priv->out_clip);
			//gdk_gc_set_foreground(priv->clear_gc, &priv->clip_color);
			//set_clip(priv, priv->clear_gc);
			//gdk_draw_rectangle(priv->out_clip, priv->clear_gc, TRUE, 0, 0, gport->view.canvas_width, gport->view.canvas_height);
			break;

		case PCB_HID_COMP_POSITIVE:
		case PCB_HID_COMP_POSITIVE_XOR:
			//priv->clip_color.pixel = 1;
			cairo_set_operator(priv->cr, CAIRO_OPERATOR_SOURCE);
			break;

		case PCB_HID_COMP_NEGATIVE:
			//priv->clip_color.pixel = 0;
			cairo_set_operator(priv->cr, CAIRO_OPERATOR_CLEAR);
			break;

		case PCB_HID_COMP_FLUSH:
			if (direct)
				end_subcomposite();
			else {
				priv->cr = priv->cr_target;
				cairo_mask_surface(priv->cr, priv->surf_layer, 0, 0);
				cairo_fill(priv->cr);
			}
			//cairo_paint_with_alpha(priv->cr, conf_core.appearance.layer_alpha);
			//cairo_paint_with_alpha(priv->cr, 1.0);

			//if (priv->copy_gc == NULL)
			//	priv->copy_gc = gdk_gc_new(priv->out_pixel);
			//gdk_gc_set_clip_mask(priv->copy_gc, priv->sketch_clip);
			//gdk_gc_set_clip_origin(priv->copy_gc, 0, 0);
			//gdk_draw_drawable(priv->base_pixel, priv->copy_gc, priv->sketch_pixel, 0, 0, 0, 0, gport->view.canvas_width, gport->view.canvas_height);
      //
			//priv->out_pixel = priv->base_pixel;
			//priv->out_clip = NULL;
			break;
	}
	curr_drawing_mode = op;
}

typedef struct {
	int color_set;
	GdkRGBA color;
	int xor_set;
	GdkRGBA xor_color;
} ColorCache;


/*  Config helper functions for when the user changes color preferences.
    set_special colors used in the gtkhid.
 */
//static void set_special_grid_color(void)
//{
//  render_priv_t *priv = gport->render_priv;
//  int red, green, blue;
//
//  //if (!gport->colormap)
//  //  return;
//
//  red = priv->grid_color.red;
//  green = priv->grid_color.green;
//  blue = priv->grid_color.blue;
//  conf_setf(CFR_DESIGN, "appearance/color/grid", -1, "#%02x%02x%02x", red, green, blue);
//  map_color_string(conf_core.appearance.color.grid, &priv->grid_color);
//
//  config_color_button_update(&ghidgui->common, conf_get_field("appearance/color/grid"), -1);
//
//  //if (priv->grid_gc)
//  //  gdk_gc_set_foreground(priv->grid_gc, &gport->grid_color);
//}

static void ghid_cairo_set_special_colors(conf_native_t * cfg)
{
	render_priv_t *priv = gport->render_priv;

	if (((CFT_COLOR *) cfg->val.color == &conf_core.appearance.color.background) /*&& priv->bg_gc */ ) {
		if (map_color_string(cfg->val.color[0].str, &priv->bg_color)) {
			config_color_button_update(&ghidgui->common, conf_get_field("appearance/color/background"), -1);
			//gdk_gc_set_foreground(priv->bg_gc, &priv->bg_color);
			//set_special_grid_color();
		}
	}
	else if (((CFT_COLOR *) cfg->val.color == &conf_core.appearance.color.off_limit) /*&& priv->offlimits_gc */ ) {
		if (map_color_string(cfg->val.color[0].str, &priv->offlimits_color)) {
			config_color_button_update(&ghidgui->common, conf_get_field("appearance/color/off_limit"), -1);
			//gdk_gc_set_foreground(priv->offlimits_gc, &priv->offlimits_color);
		}
	}
	else if (((CFT_COLOR *) cfg->val.color == &conf_core.appearance.color.grid) /*&& priv->grid_gc */ ) {
		if (map_color_string(cfg->val.color[0].str, &priv->grid_color)) {
			conf_setf(CFR_DESIGN, "appearance/color/grid", -1, "%s", get_color_name(&priv->grid_color));
			//set_special_grid_color();
		}
	}
}

static void ghid_cairo_set_color(pcb_hid_gc_t gc, const pcb_color_t *color)
{
	static void *cache = 0;
	pcb_hidval_t cval;
	render_priv_t *priv = gport->render_priv;
	const char *name = color->str;

	if (color == NULL) {
		fprintf(stderr, "ghid_cairo_set_color():  name = NULL, setting to magenta\n");
		color = &pcb_color_cyan;
	}

	//if (name != gc->colorname) {
	//	if (gc->colorname != NULL)
	//		g_free(gc->colorname);
	//	gc->colorname = g_strdup(name);
	//}
	gc->colorname = name;

	//if (!gc->gc)
	//  return;
	//if (gport->colormap == 0)
	//  gport->colormap = gtk_widget_get_colormap(gport->top_window);

	if (pcb_color_is_drill(color)) {
		copy_color(&gc->color, &priv->offlimits_color);
		//gdk_cairo_set_source_rgba(cr, &priv->offlimits_color);
		//gdk_gc_set_foreground(gc->gc, &gport->offlimits_color);
	}
	else {
		ColorCache *cc;
		if (pcb_hid_cache_color(0, name, &cval, &cache))
			cc = (ColorCache *) cval.ptr;
		else {
			cc = (ColorCache *) malloc(sizeof(ColorCache));
			memset(cc, 0, sizeof(*cc));
			cval.ptr = cc;
			pcb_hid_cache_color(1, name, &cval, &cache);
		}

		if (!cc->color_set) {
			if (! gdk_rgba_parse(&cc->color, name))
				gdk_rgba_parse(&cc->color, "white");
				//gdk_color_white(gport->colormap, &cc->color);
			//else
				//gdk_color_alloc(gport->colormap, &cc->color);
			cc->color_set = 1;
		}
		//if (gc->xor_mask) {
		//  if (!cc->xor_set) {
		//    cc->xor_color.red = cc->color.red ^ gport->bg_color.red;
		//    cc->xor_color.green = cc->color.green ^ gport->bg_color.green;
		//    cc->xor_color.blue = cc->color.blue ^ gport->bg_color.blue;
		//    gdk_color_alloc(gport->colormap, &cc->xor_color);
		//    cc->xor_set = 1;
		//  }
		//  gdk_gc_set_foreground(gc->gc, &cc->xor_color);
		//}
		//else {
		//  gdk_gc_set_foreground(gc->gc, &cc->color);
		//}
		cc->color.alpha = 1.0;
		copy_color(&gc->color, &cc->color);
	}
}

static void ghid_cairo_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	cairo_line_cap_t cap = CAIRO_LINE_CAP_BUTT;
	cairo_line_join_t join = CAIRO_LINE_JOIN_MITER;

	switch (style) {
	case pcb_cap_round:
		cap = CAIRO_LINE_CAP_ROUND;
		join = CAIRO_LINE_JOIN_ROUND;
		break;
	case pcb_cap_square:
		cap = CAIRO_LINE_CAP_SQUARE;
		join = CAIRO_LINE_JOIN_MITER;
		break;
	default:
		assert(!"unhandled cap");
	}
	gc->cap = cap;
	gc->join = join;

	//if (gc->gc)
	//  gdk_gc_set_line_attributes(WHICH_GC(gc), Vz(gc->width), GDK_LINE_SOLID, (GdkCapStyle) gc->cap, (GdkJoinStyle) gc->join);
}

static void ghid_cairo_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	//render_priv_t *priv = gport->render_priv;

	//if (priv->cr == NULL)
	//	return;

	gc->width = width;
	//cairo_set_line_width(priv->cr, Vz(gc->width));
	//if (gc->gc)
	//  gdk_gc_set_line_attributes(WHICH_GC(gc), Vz(gc->width), GDK_LINE_SOLID, (GdkCapStyle) gc->cap, (GdkJoinStyle) gc->join);
}

static void ghid_cairo_set_draw_xor(pcb_hid_gc_t gc, int xor_mask)
{
	render_priv_t *priv = gport->render_priv;

	priv->xor_mode = (xor_mask) ? 1 : 0;
	//gc->xor_mask = xor_mask;
	//if (!gc->gc)
	//  return;
	//gdk_gc_set_function(gc->gc, xor_mask ? GDK_XOR : GDK_COPY);
	//ghid_cairo_set_color(gc, gc->colorname);
}

static int use_gc(pcb_hid_gc_t gc)
{
	render_priv_t *priv = gport->render_priv;
	cairo_t *cr = priv->cr;
	//GdkWindow *window = gtk_widget_get_window(gport->top_window);

	if (gc->me_pointer != &gtk3_cairo_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to GTK HID\n");
		abort();
	}

	if (cr == NULL)
		return 0;

	//ghid_cairo_set_color(gc, gc->colorname);
	gdk_cairo_set_source_rgba(cr, &gc->color);

	/* negative line width means "unit is pixels", so -1 is "1 pixel", -5 is "5 pixels", regardless of the zoom. */
	if (gc->width <= 0)
		cairo_set_line_width(cr, 1.0 - gc->width);
	else
		cairo_set_line_width(cr, Vz(gc->width));

	//ghid_cairo_set_line_cap(gc, (pcb_cap_style_t) gc->cap);
	cairo_set_line_cap(cr, gc->cap);
	cairo_set_line_join(cr, gc->join);


	//if (!gc->gc) {
	//  gc->gc = gdk_gc_new(window);
	//  ghid_cairo_set_color(gc, gc->colorname);
	//  ghid_cairo_set_line_width(gc, gc->width);
	//  ghid_cairo_set_line_cap(gc, (pcb_cap_style_t) gc->cap);
	//  ghid_cairo_set_draw_xor(gc, gc->xor_mask);
	//  gdk_gc_set_clip_origin(gc->gc, 0, 0);
	//}
	//if (gc->mask_seq != mask_seq) {
	//  if (mask_seq)
	//    gdk_gc_set_clip_mask(gc->gc, gport->mask);
	//  else
	//    set_clip(priv, gc->gc);
	//  gc->mask_seq = mask_seq;
	//}
	//priv->u_gc = WHICH_GC(gc);
	return 1;
}

static void ghid_cairo_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double dx1, dy1, dx2, dy2;
	render_priv_t *priv = gport->render_priv;

	dx1 = Vx((double) x1);
	dy1 = Vy((double) y1);
	dx2 = Vx((double) x2);
	dy2 = Vy((double) y2);

	if (!pcb_line_clip
			(0, 0, gport->view.canvas_width, gport->view.canvas_height, &dx1, &dy1, &dx2, &dy2, gc->width / gport->view.coord_per_px))
		return;

	USE_GC(gc);
	cr_draw_line(priv->cr, FALSE, dx1, dy1, dx2, dy2);
	//gdk_draw_line(gport->drawable, priv->u_gc, dx1, dy1, dx2, dy2);
}

/* Draws an arc from center (cx, cy), cx>0 to the right, cy>0 to the bottom, using
   xradius on X axis, and yradius on Y axis, sweeping a delta_angle from start_angle.

   Angles (in degres) originate at 9 o'clock and are positive CCW (counter clock wise).
   delta_angle sweeps the angle from start_angle, CW if negative. */
static void ghid_cairo_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy,
																pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	double w, h, radius, angle1, angle2;
	render_priv_t *priv = gport->render_priv;

	w = gport->view.canvas_width * gport->view.coord_per_px;
	h = gport->view.canvas_height * gport->view.coord_per_px;
	radius = (xradius > yradius) ? xradius : yradius;
	if (SIDE_X(cx) < gport->view.x0 - radius
			|| SIDE_X(cx) > gport->view.x0 + w + radius
			|| SIDE_Y(cy) < gport->view.y0 - radius || SIDE_Y(cy) > gport->view.y0 + h + radius)
		return;

	USE_GC(gc);

	if ((delta_angle > 360.0) || (delta_angle < -360.0)) {
		start_angle = 0;
		delta_angle = 360;
	}

	if (conf_core.editor.view.flip_x) {
		start_angle = 180 - start_angle;
		delta_angle = -delta_angle;
	}
	if (conf_core.editor.view.flip_y) {
		start_angle = -start_angle;
		delta_angle = -delta_angle;
	}
	/* make sure we fall in the -180 to +180 range */
	start_angle = pcb_normalize_angle(start_angle);
	if (start_angle >= 180)
		start_angle -= 360;

	/* cairo lib. originates angles at 3 o'clock, positive traveling CW, requires angle1 < angle2 */
	angle1 = (180.0 - start_angle);
	angle2 = (delta_angle < 0) ? (angle1 - delta_angle) : angle1;
	if (delta_angle > 0) {
		angle1 = (180.0 - start_angle - delta_angle);
	}
	angle1 *= (M_PI / 180.0);
	angle2 *= (M_PI / 180.0);
	cairo_save(priv->cr);
TODO("gtk3: this will draw an arc of a circle, not an ellipse! Explore matrix transformation here.")
	cairo_arc(priv->cr, pcb_round(Vxd(cx)), pcb_round(Vyd(cy)), Vzd(radius), angle1, angle2);
	cairo_stroke(priv->cr);
	cairo_restore(priv->cr);
	//gdk_draw_arc(gport->drawable, priv->u_gc, 0,
	//             pcb_round(Vxd(cx) - Vzd(xradius) + 0.5), pcb_round(Vyd(cy) - Vzd(yradius) + 0.5),
	//             pcb_round(vrx2), pcb_round(vry2), (start_angle + 180) * 64, delta_angle * 64);
}

static void cr_draw_rect(pcb_hid_gc_t gc, int fill, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gint w, h, lw;
	render_priv_t *priv = gport->render_priv;

	if (priv->cr == NULL)
		return;

	lw = gc->width;
	w = gport->view.canvas_width * gport->view.coord_per_px;
	h = gport->view.canvas_height * gport->view.coord_per_px;

	if ((SIDE_X(x1) < gport->view.x0 - lw && SIDE_X(x2) < gport->view.x0 - lw)
			|| (SIDE_X(x1) > gport->view.x0 + w + lw && SIDE_X(x2) > gport->view.x0 + w + lw)
			|| (SIDE_Y(y1) < gport->view.y0 - lw && SIDE_Y(y2) < gport->view.y0 - lw)
			|| (SIDE_Y(y1) > gport->view.y0 + h + lw && SIDE_Y(y2) > gport->view.y0 + h + lw))
		return;

	x1 = Vx(x1);
	y1 = Vy(y1);
	x2 = Vx(x2);
	y2 = Vy(y2);

	if (x1 > x2) {
		gint xt = x1;
		x1 = x2;
		x2 = xt;
	}
	if (y1 > y2) {
		gint yt = y1;
		y1 = y2;
		y2 = yt;
	}

	USE_GC(gc);
	cairo_rectangle(priv->cr, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	//gdk_draw_rectangle(gport->drawable, priv->u_gc, FALSE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);

	if (fill)
		cairo_fill(priv->cr);
	else
		cairo_stroke(priv->cr);
}

static void cr_paint_from_surface(cairo_t * cr, cairo_surface_t * surface)
{
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_paint_with_alpha(cr, 1.0);
}

static void ghid_cairo_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	cr_draw_rect(gc, FALSE, x1, y1,x2, y2);
}

static void ghid_cairo_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	cr_draw_rect(gc, TRUE, x1, y1,x2, y2);
}

static void ghid_cairo_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	gint w, h, vr;
	render_priv_t *priv = gport->render_priv;

	if (priv->cr == NULL)
		return;

	w = gport->view.canvas_width * gport->view.coord_per_px;
	h = gport->view.canvas_height * gport->view.coord_per_px;
	if (SIDE_X(cx) < gport->view.x0 - radius
			|| SIDE_X(cx) > gport->view.x0 + w + radius
			|| SIDE_Y(cy) < gport->view.y0 - radius || SIDE_Y(cy) > gport->view.y0 + h + radius)
		return;

	USE_GC(gc);
	vr = Vz(radius);
	cairo_arc(priv->cr, Vx(cx), Vy(cy), vr, 0.0, 2 * M_PI);
	cairo_fill(priv->cr);
	//gdk_draw_arc(gport->drawable, priv->u_gc, TRUE, Vx(cx) - vr, Vy(cy) - vr, vr * 2, vr * 2, 0, 360 * 64);
}

/* Intentional code duplication from  ghid_cairo_fill_polygon_offs(), for performance */
static void ghid_cairo_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	int i;
	render_priv_t *priv = gport->render_priv;
	cairo_t *cr = priv->cr;
	int _x, _y;

	if (priv->cr == NULL)
		return;

	USE_GC(gc);

	for (i = 0; i < n_coords; i++) {
		_x = Vx(x[i]);
		_y = Vy(y[i]);
		if (i == 0)
			cairo_move_to(cr, _x, _y);
		else
			cairo_line_to(cr, _x, _y);
	}
	cairo_fill(cr);
}

static void ghid_cairo_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	int i;
	render_priv_t *priv = gport->render_priv;
	cairo_t *cr = priv->cr;
	int _x, _y;

	if (priv->cr == NULL)
		return;

	USE_GC(gc);

	for (i = 0; i < n_coords; i++) {
		_x = Vx(x[i]);
		_y = Vy(y[i]);
		if (i == 0)
			cairo_move_to(cr, _x + Vz(dx), _y + Vz(dy));
		else
			cairo_line_to(cr, _x + Vz(dx), _y + Vz(dy));
	}
	cairo_fill(cr);
}

/* Fills the drawing with only bg_color */
static void erase_with_background_color(cairo_t * cr, GdkRGBA * bg_color)
{
	gdk_cairo_set_source_rgba(cr, bg_color);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint_with_alpha(cr, 1.0);
}

static void redraw_region(GdkRectangle * rect)
{
	int eleft, eright, etop, ebottom;
	pcb_hid_expose_ctx_t ctx;
	render_priv_t *priv = gport->render_priv;

	if (priv->cr_drawing_area == NULL)
	  return;

	if (rect != NULL) {
		priv->clip_rect = *rect;
		priv->clip = pcb_true;
	}
	else {
		priv->clip_rect.x = 0;
		priv->clip_rect.y = 0;
		priv->clip_rect.width = gport->view.canvas_width;
		priv->clip_rect.height = gport->view.canvas_height;
		priv->clip = pcb_false;
	}

	//set_clip(priv, priv->bg_gc);
	//set_clip(priv, priv->offlimits_gc);
	//set_clip(priv, priv->mask_gc);
	//set_clip(priv, priv->grid_gc);

	ctx.view.X1 = MIN(Px(priv->clip_rect.x), Px(priv->clip_rect.x + priv->clip_rect.width + 1));
	ctx.view.Y1 = MIN(Py(priv->clip_rect.y), Py(priv->clip_rect.y + priv->clip_rect.height + 1));
	ctx.view.X2 = MAX(Px(priv->clip_rect.x), Px(priv->clip_rect.x + priv->clip_rect.width + 1));
	ctx.view.Y2 = MAX(Py(priv->clip_rect.y), Py(priv->clip_rect.y + priv->clip_rect.height + 1));

	ctx.view.X1 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X1));
	ctx.view.X2 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X2));
	ctx.view.Y1 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y1));
	ctx.view.Y2 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y2));

	eleft = Vx(0);
	eright = Vx(PCB->MaxWidth);
	etop = Vy(0);
	ebottom = Vy(PCB->MaxHeight);
	if (eleft > eright) {
		int tmp = eleft;
		eleft = eright;
		eright = tmp;
	}
	if (etop > ebottom) {
		int tmp = etop;
		etop = ebottom;
		ebottom = tmp;
	}

	/* Paints only on the drawing_area */
	priv->cr = priv->cr_drawing_area;
	priv->cr_target = priv->cr_drawing_area;

	erase_with_background_color(priv->cr, &priv->bg_color);

	gdk_cairo_set_source_rgba(priv->cr, &priv->offlimits_color);
	if (eleft > 0) {
		cairo_rectangle(priv->cr, 0.0, 0.0, eleft, gport->view.canvas_height);
	}
	else
		eleft = 0;
	if (eright < gport->view.canvas_width) {
		cairo_rectangle(priv->cr, eright, 0.0, gport->view.canvas_width - eright, gport->view.canvas_height);
	}
	else
		eright = gport->view.canvas_width;
	if (etop > 0) {
		cairo_rectangle(priv->cr, eleft, 0.0, eright - eleft + 1, etop);
	}
	else
		etop = 0;
	if (ebottom < gport->view.canvas_height) {
		cairo_rectangle(priv->cr, eleft, ebottom, eright - eleft + 1, gport->view.canvas_height - ebottom);
	}
	else
		ebottom = gport->view.canvas_height;

	cairo_fill(priv->cr);
	//gdk_draw_rectangle(gport->drawable, priv->bg_gc, 1, eleft, etop, eright - eleft + 1, ebottom - etop + 1);

	ghid_cairo_draw_bg_image();

	pcb_hid_expose_all(pcb_gui, &ctx, NULL);
	ghid_cairo_draw_grid();

	/* Draws "GUI" information on top of design */
	priv->cr = priv->cr_drawing_area;
	pcb_draw_attached(0);
	pcb_draw_mark(0);

	draw_lead_user(priv);

	priv->clip = pcb_false;

	/* Rest the clip for bg_gc, as it is used outside this function */
	//gdk_gc_set_clip_mask(priv->bg_gc, NULL);
	ghid_cairo_screen_update();
}

static void ghid_cairo_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
{
	int dleft, dright, dtop, dbottom;
	int minx, maxx, miny, maxy;
	GdkRectangle rect;

	dleft = Vx(left);
	dright = Vx(right);
	dtop = Vy(top);
	dbottom = Vy(bottom);

	minx = MIN(dleft, dright);
	maxx = MAX(dleft, dright);
	miny = MIN(dtop, dbottom);
	maxy = MAX(dtop, dbottom);

	rect.x = minx;
	rect.y = miny;
	rect.width = maxx - minx;
	rect.height = maxy - miny;

	redraw_region(&rect);
}

static void ghid_cairo_invalidate_all()
{
	if (ghidgui && ghidgui->topwin.menu.menu_bar) {
		redraw_region(NULL);
	}
}

static void ghid_cairo_notify_crosshair_change(pcb_bool changes_complete)
{
	render_priv_t *priv = gport->render_priv;

	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	/* Keeps track of calls (including XOR draw mode), and draws only when complete and final. */
	if (changes_complete)
		priv->attached_invalidate_depth--;
	else
		priv->attached_invalidate_depth = 1 + priv->xor_mode;

	if (changes_complete && gport->drawing_area != NULL && priv->attached_invalidate_depth <= 0) {
		/* Queues a GTK redraw when changes are complete */
		ghid_cairo_invalidate_all();
		priv->attached_invalidate_depth = 0;
	}
}

static void ghid_cairo_notify_mark_change(pcb_bool changes_complete)
{
	render_priv_t *priv = gport->render_priv;

	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	if (changes_complete)
		priv->mark_invalidate_depth--;

	if (priv->mark_invalidate_depth < 0) {
		priv->mark_invalidate_depth = 0;
		/* A mismatch of changes_complete == pcb_false and == pcb_true notifications
		   is not expected to occur, but we will try to handle it gracefully.
		   As we know the mark will have been shown already, we must
		   repaint the entire view to be sure not to leave an artefact.
		 */
		ghid_cairo_invalidate_all();
		return;
	}

	if (priv->mark_invalidate_depth == 0)
		pcb_draw_mark(0);

	if (!changes_complete) {
		priv->mark_invalidate_depth++;
	}
	else if (gport->drawing_area != NULL) {
		/* Queue a GTK expose when changes are complete */
		ghid_draw_area_update(gport, NULL);
	}
}

static void draw_right_cross(cairo_t * cr, gint x, gint y)
{
	//GdkWindow *window = gtk_widget_get_window(gport->drawing_area);

	cr_draw_line(cr, FALSE, x, 0, x, gport->view.canvas_height);
	cr_draw_line(cr, FALSE, 0, y, gport->view.canvas_width, y);
	//gdk_draw_line(window, xor_gc, x, 0, x, gport->view.canvas_height);
	//gdk_draw_line(window, xor_gc, 0, y, gport->view.canvas_width, y);
}

static void draw_slanted_cross(cairo_t * xor_gc, gint x, gint y)
{
	//GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
	gint x0, y0, x1, y1;

	x0 = x + (gport->view.canvas_height - y);
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x - y;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + (gport->view.canvas_width - x);
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - x;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	cr_draw_line(gport->render_priv->cr, FALSE, x0, y0, x1, y1);
	//gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (gport->view.canvas_height - y);
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x + y;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + x;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - (gport->view.canvas_width - x);
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	cr_draw_line(gport->render_priv->cr, FALSE, x0, y0, x1, y1);
	//gdk_draw_line(window, xor_gc, x0, y0, x1, y1);
}

static void draw_dozen_cross(cairo_t * xor_gc, gint x, gint y)
{
	//GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
	gint x0, y0, x1, y1;
	gdouble tan60 = sqrt(3);

	x0 = x + (gport->view.canvas_height - y) / tan60;
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x - y / tan60;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + (gport->view.canvas_width - x) * tan60;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - x * tan60;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	cr_draw_line(gport->render_priv->cr, FALSE, x0, y0, x1, y1);
	//gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x + (gport->view.canvas_height - y) * tan60;
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x - y * tan60;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + (gport->view.canvas_width - x) / tan60;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - x / tan60;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	cr_draw_line(gport->render_priv->cr, FALSE, x0, y0, x1, y1);
	//gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (gport->view.canvas_height - y) / tan60;
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x + y / tan60;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + x * tan60;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - (gport->view.canvas_width - x) * tan60;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	cr_draw_line(gport->render_priv->cr, FALSE, x0, y0, x1, y1);
	//gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (gport->view.canvas_height - y) * tan60;
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x + y * tan60;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + x / tan60;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - (gport->view.canvas_width - x) / tan60;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	cr_draw_line(gport->render_priv->cr, FALSE, x0, y0, x1, y1);
	//gdk_draw_line(window, xor_gc, x0, y0, x1, y1);
}

static void draw_crosshair(cairo_t * xor_gc, gint x, gint y)
{
	static enum pcb_crosshair_shape_e prev = pcb_ch_shape_basic;

	if (gport->view.crosshair_x < 0 || !ghidgui->topwin.active || !gport->view.has_entered)
		return;

	draw_right_cross(xor_gc, x, y);
	if (prev == pcb_ch_shape_union_jack)
		draw_slanted_cross(xor_gc, x, y);
	if (prev == pcb_ch_shape_dozen)
		draw_dozen_cross(xor_gc, x, y);
	prev = pcb_crosshair.shape;
}

static void show_crosshair(gboolean paint_new_location)
{
	render_priv_t *priv = gport->render_priv;
	//GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
	//GtkStyle *style = gtk_widget_get_style(gport->drawing_area);
	gint x, y;
	static gint x_prev = -1, y_prev = -1;
	//static cairo_t *xor_gc;
	//static GdkColor cross_color;
	cairo_t *cr;

	if (gport->view.crosshair_x < 0 || !ghidgui->topwin.active || !gport->view.has_entered)
		return;

TODO("gtk3: when CrossColor changed from config")
	map_color_string(conf_core.appearance.color.cross.str, &priv->crosshair_color);
	//cr = priv->cr_drawing_area;
	cr = priv->cr;
	gdk_cairo_set_source_rgba(cr, &priv->crosshair_color);
	cairo_set_line_width(cr, 1.0);

	//if (!xor_gc) {
	//  xor_gc = gdk_gc_new(window);
	//  gdk_gc_copy(xor_gc, style->white_gc);
	//  gdk_gc_set_function(xor_gc, GDK_XOR);
	//  gdk_gc_set_clip_origin(xor_gc, 0, 0);
	//  set_clip(priv, xor_gc);
TODO("gtk3:FIXME: when CrossColor changed from config")
	//  map_color_string(conf_core.appearance.color.cross, &cross_color);
	//}
	x = Vx(gport->view.crosshair_x);
	y = Vy(gport->view.crosshair_y);

	//gdk_gc_set_foreground(xor_gc, &cross_color);

	if (x_prev >= 0 && !paint_new_location)
		draw_crosshair(cr, x_prev, y_prev);

	if (x >= 0 && paint_new_location) {
		draw_crosshair(cr, x, y);
		x_prev = x;
		y_prev = y;
	}
	else
		x_prev = y_prev = -1;
}

static void ghid_cairo_init_renderer(int *argc, char ***argv, void *vport)
{
	GHidPort *port = vport;

	/* Init any GC's required */
	port->render_priv = g_new0(render_priv_t, 1);
	port->render_priv->cr = NULL;
	port->render_priv->surf_da = NULL;
	port->render_priv->cr_drawing_area = NULL;
}

static void ghid_cairo_shutdown_renderer(void *vport)
{
	GHidPort *port = vport;
	render_priv_t *priv = port->render_priv;

	cairo_surface_destroy(priv->surf_da);
	priv->surf_da = NULL;
	cairo_destroy(priv->cr_drawing_area);
	priv->cr_drawing_area = NULL;
	priv->cr = NULL;

	g_free(port->render_priv);
	port->render_priv = NULL;
}

static void ghid_cairo_init_drawing_widget(GtkWidget * widget, void *vport)
{
}

/* Callback function only applied to drawing_area, after some resizing or initialisation. */
static void ghid_cairo_drawing_area_configure_hook(void *vport)
{
	GHidPort *port = vport;
	static int done_once = 0;
	render_priv_t *priv = port->render_priv;
	cairo_t *cr;

	gport->drawing_allowed = pcb_true;

	if (!done_once) {
		//priv->bg_gc = gdk_gc_new(port->drawable);
		//gdk_gc_set_foreground(priv->bg_gc, &port->bg_color);
		//gdk_gc_set_clip_origin(priv->bg_gc, 0, 0);
		//
		//priv->offlimits_gc = gdk_gc_new(port->drawable);
		//gdk_gc_set_foreground(priv->offlimits_gc, &port->offlimits_color);
		//gdk_gc_set_clip_origin(priv->offlimits_gc, 0, 0);

		if (!map_color_string(conf_core.appearance.color.background.str, &priv->bg_color))
			map_color_string("white", &priv->bg_color);

		if (!map_color_string(conf_core.appearance.color.off_limit.str, &priv->offlimits_color))
			map_color_string("white", &priv->offlimits_color);

		if (!map_color_string(conf_core.appearance.color.grid.str, &priv->grid_color))
			map_color_string("blue", &priv->grid_color);
		//set_special_grid_color();

		//if (port->mask) {
		//  gdk_pixmap_unref(port->mask);
		//  port->mask = gdk_pixmap_new(0, port->view.canvas_width, port->view.canvas_height, 1);
		//}
		done_once = 1;
	}

	/* Creates a single cairo surface/context for off-line painting */
	cr_create_similar_surface_and_context(&priv->surf_da, &priv->cr_drawing_area, port);
  cr_create_similar_surface_and_context(&priv->surf_layer, &priv->cr_layer, port);
	priv->cr = priv->cr_drawing_area;
}

/* GtkDrawingArea -> GtkWidget "draw" signal Call-Back function */
static gboolean ghid_cairo_drawing_area_expose_cb(GtkWidget * widget, pcb_gtk_expose_t * p, void *vport)
{
	GHidPort *port = vport;
	render_priv_t *priv = port->render_priv;
	//GdkWindow *window = gtk_widget_get_window(gport->drawing_area);

	//gdk_draw_drawable(window, priv->bg_gc, port->pixmap,
	//                  ev->area.x, ev->area.y, ev->area.x, ev->area.y, ev->area.width, ev->area.height);

	cr_paint_from_surface(p, priv->surf_da);

	priv->cr = p;
	show_crosshair(TRUE);
	priv->cr = priv->cr_drawing_area;

	return FALSE;
}

static void ghid_cairo_screen_update(void)
{
	render_priv_t *priv = gport->render_priv;
	//GdkWindow *window = gtk_widget_get_window(gport->drawing_area);

	if (priv->cr == NULL)
	  return;

	//gdk_draw_drawable(window, priv->bg_gc, gport->pixmap, 0, 0, 0, 0, gport->view.canvas_width, gport->view.canvas_height);
	//show_crosshair(TRUE);
	gtk_widget_queue_draw(gport->drawing_area);
}

static void ghid_cairo_port_drawing_realize_cb(GtkWidget * widget, gpointer data)
{
}

static gboolean ghid_cairo_preview_expose(GtkWidget * widget, pcb_gtk_expose_t * p,
																					pcb_hid_expose_t expcall, pcb_hid_expose_ctx_t *ctx)
{
	//GdkWindow *window = gtk_widget_get_window(widget);
	//GdkDrawable *save_drawable;
	GtkAllocation allocation;			/* Assuming widget is a drawing widget, get the Rectangle allowed for drawing. */
	pcb_gtk_view_t save_view;
	int save_width, save_height;
	double xz, yz, vw, vh;
	render_priv_t *priv = gport->render_priv;
	pcb_coord_t ox1 = ctx->view.X1, oy1 = ctx->view.Y1, ox2 = ctx->view.X2, oy2 = ctx->view.Y2;

	vw = ctx->view.X2 - ctx->view.X1;
	vh = ctx->view.Y2 - ctx->view.Y1;

	/* Setup drawable and zoom factor for drawing routines
	 */
	//save_drawable = gport->drawable;
	save_view = gport->view;
	save_width = gport->view.canvas_width;
	save_height = gport->view.canvas_height;

	gtk_widget_get_allocation(widget, &allocation);
	xz = vw / (double) allocation.width;
	yz = vh / (double) allocation.height;
	if (xz > yz)
		gport->view.coord_per_px = xz;
	else
		gport->view.coord_per_px = yz;

	//gport->drawable = window;
	gport->view.canvas_width = allocation.width;
	gport->view.canvas_height = allocation.height;
	gport->view.width = allocation.width * gport->view.coord_per_px;
	gport->view.height = allocation.height * gport->view.coord_per_px;
	gport->view.x0 = (vw - gport->view.width) / 2 + ctx->view.X1;
	gport->view.y0 = (vh - gport->view.height) / 2 + ctx->view.Y1;

	PCB_GTK_PREVIEW_TUNE_EXTENT(ctx, allocation);

	/* Change current pointer to draw in this widget, draws, then come back to pointer to drawing_area. */
	priv->cr_target = p;
	priv->cr = p;
	cairo_save(p);
	/* calls the off-line drawing routine */
	expcall(&gtk3_cairo_hid, ctx);
	cairo_restore(p);
	priv->cr = priv->cr_drawing_area;
	//gport->drawable = save_drawable;

	/* restore the original context and priv */
	ctx->view.X1 = ox1; ctx->view.X2 = ox2; ctx->view.Y1 = oy1; ctx->view.Y2 = oy2;
	gport->view = save_view;
	gport->view.canvas_width = save_width;
	gport->view.canvas_height = save_height;

	return FALSE;
}

static void draw_lead_user(render_priv_t * priv)
{
	//GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
	//GtkStyle *style = gtk_widget_get_style(gport->drawing_area);
	int i;
	pcb_lead_user_t *lead_user = &gport->lead_user;
	pcb_coord_t radius = lead_user->radius;
	pcb_coord_t width = PCB_MM_TO_COORD(LEAD_USER_WIDTH);
	pcb_coord_t separation = PCB_MM_TO_COORD(LEAD_USER_ARC_SEPARATION);
	//static GdkGC *lead_gc = NULL;
	//GdkColor lead_color;

	if (!lead_user->lead_user)
		return;

	//if (lead_gc == NULL) {
	//  lead_gc = gdk_gc_new(window);
	//  gdk_gc_copy(lead_gc, style->white_gc);
	//  gdk_gc_set_function(lead_gc, GDK_XOR);
	//  gdk_gc_set_clip_origin(lead_gc, 0, 0);
	//  lead_color.pixel = 0;
	//  lead_color.red = (int) (65535. * LEAD_USER_COLOR_R);
	//  lead_color.green = (int) (65535. * LEAD_USER_COLOR_G);
	//  lead_color.blue = (int) (65535. * LEAD_USER_COLOR_B);
	//  gdk_color_alloc(gport->colormap, &lead_color);
	//  gdk_gc_set_foreground(lead_gc, &lead_color);
	//}

	//set_clip(priv, lead_gc);
	//gdk_gc_set_line_attributes(lead_gc, Vz(width), GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

	/* arcs at the appropriate radii */

	for (i = 0; i < LEAD_USER_ARC_COUNT; i++, radius -= separation) {
		if (radius < width)
			radius += PCB_MM_TO_COORD(LEAD_USER_INITIAL_RADIUS);

		/* Draw an arc at radius */
		//gdk_draw_arc(gport->drawable, lead_gc, FALSE,
		//             Vx(lead_user->x - radius), Vy(lead_user->y - radius), Vz(2. * radius), Vz(2. * radius), 0, 360 * 64);
	}
}

static GtkWidget *ghid_cairo_new_drawing_widget(pcb_gtk_common_t *common)
{
	GtkWidget *w = gtk_drawing_area_new();

	g_signal_connect(G_OBJECT(w), "draw", G_CALLBACK(common->drawing_area_expose), common->gport);

	return w;
}


void ghid_cairo_install(pcb_gtk_common_t * common, pcb_hid_t * hid)
{
	if (common != NULL) {
		common->new_drawing_widget = ghid_cairo_new_drawing_widget;
		common->init_drawing_widget = ghid_cairo_init_drawing_widget;
		common->drawing_realize = ghid_cairo_port_drawing_realize_cb;
		common->drawing_area_expose = ghid_cairo_drawing_area_expose_cb;
		common->preview_expose = ghid_cairo_preview_expose;
		common->invalidate_all = ghid_cairo_invalidate_all;
		common->set_special_colors = ghid_cairo_set_special_colors;
		common->init_renderer = ghid_cairo_init_renderer;
		common->screen_update = ghid_cairo_screen_update;
		common->draw_grid_local = ghid_cairo_draw_grid_local;
		common->drawing_area_configure_hook = ghid_cairo_drawing_area_configure_hook;
		common->shutdown_renderer = ghid_cairo_shutdown_renderer;
		common->get_color_name = get_color_name;
		common->map_color_string = map_color_string;
	}

	if (hid != NULL) {
		hid->invalidate_lr = ghid_cairo_invalidate_lr;
		hid->invalidate_all = ghid_cairo_invalidate_all;
		hid->notify_crosshair_change = ghid_cairo_notify_crosshair_change;
		hid->notify_mark_change = ghid_cairo_notify_mark_change;
		hid->set_layer_group = ghid_cairo_set_layer_group;
		hid->end_layer = ghid_cairo_end_layer_group;
		hid->make_gc = ghid_cairo_make_gc;
		hid->destroy_gc = ghid_cairo_destroy_gc;
		hid->render_burst = ghid_cairo_render_burst;
		hid->set_drawing_mode = ghid_cairo_set_drawing_mode;
		hid->set_color = ghid_cairo_set_color;
		hid->set_line_cap = ghid_cairo_set_line_cap;
		hid->set_line_width = ghid_cairo_set_line_width;
		hid->set_draw_xor = ghid_cairo_set_draw_xor;
		hid->draw_line = ghid_cairo_draw_line;
		hid->draw_arc = ghid_cairo_draw_arc;
		hid->draw_rect = ghid_cairo_draw_rect;
		hid->fill_circle = ghid_cairo_fill_circle;
		hid->fill_polygon = ghid_cairo_fill_polygon;
		hid->fill_polygon_offs = ghid_cairo_fill_polygon_offs;
		hid->fill_rect = ghid_cairo_fill_rect;
	}
}

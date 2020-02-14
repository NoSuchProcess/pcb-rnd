#include "config.h"
#include <librnd/core/hidlib_conf.h>

#include <stdio.h>

#include "crosshair.h"
#include "draw.h"
#include <librnd/core/grid.h>
#include <librnd/core/color.h>
#include <librnd/core/color_cache.h>
#include <librnd/core/hid_attrib.h>
#include "funchash_core.h"
#include <librnd/core/pixmap.h>

#include "../src_plugins/lib_gtk_common/pcb_gtk.h"
#include "../src_plugins/lib_gtk_common/compat.h"
#include "../src_plugins/lib_gtk_common/glue_common.h"
#include "../src_plugins/lib_gtk_common/coord_conv.h"

#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

#include "../src_plugins/lib_hid_common/clip.h"

extern pcb_hid_t gtk2_gdk_hid;
static void ghid_gdk_screen_update(void);

/* Sets priv->u_gc to the "right" GC to use (wrt mask or window) */
#define USE_GC(gc)        if (!use_gc(gc, 1)) return
#define USE_GC_NOPEN(gc)  if (!use_gc(gc, 0)) return

typedef struct render_priv_s {
	GdkGC *bg_gc;
	GdkColor bg_color;
	GdkGC *offlimits_gc;
	GdkColor offlimits_color;
	GdkGC *grid_gc;
	GdkGC *clear_gc, *copy_gc;
	GdkColor grid_color;
	GdkRectangle clip_rect;
	pcb_bool clip_rect_valid;
	pcb_bool direct;
	int attached_invalidate_depth;
	int mark_invalidate_depth;

	/* available canvases */
	GdkPixmap *base_pixel;                  /* base canvas, pixel colors only */
	GdkPixmap *sketch_pixel, *sketch_clip;  /* sketch canvas for compositing (non-direct) */

	/* currently active: drawing routines draw to these*/
	GdkDrawable *out_pixel, *out_clip;
	GdkGC *pixel_gc, *clip_gc;
	GdkColor clip_color;

	/* color cache for set_color */
	pcb_clrcache_t ccache;
	int ccache_inited;
} render_priv_t;


typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	pcb_hid_t *me_pointer;
	GdkGC *pixel_gc;
	GdkGC *clip_gc;

	pcb_color_t pcolor;
	pcb_coord_t width;
	gint cap, join;
	gchar xor_mask;
	gint sketch_seq;
} hid_gc_s;

static const gchar *get_color_name(pcb_gtk_color_t *color)
{
	static char tmp[16];

	if (!color)
		return "#000000";

	sprintf(tmp, "#%2.2x%2.2x%2.2x", (color->red >> 8) & 0xff, (color->green >> 8) & 0xff, (color->blue >> 8) & 0xff);
	return tmp;
}

/* Returns TRUE only if color_string has been allocated to color. */
static pcb_bool map_color(const pcb_color_t *inclr, pcb_gtk_color_t *color)
{
	static GdkColormap *colormap = NULL;

	if (!color || !ghidgui->port.top_window)
		return FALSE;
	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(ghidgui->port.top_window);
	if (color->red || color->green || color->blue)
		gdk_colormap_free_colors(colormap, color, 1);

	color->red = (unsigned)inclr->r << 8;
	color->green = (unsigned)inclr->g << 8;
	color->blue = (unsigned)inclr->b << 8;
	gdk_color_alloc(colormap, color);

	return TRUE;
}


static int ghid_gdk_set_layer_group(pcb_hid_t *hid, pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	/* draw anything */
	return 1;
}

static void ghid_gdk_destroy_gc(pcb_hid_gc_t gc)
{
	if (gc->pixel_gc)
		g_object_unref(gc->pixel_gc);
	if (gc->clip_gc)
		g_object_unref(gc->clip_gc);
	g_free(gc);
}

static pcb_hid_gc_t ghid_gdk_make_gc(pcb_hid_t *hid)
{
	pcb_hid_gc_t rv;

	rv = g_new0(hid_gc_s, 1);
	rv->me_pointer = &gtk2_gdk_hid;
	rv->pcolor = pcbhl_conf.appearance.color.background;
	return rv;
}

static void set_clip(render_priv_t *priv, GdkGC *gc)
{
	if (gc == NULL)
		return;
	if (priv->clip_rect_valid)
		gdk_gc_set_clip_rectangle(gc, &priv->clip_rect);
	else
		gdk_gc_set_clip_mask(gc, NULL);
}

static inline void ghid_gdk_draw_grid_global(pcb_hidlib_t *hidlib)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	pcb_coord_t x, y, x1, y1, x2, y2, grd;
	int n, i;
	static GdkPoint *points = NULL;
	static int npoints = 0;

	x1 = pcb_grid_fit(MAX(0, SIDE_X(&ghidgui->port.view, ghidgui->port.view.x0)), hidlib->grid, hidlib->grid_ox);
	y1 = pcb_grid_fit(MAX(0, SIDE_Y(&ghidgui->port.view, ghidgui->port.view.y0)), hidlib->grid, hidlib->grid_oy);
	x2 = pcb_grid_fit(MIN(hidlib->size_x, SIDE_X(&ghidgui->port.view, ghidgui->port.view.x0 + ghidgui->port.view.width - 1)), hidlib->grid, hidlib->grid_ox);
	y2 = pcb_grid_fit(MIN(hidlib->size_y, SIDE_Y(&ghidgui->port.view, ghidgui->port.view.y0 + ghidgui->port.view.height - 1)), hidlib->grid, hidlib->grid_oy);

	grd = hidlib->grid;
	if (grd <= 0)
		grd = 1;

	if (Vz(grd) < pcb_conf_hid_gtk.plugins.hid_gtk.global_grid.min_dist_px) {
		if (!pcb_conf_hid_gtk.plugins.hid_gtk.global_grid.sparse)
			return;
		grd *= (pcb_conf_hid_gtk.plugins.hid_gtk.global_grid.min_dist_px / Vz(grd));
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
	if (Vx(x2) >= ghidgui->port.view.canvas_width)
		x2 -= grd;
	if (Vy(y2) >= ghidgui->port.view.canvas_height)
		y2 -= grd;


	n = (x2 - x1) / grd + 1;
	if (n <= 0)
		n = 1;
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
	for (y = y1; y <= y2; y += grd) {
		int vy = Vy(y);
		for (i = 0; i < n; i++)
			points[i].y = vy;
		gdk_draw_points(priv->out_pixel, priv->grid_gc, points, n);
	}
}

static void ghid_gdk_draw_grid_local_(pcb_hidlib_t *hidlib, pcb_coord_t cx, pcb_coord_t cy, int radius)
{
	render_priv_t *priv = ghidgui->port.render_priv;
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
		points_abs  = (GdkPoint *) realloc(points_abs,  apoints * sizeof(GdkPoint));
	}

	if (radius != old_radius) {
		old_radius = radius;
		recalc = 1;
	}

	if (last_grid != hidlib->grid) {
		last_grid = hidlib->grid;
		recalc = 1;
	}

	/* reclaculate the 'filled circle' mask (base relative coords) if grid or radius changed */
	if (recalc) {

		npoints = 0;
		for(y = -radius; y <= radius; y++) {
			int y2 = y*y;
			for(x = -radius; x <= radius; x++) {
				if (x*x + y2 < r2) {
					points_base[npoints].x = x*hidlib->grid;
					points_base[npoints].y = y*hidlib->grid;
					npoints++;
				}
			}
		}
	}

	/* calculate absolute positions */
	for(n = 0; n < npoints; n++) {
		points_abs[n].x = Vx(points_base[n].x + cx);
		points_abs[n].y = Vy(points_base[n].y + cy);
	}

	gdk_draw_points(priv->out_pixel, priv->grid_gc, points_abs, npoints);
}


static int grid_local_have_old = 0, grid_local_old_r = 0;
static pcb_coord_t grid_local_old_x, grid_local_old_y;

static void ghid_gdk_draw_grid_local(pcb_hidlib_t *hidlib, pcb_coord_t cx, pcb_coord_t cy)
{
	if (grid_local_have_old) {
		ghid_gdk_draw_grid_local_(hidlib, grid_local_old_x, grid_local_old_y, grid_local_old_r);
		grid_local_have_old = 0;
	}

	if (!pcb_conf_hid_gtk.plugins.hid_gtk.local_grid.enable)
		return;

	if ((Vz(hidlib->grid) < PCB_MIN_GRID_DISTANCE) || (!pcbhl_conf.editor.draw_grid))
		return;

	/* cx and cy are the actual cursor snapped to wherever - round them to the nearest real grid point */
	cx = (cx / hidlib->grid) * hidlib->grid + hidlib->grid_ox;
	cy = (cy / hidlib->grid) * hidlib->grid + hidlib->grid_oy;

	grid_local_have_old = 1;
	ghid_gdk_draw_grid_local_(hidlib, cx, cy, pcb_conf_hid_gtk.plugins.hid_gtk.local_grid.radius);
	grid_local_old_x = cx;
	grid_local_old_y = cy;
	grid_local_old_r = pcb_conf_hid_gtk.plugins.hid_gtk.local_grid.radius;
}

static void ghid_gdk_draw_grid(pcb_hidlib_t *hidlib)
{
	static GdkColormap *colormap = NULL;
	render_priv_t *priv = ghidgui->port.render_priv;

	grid_local_have_old = 0;

	if (!pcbhl_conf.editor.draw_grid)
		return;
	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(ghidgui->port.top_window);

	if (!priv->grid_gc) {
		if (gdk_color_parse(pcbhl_conf.appearance.color.grid.str, &priv->grid_color)) {
			priv->grid_color.red ^= priv->bg_color.red;
			priv->grid_color.green ^= priv->bg_color.green;
			priv->grid_color.blue ^= priv->bg_color.blue;
			gdk_color_alloc(colormap, &priv->grid_color);
		}
		priv->grid_gc = gdk_gc_new(priv->out_pixel);
		gdk_gc_set_function(priv->grid_gc, GDK_XOR);
		gdk_gc_set_foreground(priv->grid_gc, &priv->grid_color);
		gdk_gc_set_clip_origin(priv->grid_gc, 0, 0);
		set_clip(priv, priv->grid_gc);
	}

	if (pcb_conf_hid_gtk.plugins.hid_gtk.local_grid.enable) {
		ghid_gdk_draw_grid_local(hidlib, grid_local_old_x, grid_local_old_y);
		return;
	}

	ghid_gdk_draw_grid_global(hidlib);
}

/* ------------------------------------------------------------ */
static void ghid_gdk_draw_pixmap(pcb_hidlib_t *hidlib, pcb_gtk_pixmap_t *gpm, pcb_coord_t ox, pcb_coord_t oy, pcb_coord_t dw, pcb_coord_t dh)
{
	GdkInterpType interp_type;
	gint src_x, src_y, dst_x, dst_y, w, h, w_src, h_src;
	render_priv_t *priv = ghidgui->port.render_priv;

	src_x = 0;
	src_y = 0;
	dst_x = Vx(ox);
	dst_y = Vy(oy);

	w = dw / ghidgui->port.view.coord_per_px;
	h = dh / ghidgui->port.view.coord_per_px;

	if (gpm->w_scaled != w || gpm->h_scaled != h) {
		if (gpm->cache.pb != NULL)
			g_object_unref(G_OBJECT(gpm->cache.pb));

		w_src = gpm->pxm->sx;
		h_src = gpm->pxm->sy;
		if (w > w_src && h > h_src)
			interp_type = GDK_INTERP_NEAREST;
		else
			interp_type = GDK_INTERP_BILINEAR;

		gpm->cache.pb = gdk_pixbuf_scale_simple(gpm->image, w, h, interp_type);
		gpm->w_scaled = w;
		gpm->h_scaled = h;
	}

	if (gpm->cache.pb != NULL)
		gdk_pixbuf_render_to_drawable(gpm->cache.pb, priv->out_pixel, priv->bg_gc, src_x, src_y, dst_x, dst_y, w - src_x, h - src_y, GDK_RGB_DITHER_NORMAL, 0, 0);
}

static void ghid_gdk_draw_bg_image(pcb_hidlib_t *hidlib)
{
	if (ghidgui->bg_pixmap.image != NULL)
		ghid_gdk_draw_pixmap(hidlib, &ghidgui->bg_pixmap, 0, 0, hidlib->size_x, hidlib->size_y);
}


static pcb_composite_op_t curr_drawing_mode;

static void ghid_sketch_setup(render_priv_t *priv)
{
	if (priv->sketch_pixel == NULL)
		priv->sketch_pixel = gdk_pixmap_new(gtkc_widget_get_window(ghidgui->port.drawing_area), ghidgui->port.view.canvas_width, ghidgui->port.view.canvas_height, -1);
	if (priv->sketch_clip == NULL)
		priv->sketch_clip = gdk_pixmap_new(0, ghidgui->port.view.canvas_width, ghidgui->port.view.canvas_height, 1);

	priv->out_pixel = priv->sketch_pixel;
	priv->out_clip = priv->sketch_clip;
}

static void ghid_gdk_set_drawing_mode(pcb_hid_t *hid, pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	render_priv_t *priv = ghidgui->port.render_priv;

	if (!priv->base_pixel) {
		abort();
		return;
	}

	priv->direct = direct;

	if (direct) {
		priv->out_pixel = priv->base_pixel;
		priv->out_clip = NULL;
		curr_drawing_mode = PCB_HID_COMP_POSITIVE;
		return;
	}

	switch(op) {
		case PCB_HID_COMP_RESET:
			ghid_sketch_setup(priv);

		/* clear the canvas */
			priv->clip_color.pixel = 0;
			if (priv->clear_gc == NULL)
				priv->clear_gc = gdk_gc_new(priv->out_clip);
			gdk_gc_set_foreground(priv->clear_gc, &priv->clip_color);
			set_clip(priv, priv->clear_gc);
			gdk_draw_rectangle(priv->out_clip, priv->clear_gc, TRUE, 0, 0, ghidgui->port.view.canvas_width, ghidgui->port.view.canvas_height);
			break;

		case PCB_HID_COMP_POSITIVE:
		case PCB_HID_COMP_POSITIVE_XOR:
			priv->clip_color.pixel = 1;
			break;

		case PCB_HID_COMP_NEGATIVE:
			priv->clip_color.pixel = 0;
			break;

		case PCB_HID_COMP_FLUSH:
			if (priv->copy_gc == NULL)
				priv->copy_gc = gdk_gc_new(priv->out_pixel);
			gdk_gc_set_clip_mask(priv->copy_gc, priv->sketch_clip);
			gdk_gc_set_clip_origin(priv->copy_gc, 0, 0);
			gdk_draw_drawable(priv->base_pixel, priv->copy_gc, priv->sketch_pixel, 0, 0, 0, 0, ghidgui->port.view.canvas_width, ghidgui->port.view.canvas_height);

			priv->out_pixel = priv->base_pixel;
			priv->out_clip = NULL;
			break;
	}
	curr_drawing_mode = op;
}

static void ghid_gdk_render_burst(pcb_hid_t *hid, pcb_burst_op_t op, const pcb_box_t *screen)
{
	pcb_gui->coord_per_pix = ghidgui->port.view.coord_per_px;
}

typedef struct {
	int color_set;
	GdkColor color;
	int xor_set;
	GdkColor xor_color;
} pcb_gtk_color_cache_t;


/* Config helper functions for when the user changes color preferences.
   set_special colors used in the gtkhid. */
static void set_special_grid_color(void)
{
	render_priv_t *priv = ghidgui->port.render_priv;

	/* The color grid is combined with background color */
	map_color(&pcbhl_conf.appearance.color.grid, &priv->grid_color);
	priv->grid_color.red = (priv->grid_color.red ^ priv->bg_color.red) & 0xffff;
	priv->grid_color.green = (priv->grid_color.green ^ priv->bg_color.green) & 0xffff;
	priv->grid_color.blue = (priv->grid_color.blue ^ priv->bg_color.blue) & 0xffff;

	gdk_color_alloc(gtk_widget_get_colormap(ghidgui->port.top_window), &priv->grid_color);

	if (priv->grid_gc)
		gdk_gc_set_foreground(priv->grid_gc, &priv->grid_color);
}

static void ghid_gdk_set_special_colors(conf_native_t *cfg)
{
	render_priv_t *priv = ghidgui->port.render_priv;

	if (((CFT_COLOR *)cfg->val.color == &pcbhl_conf.appearance.color.background) && priv->bg_gc) {
		if (map_color(&cfg->val.color[0], &priv->bg_color)) {
			gdk_gc_set_foreground(priv->bg_gc, &priv->bg_color);
			set_special_grid_color();
		}
	}
	else if (((CFT_COLOR *)cfg->val.color == &pcbhl_conf.appearance.color.off_limit) && priv->offlimits_gc) {
		if (map_color(&cfg->val.color[0], &priv->offlimits_color))
			gdk_gc_set_foreground(priv->offlimits_gc, &priv->offlimits_color);
	}
	else if (((CFT_COLOR *)cfg->val.color == &pcbhl_conf.appearance.color.grid) && priv->grid_gc) {
		if (map_color(&cfg->val.color[0], &priv->grid_color))
			set_special_grid_color();
	}
}

static void ghid_gdk_set_color(pcb_hid_gc_t gc, const pcb_color_t *color)
{
	static GdkColormap *colormap = NULL;
	render_priv_t *priv = ghidgui->port.render_priv;

	if (*color->str == '\0') {
		fprintf(stderr, "ghid_gdk_set_color():  name = NULL, setting to magenta\n");
		color = pcb_color_magenta;
	}

	gc->pcolor = *color;

	if (!gc->pixel_gc)
		return;
	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(ghidgui->port.top_window);

	if (pcb_color_is_drill(color)) {
		gdk_gc_set_foreground(gc->pixel_gc, &priv->offlimits_color);
	}
	else {
		pcb_gtk_color_cache_t *cc;

		if (!priv->ccache_inited) {
			pcb_clrcache_init(&priv->ccache, sizeof(pcb_gtk_color_cache_t), NULL);
			priv->ccache_inited = 1;
		}
		cc = pcb_clrcache_get(&priv->ccache, color, 1);
		if (!cc->color_set) {
			map_color(color, &cc->color);
			cc->color_set = 1;
		}
		if (gc->xor_mask) {
			if (!cc->xor_set) {
				cc->xor_color.red = cc->color.red ^ priv->bg_color.red;
				cc->xor_color.green = cc->color.green ^ priv->bg_color.green;
				cc->xor_color.blue = cc->color.blue ^ priv->bg_color.blue;
				gdk_color_alloc(colormap, &cc->xor_color);
				cc->xor_set = 1;
			}
			gdk_gc_set_foreground(gc->pixel_gc, &cc->xor_color);
		}
		else {
			gdk_gc_set_foreground(gc->pixel_gc, &cc->color);
		}
	}
}

static void ghid_gdk_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	switch (style) {
	case pcb_cap_round:
		gc->cap = GDK_CAP_ROUND;
		gc->join = GDK_JOIN_ROUND;
		break;
	case pcb_cap_square:
		gc->cap = GDK_CAP_PROJECTING;
		gc->join = GDK_JOIN_MITER;
		break;
	default:
		assert(!"unhandled cap");
		gc->cap = GDK_CAP_ROUND;
		gc->join = GDK_JOIN_ROUND;
	}
	if (gc->pixel_gc)
		gdk_gc_set_line_attributes(gc->pixel_gc, Vz(gc->width), GDK_LINE_SOLID, (GdkCapStyle) gc->cap, (GdkJoinStyle) gc->join);
}

static void ghid_gdk_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	/* If width is negative then treat it as pixel width, otherwise it is world coordinates. */
	if(width < 0) {
		gc->width = -width;
		width = -width;
	}
	else {
		gc->width = width;
		width = Vz(width);
	}

	if (gc->pixel_gc)
		gdk_gc_set_line_attributes(gc->pixel_gc, width, GDK_LINE_SOLID, (GdkCapStyle) gc->cap, (GdkJoinStyle) gc->join);
	if (gc->clip_gc)
		gdk_gc_set_line_attributes(gc->clip_gc, width, GDK_LINE_SOLID, (GdkCapStyle) gc->cap, (GdkJoinStyle) gc->join);
}

static void ghid_gdk_set_draw_xor(pcb_hid_gc_t gc, int xor_mask)
{
	render_priv_t *priv = ghidgui->port.render_priv;

	gc->xor_mask = xor_mask;
	if (gc->pixel_gc != NULL)
		gdk_gc_set_function(gc->pixel_gc, xor_mask ? GDK_XOR : GDK_COPY);
	if (gc->clip_gc != NULL)
		gdk_gc_set_function(gc->clip_gc, xor_mask ? GDK_XOR : GDK_COPY);
	ghid_gdk_set_color(gc, &gc->pcolor);

	/* If not in direct mode then select the correct drawables so that the sketch and clip
	 * drawables are not drawn to in XOR mode */
	if (!priv->direct) {
		/* If xor mode then draw directly to the base drawable */
		if (xor_mask) {
			priv->out_pixel = priv->base_pixel;
			priv->out_clip = NULL;
		}
		else /* If not in direct mode then draw to the sketch and clip drawables */
			ghid_sketch_setup(priv);
	}
}

static int use_gc(pcb_hid_gc_t gc, int need_pen)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	GdkWindow *window = gtkc_widget_get_window(ghidgui->port.top_window);
	int need_setup = 0;

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	if (gc->me_pointer != &gtk2_gdk_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to GTK HID\n");
		abort();
	}

	if (!priv->base_pixel)
		return 0;

	if ((!gc->clip_gc) && (priv->out_clip != NULL)) {
		gc->clip_gc = gdk_gc_new(priv->out_clip);
		need_setup = 1;
	}

	if (!gc->pixel_gc) {
		gc->pixel_gc = gdk_gc_new(window);
		need_setup = 1;
	}

	if (need_setup) {
		ghid_gdk_set_color(gc, &gc->pcolor);
		ghid_gdk_set_line_width(gc, gc->core_gc.width);
		if ((need_pen) || (gc->core_gc.cap != pcb_cap_invalid))
			ghid_gdk_set_line_cap(gc, (pcb_cap_style_t) gc->core_gc.cap);
		ghid_gdk_set_draw_xor(gc, gc->xor_mask);
		gdk_gc_set_clip_origin(gc->pixel_gc, 0, 0);
	}

	if (priv->out_clip != NULL)
		gdk_gc_set_foreground(gc->clip_gc, &priv->clip_color);

	priv->pixel_gc = gc->pixel_gc;
	priv->clip_gc = gc->clip_gc;
	return 1;
}

static void ghid_gdk_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double dx1, dy1, dx2, dy2;
	render_priv_t *priv = ghidgui->port.render_priv;

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	dx1 = Vx((double) x1);
	dy1 = Vy((double) y1);

	/* optimization: draw a single dot if object is too small */
	if ((gc->core_gc.width > 0) && (pcb_gtk_1dot(gc->width, x1, y1, x2, y2))) {
		if (pcb_gtk_dot_in_canvas(gc->width, dx1, dy1)) {
			USE_GC(gc);
			gdk_draw_point(priv->out_pixel, priv->pixel_gc, dx1, dy1);
			if (priv->out_clip != NULL)
				gdk_draw_point(priv->out_clip, priv->clip_gc, dx1, dy1);
		}
		return;
	}

	dx2 = Vx((double) x2);
	dy2 = Vy((double) y2);

	if (!pcb_line_clip(0, 0, ghidgui->port.view.canvas_width, ghidgui->port.view.canvas_height, &dx1, &dy1, &dx2, &dy2, gc->width / ghidgui->port.view.coord_per_px))
		return;

	USE_GC(gc);
	gdk_draw_line(priv->out_pixel, priv->pixel_gc, dx1, dy1, dx2, dy2);

	if (priv->out_clip != NULL)
		gdk_draw_line(priv->out_clip, priv->clip_gc, dx1, dy1, dx2, dy2);
}

static void ghid_gdk_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	gint vrx2, vry2;
	double w, h, radius;
	render_priv_t *priv = ghidgui->port.render_priv;

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	w = ghidgui->port.view.canvas_width * ghidgui->port.view.coord_per_px;
	h = ghidgui->port.view.canvas_height * ghidgui->port.view.coord_per_px;
	radius = (xradius > yradius) ? xradius : yradius;
	if (SIDE_X(&ghidgui->port.view, cx) < ghidgui->port.view.x0 - radius
			|| SIDE_X(&ghidgui->port.view, cx) > ghidgui->port.view.x0 + w + radius
			|| SIDE_Y(&ghidgui->port.view, cy) < ghidgui->port.view.y0 - radius
			|| SIDE_Y(&ghidgui->port.view, cy) > ghidgui->port.view.y0 + h + radius)
		return;

	USE_GC(gc);
	vrx2 = Vz(xradius*2.0);
	vry2 = Vz(yradius*2.0);

	if ((delta_angle > 360.0) || (delta_angle < -360.0)) {
		start_angle = 0;
		delta_angle = 360;
	}

	if (pcbhl_conf.editor.view.flip_x) {
		start_angle = 180 - start_angle;
		delta_angle = -delta_angle;
	}
	if (pcbhl_conf.editor.view.flip_y) {
		start_angle = -start_angle;
		delta_angle = -delta_angle;
	}
	/* make sure we fall in the -180 to +180 range */
	start_angle = pcb_normalize_angle(start_angle);
	if (start_angle >= 180)
		start_angle -= 360;

	gdk_draw_arc(priv->out_pixel, priv->pixel_gc, 0,
							 pcb_round(Vxd(cx) - Vzd(xradius) + 0.5), pcb_round(Vyd(cy) - Vzd(yradius) + 0.5),
							 pcb_round(vrx2), pcb_round(vry2),
							 (start_angle + 180) * 64, delta_angle * 64);

	if (priv->out_clip != NULL)
		gdk_draw_arc(priv->out_clip, priv->clip_gc, 0,
							 pcb_round(Vxd(cx) - Vzd(xradius) + 0.5), pcb_round(Vyd(cy) - Vzd(yradius) + 0.5),
							 pcb_round(vrx2), pcb_round(vry2),
							 (start_angle + 180) * 64, delta_angle * 64);
}

static void ghid_gdk_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gint w, h, lw, sx1, sy1;
	render_priv_t *priv = ghidgui->port.render_priv;

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	lw = gc->width;
	w = ghidgui->port.view.canvas_width * ghidgui->port.view.coord_per_px;
	h = ghidgui->port.view.canvas_height * ghidgui->port.view.coord_per_px;

	if ((SIDE_X(&ghidgui->port.view, x1) < ghidgui->port.view.x0 - lw && SIDE_X(&ghidgui->port.view, x2) < ghidgui->port.view.x0 - lw)
			|| (SIDE_X(&ghidgui->port.view, x1) > ghidgui->port.view.x0 + w + lw && SIDE_X(&ghidgui->port.view, x2) > ghidgui->port.view.x0 + w + lw)
			|| (SIDE_Y(&ghidgui->port.view, y1) < ghidgui->port.view.y0 - lw && SIDE_Y(&ghidgui->port.view, y2) < ghidgui->port.view.y0 - lw)
			|| (SIDE_Y(&ghidgui->port.view, y1) > ghidgui->port.view.y0 + h + lw && SIDE_Y(&ghidgui->port.view, y2) > ghidgui->port.view.y0 + h + lw))
		return;

	sx1 = Vx(x1);
	sy1 = Vy(y1);

	/* optimization: draw a single dot if object is too small */
	if (pcb_gtk_1dot(gc->width, x1, y1, x2, y2)) {
		if (pcb_gtk_dot_in_canvas(gc->width, sx1, sy1)) {
			USE_GC(gc);
			gdk_draw_point(priv->out_pixel, priv->pixel_gc, sx1, sy1);
		}
		return;
	}

	x1 = sx1;
	y1 = sy1;
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
	gdk_draw_rectangle(priv->out_pixel, priv->pixel_gc, FALSE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);

	if (priv->out_clip != NULL)
		gdk_draw_rectangle(priv->out_clip, priv->clip_gc, FALSE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}


static void ghid_gdk_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	gint w, h, vr;
	render_priv_t *priv = ghidgui->port.render_priv;

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	w = ghidgui->port.view.canvas_width * ghidgui->port.view.coord_per_px;
	h = ghidgui->port.view.canvas_height * ghidgui->port.view.coord_per_px;
	if (SIDE_X(&ghidgui->port.view, cx) < ghidgui->port.view.x0 - radius
			|| SIDE_X(&ghidgui->port.view, cx) > ghidgui->port.view.x0 + w + radius
			|| SIDE_Y(&ghidgui->port.view, cy) < ghidgui->port.view.y0 - radius
			|| SIDE_Y(&ghidgui->port.view, cy) > ghidgui->port.view.y0 + h + radius)
		return;

	USE_GC(gc);

	/* optimization: draw a single dot if object is too small */
	if (pcb_gtk_1dot(radius*2, cx, cy, cx, cy)) {
		pcb_coord_t sx1 = Vx(cx), sy1 = Vy(cy);
		if (pcb_gtk_dot_in_canvas(radius*2, sx1, sy1)) {
			USE_GC(gc);
			gdk_draw_point(priv->out_pixel, priv->pixel_gc, sx1, sy1);
		}
		return;
	}

	vr = Vz(radius);
	gdk_draw_arc(priv->out_pixel, priv->pixel_gc, TRUE, Vx(cx) - vr, Vy(cy) - vr, vr * 2, vr * 2, 0, 360 * 64);

	if (priv->out_clip != NULL)
		gdk_draw_arc(priv->out_clip, priv->clip_gc, TRUE, Vx(cx) - vr, Vy(cy) - vr, vr * 2, vr * 2, 0, 360 * 64);
}

/* Decide if a polygon is an axis aligned rectangle; if so, return non-zero
   and save the corners in b */
static int poly_is_aligned_rect(pcb_box_t *b, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	int n, xi1 = 0, yi1 = 0, xi2 = 0, yi2 = 0, xs1 = 0, ys1 = 0, xs2 = 0, ys2 = 0;
	if (n_coords != 4)
		return 0;
	b->X1 = b->Y1 = PCB_MAX_COORD;
	b->X2 = b->Y2 = -PCB_MAX_COORD;
	for(n = 0; n < 4; n++) {
		if (x[n] == b->X1)
			xs1++;
		else if (x[n] < b->X1) {
			b->X1 = x[n];
			xi1++;
		}
		else if (x[n] == b->X2)
			xs2++;
		else if (x[n] > b->X2) {
			b->X2 = x[n];
			xi2++;
		}
		else
			return 0;
		if (y[n] == b->Y1)
			ys1++;
		else if (y[n] < b->Y1) {
			b->Y1 = y[n];
			yi1++;
		}
		else if (y[n] == b->Y2)
			ys2++;
		else if (y[n] > b->Y2) {
			b->Y2 = y[n];
			yi2++;
		}
		else
			return 0;
	}
	return (xi1 == 1) && (yi1 == 1) && (xi2 == 1) && (yi2 == 1) && \
	       (xs1 == 1) && (ys1 == 1) && (xs2 == 1) && (ys2 == 1);
}

/* Intentional code duplication for performance */
static void ghid_gdk_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	pcb_box_t b;
	static GdkPoint *points = 0;
	static int npoints = 0;
	int i, len, sup = 0;
	pcb_coord_t lsx, lsy, lastx = PCB_MAX_COORD, lasty = PCB_MAX_COORD, mindist = ghidgui->port.view.coord_per_px * 2;

	render_priv_t *priv = ghidgui->port.render_priv;
	USE_GC_NOPEN(gc);

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	/* optimization: axis aligned rectangles can be drawn cheaper than polygons and they are common because of smd pads */
	if (poly_is_aligned_rect(&b, n_coords, x, y)) {
		gint x1 = Vx(b.X1), y1 = Vy(b.Y1), x2 = Vx(b.X2), y2 = Vy(b.Y2), tmp;

		if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
		if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
		gdk_draw_rectangle(priv->out_pixel, priv->pixel_gc, TRUE, x1, y1, x2 - x1, y2 - y1);
		if (priv->out_clip != NULL)
			gdk_draw_rectangle(priv->out_clip, priv->clip_gc, TRUE, x1, y1, x2 - x1, y2 - y1);
		return;
	}

	if (npoints < n_coords) {
		npoints = n_coords + 1;
		points = (GdkPoint *) realloc(points, npoints * sizeof(GdkPoint));
	}
	for (len = i = 0; i < n_coords; i++) {
		if ((i != n_coords-1) && (PCB_ABS(x[i] - lastx) < mindist) && (PCB_ABS(y[i] - lasty) < mindist)) {
			lsx = x[i];
			lsy = y[i];
			sup = 1;
			continue;
		}
		if (sup) { /* before a big jump, make sure to use the accurate coords of the last (suppressed) point of the crowd */
			points[len].x = Vx(lsx);
			points[len].y = Vy(lsy);
			len++;
			sup = 0;
		}
		points[len].x = Vx(x[i]);
		points[len].y = Vy(y[i]);
		len++;
		lastx = x[i];
		lasty = y[i];
	}
	if (len < 3) {
		gdk_draw_point(priv->out_pixel, priv->pixel_gc, points[0].x, points[0].y);
		if (priv->out_clip != NULL)
			gdk_draw_point(priv->out_clip, priv->pixel_gc, points[0].x, points[0].y);
		return;
	}
	gdk_draw_polygon(priv->out_pixel, priv->pixel_gc, 1, points, len);
	if (priv->out_clip != NULL)
		gdk_draw_polygon(priv->out_clip, priv->clip_gc, 1, points, len);
}

/* Intentional code duplication for performance */
static void ghid_gdk_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_box_t b;
	static GdkPoint *points = 0;
	static int npoints = 0;
	int i, len, sup = 0;
	render_priv_t *priv = ghidgui->port.render_priv;
	pcb_coord_t lsx, lsy, lastx = PCB_MAX_COORD, lasty = PCB_MAX_COORD, mindist = ghidgui->port.view.coord_per_px * 2;
	USE_GC_NOPEN(gc);

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	/* optimization: axis aligned rectangles can be drawn cheaper than polygons and they are common because of smd pads */
	if (poly_is_aligned_rect(&b, n_coords, x, y)) {
		gint x1 = Vx(b.X1+dx), y1 = Vy(b.Y1+dy), x2 = Vx(b.X2+dx), y2 = Vy(b.Y2+dy), tmp;

		if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
		if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
		gdk_draw_rectangle(priv->out_pixel, priv->pixel_gc, TRUE, x1, y1, x2 - x1, y2 - y1);
		if (priv->out_clip != NULL)
			gdk_draw_rectangle(priv->out_clip, priv->clip_gc, TRUE, x1, y1, x2 - x1, y2 - y1);
		return;
	}

	if (npoints < n_coords) {
		npoints = n_coords + 1;
		points = (GdkPoint *) realloc(points, npoints * sizeof(GdkPoint));
	}
	for (len = i = 0; i < n_coords; i++) {
		if ((i != n_coords-1) && (PCB_ABS(x[i] - lastx) < mindist) && (PCB_ABS(y[i] - lasty) < mindist)) {
			lsx = x[i];
			lsy = y[i];
			sup = 1;
			continue;
		}
		if (sup) { /* before a big jump, make sure to use the accurate coords of the last (suppressed) point of the crowd */
			points[len].x = Vx(lsx+dx);
			points[len].y = Vy(lsy+dy);
			len++;
			sup = 0;
		}
		points[len].x = Vx(x[i]+dx);
		points[len].y = Vy(y[i]+dy);
		len++;
		lastx = x[i];
		lasty = y[i];
	}
	if (len < 3) {
		gdk_draw_point(priv->out_pixel, priv->pixel_gc, points[0].x, points[0].y);
		if (priv->out_clip != NULL)
			gdk_draw_point(priv->out_clip, priv->pixel_gc, points[0].x, points[0].y);
		return;
	}
	gdk_draw_polygon(priv->out_pixel, priv->pixel_gc, 1, points, len);
	if (priv->out_clip != NULL)
		gdk_draw_polygon(priv->out_clip, priv->clip_gc, 1, points, len);
}

static void ghid_gdk_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gint w, h, lw, xx, yy, sx1, sy1;
	render_priv_t *priv = ghidgui->port.render_priv;

	assert((curr_drawing_mode == PCB_HID_COMP_POSITIVE) || (curr_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (curr_drawing_mode == PCB_HID_COMP_NEGATIVE));

	lw = gc->width;
	w = ghidgui->port.view.canvas_width * ghidgui->port.view.coord_per_px;
	h = ghidgui->port.view.canvas_height * ghidgui->port.view.coord_per_px;

	if ((SIDE_X(&ghidgui->port.view, x1) < ghidgui->port.view.x0 - lw && SIDE_X(&ghidgui->port.view, x2) < ghidgui->port.view.x0 - lw)
			|| (SIDE_X(&ghidgui->port.view, x1) > ghidgui->port.view.x0 + w + lw && SIDE_X(&ghidgui->port.view, x2) > ghidgui->port.view.x0 + w + lw)
			|| (SIDE_Y(&ghidgui->port.view, y1) < ghidgui->port.view.y0 - lw && SIDE_Y(&ghidgui->port.view, y2) < ghidgui->port.view.y0 - lw)
			|| (SIDE_Y(&ghidgui->port.view, y1) > ghidgui->port.view.y0 + h + lw && SIDE_Y(&ghidgui->port.view, y2) > ghidgui->port.view.y0 + h + lw))
		return;

	sx1 = Vx(x1);
	sy1 = Vy(y1);

	/* optimization: draw a single dot if object is too small */
	if (pcb_gtk_1dot(gc->width, x1, y1, x2, y2)) {
		if (pcb_gtk_dot_in_canvas(gc->width, sx1, sy1)) {
			USE_GC_NOPEN(gc);
			gdk_draw_point(priv->out_pixel, priv->pixel_gc, sx1, sy1);
		}
		return;
	}

	x1 = sx1;
	y1 = sy1;
	x2 = Vx(x2);
	y2 = Vy(y2);
	if (x2 < x1) {
		xx = x1;
		x1 = x2;
		x2 = xx;
	}
	if (y2 < y1) {
		yy = y1;
		y1 = y2;
		y2 = yy;
	}
	USE_GC_NOPEN(gc);
	gdk_draw_rectangle(priv->out_pixel, priv->pixel_gc, TRUE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);

	if (priv->out_clip != NULL)
		gdk_draw_rectangle(priv->out_clip, priv->clip_gc, TRUE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

static void redraw_region(pcb_hidlib_t *hidlib, GdkRectangle *rect)
{
	int eleft, eright, etop, ebottom;
	pcb_hid_expose_ctx_t ctx;
	render_priv_t *priv = ghidgui->port.render_priv;

	if (!priv->base_pixel)
		return;

	if (rect != NULL) {
		priv->clip_rect = *rect;
		priv->clip_rect_valid = pcb_true;
	}
	else {
		priv->clip_rect.x = 0;
		priv->clip_rect.y = 0;
		priv->clip_rect.width = ghidgui->port.view.canvas_width;
		priv->clip_rect.height = ghidgui->port.view.canvas_height;
		priv->clip_rect_valid = pcb_false;
	}

	set_clip(priv, priv->bg_gc);
	set_clip(priv, priv->offlimits_gc);
	set_clip(priv, priv->grid_gc);

	ctx.view.X1 = MIN(Px(priv->clip_rect.x), Px(priv->clip_rect.x + priv->clip_rect.width + 1));
	ctx.view.Y1 = MIN(Py(priv->clip_rect.y), Py(priv->clip_rect.y + priv->clip_rect.height + 1));
	ctx.view.X2 = MAX(Px(priv->clip_rect.x), Px(priv->clip_rect.x + priv->clip_rect.width + 1));
	ctx.view.Y2 = MAX(Py(priv->clip_rect.y), Py(priv->clip_rect.y + priv->clip_rect.height + 1));

	ctx.view.X1 = ctx.view.X1;
	ctx.view.X2 = ctx.view.X2;
	ctx.view.Y1 = ctx.view.Y1;
	ctx.view.Y2 = ctx.view.Y2;

	eleft = Vx(0);
	eright = Vx(hidlib->size_x);
	etop = Vy(0);
	ebottom = Vy(hidlib->size_y);
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

	if (eleft > 0)
		gdk_draw_rectangle(priv->out_pixel, priv->offlimits_gc, 1, 0, 0, eleft, ghidgui->port.view.canvas_height);
	else
		eleft = 0;
	if (eright < ghidgui->port.view.canvas_width)
		gdk_draw_rectangle(priv->out_pixel, priv->offlimits_gc, 1, eright, 0, ghidgui->port.view.canvas_width - eright, ghidgui->port.view.canvas_height);
	else
		eright = ghidgui->port.view.canvas_width;
	if (etop > 0)
		gdk_draw_rectangle(priv->out_pixel, priv->offlimits_gc, 1, eleft, 0, eright - eleft + 1, etop);
	else
		etop = 0;
	if (ebottom < ghidgui->port.view.canvas_height)
		gdk_draw_rectangle(priv->out_pixel, priv->offlimits_gc, 1, eleft, ebottom, eright - eleft + 1, ghidgui->port.view.canvas_height - ebottom);
	else
		ebottom = ghidgui->port.view.canvas_height;

	gdk_draw_rectangle(priv->out_pixel, priv->bg_gc, 1, eleft, etop, eright - eleft + 1, ebottom - etop + 1);

	ghid_gdk_draw_bg_image(hidlib);

	pcbhl_expose_main(&gtk2_gdk_hid, &ctx, NULL);
	ghid_gdk_draw_grid(hidlib);

	/* In some cases we are called with the crosshair still off */
	if (priv->attached_invalidate_depth == 0)
		pcbhl_draw_attached(hidlib, 0);

	/* In some cases we are called with the mark still off */
	if (priv->mark_invalidate_depth == 0)
		pcbhl_draw_marks(hidlib, 0);

	priv->clip_rect_valid = pcb_false;

	/* Rest the clip for bg_gc, as it is used outside this function */
	gdk_gc_set_clip_mask(priv->bg_gc, NULL);
}

static int preview_lock = 0;
static void ghid_gdk_invalidate_lr(pcb_hid_t *hid, pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
{
	pcb_hidlib_t *hidlib = ghidgui->hidlib;
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

	redraw_region(hidlib, &rect);
	if (!preview_lock) {
		preview_lock++;
		pcb_gtk_previews_invalidate_lr(minx, maxx, miny, maxy);
		preview_lock--;
	}

	ghid_gdk_screen_update();
}


static void ghid_gdk_invalidate_all(pcb_hid_t *hid)
{
	pcb_hidlib_t *hidlib = ghidgui->hidlib;
	if (ghidgui && ghidgui->topwin.menu.menu_bar) {
		redraw_region(hidlib, NULL);
		if (!preview_lock) {
			preview_lock++;
			pcb_gtk_previews_invalidate_all();
			preview_lock--;
		}
		ghid_gdk_screen_update();
	}
}

static void ghid_gdk_notify_crosshair_change(pcb_hid_t *hid, pcb_bool changes_complete)
{
	pcb_hidlib_t *hidlib = ghidgui->hidlib;
	render_priv_t *priv = ghidgui->port.render_priv;

	/* We sometimes get called before the GUI is up */
	if (ghidgui->port.drawing_area == NULL)
		return;

	if (changes_complete)
		priv->attached_invalidate_depth--;

	assert(priv->attached_invalidate_depth >= 0);
	if (priv->attached_invalidate_depth < 0) {
		priv->attached_invalidate_depth = 0;
		/* A mismatch of changes_complete == pcb_false and == pcb_true notifications
		   is not expected to occur, but we will try to handle it gracefully.
		   As we know the crosshair will have been shown already, we must
		   repaint the entire view to be sure not to leave an artaefact. */
		ghid_gdk_invalidate_all(pcb_gui);
		return;
	}

	if (priv->attached_invalidate_depth == 0)
		pcbhl_draw_attached(hidlib, 0);

	if (!changes_complete) {
		priv->attached_invalidate_depth++;
	}
	else if (ghidgui->port.drawing_area != NULL) {
		/* Queue a GTK expose when changes are complete */
		ghid_draw_area_update(&ghidgui->port, NULL);
	}
}

static void ghid_gdk_notify_mark_change(pcb_hid_t *hid, pcb_bool changes_complete)
{
	pcb_hidlib_t *hidlib = ghidgui->hidlib;
	render_priv_t *priv = ghidgui->port.render_priv;

	/* We sometimes get called before the GUI is up */
	if (ghidgui->port.drawing_area == NULL)
		return;

	if (changes_complete)
		priv->mark_invalidate_depth--;

	if (priv->mark_invalidate_depth < 0) {
		priv->mark_invalidate_depth = 0;
		/* A mismatch of changes_complete == pcb_false and == pcb_true notifications
		   is not expected to occur, but we will try to handle it gracefully.
		   As we know the mark will have been shown already, we must
		   repaint the entire view to be sure not to leave an artaefact. */
		ghid_gdk_invalidate_all(pcb_gui);
		return;
	}

	if (priv->mark_invalidate_depth == 0)
		pcbhl_draw_marks(hidlib, 0);

	if (!changes_complete) {
		priv->mark_invalidate_depth++;
	}
	else if (ghidgui->port.drawing_area != NULL) {
		/* Queue a GTK expose when changes are complete */
		ghid_draw_area_update(&ghidgui->port, NULL);
	}
}

static void draw_right_cross(GdkGC *xor_gc, gint x, gint y)
{
	GdkWindow *window = gtkc_widget_get_window(ghidgui->port.drawing_area);

	gdk_draw_line(window, xor_gc, x, 0, x, ghidgui->port.view.canvas_height);
	gdk_draw_line(window, xor_gc, 0, y, ghidgui->port.view.canvas_width, y);
}

static void draw_slanted_cross(GdkGC *xor_gc, gint x, gint y)
{
	GdkWindow *window = gtkc_widget_get_window(ghidgui->port.drawing_area);
	gint x0, y0, x1, y1;

	x0 = x + (ghidgui->port.view.canvas_height - y);
	x0 = MAX(0, MIN(x0, ghidgui->port.view.canvas_width));
	x1 = x - y;
	x1 = MAX(0, MIN(x1, ghidgui->port.view.canvas_width));
	y0 = y + (ghidgui->port.view.canvas_width - x);
	y0 = MAX(0, MIN(y0, ghidgui->port.view.canvas_height));
	y1 = y - x;
	y1 = MAX(0, MIN(y1, ghidgui->port.view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (ghidgui->port.view.canvas_height - y);
	x0 = MAX(0, MIN(x0, ghidgui->port.view.canvas_width));
	x1 = x + y;
	x1 = MAX(0, MIN(x1, ghidgui->port.view.canvas_width));
	y0 = y + x;
	y0 = MAX(0, MIN(y0, ghidgui->port.view.canvas_height));
	y1 = y - (ghidgui->port.view.canvas_width - x);
	y1 = MAX(0, MIN(y1, ghidgui->port.view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);
}

static void draw_dozen_cross(GdkGC *xor_gc, gint x, gint y)
{
	GdkWindow *window = gtkc_widget_get_window(ghidgui->port.drawing_area);
	gint x0, y0, x1, y1;
	gdouble tan60 = sqrt(3);

	x0 = x + (ghidgui->port.view.canvas_height - y) / tan60;
	x0 = MAX(0, MIN(x0, ghidgui->port.view.canvas_width));
	x1 = x - y / tan60;
	x1 = MAX(0, MIN(x1, ghidgui->port.view.canvas_width));
	y0 = y + (ghidgui->port.view.canvas_width - x) * tan60;
	y0 = MAX(0, MIN(y0, ghidgui->port.view.canvas_height));
	y1 = y - x * tan60;
	y1 = MAX(0, MIN(y1, ghidgui->port.view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x + (ghidgui->port.view.canvas_height - y) * tan60;
	x0 = MAX(0, MIN(x0, ghidgui->port.view.canvas_width));
	x1 = x - y * tan60;
	x1 = MAX(0, MIN(x1, ghidgui->port.view.canvas_width));
	y0 = y + (ghidgui->port.view.canvas_width - x) / tan60;
	y0 = MAX(0, MIN(y0, ghidgui->port.view.canvas_height));
	y1 = y - x / tan60;
	y1 = MAX(0, MIN(y1, ghidgui->port.view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (ghidgui->port.view.canvas_height - y) / tan60;
	x0 = MAX(0, MIN(x0, ghidgui->port.view.canvas_width));
	x1 = x + y / tan60;
	x1 = MAX(0, MIN(x1, ghidgui->port.view.canvas_width));
	y0 = y + x * tan60;
	y0 = MAX(0, MIN(y0, ghidgui->port.view.canvas_height));
	y1 = y - (ghidgui->port.view.canvas_width - x) * tan60;
	y1 = MAX(0, MIN(y1, ghidgui->port.view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (ghidgui->port.view.canvas_height - y) * tan60;
	x0 = MAX(0, MIN(x0, ghidgui->port.view.canvas_width));
	x1 = x + y * tan60;
	x1 = MAX(0, MIN(x1, ghidgui->port.view.canvas_width));
	y0 = y + x / tan60;
	y0 = MAX(0, MIN(y0, ghidgui->port.view.canvas_height));
	y1 = y - (ghidgui->port.view.canvas_width - x) / tan60;
	y1 = MAX(0, MIN(y1, ghidgui->port.view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);
}

static void draw_crosshair(GdkGC *xor_gc, gint x, gint y)
{
	static enum pcb_crosshair_shape_e prev = pcb_ch_shape_basic;

	draw_right_cross(xor_gc, x, y);
	if (prev == pcb_ch_shape_union_jack)
		draw_slanted_cross(xor_gc, x, y);
	if (prev == pcb_ch_shape_dozen)
		draw_dozen_cross(xor_gc, x, y);
	prev = pcbhl_conf.editor.crosshair_shape_idx;
}

static void show_crosshair(gboolean paint_new_location)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	GdkWindow *window = gtkc_widget_get_window(ghidgui->port.drawing_area);
	GtkStyle *style = gtk_widget_get_style(ghidgui->port.drawing_area);
	gint x, y;
	static gint x_prev = -1, y_prev = -1, prev_valid = 0;
	static GdkGC *xor_gc;
	static GdkColor cross_color;
	static unsigned long cross_color_packed;

	if (!ghidgui->topwin.active || !ghidgui->port.view.has_entered) {
		prev_valid = 0; /* if leaving the drawing area, invalidate last known coord to make sure we redraw on reenter, even on the same coords */
		return;
	}

	if (!xor_gc || (cross_color_packed != pcbhl_conf.appearance.color.cross.packed)) {
		xor_gc = gdk_gc_new(window);
		gdk_gc_copy(xor_gc, style->white_gc);
		gdk_gc_set_function(xor_gc, GDK_XOR);
		gdk_gc_set_clip_origin(xor_gc, 0, 0);
		set_clip(priv, xor_gc);
		map_color(&pcbhl_conf.appearance.color.cross, &cross_color);
		cross_color_packed = pcbhl_conf.appearance.color.cross.packed;
	}
	x = Vx(ghidgui->port.view.crosshair_x);
	y = Vy(ghidgui->port.view.crosshair_y);

	gdk_gc_set_foreground(xor_gc, &cross_color);

	if (prev_valid && !paint_new_location)
		draw_crosshair(xor_gc, x_prev, y_prev);

	if (paint_new_location) {
		draw_crosshair(xor_gc, x, y);
		x_prev = x;
		y_prev = y;
		prev_valid = 1;
	}
	else
		prev_valid = 0;
}

static void ghid_gdk_init_renderer(int *argc, char ***argv, void *vport)
{
	pcb_gtk_port_t *port = vport;
	/* Init any GC's required */
	port->render_priv = g_new0(render_priv_t, 1);
}

static void ghid_gdk_shutdown_renderer(void *vport)
{
	pcb_gtk_port_t *port = vport;
	g_free(port->render_priv);
	port->render_priv = NULL;
}

static void ghid_gdk_init_drawing_widget(GtkWidget *widget, void *port)
{
}

static void ghid_gdk_drawing_area_configure_hook(void *vport)
{
	pcb_gtk_port_t *port = vport;
	static int done_once = 0;
	render_priv_t *priv = port->render_priv;

	if (priv->base_pixel)
		gdk_pixmap_unref(priv->base_pixel);

	priv->base_pixel = gdk_pixmap_new(gtkc_widget_get_window(ghidgui->port.drawing_area), ghidgui->port.view.canvas_width, ghidgui->port.view.canvas_height, -1);
	priv->out_pixel = priv->base_pixel;
	ghidgui->port.drawing_allowed = pcb_true;

	if (!done_once) {
		priv->bg_gc = gdk_gc_new(priv->out_pixel);
		if (!map_color(&pcbhl_conf.appearance.color.background, &priv->bg_color))
			map_color(pcb_color_white, &priv->bg_color);
		gdk_gc_set_foreground(priv->bg_gc, &priv->bg_color);
		gdk_gc_set_clip_origin(priv->bg_gc, 0, 0);

		priv->offlimits_gc = gdk_gc_new(priv->out_pixel);
		if (!map_color(&pcbhl_conf.appearance.color.off_limit, &priv->offlimits_color))
			map_color(pcb_color_white, &priv->offlimits_color);
		gdk_gc_set_foreground(priv->offlimits_gc, &priv->offlimits_color);
		gdk_gc_set_clip_origin(priv->offlimits_gc, 0, 0);
		done_once = 1;
	}

	if (priv->sketch_pixel) {
		gdk_pixmap_unref(priv->sketch_pixel);
		priv->sketch_pixel = gdk_pixmap_new(gtkc_widget_get_window(ghidgui->port.drawing_area), port->view.canvas_width, port->view.canvas_height, -1);
	}
	if (priv->sketch_clip) {
		gdk_pixmap_unref(priv->sketch_clip);
		priv->sketch_clip = gdk_pixmap_new(0, port->view.canvas_width, port->view.canvas_height, 1);
	}
}

static void ghid_gdk_screen_update(void)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	GdkWindow *window = gtkc_widget_get_window(ghidgui->port.drawing_area);

	if (priv->base_pixel == NULL)
		return;

	gdk_draw_drawable(window, priv->bg_gc, priv->base_pixel, 0, 0, 0, 0, ghidgui->port.view.canvas_width, ghidgui->port.view.canvas_height);
	show_crosshair(TRUE);
}

static gboolean ghid_gdk_drawing_area_expose_cb(GtkWidget *widget, pcb_gtk_expose_t *ev, void *vport)
{
	pcb_gtk_port_t *port = vport;
	render_priv_t *priv = port->render_priv;
	GdkWindow *window = gtkc_widget_get_window(ghidgui->port.drawing_area);

	gdk_draw_drawable(window, priv->bg_gc, priv->base_pixel,
		ev->area.x, ev->area.y, ev->area.x, ev->area.y, ev->area.width, ev->area.height);
	show_crosshair(TRUE);
	return FALSE;
}

static void ghid_gdk_port_drawing_realize_cb(GtkWidget *widget, gpointer data)
{
}

static gboolean ghid_gdk_preview_expose(GtkWidget *widget, pcb_gtk_expose_t *ev, pcb_hid_expose_t expcall, pcb_hid_expose_ctx_t *ctx)
{
	GdkWindow *window = gtkc_widget_get_window(widget);
	GdkDrawable *save_drawable;
	GtkAllocation allocation;
	pcb_gtk_view_t save_view;
	int save_width, save_height;
	double xz, yz, vw, vh;
	render_priv_t *priv = ghidgui->port.render_priv;
	pcb_coord_t ox1 = ctx->view.X1, oy1 = ctx->view.Y1, ox2 = ctx->view.X2, oy2 = ctx->view.Y2;
	pcb_coord_t save_cpp;

	vw = ctx->view.X2 - ctx->view.X1;
	vh = ctx->view.Y2 - ctx->view.Y1;

	/* Setup drawable and zoom factor for drawing routines */
	save_drawable = priv->base_pixel;
	save_view = ghidgui->port.view;
	save_width = ghidgui->port.view.canvas_width;
	save_height = ghidgui->port.view.canvas_height;
	save_cpp = pcb_gui->coord_per_pix;

	gtkc_widget_get_allocation(widget, &allocation);
	xz = vw / (double)allocation.width;
	yz = vh / (double)allocation.height;
	if (xz > yz)
		ghidgui->port.view.coord_per_px = xz;
	else
		ghidgui->port.view.coord_per_px = yz;

	priv->base_pixel = priv->out_pixel = window;
	ghidgui->port.view.canvas_width = allocation.width;
	ghidgui->port.view.canvas_height = allocation.height;
	ghidgui->port.view.width = allocation.width * ghidgui->port.view.coord_per_px;
	ghidgui->port.view.height = allocation.height * ghidgui->port.view.coord_per_px;
	ghidgui->port.view.x0 = (vw - ghidgui->port.view.width) / 2 + ctx->view.X1;
	ghidgui->port.view.y0 = (vh - ghidgui->port.view.height) / 2 + ctx->view.Y1;

	/* clear background */
	gdk_draw_rectangle(window, priv->bg_gc, TRUE, 0, 0, allocation.width, allocation.height);

	PCB_GTK_PREVIEW_TUNE_EXTENT(ctx, allocation);

	/* call the drawing routine */
	pcb_gui->coord_per_pix = ghidgui->port.view.coord_per_px;
	expcall(&gtk2_gdk_hid, ctx);

	/* restore the original context and priv */
	ctx->view.X1 = ox1; ctx->view.X2 = ox2; ctx->view.Y1 = oy1; ctx->view.Y2 = oy2;
	priv->out_pixel = priv->base_pixel = save_drawable;

	pcb_gui->coord_per_pix = save_cpp;
	ghidgui->port.view = save_view;
	ghidgui->port.view.canvas_width = save_width;
	ghidgui->port.view.canvas_height = save_height;

	return FALSE;
}

static GtkWidget *ghid_gdk_new_drawing_widget(pcb_gtk_impl_t *common)
{
	GtkWidget *w = gtk_drawing_area_new();

	g_signal_connect(G_OBJECT(w), "expose_event", G_CALLBACK(common->drawing_area_expose), common->gport);

	return w;
}


void ghid_gdk_install(pcb_gtk_impl_t *impl, pcb_hid_t *hid)
{
	if (impl != NULL) {
		impl->new_drawing_widget = ghid_gdk_new_drawing_widget;
		impl->init_drawing_widget = ghid_gdk_init_drawing_widget;
		impl->drawing_realize = ghid_gdk_port_drawing_realize_cb;
		impl->drawing_area_expose = ghid_gdk_drawing_area_expose_cb;
		impl->preview_expose = ghid_gdk_preview_expose;
		impl->set_special_colors = ghid_gdk_set_special_colors;
		impl->init_renderer = ghid_gdk_init_renderer;
		impl->screen_update = ghid_gdk_screen_update;
		impl->draw_grid_local = ghid_gdk_draw_grid_local;
		impl->drawing_area_configure_hook = ghid_gdk_drawing_area_configure_hook;
		impl->shutdown_renderer = ghid_gdk_shutdown_renderer;
		impl->get_color_name = get_color_name;
		impl->map_color = map_color;
		impl->draw_pixmap = ghid_gdk_draw_pixmap;
	}

	if (hid != NULL) {
		hid->invalidate_lr = ghid_gdk_invalidate_lr;
		hid->invalidate_all = ghid_gdk_invalidate_all;
		hid->notify_crosshair_change = ghid_gdk_notify_crosshair_change;
		hid->notify_mark_change = ghid_gdk_notify_mark_change;
		hid->set_layer_group = ghid_gdk_set_layer_group;
		hid->make_gc = ghid_gdk_make_gc;
		hid->destroy_gc = ghid_gdk_destroy_gc;
		hid->set_drawing_mode = ghid_gdk_set_drawing_mode;
		hid->render_burst = ghid_gdk_render_burst;
		hid->set_color = ghid_gdk_set_color;
		hid->set_line_cap = ghid_gdk_set_line_cap;
		hid->set_line_width = ghid_gdk_set_line_width;
		hid->set_draw_xor = ghid_gdk_set_draw_xor;
		hid->draw_line = ghid_gdk_draw_line;
		hid->draw_arc = ghid_gdk_draw_arc;
		hid->draw_rect = ghid_gdk_draw_rect;
		hid->fill_circle = ghid_gdk_fill_circle;
		hid->fill_polygon = ghid_gdk_fill_polygon;
		hid->fill_polygon_offs = ghid_gdk_fill_polygon_offs;
		hid->fill_rect = ghid_gdk_fill_rect;
	}
}

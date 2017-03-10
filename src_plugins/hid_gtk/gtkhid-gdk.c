#include "config.h"
#include "conf_core.h"

#include <stdio.h>

#include "crosshair.h"
#include "clip.h"
#include "data.h"
#include "layer.h"
#include "gui.h"
#include "hid_draw_helpers.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_color.h"

#include "../src_plugins/lib_gtk_common/colors.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"

extern pcb_hid_t ghid_hid;
void ghid_cancel_lead_user(void);
static void ghid_gdk_screen_update(void);

/* Sets priv->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static int cur_mask = -1;
static int mask_seq = 0;

typedef struct render_priv {
	GdkGC *bg_gc;
	GdkGC *offlimits_gc;
	GdkGC *mask_gc;
	GdkGC *u_gc;
	GdkGC *grid_gc;
	pcb_bool clip;
	GdkRectangle clip_rect;
	int attached_invalidate_depth;
	int mark_invalidate_depth;

	/* Feature for leading the user to a particular location */
	guint lead_user_timeout;
	GTimer *lead_user_timer;
	pcb_bool lead_user;
	pcb_coord_t lead_user_radius;
	pcb_coord_t lead_user_x;
	pcb_coord_t lead_user_y;

} render_priv;


typedef struct hid_gc_s {
	pcb_hid_t *me_pointer;
	GdkGC *gc;

	gchar *colorname;
	pcb_coord_t width;
	gint cap, join;
	gchar xor_mask;
	gint mask_seq;
} hid_gc_s;


static void draw_lead_user(render_priv * priv);


static int ghid_gdk_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	int idx = group;
	if (idx >= 0 && idx < pcb_max_group) {
		int n = PCB->LayerGroups.grp[group].len;
		for (idx = 0; idx < n - 1; idx++) {
			int ni = PCB->LayerGroups.grp[group].lid[idx];
			if (ni >= 0 && ni < pcb_max_layer && PCB->Data->Layer[ni].On)
				break;
		}
		idx = PCB->LayerGroups.grp[group].lid[idx];
	}

	/* non-virtual layers with special visibility */
	switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_MASK:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags) /*&& !pinout */ )
				return conf_core.editor.show_mask;
			return 0;
		case PCB_LYT_PASTE: /* Never draw the paste layer */
			return 0;
	}

	if (idx >= 0 && idx < pcb_max_layer && ((flags & PCB_LYT_ANYTHING) != PCB_LYT_SILK))
		return /*pinout ? 1 : */ PCB->Data->Layer[idx].On;

	/* virtual layers */
	{
		switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_INVIS:
			return /* pinout ? 0 : */ PCB->InvisibleObjectsOn;
		case PCB_LYT_SILK:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags) /*|| pinout */ )
				return PCB->ElementOn;
			return 0;
		case PCB_LYT_ASSY:
			return 0;
		case PCB_LYT_PDRILL:
		case PCB_LYT_UDRILL:
			return 1;
		case PCB_LYT_UI:
			return 1;
		case PCB_LYT_RAT:
			return PCB->RatOn;
		}
	}
	return 0;
}

static void ghid_gdk_destroy_gc(pcb_hid_gc_t gc)
{
	if (gc->gc)
		g_object_unref(gc->gc);
	if (gc->colorname != NULL)
		g_free(gc->colorname);
	g_free(gc);
}

static pcb_hid_gc_t ghid_gdk_make_gc(void)
{
	pcb_hid_gc_t rv;

	rv = g_new0(hid_gc_s, 1);
	rv->me_pointer = &ghid_hid;
	rv->colorname = g_strdup(conf_core.appearance.color.background);
	return rv;
}

static void set_clip(render_priv * priv, GdkGC * gc)
{
	if (gc == NULL)
		return;

	if (priv->clip)
		gdk_gc_set_clip_rectangle(gc, &priv->clip_rect);
	else
		gdk_gc_set_clip_mask(gc, NULL);
}

static inline void ghid_gdk_draw_grid_global(void)
{
	render_priv *priv = gport->render_priv;
	pcb_coord_t x, y, x1, y1, x2, y2, grd;
	int n, i;
	static GdkPoint *points = NULL;
	static int npoints = 0;

	x1 = pcb_grid_fit(MAX(0, SIDE_X(gport->view.x0)), PCB->Grid, PCB->GridOffsetX);
	y1 = pcb_grid_fit(MAX(0, SIDE_Y(gport->view.y0)), PCB->Grid, PCB->GridOffsetY);
	x2 = pcb_grid_fit(MIN(PCB->MaxWidth,  SIDE_X(gport->view.x0 + gport->view.width - 1)), PCB->Grid, PCB->GridOffsetX);
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
	for (y = y1; y <= y2; y += grd) {
		for (i = 0; i < n; i++)
			points[i].y = Vy(y);
		gdk_draw_points(gport->drawable, priv->grid_gc, points, n);
	}
}

static void ghid_gdk_draw_grid_local_(pcb_coord_t cx, pcb_coord_t cy, int radius)
{
	render_priv *priv = gport->render_priv;
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

	if (last_grid != PCB->Grid) {
		last_grid = PCB->Grid;
		recalc = 1;
	}

	/* reclaculate the 'filled circle' mask (base relative coords) if grid or radius changed */
	if (recalc) {

		npoints = 0;
		for(y = -radius; y <= radius; y++) {
			int y2 = y*y;
			for(x = -radius; x <= radius; x++) {
				if (x*x + y2 < r2) {
					points_base[npoints].x = x*PCB->Grid;
					points_base[npoints].y = y*PCB->Grid;
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

	gdk_draw_points(gport->drawable, priv->grid_gc, points_abs, npoints);
}


static int grid_local_have_old = 0, grid_local_old_r = 0;
static pcb_coord_t grid_local_old_x, grid_local_old_y;

static void ghid_gdk_draw_grid_local(pcb_coord_t cx, pcb_coord_t cy)
{
	if (grid_local_have_old) {
		ghid_gdk_draw_grid_local_(grid_local_old_x, grid_local_old_y, grid_local_old_r);
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
	ghid_gdk_draw_grid_local_(cx, cy, conf_hid_gtk.plugins.hid_gtk.local_grid.radius);
	grid_local_old_x = cx;
	grid_local_old_y = cy;
	grid_local_old_r = conf_hid_gtk.plugins.hid_gtk.local_grid.radius;
}

static void ghid_gdk_draw_grid(void)
{
	render_priv *priv = gport->render_priv;

	grid_local_have_old = 0;

	if (!conf_core.editor.draw_grid)
		return;
	if (!priv->grid_gc) {
		if (gdk_color_parse(conf_core.appearance.color.grid, &gport->grid_color)) {
			gport->grid_color.red ^= gport->bg_color.red;
			gport->grid_color.green ^= gport->bg_color.green;
			gport->grid_color.blue ^= gport->bg_color.blue;
			gdk_color_alloc(gport->colormap, &gport->grid_color);
		}
		priv->grid_gc = gdk_gc_new(gport->drawable);
		gdk_gc_set_function(priv->grid_gc, GDK_XOR);
		gdk_gc_set_foreground(priv->grid_gc, &gport->grid_color);
		gdk_gc_set_clip_origin(priv->grid_gc, 0, 0);
		set_clip(priv, priv->grid_gc);
	}

	if (conf_hid_gtk.plugins.hid_gtk.local_grid.enable) {
		ghid_gdk_draw_grid_local(grid_local_old_x, grid_local_old_y);
		return;
	}


	ghid_gdk_draw_grid_global();
}

/* ------------------------------------------------------------ */
static void ghid_gdk_draw_bg_image(void)
{
	static GdkPixbuf *pixbuf;
	GdkInterpType interp_type;
	gint src_x, src_y, dst_x, dst_y, w, h, w_src, h_src;
	static gint w_scaled, h_scaled;
	render_priv *priv = gport->render_priv;

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

	if (pixbuf)
		gdk_pixbuf_render_to_drawable(pixbuf, gport->drawable, priv->bg_gc, src_x, src_y, dst_x, dst_y, w - src_x, h - src_y, GDK_RGB_DITHER_NORMAL, 0, 0);
}

#define WHICH_GC(gc) (cur_mask == HID_MASK_CLEAR ? priv->mask_gc : (gc)->gc)

static void ghid_gdk_use_mask(int use_it)
{
	static int mask_seq_id = 0;
	GdkColor color;
	render_priv *priv = gport->render_priv;

	if (!gport->pixmap)
		return;
	if (use_it == cur_mask)
		return;
	switch (use_it) {
	case HID_MASK_OFF:
		gport->drawable = gport->pixmap;
		mask_seq = 0;
		break;

	case HID_MASK_BEFORE:
		/* The HID asks not to receive this mask type, so warn if we get it */
		g_return_if_reached();

	case HID_MASK_CLEAR:
		if (!gport->mask)
			gport->mask = gdk_pixmap_new(0, gport->view.canvas_width, gport->view.canvas_height, 1);
		gport->drawable = gport->mask;
		mask_seq = 0;
		if (!priv->mask_gc) {
			priv->mask_gc = gdk_gc_new(gport->drawable);
			gdk_gc_set_clip_origin(priv->mask_gc, 0, 0);
			set_clip(priv, priv->mask_gc);
		}
		color.pixel = 1;
		gdk_gc_set_foreground(priv->mask_gc, &color);
		gdk_draw_rectangle(gport->drawable, priv->mask_gc, TRUE, 0, 0, gport->view.canvas_width, gport->view.canvas_height);
		color.pixel = 0;
		gdk_gc_set_foreground(priv->mask_gc, &color);
		break;

	case HID_MASK_AFTER:
		mask_seq_id++;
		if (!mask_seq_id)
			mask_seq_id = 1;
		mask_seq = mask_seq_id;

		gport->drawable = gport->pixmap;
		break;

	}
	cur_mask = use_it;
}


typedef struct {
	int color_set;
	GdkColor color;
	int xor_set;
	GdkColor xor_color;
} ColorCache;


	/* Config helper functions for when the user changes color preferences.
	   |  set_special colors used in the gtkhid.
	 */
static void set_special_grid_color(void)
{
	render_priv *priv = gport->render_priv;
	int red, green, blue;

	if (!gport->colormap)
		return;

	red = (gport->grid_color.red ^ gport->bg_color.red) & 0xFF;
	green = (gport->grid_color.green ^ gport->bg_color.green) & 0xFF;
	blue = (gport->grid_color.blue ^ gport->bg_color.blue) & 0xFF;
	conf_setf(CFR_DESIGN, "appearance/color/grid", -1, "#%02x%02x%02x", red, green, blue);
	ghid_map_color_string(conf_core.appearance.color.grid, &gport->grid_color);

	config_color_button_update(&ghidgui->common, conf_get_field("appearance/color/grid"), -1);

	if (priv->grid_gc)
		gdk_gc_set_foreground(priv->grid_gc, &gport->grid_color);
}

static void ghid_gdk_set_special_colors(conf_native_t *cfg)
{
	render_priv *priv = gport->render_priv;
	if (((CFT_COLOR *)cfg->val.color == &conf_core.appearance.color.background) && priv->bg_gc) {
		ghid_map_color_string(cfg->val.color[0], &gport->bg_color);
		gdk_gc_set_foreground(priv->bg_gc, &gport->bg_color);
		set_special_grid_color();
	}
	else if (((CFT_COLOR *)cfg->val.color == &conf_core.appearance.color.off_limit) && priv->offlimits_gc) {
		ghid_map_color_string(cfg->val.color[0], &gport->offlimits_color);
		gdk_gc_set_foreground(priv->offlimits_gc, &gport->offlimits_color);
	}
	else if (((CFT_COLOR *)cfg->val.color == &conf_core.appearance.color.grid) && priv->grid_gc) {
		ghid_map_color_string(cfg->val.color[0], &gport->grid_color);
		set_special_grid_color();
	}
}

static void ghid_gdk_set_color(pcb_hid_gc_t gc, const char *name)
{
	static void *cache = 0;
	pcb_hidval_t cval;

	if (name == NULL) {
		fprintf(stderr, "ghid_gdk_set_color():  name = NULL, setting to magenta\n");
		name = "magenta";
	}

	if (name != gc->colorname) {
		if (gc->colorname != NULL)
			g_free(gc->colorname);
		gc->colorname = g_strdup(name);
	}

	if (!gc->gc)
		return;
	if (gport->colormap == 0)
		gport->colormap = gtk_widget_get_colormap(gport->top_window);

	if (strcmp(name, "erase") == 0) {
		gdk_gc_set_foreground(gc->gc, &gport->bg_color);
	}
	else if (strcmp(name, "drill") == 0) {
		gdk_gc_set_foreground(gc->gc, &gport->offlimits_color);
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
			if (gdk_color_parse(name, &cc->color))
				gdk_color_alloc(gport->colormap, &cc->color);
			else
				gdk_color_white(gport->colormap, &cc->color);
			cc->color_set = 1;
		}
		if (gc->xor_mask) {
			if (!cc->xor_set) {
				cc->xor_color.red = cc->color.red ^ gport->bg_color.red;
				cc->xor_color.green = cc->color.green ^ gport->bg_color.green;
				cc->xor_color.blue = cc->color.blue ^ gport->bg_color.blue;
				gdk_color_alloc(gport->colormap, &cc->xor_color);
				cc->xor_set = 1;
			}
			gdk_gc_set_foreground(gc->gc, &cc->xor_color);
		}
		else {
			gdk_gc_set_foreground(gc->gc, &cc->color);
		}
	}
}

static void ghid_gdk_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	render_priv *priv = gport->render_priv;

	switch (style) {
	case Trace_Cap:
	case Round_Cap:
		gc->cap = GDK_CAP_ROUND;
		gc->join = GDK_JOIN_ROUND;
		break;
	case Square_Cap:
	case Beveled_Cap:
		gc->cap = GDK_CAP_PROJECTING;
		gc->join = GDK_JOIN_MITER;
		break;
	}
	if (gc->gc)
		gdk_gc_set_line_attributes(WHICH_GC(gc), Vz(gc->width), GDK_LINE_SOLID, (GdkCapStyle) gc->cap, (GdkJoinStyle) gc->join);
}

static void ghid_gdk_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	render_priv *priv = gport->render_priv;

	gc->width = width;
	if (gc->gc)
		gdk_gc_set_line_attributes(WHICH_GC(gc), Vz(gc->width), GDK_LINE_SOLID, (GdkCapStyle) gc->cap, (GdkJoinStyle) gc->join);
}

static void ghid_gdk_set_draw_xor(pcb_hid_gc_t gc, int xor_mask)
{
	gc->xor_mask = xor_mask;
	if (!gc->gc)
		return;
	gdk_gc_set_function(gc->gc, xor_mask ? GDK_XOR : GDK_COPY);
	ghid_gdk_set_color(gc, gc->colorname);
}

static int use_gc(pcb_hid_gc_t gc)
{
	render_priv *priv = gport->render_priv;
	GdkWindow *window = gtk_widget_get_window(gport->top_window);

	if (gc->me_pointer != &ghid_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to GTK HID\n");
		abort();
	}

	if (!gport->pixmap)
		return 0;
	if (!gc->gc) {
		gc->gc = gdk_gc_new(window);
		ghid_gdk_set_color(gc, gc->colorname);
		ghid_gdk_set_line_width(gc, gc->width);
		ghid_gdk_set_line_cap(gc, (pcb_cap_style_t) gc->cap);
		ghid_gdk_set_draw_xor(gc, gc->xor_mask);
		gdk_gc_set_clip_origin(gc->gc, 0, 0);
	}
	if (gc->mask_seq != mask_seq) {
		if (mask_seq)
			gdk_gc_set_clip_mask(gc->gc, gport->mask);
		else
			set_clip(priv, gc->gc);
		gc->mask_seq = mask_seq;
	}
	priv->u_gc = WHICH_GC(gc);
	return 1;
}

static void ghid_gdk_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double dx1, dy1, dx2, dy2;
	render_priv *priv = gport->render_priv;

	dx1 = Vx((double) x1);
	dy1 = Vy((double) y1);
	dx2 = Vx((double) x2);
	dy2 = Vy((double) y2);

	if (!pcb_line_clip(0, 0, gport->view.canvas_width, gport->view.canvas_height, &dx1, &dy1, &dx2, &dy2, gc->width / gport->view.coord_per_px))
		return;

	USE_GC(gc);
	gdk_draw_line(gport->drawable, priv->u_gc, dx1, dy1, dx2, dy2);
}

static void ghid_gdk_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	gint vrx2, vry2;
	double w, h, radius;
	render_priv *priv = gport->render_priv;

	w = gport->view.canvas_width * gport->view.coord_per_px;
	h = gport->view.canvas_height * gport->view.coord_per_px;
	radius = (xradius > yradius) ? xradius : yradius;
	if (SIDE_X(cx) < gport->view.x0 - radius
			|| SIDE_X(cx) > gport->view.x0 + w + radius
			|| SIDE_Y(cy) < gport->view.y0 - radius || SIDE_Y(cy) > gport->view.y0 + h + radius)
		return;

	USE_GC(gc);
	vrx2 = Vz(xradius*2.0);
	vry2 = Vz(yradius*2.0);

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

	gdk_draw_arc(gport->drawable, priv->u_gc, 0,
							 pcb_round(Vxd(cx) - Vzd(xradius) + 0.5), pcb_round(Vyd(cy) - Vzd(yradius) + 0.5),
							 pcb_round(vrx2), pcb_round(vry2),
							 (start_angle + 180) * 64, delta_angle * 64);
}

static void ghid_gdk_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gint w, h, lw;
	render_priv *priv = gport->render_priv;

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
	gdk_draw_rectangle(gport->drawable, priv->u_gc, FALSE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}


static void ghid_gdk_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	gint w, h, vr;
	render_priv *priv = gport->render_priv;

	w = gport->view.canvas_width * gport->view.coord_per_px;
	h = gport->view.canvas_height * gport->view.coord_per_px;
	if (SIDE_X(cx) < gport->view.x0 - radius
			|| SIDE_X(cx) > gport->view.x0 + w + radius
			|| SIDE_Y(cy) < gport->view.y0 - radius || SIDE_Y(cy) > gport->view.y0 + h + radius)
		return;

	USE_GC(gc);
	vr = Vz(radius);
	gdk_draw_arc(gport->drawable, priv->u_gc, TRUE, Vx(cx) - vr, Vy(cy) - vr, vr * 2, vr * 2, 0, 360 * 64);
}

static void ghid_gdk_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	static GdkPoint *points = 0;
	static int npoints = 0;
	int i;
	render_priv *priv = gport->render_priv;
	USE_GC(gc);

	if (npoints < n_coords) {
		npoints = n_coords + 1;
		points = (GdkPoint *) realloc(points, npoints * sizeof(GdkPoint));
	}
	for (i = 0; i < n_coords; i++) {
		points[i].x = Vx(x[i]);
		points[i].y = Vy(y[i]);
	}
	gdk_draw_polygon(gport->drawable, priv->u_gc, 1, points, n_coords);
}

static void ghid_gdk_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gint w, h, lw, xx, yy;
	render_priv *priv = gport->render_priv;

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
	USE_GC(gc);
	gdk_draw_rectangle(gport->drawable, priv->u_gc, TRUE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

static void redraw_region(GdkRectangle * rect)
{
	int eleft, eright, etop, ebottom;
	pcb_hid_expose_ctx_t ctx;
	render_priv *priv = gport->render_priv;

	if (!gport->pixmap)
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

	set_clip(priv, priv->bg_gc);
	set_clip(priv, priv->offlimits_gc);
	set_clip(priv, priv->mask_gc);
	set_clip(priv, priv->grid_gc);

	ctx.view.X1 = MIN(Px(priv->clip_rect.x), Px(priv->clip_rect.x + priv->clip_rect.width + 1));
	ctx.view.Y1 = MIN(Py(priv->clip_rect.y), Py(priv->clip_rect.y + priv->clip_rect.height + 1));
	ctx.view.X2 = MAX(Px(priv->clip_rect.x), Px(priv->clip_rect.x + priv->clip_rect.width + 1));
	ctx.view.Y2 = MAX(Py(priv->clip_rect.y), Py(priv->clip_rect.y + priv->clip_rect.height + 1));

	ctx.view.X1 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X1));
	ctx.view.X2 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X2));
	ctx.view.Y1 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y1));
	ctx.view.Y2 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y2));
	
	ctx.force = 0;
	ctx.content.elem = NULL;

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

	if (eleft > 0)
		gdk_draw_rectangle(gport->drawable, priv->offlimits_gc, 1, 0, 0, eleft, gport->view.canvas_height);
	else
		eleft = 0;
	if (eright < gport->view.canvas_width)
		gdk_draw_rectangle(gport->drawable, priv->offlimits_gc, 1, eright, 0, gport->view.canvas_width - eright, gport->view.canvas_height);
	else
		eright = gport->view.canvas_width;
	if (etop > 0)
		gdk_draw_rectangle(gport->drawable, priv->offlimits_gc, 1, eleft, 0, eright - eleft + 1, etop);
	else
		etop = 0;
	if (ebottom < gport->view.canvas_height)
		gdk_draw_rectangle(gport->drawable, priv->offlimits_gc, 1, eleft, ebottom, eright - eleft + 1, gport->view.canvas_height - ebottom);
	else
		ebottom = gport->view.canvas_height;

	gdk_draw_rectangle(gport->drawable, priv->bg_gc, 1, eleft, etop, eright - eleft + 1, ebottom - etop + 1);

	ghid_gdk_draw_bg_image();

	pcb_hid_expose_all(&ghid_hid, &ctx);
	ghid_gdk_draw_grid();

	/* In some cases we are called with the crosshair still off */
	if (priv->attached_invalidate_depth == 0)
		pcb_draw_attached();

	/* In some cases we are called with the mark still off */
	if (priv->mark_invalidate_depth == 0)
		pcb_draw_mark();

	draw_lead_user(priv);

	priv->clip = pcb_false;

	/* Rest the clip for bg_gc, as it is used outside this function */
	gdk_gc_set_clip_mask(priv->bg_gc, NULL);
}

static void ghid_gdk_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
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
	ghid_gdk_screen_update();
}


static void ghid_gdk_invalidate_all()
{
	if (ghidgui && ghidgui->topwin.menu.menu_bar) {
		redraw_region(NULL);
		ghid_gdk_screen_update();
	}
}

static void ghid_gdk_notify_crosshair_change(pcb_bool changes_complete)
{
	render_priv *priv = gport->render_priv;

	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	if (changes_complete)
		priv->attached_invalidate_depth--;

	if (priv->attached_invalidate_depth < 0) {
		priv->attached_invalidate_depth = 0;
		/* A mismatch of changes_complete == pcb_false and == pcb_true notifications
		 * is not expected to occur, but we will try to handle it gracefully.
		 * As we know the crosshair will have been shown already, we must
		 * repaint the entire view to be sure not to leave an artaefact.
		 */
		ghid_gdk_invalidate_all();
		return;
	}

	if (priv->attached_invalidate_depth == 0)
		pcb_draw_attached();

	if (!changes_complete) {
		priv->attached_invalidate_depth++;
	}
	else if (gport->drawing_area != NULL) {
		/* Queue a GTK expose when changes are complete */
		ghid_draw_area_update(gport, NULL);
	}
}

static void ghid_gdk_notify_mark_change(pcb_bool changes_complete)
{
	render_priv *priv = gport->render_priv;

	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	if (changes_complete)
		priv->mark_invalidate_depth--;

	if (priv->mark_invalidate_depth < 0) {
		priv->mark_invalidate_depth = 0;
		/* A mismatch of changes_complete == pcb_false and == pcb_true notifications
		 * is not expected to occur, but we will try to handle it gracefully.
		 * As we know the mark will have been shown already, we must
		 * repaint the entire view to be sure not to leave an artaefact.
		 */
		ghid_gdk_invalidate_all();
		return;
	}

	if (priv->mark_invalidate_depth == 0)
		pcb_draw_mark();

	if (!changes_complete) {
		priv->mark_invalidate_depth++;
	}
	else if (gport->drawing_area != NULL) {
		/* Queue a GTK expose when changes are complete */
		ghid_draw_area_update(gport, NULL);
	}
}

static void draw_right_cross(GdkGC * xor_gc, gint x, gint y)
{
	GdkWindow *window = gtk_widget_get_window(gport->drawing_area);

	gdk_draw_line(window, xor_gc, x, 0, x, gport->view.canvas_height);
	gdk_draw_line(window, xor_gc, 0, y, gport->view.canvas_width, y);
}

static void draw_slanted_cross(GdkGC * xor_gc, gint x, gint y)
{
	GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
	gint x0, y0, x1, y1;

	x0 = x + (gport->view.canvas_height - y);
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x - y;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + (gport->view.canvas_width - x);
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - x;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (gport->view.canvas_height - y);
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x + y;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + x;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - (gport->view.canvas_width - x);
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);
}

static void draw_dozen_cross(GdkGC * xor_gc, gint x, gint y)
{
	GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
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
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x + (gport->view.canvas_height - y) * tan60;
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x - y * tan60;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + (gport->view.canvas_width - x) / tan60;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - x / tan60;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (gport->view.canvas_height - y) / tan60;
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x + y / tan60;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + x * tan60;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - (gport->view.canvas_width - x) * tan60;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);

	x0 = x - (gport->view.canvas_height - y) * tan60;
	x0 = MAX(0, MIN(x0, gport->view.canvas_width));
	x1 = x + y * tan60;
	x1 = MAX(0, MIN(x1, gport->view.canvas_width));
	y0 = y + x / tan60;
	y0 = MAX(0, MIN(y0, gport->view.canvas_height));
	y1 = y - (gport->view.canvas_width - x) / tan60;
	y1 = MAX(0, MIN(y1, gport->view.canvas_height));
	gdk_draw_line(window, xor_gc, x0, y0, x1, y1);
}

static void draw_crosshair(GdkGC * xor_gc, gint x, gint y)
{
	static enum pcb_crosshair_shape_e prev = pcb_ch_shape_basic;

	draw_right_cross(xor_gc, x, y);
	if (prev == pcb_ch_shape_union_jack)
		draw_slanted_cross(xor_gc, x, y);
	if (prev == pcb_ch_shape_dozen)
		draw_dozen_cross(xor_gc, x, y);
	prev = pcb_crosshair.shape;
}

static void show_crosshair(gboolean paint_new_location)
{
	render_priv *priv = gport->render_priv;
	GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
	GtkStyle *style = gtk_widget_get_style(gport->drawing_area);
	gint x, y;
	static gint x_prev = -1, y_prev = -1;
	static GdkGC *xor_gc;
	static GdkColor cross_color;

	if (gport->view.crosshair_x < 0 || !ghidgui->topwin.active || !gport->view.has_entered)
		return;

	if (!xor_gc) {
		xor_gc = gdk_gc_new(window);
		gdk_gc_copy(xor_gc, style->white_gc);
		gdk_gc_set_function(xor_gc, GDK_XOR);
		gdk_gc_set_clip_origin(xor_gc, 0, 0);
		set_clip(priv, xor_gc);
		/* FIXME: when CrossColor changed from config */
		ghid_map_color_string(conf_core.appearance.color.cross, &cross_color);
	}
	x = DRAW_X(&gport->view, gport->view.crosshair_x);
	y = DRAW_Y(&gport->view, gport->view.crosshair_y);

	gdk_gc_set_foreground(xor_gc, &cross_color);

	if (x_prev >= 0 && !paint_new_location)
		draw_crosshair(xor_gc, x_prev, y_prev);

	if (x >= 0 && paint_new_location) {
		draw_crosshair(xor_gc, x, y);
		x_prev = x;
		y_prev = y;
	}
	else
		x_prev = y_prev = -1;
}

static void ghid_gdk_init_renderer(int *argc, char ***argv, void *vport)
{
	GHidPort * port = vport;
	/* Init any GC's required */
	port->render_priv = g_new0(render_priv, 1);
}

static void ghid_gdk_shutdown_renderer(void *port_)
{
	GHidPort *port = port_;
	ghid_cancel_lead_user();
	g_free(port->render_priv);
	port->render_priv = NULL;
}

static void ghid_gdk_init_drawing_widget(GtkWidget * widget, void * port)
{
}

static void ghid_gdk_drawing_area_configure_hook(void *port_)
{
	GHidPort *port = port_;
	static int done_once = 0;
	render_priv *priv = port->render_priv;

	if (!done_once) {
		priv->bg_gc = gdk_gc_new(port->drawable);
		gdk_gc_set_foreground(priv->bg_gc, &port->bg_color);
		gdk_gc_set_clip_origin(priv->bg_gc, 0, 0);

		priv->offlimits_gc = gdk_gc_new(port->drawable);
		gdk_gc_set_foreground(priv->offlimits_gc, &port->offlimits_color);
		gdk_gc_set_clip_origin(priv->offlimits_gc, 0, 0);
		done_once = 1;
	}

	if (port->mask) {
		gdk_pixmap_unref(port->mask);
		port->mask = gdk_pixmap_new(0, port->view.canvas_width, port->view.canvas_height, 1);
	}
}

static void ghid_gdk_screen_update(void)
{
	render_priv *priv = gport->render_priv;
	GdkWindow *window = gtk_widget_get_window(gport->drawing_area);

	if (gport->pixmap == NULL)
		return;

	gdk_draw_drawable(window, priv->bg_gc, gport->pixmap, 0, 0, 0, 0, gport->view.canvas_width, gport->view.canvas_height);
	show_crosshair(TRUE);
}

static gboolean ghid_gdk_drawing_area_expose_cb(GtkWidget * widget, GdkEventExpose * ev, void *vport)
{
	GHidPort *port = vport;
	render_priv *priv = port->render_priv;
	GdkWindow *window = gtk_widget_get_window(gport->drawing_area);

	gdk_draw_drawable(window, priv->bg_gc, port->pixmap,
										ev->area.x, ev->area.y, ev->area.x, ev->area.y, ev->area.width, ev->area.height);
	show_crosshair(TRUE);
	return FALSE;
}

static void ghid_gdk_port_drawing_realize_cb(GtkWidget * widget, gpointer data)
{
}

static gboolean ghid_gdk_preview_expose(GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx)
{
	GdkWindow *window = gtk_widget_get_window(widget);
	GdkDrawable *save_drawable;
	GtkAllocation allocation;
	pcb_gtk_view_t save_view;
	int save_width, save_height;
	double xz, yz, vw, vh;
	render_priv *priv = gport->render_priv;

	vw = ctx->view.X2 - ctx->view.X1;
	vh = ctx->view.Y2 - ctx->view.Y1;

	/* Setup drawable and zoom factor for drawing routines
	 */
	save_drawable = gport->drawable;
	save_view = gport->view;
	save_width = gport->view.canvas_width;
	save_height = gport->view.canvas_height;

	gtk_widget_get_allocation(widget, &allocation);
	xz = vw / (double)allocation.width;
	yz = vh / (double)allocation.height;
	if (xz > yz)
		gport->view.coord_per_px = xz;
	else
		gport->view.coord_per_px = yz;

	gport->drawable = window;
	gport->view.canvas_width = allocation.width;
	gport->view.canvas_height = allocation.height;
	gport->view.width = allocation.width * gport->view.coord_per_px;
	gport->view.height = allocation.height * gport->view.coord_per_px;
	gport->view.x0 = (vw - gport->view.width) / 2 + ctx->view.X1;
	gport->view.y0 = (vh - gport->view.height) / 2 + ctx->view.Y1;

	/* clear background */
	gdk_draw_rectangle(window, priv->bg_gc, TRUE, 0, 0, allocation.width, allocation.height);

	/* call the drawing routine */
	expcall(&ghid_hid, ctx);

	gport->drawable = save_drawable;
	gport->view = save_view;
	gport->view.canvas_width = save_width;
	gport->view.canvas_height = save_height;

	return FALSE;
}

static GdkPixmap *ghid_gdk_render_pixmap(int cx, int cy, double zoom, int width, int height, int depth)
{
	GdkPixmap *pixmap;
	GdkDrawable *save_drawable;
	pcb_gtk_view_t save_view;
	int save_width, save_height;
	pcb_hid_expose_ctx_t ectx;
	render_priv *priv = gport->render_priv;

	save_drawable = gport->drawable;
	save_view = gport->view;
	save_width = gport->view.canvas_width;
	save_height = gport->view.canvas_height;

	pixmap = gdk_pixmap_new(NULL, width, height, depth);

	/* Setup drawable and zoom factor for drawing routines
	 */

	gport->drawable = pixmap;
	gport->view.coord_per_px = zoom;
	gport->view.canvas_width = width;
	gport->view.canvas_height = height;
	gport->view.width = width * gport->view.coord_per_px;
	gport->view.height = height * gport->view.coord_per_px;
	gport->view.x0 = conf_core.editor.view.flip_x ? PCB->MaxWidth - cx : cx;
	gport->view.x0 -= gport->view.height / 2;
	gport->view.y0 = conf_core.editor.view.flip_y ? PCB->MaxHeight - cy : cy;
	gport->view.y0 -= gport->view.width / 2;

	/* clear background */
	gdk_draw_rectangle(pixmap, priv->bg_gc, TRUE, 0, 0, width, height);

	/* call the drawing routine */
	ectx.view.X1 = MIN(Px(0), Px(gport->view.canvas_width + 1));
	ectx.view.Y1 = MIN(Py(0), Py(gport->view.canvas_height + 1));
	ectx.view.X2 = MAX(Px(0), Px(gport->view.canvas_width + 1));
	ectx.view.Y2 = MAX(Py(0), Py(gport->view.canvas_height + 1));

	ectx.view.X1 = MAX(0, MIN(PCB->MaxWidth, ectx.view.X1));
	ectx.view.X2 = MAX(0, MIN(PCB->MaxWidth, ectx.view.X2));
	ectx.view.Y1 = MAX(0, MIN(PCB->MaxHeight, ectx.view.Y1));
	ectx.view.Y2 = MAX(0, MIN(PCB->MaxHeight, ectx.view.Y2));

	ectx.force = 0;
	ectx.content.elem = NULL;

	pcb_hid_expose_all(&ghid_hid, &ectx);

	gport->drawable = save_drawable;
	gport->view = save_view;
	gport->view.canvas_width = save_width;
	gport->view.canvas_height = save_height;

	return pixmap;
}

static pcb_hid_t *ghid_gdk_request_debug_draw(void)
{
	/* No special setup requirements, drawing goes into
	 * the backing pixmap. */
	return &ghid_hid;
}

static void ghid_gdk_flush_debug_draw(void)
{
	ghid_gdk_screen_update();
	gdk_flush();
}

static void ghid_gdk_finish_debug_draw(void)
{
	ghid_gdk_flush_debug_draw();
	/* No special tear down requirements
	 */
}

#define LEAD_USER_WIDTH           0.2	/* millimeters */
#define LEAD_USER_PERIOD          (1000 / 5)	/* 5fps (in ms) */
#define LEAD_USER_VELOCITY        3.	/* millimeters per second */
#define LEAD_USER_ARC_COUNT       3
#define LEAD_USER_ARC_SEPARATION  3.	/* millimeters */
#define LEAD_USER_INITIAL_RADIUS  10.	/* millimetres */
#define LEAD_USER_COLOR_R         1.
#define LEAD_USER_COLOR_G         1.
#define LEAD_USER_COLOR_B         0.

static void draw_lead_user(render_priv * priv)
{
	GdkWindow *window = gtk_widget_get_window(gport->drawing_area);
	GtkStyle *style = gtk_widget_get_style(gport->drawing_area);
	int i;
	pcb_coord_t radius = priv->lead_user_radius;
	pcb_coord_t width = PCB_MM_TO_COORD(LEAD_USER_WIDTH);
	pcb_coord_t separation = PCB_MM_TO_COORD(LEAD_USER_ARC_SEPARATION);
	static GdkGC *lead_gc = NULL;
	GdkColor lead_color;

	if (!priv->lead_user)
		return;

	if (lead_gc == NULL) {
		lead_gc = gdk_gc_new(window);
		gdk_gc_copy(lead_gc, style->white_gc);
		gdk_gc_set_function(lead_gc, GDK_XOR);
		gdk_gc_set_clip_origin(lead_gc, 0, 0);
		lead_color.pixel = 0;
		lead_color.red = (int) (65535. * LEAD_USER_COLOR_R);
		lead_color.green = (int) (65535. * LEAD_USER_COLOR_G);
		lead_color.blue = (int) (65535. * LEAD_USER_COLOR_B);
		gdk_color_alloc(gport->colormap, &lead_color);
		gdk_gc_set_foreground(lead_gc, &lead_color);
	}

	set_clip(priv, lead_gc);
	gdk_gc_set_line_attributes(lead_gc, Vz(width), GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

	/* arcs at the approrpriate radii */

	for (i = 0; i < LEAD_USER_ARC_COUNT; i++, radius -= separation) {
		if (radius < width)
			radius += PCB_MM_TO_COORD(LEAD_USER_INITIAL_RADIUS);

		/* Draw an arc at radius */
		gdk_draw_arc(gport->drawable, lead_gc, FALSE,
								 Vx(priv->lead_user_x - radius), Vy(priv->lead_user_y - radius), Vz(2. * radius), Vz(2. * radius), 0, 360 * 64);
	}
}

static gboolean lead_user_cb(gpointer data)
{
	render_priv *priv = data;
	pcb_coord_t step;
	double elapsed_time;

	/* Queue a redraw */
	ghid_invalidate_all();

	/* Update radius */
	elapsed_time = g_timer_elapsed(priv->lead_user_timer, NULL);
	g_timer_start(priv->lead_user_timer);

	step = PCB_MM_TO_COORD(LEAD_USER_VELOCITY * elapsed_time);
	if (priv->lead_user_radius > step)
		priv->lead_user_radius -= step;
	else
		priv->lead_user_radius = PCB_MM_TO_COORD(LEAD_USER_INITIAL_RADIUS);

	return TRUE;
}

#warning move this out to common
void ghid_lead_user_to_location(pcb_coord_t x, pcb_coord_t y)
{
	render_priv *priv = gport->render_priv;

	ghid_cancel_lead_user();

	priv->lead_user = pcb_true;
	priv->lead_user_x = x;
	priv->lead_user_y = y;
	priv->lead_user_radius = PCB_MM_TO_COORD(LEAD_USER_INITIAL_RADIUS);
	priv->lead_user_timeout = g_timeout_add(LEAD_USER_PERIOD, lead_user_cb, priv);
	priv->lead_user_timer = g_timer_new();
}

#warning move this out to common
void ghid_cancel_lead_user(void)
{
	render_priv *priv = gport->render_priv;

	if (priv->lead_user_timeout)
		g_source_remove(priv->lead_user_timeout);

	if (priv->lead_user_timer)
		g_timer_destroy(priv->lead_user_timer);

	if (priv->lead_user)
		ghid_invalidate_all();

	priv->lead_user_timeout = 0;
	priv->lead_user_timer = NULL;
	priv->lead_user = pcb_false;
}

void ghid_gdk_install(pcb_gtk_common_t *common, pcb_hid_t *hid)
{
	common->render_pixmap = ghid_gdk_render_pixmap;
	common->init_drawing_widget = ghid_gdk_init_drawing_widget;
	common->drawing_realize = ghid_gdk_port_drawing_realize_cb;
	common->drawing_area_expose = ghid_gdk_drawing_area_expose_cb;
	common->preview_expose = ghid_gdk_preview_expose;
	common->invalidate_all = ghid_gdk_invalidate_all;
	common->set_special_colors = ghid_gdk_set_special_colors;
	common->init_renderer = ghid_gdk_init_renderer;
	common->screen_update = ghid_gdk_screen_update;
	common->draw_grid_local = ghid_gdk_draw_grid_local;
	common->drawing_area_configure_hook = ghid_gdk_drawing_area_configure_hook;
	common->shutdown_renderer = ghid_gdk_shutdown_renderer;

	hid->invalidate_lr = ghid_gdk_invalidate_lr;
	hid->invalidate_all = ghid_gdk_invalidate_all;
	hid->notify_crosshair_change = ghid_gdk_notify_crosshair_change;
	hid->notify_mark_change = ghid_gdk_notify_mark_change;
	hid->set_layer_group = ghid_gdk_set_layer_group;
	hid->make_gc = ghid_gdk_make_gc;
	hid->destroy_gc = ghid_gdk_destroy_gc;
	hid->use_mask = ghid_gdk_use_mask;
	hid->set_color = ghid_gdk_set_color;
	hid->set_line_cap = ghid_gdk_set_line_cap;
	hid->set_line_width = ghid_gdk_set_line_width;
	hid->set_draw_xor = ghid_gdk_set_draw_xor;
	hid->draw_line = ghid_gdk_draw_line;
	hid->draw_arc = ghid_gdk_draw_arc;
	hid->draw_rect = ghid_gdk_draw_rect;
	hid->fill_circle = ghid_gdk_fill_circle;
	hid->fill_polygon = ghid_gdk_fill_polygon;
	hid->fill_rect = ghid_gdk_fill_rect;

	hid->request_debug_draw = ghid_gdk_request_debug_draw;
	hid->flush_debug_draw = ghid_gdk_flush_debug_draw;
	hid->finish_debug_draw = ghid_gdk_finish_debug_draw;
}

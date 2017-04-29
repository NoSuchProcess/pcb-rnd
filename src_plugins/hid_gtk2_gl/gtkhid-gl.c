#include "config.h"

#include <stdio.h>

#include "crosshair.h"
#include "clip.h"
#include "data.h"
#include "layer.h"
#include "hid_draw_helpers.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_color.h"

#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"

#include "../src_plugins/lib_gtk_hid/gui.h"
#include "../src_plugins/lib_gtk_hid/coord_conv.h"
#include "../src_plugins/lib_gtk_hid/render.h"


/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */

#define GL_GLEXT_PROTOTYPES 1
#ifdef HAVE_OPENGL_GL_H
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <gtk/gtkgl.h>
#include "hidgl.h"
#include "hid_draw_helpers.h"

#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

extern pcb_hid_t gtk2_gl_hid;

static pcb_hid_gc_t current_gc = NULL;

/* Sets gport->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static int cur_mask = -1;

typedef struct render_priv_s {
	GdkGLConfig *glconfig;
	GdkColor bg_color, offlimits_color, grid_color;
	pcb_bool trans_lines;
	pcb_bool in_context;
	int subcomposite_stencil_bit;
	char *current_colorname;
	double current_alpha_mult;
} render_priv_t;


typedef struct hid_gc_s {
	pcb_hid_t *me_pointer;

	const char *colorname;
	double alpha_mult;
	pcb_coord_t width;
	gint cap, join;
	gchar xor;
} hid_gc_s;

static void draw_lead_user(render_priv_t * priv);

static const gchar *get_color_name(GdkColor * color)
{
	static char tmp[16];

	if (!color)
		return "#000000";

	sprintf(tmp, "#%2.2x%2.2x%2.2x", (color->red >> 8) & 0xff, (color->green >> 8) & 0xff, (color->blue >> 8) & 0xff);
	return tmp;
}

/** Returns TRUE only if \p color_string has been allocated to \p color. */
static pcb_bool map_color_string(const char *color_string, GdkColor * color)
{
	static GdkColormap *colormap = NULL;
	GHidPort *out = &ghid_port;
	pcb_bool parsed;

	if (!color || !out->top_window)
		return FALSE;
	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(out->top_window);
	if (color->red || color->green || color->blue)
		gdk_colormap_free_colors(colormap, color, 1);
	parsed = gdk_color_parse(color_string, color);
	if (parsed)
		gdk_color_alloc(colormap, color);

	return parsed;
}


static void start_subcomposite(void)
{
	render_priv_t *priv = gport->render_priv;
	int stencil_bit;

	/* Flush out any existing geoemtry to be rendered */
	hidgl_flush_triangles(&buffer);

	glEnable(GL_STENCIL_TEST);		/* Enable Stencil test */
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);	/* Stencil pass => replace stencil value (with 1) */

	stencil_bit = hidgl_assign_clear_stencil_bit();	/* Get a new (clean) bitplane to stencil with */
	glStencilMask(stencil_bit);		/* Only write to our subcompositing stencil bitplane */
	glStencilFunc(GL_GREATER, stencil_bit, stencil_bit);	/* Pass stencil test if our assigned bit is clear */

	priv->subcomposite_stencil_bit = stencil_bit;
}

static void end_subcomposite(void)
{
	render_priv_t *priv = gport->render_priv;

	/* Flush out any existing geoemtry to be rendered */
	hidgl_flush_triangles(&buffer);

	hidgl_return_stencil_bit(priv->subcomposite_stencil_bit);	/* Relinquish any bitplane we previously used */

	glStencilMask(0);
	glStencilFunc(GL_ALWAYS, 0, 0);	/* Always pass stencil test */
	glDisable(GL_STENCIL_TEST);		/* Disable Stencil test */

	priv->subcomposite_stencil_bit = 0;
}


int ghid_gl_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	render_priv_t *priv = gport->render_priv;
	int idx = group;
	if (idx >= 0 && idx < pcb_max_group(PCB)) {
		int n = PCB->LayerGroups.grp[group].len;
		for (idx = 0; idx < n - 1; idx++) {
			int ni = PCB->LayerGroups.grp[group].lid[idx];
			if (ni >= 0 && ni < pcb_max_layer && PCB->Data->Layer[ni].On)
				break;
		}
		idx = PCB->LayerGroups.grp[group].lid[idx];
	}

	end_subcomposite();
	start_subcomposite();

	priv->trans_lines = pcb_true;

	/* non-virtual layers with special visibility */
	switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_MASK:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				return pcb_mask_on(PCB);
			return 0;
		case PCB_LYT_SILK:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				return pcb_silk_on(PCB);
			return 0;
		case PCB_LYT_PASTE:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				return pcb_paste_on(PCB);
			return 0;
	}

	/* normal layers */
	if (idx >= 0 && idx < pcb_max_layer) {
		priv->trans_lines = pcb_true;
		return PCB->Data->Layer[idx].On;
	}

	/* virtual layers */
	{
		switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_INVIS:
			return PCB->InvisibleObjectsOn;
		case PCB_LYT_ASSY:
			return 0;
		case PCB_LYT_PDRILL:
		case PCB_LYT_UDRILL:
			return 1;
		case PCB_LYT_RAT:
			return PCB->RatOn;
		case PCB_LYT_CSECT:
			/* Opaque draw */
			priv->trans_lines = pcb_false;
			/* Disable stencil for cross-section drawing; that code
			 * relies on overdraw doing the right thing and doesn't
			 * use layers */
			glDisable(GL_STENCIL_TEST);
			return 0;
		}
	}
	return 0;
}

static void ghid_gl_end_layer(void)
{
	end_subcomposite();
}

void ghid_gl_destroy_gc(pcb_hid_gc_t gc)
{
	g_free(gc);
}

pcb_hid_gc_t ghid_gl_make_gc(void)
{
	pcb_hid_gc_t rv;

	rv = g_new0(hid_gc_s, 1);
	rv->me_pointer = &gtk2_gl_hid;
	rv->colorname = conf_core.appearance.color.background;
	rv->alpha_mult = 1.0;
	return rv;
}

void ghid_gl_draw_grid_local(pcb_coord_t cx, pcb_coord_t cy)
{
#warning draw_grid_local stubbed out for now
}

static void ghid_gl_draw_grid(pcb_box_t *drawn_area)
{
	render_priv_t *priv = gport->render_priv;

	if (Vz(PCB->Grid) < PCB_MIN_GRID_DISTANCE)
		return;

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);

	glColor3f(priv->grid_color.red / 65535., priv->grid_color.green / 65535., priv->grid_color.blue / 65535.);

#warning this does not draw the local grid and ignores other new grid options
	hidgl_draw_grid(drawn_area);

	glDisable(GL_COLOR_LOGIC_OP);
}

static void ghid_gl_draw_bg_image(void)
{
	static GLuint texture_handle = 0;

	if (!ghidgui->bg_pixbuf)
		return;

	if (texture_handle == 0) {
		int width = gdk_pixbuf_get_width(ghidgui->bg_pixbuf);
		int height = gdk_pixbuf_get_height(ghidgui->bg_pixbuf);
		int rowstride = gdk_pixbuf_get_rowstride(ghidgui->bg_pixbuf);
		int bits_per_sample = gdk_pixbuf_get_bits_per_sample(ghidgui->bg_pixbuf);
		int n_channels = gdk_pixbuf_get_n_channels(ghidgui->bg_pixbuf);
		unsigned char *pixels = gdk_pixbuf_get_pixels(ghidgui->bg_pixbuf);

		g_warn_if_fail(bits_per_sample == 8);
		g_warn_if_fail(rowstride == width * n_channels);

		glGenTextures(1, &texture_handle);
		glBindTexture(GL_TEXTURE_2D, texture_handle);

		/* XXX: We should proabbly determine what the maxmimum texture supported is,
		 *      and if our image is larger, shrink it down using GDK pixbuf routines
		 *      rather than having it fail below.
		 */

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);
	}

	glBindTexture(GL_TEXTURE_2D, texture_handle);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glEnable(GL_TEXTURE_2D);

	/* Render a quad with the background as a texture */

	glBegin(GL_QUADS);
	glTexCoord2d(0., 0.);
	glVertex3i(0, 0, 0);
	glTexCoord2d(1., 0.);
	glVertex3i(PCB->MaxWidth, 0, 0);
	glTexCoord2d(1., 1.);
	glVertex3i(PCB->MaxWidth, PCB->MaxHeight, 0);
	glTexCoord2d(0., 1.);
	glVertex3i(0, PCB->MaxHeight, 0);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void ghid_gl_use_mask(pcb_mask_op_t use_it)
{
	static int stencil_bit = 0;

	if (use_it == cur_mask)
		return;

	/* Flush out any existing geoemtry to be rendered */
	hidgl_flush_triangles(&buffer);

	switch (use_it) {
	case HID_MASK_BEFORE:
		/* The HID asks not to receive this mask type, so warn if we get it */
		g_return_if_reached();

	case HID_MASK_INIT:
		glColorMask(0, 0, 0, 0);		/* Disable writting in color buffer */
		glEnable(GL_STENCIL_TEST);	/* Enable Stencil test */
		stencil_bit = hidgl_assign_clear_stencil_bit();	/* Get a new (clean) bitplane to stencil with */
		break;

	case HID_MASK_CLEAR:
		/* Write '0' to the stencil buffer where the solder-mask should not be drawn ("transparent", "cutout"). */
		glStencilFunc(GL_ALWAYS, stencil_bit, stencil_bit);	/* Always pass stencil test, write stencil_bit */
		glStencilMask(stencil_bit);	/* Only write to our subcompositing stencil bitplane */
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);	/* Stencil pass => replace stencil value (with 1) */
		break;

	case HID_MASK_SET:
		/* Write '1' to the stencil buffer where the solder-mask should be drawn ("red", "solid"). */
		glStencilFunc(GL_ALWAYS, stencil_bit, stencil_bit);	/* Always pass stencil test, write stencil_bit */
		glStencilMask(stencil_bit);	/* Only write to our subcompositing stencil bitplane */
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);	/* Stencil pass => replace stencil value (with 1) */
		break;

	case HID_MASK_AFTER:
		/* Drawing operations as masked to areas where the stencil buffer is '0' */
		glColorMask(1, 1, 1, 1);		/* Enable drawing of r, g, b & a */
		glStencilFunc(GL_GEQUAL, 0, stencil_bit);	/* Draw only where our bit of the stencil buffer is clear */
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);	/* Stencil buffer read only */
		break;

	case HID_MASK_OFF:
		/* Disable stenciling */
		hidgl_return_stencil_bit(stencil_bit);	/* Relinquish any bitplane we previously used */
		glDisable(GL_STENCIL_TEST);	/* Disable Stencil test */
		break;
	}
	cur_mask = use_it;
}


	/* Config helper functions for when the user changes color preferences.
	   |  set_special colors used in the gtkhid.
	 */
static void set_special_grid_color(void)
{
	render_priv_t *priv = gport->render_priv;

	priv->grid_color.red ^= priv->bg_color.red;
	priv->grid_color.green ^= priv->bg_color.green;
	priv->grid_color.blue ^= priv->bg_color.blue;
}

void ghid_gl_set_special_colors(conf_native_t *cfg)
{
	render_priv_t *priv = gport->render_priv;

	if (((CFT_COLOR *)cfg->val.color == &conf_core.appearance.color.background)) {
		if (map_color_string(cfg->val.color[0], &priv->bg_color)) {
			config_color_button_update(&ghidgui->common, conf_get_field("appearance/color/background"), -1);
			set_special_grid_color();
		}
	}
	else if (((CFT_COLOR *)cfg->val.color == &conf_core.appearance.color.off_limit)) {
		if (map_color_string(cfg->val.color[0], &priv->offlimits_color))
      config_color_button_update(&ghidgui->common, conf_get_field("appearance/color/off_limit"), -1);
	}
	else if (((CFT_COLOR *)cfg->val.color == &conf_core.appearance.color.grid)) {
		if (map_color_string(cfg->val.color[0], &priv->grid_color)) {
			config_color_button_update(&ghidgui->common, conf_get_field("appearance/color/grid"), -1);
			set_special_grid_color();
		}
	}
}

typedef struct {
	int color_set;
	GdkColor color;
	int xor_set;
	GdkColor xor_color;
	double red;
	double green;
	double blue;
} ColorCache;

static void set_gl_color_for_gc(pcb_hid_gc_t gc)
{
	render_priv_t *priv = gport->render_priv;
	static void *cache = NULL;
	static GdkColormap *colormap = NULL;
	pcb_hidval_t cval;
	ColorCache *cc;
	double r, g, b, a;

	if (priv->current_colorname != NULL &&
			strcmp(priv->current_colorname, gc->colorname) == 0 && priv->current_alpha_mult == gc->alpha_mult)
		return;

	free(priv->current_colorname);
	priv->current_colorname = pcb_strdup(gc->colorname);
	priv->current_alpha_mult = gc->alpha_mult;

	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(gport->top_window);
	if (strcmp(gc->colorname, "erase") == 0) {
		r = priv->bg_color.red / 65535.;
		g = priv->bg_color.green / 65535.;
		b = priv->bg_color.blue / 65535.;
		a = 1.0;
	}
	else if (strcmp(gc->colorname, "drill") == 0) {
		r = priv->offlimits_color.red / 65535.;
		g = priv->offlimits_color.green / 65535.;
		b = priv->offlimits_color.blue / 65535.;
		a = conf_core.appearance.drill_alpha;
	}
	else {
		if (pcb_hid_cache_color(0, gc->colorname, &cval, &cache))
			cc = (ColorCache *) cval.ptr;
		else {
			cc = (ColorCache *) malloc(sizeof(ColorCache));
			memset(cc, 0, sizeof(*cc));
			cval.ptr = cc;
			pcb_hid_cache_color(1, gc->colorname, &cval, &cache);
		}

		if (!cc->color_set) {
			if (gdk_color_parse(gc->colorname, &cc->color))
				gdk_color_alloc(colormap, &cc->color);
			else
				gdk_color_white(colormap, &cc->color);
			cc->red = cc->color.red / 65535.;
			cc->green = cc->color.green / 65535.;
			cc->blue = cc->color.blue / 65535.;
			cc->color_set = 1;
		}
		if (gc->xor) {
			if (!cc->xor_set) {
				cc->xor_color.red = cc->color.red ^ priv->bg_color.red;
				cc->xor_color.green = cc->color.green ^ priv->bg_color.green;
				cc->xor_color.blue = cc->color.blue ^ priv->bg_color.blue;
				gdk_color_alloc(colormap, &cc->xor_color);
				cc->red = cc->color.red / 65535.;
				cc->green = cc->color.green / 65535.;
				cc->blue = cc->color.blue / 65535.;
				cc->xor_set = 1;
			}
		}
		r = cc->red;
		g = cc->green;
		b = cc->blue;
		a = conf_core.appearance.layer_alpha;
	}
	if (1) {
		double maxi, mult;
		a *= gc->alpha_mult;
		if (!priv->trans_lines)
			a = 1.0;
		maxi = r;
		if (g > maxi)
			maxi = g;
		if (b > maxi)
			maxi = b;
		mult = MIN(1 / a, 1 / maxi);
#if 1
		r = r * mult;
		g = g * mult;
		b = b * mult;
#endif
	}

	if (!priv->in_context)
		return;

	hidgl_flush_triangles(&buffer);
	glColor4d(r, g, b, a);
}

void ghid_gl_set_color(pcb_hid_gc_t gc, const char *name)
{
	gc->colorname = name;
	set_gl_color_for_gc(gc);
}

void ghid_gl_set_alpha_mult(pcb_hid_gc_t gc, double alpha_mult)
{
	gc->alpha_mult = alpha_mult;
	set_gl_color_for_gc(gc);
}

void ghid_gl_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

void ghid_gl_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}


void ghid_gl_set_draw_xor(pcb_hid_gc_t gc, int xor)
{
	/* NOT IMPLEMENTED */

	/* Only presently called when setting up a crosshair GC.
	 * We manage our own drawing model for that anyway. */
}

void ghid_gl_set_draw_faded(pcb_hid_gc_t gc, int faded)
{
	printf("ghid_gl_set_draw_faded(%p,%d) -- not implemented\n", (void *)gc, faded);
}

void ghid_gl_set_line_cap_angle(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	printf("ghid_gl_set_line_cap_angle() -- not implemented\n");
}

static void ghid_gl_invalidate_current_gc(void)
{
	current_gc = NULL;
}

static int use_gc(pcb_hid_gc_t gc)
{
	if (gc->me_pointer != &gtk2_gl_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to GTK HID\n");
		abort();
	}

	if (current_gc == gc)
		return 1;

	current_gc = gc;

	set_gl_color_for_gc(gc);
	return 1;
}

void ghid_gl_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	USE_GC(gc);

	hidgl_draw_line(gc->cap, gc->width, x1, y1, x2, y2, gport->view.coord_per_px);
}

void ghid_gl_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	USE_GC(gc);

	hidgl_draw_arc(gc->width, cx, cy, xradius, yradius, start_angle, delta_angle, gport->view.coord_per_px);
}

void ghid_gl_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	USE_GC(gc);

	hidgl_draw_rect(x1, y1, x2, y2);
}


void ghid_gl_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	USE_GC(gc);

	hidgl_fill_circle(cx, cy, radius, gport->view.coord_per_px);
}


void ghid_gl_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	USE_GC(gc);

	hidgl_fill_polygon(n_coords, x, y);
}

void ghid_gl_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_polygon_t * poly, const pcb_box_t * clip_box)
{
	USE_GC(gc);

	hidgl_fill_pcb_polygon(poly, clip_box, gport->view.coord_per_px);
}

void ghid_gl_thindraw_pcb_polygon(pcb_hid_gc_t gc, pcb_polygon_t * poly, const pcb_box_t * clip_box)
{
	pcb_dhlp_thindraw_pcb_polygon(gc, poly, clip_box);
	ghid_gl_set_alpha_mult(gc, 0.25);
	ghid_gl_fill_pcb_polygon(gc, poly, clip_box);
	ghid_gl_set_alpha_mult(gc, 1.0);
}

void ghid_gl_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	USE_GC(gc);

	hidgl_fill_rect(x1, y1, x2, y2);
}

void ghid_gl_invalidate_all()
{
	if (ghidgui && ghidgui->topwin.menu.menu_bar)
		ghid_draw_area_update(gport, NULL);
}

void ghid_gl_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
{
	ghid_gl_invalidate_all();
}

void ghid_gl_notify_crosshair_change(pcb_bool changes_complete)
{
	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	/* FIXME: We could just invalidate the bounds of the crosshair attached objects? */
	ghid_gl_invalidate_all();
}

void ghid_gl_notify_mark_change(pcb_bool changes_complete)
{
	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	/* FIXME: We could just invalidate the bounds of the mark? */
	ghid_gl_invalidate_all();
}

static void draw_right_cross(gint x, gint y, gint z)
{
	glVertex3i(x, 0, z);
	glVertex3i(x, PCB->MaxHeight, z);
	glVertex3i(0, y, z);
	glVertex3i(PCB->MaxWidth, y, z);
}

static void draw_slanted_cross(gint x, gint y, gint z)
{
	gint x0, y0, x1, y1;

	x0 = x + (PCB->MaxHeight - y);
	x0 = MAX(0, MIN(x0, PCB->MaxWidth));
	x1 = x - y;
	x1 = MAX(0, MIN(x1, PCB->MaxWidth));
	y0 = y + (PCB->MaxWidth - x);
	y0 = MAX(0, MIN(y0, PCB->MaxHeight));
	y1 = y - x;
	y1 = MAX(0, MIN(y1, PCB->MaxHeight));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x - (PCB->MaxHeight - y);
	x0 = MAX(0, MIN(x0, PCB->MaxWidth));
	x1 = x + y;
	x1 = MAX(0, MIN(x1, PCB->MaxWidth));
	y0 = y + x;
	y0 = MAX(0, MIN(y0, PCB->MaxHeight));
	y1 = y - (PCB->MaxWidth - x);
	y1 = MAX(0, MIN(y1, PCB->MaxHeight));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);
}

static void draw_dozen_cross(gint x, gint y, gint z)
{
	gint x0, y0, x1, y1;
	gdouble tan60 = sqrt(3);

	x0 = x + (PCB->MaxHeight - y) / tan60;
	x0 = MAX(0, MIN(x0, PCB->MaxWidth));
	x1 = x - y / tan60;
	x1 = MAX(0, MIN(x1, PCB->MaxWidth));
	y0 = y + (PCB->MaxWidth - x) * tan60;
	y0 = MAX(0, MIN(y0, PCB->MaxHeight));
	y1 = y - x * tan60;
	y1 = MAX(0, MIN(y1, PCB->MaxHeight));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x + (PCB->MaxHeight - y) * tan60;
	x0 = MAX(0, MIN(x0, PCB->MaxWidth));
	x1 = x - y * tan60;
	x1 = MAX(0, MIN(x1, PCB->MaxWidth));
	y0 = y + (PCB->MaxWidth - x) / tan60;
	y0 = MAX(0, MIN(y0, PCB->MaxHeight));
	y1 = y - x / tan60;
	y1 = MAX(0, MIN(y1, PCB->MaxHeight));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x - (PCB->MaxHeight - y) / tan60;
	x0 = MAX(0, MIN(x0, PCB->MaxWidth));
	x1 = x + y / tan60;
	x1 = MAX(0, MIN(x1, PCB->MaxWidth));
	y0 = y + x * tan60;
	y0 = MAX(0, MIN(y0, PCB->MaxHeight));
	y1 = y - (PCB->MaxWidth - x) * tan60;
	y1 = MAX(0, MIN(y1, PCB->MaxHeight));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x - (PCB->MaxHeight - y) * tan60;
	x0 = MAX(0, MIN(x0, PCB->MaxWidth));
	x1 = x + y * tan60;
	x1 = MAX(0, MIN(x1, PCB->MaxWidth));
	y0 = y + x / tan60;
	y0 = MAX(0, MIN(y0, PCB->MaxHeight));
	y1 = y - (PCB->MaxWidth - x) / tan60;
	y1 = MAX(0, MIN(y1, PCB->MaxHeight));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);
}

static void draw_crosshair(gint x, gint y, gint z)
{
	static enum pcb_crosshair_shape_e prev = pcb_ch_shape_basic;

	draw_right_cross(x, y, z);
	if (prev == pcb_ch_shape_union_jack)
		draw_slanted_cross(x, y, z);
	if (prev == pcb_ch_shape_dozen)
		draw_dozen_cross(x, y, z);
	prev = pcb_crosshair.shape;
}

void ghid_gl_show_crosshair(gboolean paint_new_location)
{
	gint x, y, z;
	static int done_once = 0;
	static GdkColor cross_color;

	if (!paint_new_location)
		return;

	if (!done_once) {
		done_once = 1;
		/* FIXME: when CrossColor changed from config */
		map_color_string(conf_core.appearance.color.cross, &cross_color);
	}
	x = gport->view.crosshair_x;
	y = gport->view.crosshair_y;
	z = 0;

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);

	glColor3f(cross_color.red / 65535., cross_color.green / 65535., cross_color.blue / 65535.);

	if (x >= 0 && paint_new_location) {
		glBegin(GL_LINES);
		draw_crosshair(x, y, z);
		glEnd();
	}

	glDisable(GL_COLOR_LOGIC_OP);
}

void ghid_gl_init_renderer(int *argc, char ***argv, void *vport)
{
	GHidPort * port = vport;
	render_priv_t *priv;

	port->render_priv = priv = g_new0(render_priv_t, 1);

	gtk_gl_init(argc, argv);

	/* setup GL-context */
	priv->glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_STENCIL | GDK_GL_MODE_DOUBLE);
	if (!priv->glconfig) {
		printf("Could not setup GL-context!\n");
		return;											/* Should we abort? */
	}

	/* Setup HID function pointers specific to the GL renderer */
	gtk2_gl_hid.end_layer = ghid_gl_end_layer;
	gtk2_gl_hid.fill_pcb_polygon = ghid_gl_fill_pcb_polygon;
	gtk2_gl_hid.thindraw_pcb_polygon = ghid_gl_thindraw_pcb_polygon;
}

void ghid_gl_shutdown_renderer(void * p)
{
	GHidPort *port = p;

	g_free(port->render_priv);
	port->render_priv = NULL;
}

void ghid_gl_init_drawing_widget(GtkWidget * widget, void * port_)
{
	GHidPort *port = port_;
	render_priv_t *priv = port->render_priv;

	gtk_widget_set_gl_capability(widget, priv->glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);
}

void ghid_gl_drawing_area_configure_hook(void * port)
{
	static int done_once = 0;
	GHidPort *p = port;
	render_priv_t *priv = p->render_priv;

	gport->drawing_allowed = pcb_true;

	if (!done_once) {
		if (!map_color_string(conf_core.appearance.color.background, &priv->bg_color))
			map_color_string("white", &priv->bg_color);

		if (!map_color_string(conf_core.appearance.color.off_limit, &priv->offlimits_color))
			map_color_string("white", &priv->offlimits_color);

		if (!map_color_string(conf_core.appearance.color.grid, &priv->grid_color))
			map_color_string("blue", &priv->grid_color);
		set_special_grid_color();

		done_once = 1;
	}
}

gboolean ghid_gl_start_drawing(GHidPort * port)
{
	GtkWidget *widget = port->drawing_area;
	GdkGLContext *pGlContext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);

	/* make GL-context "current" */
	if (!gdk_gl_drawable_gl_begin(pGlDrawable, pGlContext))
		return FALSE;

	port->render_priv->in_context = pcb_true;

	return TRUE;
}

void ghid_gl_end_drawing(GHidPort * port)
{
	GtkWidget *widget = port->drawing_area;
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);

	if (gdk_gl_drawable_is_double_buffered(pGlDrawable))
		gdk_gl_drawable_swap_buffers(pGlDrawable);
	else
		glFlush();

	port->render_priv->in_context = pcb_false;

	/* end drawing to current GL-context */
	gdk_gl_drawable_gl_end(pGlDrawable);
}

void ghid_gl_screen_update(void)
{
}

#define Z_NEAR 3.0
gboolean ghid_gl_drawing_area_expose_cb(GtkWidget * widget, GdkEventExpose * ev, void *vport)
{
	GHidPort * port = vport;
	render_priv_t *priv = port->render_priv;
	GtkAllocation allocation;
	pcb_hid_expose_ctx_t ctx;

	gtk_widget_get_allocation(widget, &allocation);

	ghid_gl_start_drawing(port);

	hidgl_init();

	/* If we don't have any stencil bits available,
	   we can't use the hidgl polygon drawing routine */
	/* TODO: We could use the GLU tessellator though */
	if (hidgl_stencil_bits() == 0)
		gtk2_gl_hid.fill_pcb_polygon = pcb_dhlp_fill_pcb_polygon;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, allocation.width, allocation.height);

	glEnable(GL_SCISSOR_TEST);
	glScissor(ev->area.x, allocation.height - ev->area.height - ev->area.y, ev->area.width, ev->area.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, allocation.width, allocation.height, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -Z_NEAR);

	glScalef((conf_core.editor.view.flip_x ? -1. : 1.) / port->view.coord_per_px,
					 (conf_core.editor.view.flip_y ? -1. : 1.) / port->view.coord_per_px,
					 ((conf_core.editor.view.flip_x == conf_core.editor.view.flip_y) ? 1. : -1.) / port->view.coord_per_px);
	glTranslatef(conf_core.editor.view.flip_x ? port->view.x0 - PCB->MaxWidth :
							 -port->view.x0, conf_core.editor.view.flip_y ? port->view.y0 - PCB->MaxHeight : -port->view.y0, 0);

	glEnable(GL_STENCIL_TEST);
	glClearColor(priv->offlimits_color.red / 65535.,
							 priv->offlimits_color.green / 65535., priv->offlimits_color.blue / 65535., 1.);
	glStencilMask(~0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	hidgl_reset_stencil_usage();

	/* Disable the stencil test until we need it - otherwise it gets dirty */
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0);
	glStencilFunc(GL_ALWAYS, 0, 0);

	ctx.view.X1 = MIN(Px(ev->area.x), Px(ev->area.x + ev->area.width + 1));
	ctx.view.X2 = MAX(Px(ev->area.x), Px(ev->area.x + ev->area.width + 1));
	ctx.view.Y1 = MIN(Py(ev->area.y), Py(ev->area.y + ev->area.height + 1));
	ctx.view.Y2 = MAX(Py(ev->area.y), Py(ev->area.y + ev->area.height + 1));

	ctx.view.X1 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X1));
	ctx.view.X2 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X2));
	ctx.view.Y1 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y1));
	ctx.view.Y2 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y2));

	glColor3f(priv->bg_color.red / 65535., priv->bg_color.green / 65535., priv->bg_color.blue / 65535.);

	glBegin(GL_QUADS);
	glVertex3i(0, 0, 0);
	glVertex3i(PCB->MaxWidth, 0, 0);
	glVertex3i(PCB->MaxWidth, PCB->MaxHeight, 0);
	glVertex3i(0, PCB->MaxHeight, 0);
	glEnd();

	ghid_gl_draw_bg_image();

	hidgl_init_triangle_array(&buffer);
	ghid_gl_invalidate_current_gc();
	pcb_hid_expose_all(&gtk2_gl_hid, &ctx);
	hidgl_flush_triangles(&buffer);

	ghid_gl_draw_grid(&ctx.view);

	ghid_gl_invalidate_current_gc();

	pcb_draw_attached();
	pcb_draw_mark();
	hidgl_flush_triangles(&buffer);

	ghid_gl_show_crosshair(TRUE);

	hidgl_flush_triangles(&buffer);

	draw_lead_user(priv);

	ghid_gl_end_drawing(port);

	return FALSE;
}

/* This realize callback is used to work around a crash bug in some mesa
 * versions (observed on a machine running the intel i965 driver. It isn't
 * obvious why it helps, but somehow fiddling with the GL context here solves
 * the issue. The problem appears to have been fixed in recent mesa versions.
 */
void ghid_gl_port_drawing_realize_cb(GtkWidget * widget, gpointer data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		return;

	gdk_gl_drawable_gl_end(gldrawable);
	return;
}

gboolean ghid_gl_preview_expose(GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx)
{
	GdkWindow *window = gtk_widget_get_window(widget);
	GdkGLContext *pGlContext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);
	GtkAllocation allocation;
	render_priv_t *priv = gport->render_priv;
	pcb_gtk_view_t save_view;
	int save_width, save_height;
	double xz, yz, vw, vh;

	vw = ctx->view.X2 - ctx->view.X1;
	vh = ctx->view.Y2 - ctx->view.Y1;

	save_view = gport->view;
	save_width = gport->view.canvas_width;
	save_height = gport->view.canvas_height;

	/* Setup zoom factor for drawing routines */

	gtk_widget_get_allocation(widget, &allocation);
	xz = vw / (double) allocation.width;
	yz = vh / (double) allocation.height;

	if (xz > yz)
		gport->view.coord_per_px = xz;
	else
		gport->view.coord_per_px = yz;

	gport->view.canvas_width = allocation.width;
	gport->view.canvas_height = allocation.height;
	gport->view.width = allocation.width * gport->view.coord_per_px;
	gport->view.height = allocation.height * gport->view.coord_per_px;
	gport->view.x0 = (vw - gport->view.width) / 2 + ctx->view.X1;
	gport->view.y0 = (vh - gport->view.height) / 2 + ctx->view.Y1;

	/* make GL-context "current" */
	if (!gdk_gl_drawable_gl_begin(pGlDrawable, pGlContext)) {
		return FALSE;
	}
	gport->render_priv->in_context = pcb_true;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, allocation.width, allocation.height);

	glEnable(GL_SCISSOR_TEST);
	if (ev)
		glScissor(ev->area.x, allocation.height - ev->area.height - ev->area.y, ev->area.width, ev->area.height);
	else
		glScissor(0, 0, allocation.width, allocation.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, allocation.width, allocation.height, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -Z_NEAR);

	glEnable(GL_STENCIL_TEST);
	glClearColor(priv->bg_color.red / 65535., priv->bg_color.green / 65535., priv->bg_color.blue / 65535., 1.);
	glStencilMask(~0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	hidgl_reset_stencil_usage();
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0);
	glStencilFunc(GL_ALWAYS, 0, 0);

	/* call the drawing routine */
	hidgl_init_triangle_array(&buffer);
	ghid_gl_invalidate_current_gc();
	glPushMatrix();
	glScalef((conf_core.editor.view.flip_x ? -1. : 1.) / gport->view.coord_per_px,
					 (conf_core.editor.view.flip_y ? -1. : 1.) / gport->view.coord_per_px, 1);
	glTranslatef(conf_core.editor.view.flip_x ? gport->view.x0 - PCB->MaxWidth :
							 -gport->view.x0, conf_core.editor.view.flip_y ? gport->view.y0 - PCB->MaxHeight : -gport->view.y0, 0);

	expcall(&gtk2_gl_hid, ctx);

	hidgl_flush_triangles(&buffer);
	glPopMatrix();

	if (gdk_gl_drawable_is_double_buffered(pGlDrawable))
		gdk_gl_drawable_swap_buffers(pGlDrawable);
	else
		glFlush();

	/* end drawing to current GL-context */
	gport->render_priv->in_context = pcb_false;
	gdk_gl_drawable_gl_end(pGlDrawable);

	gport->view = save_view;
	gport->view.canvas_width = save_width;
	gport->view.canvas_height = save_height;

	return FALSE;
}

/*GdkPixmap*/void *ghid_gl_render_pixmap(int cx, int cy, double zoom, int width, int height, int depth)
{
	GdkGLConfig *glconfig;
	GdkPixmap *pixmap;
	GdkGLPixmap *glpixmap;
	GdkGLContext *glcontext;
	GdkGLDrawable *gldrawable;
	render_priv_t *priv = gport->render_priv;
	pcb_gtk_view_t save_view;
	int save_width, save_height;
	pcb_hid_expose_ctx_t ctx;

	save_view = gport->view;
	save_width = gport->view.canvas_width;
	save_height = gport->view.canvas_height;

	/* Setup rendering context for drawing routines
	 */

	glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB | GDK_GL_MODE_STENCIL | GDK_GL_MODE_SINGLE);

	pixmap = gdk_pixmap_new(NULL, width, height, depth);
	glpixmap = gdk_pixmap_set_gl_capability(pixmap, glconfig, NULL);
	gldrawable = GDK_GL_DRAWABLE(glpixmap);
	glcontext = gdk_gl_context_new(gldrawable, NULL, TRUE, GDK_GL_RGBA_TYPE);

	/* Setup zoom factor for drawing routines */

	gport->view.coord_per_px = zoom;
	gport->view.canvas_width = width;
	gport->view.canvas_height = height;
	gport->view.width = width * gport->view.coord_per_px;
	gport->view.height = height * gport->view.coord_per_px;
	gport->view.x0 = conf_core.editor.view.flip_x ? PCB->MaxWidth - cx : cx;
	gport->view.x0 -= gport->view.height / 2;
	gport->view.y0 = conf_core.editor.view.flip_y ? PCB->MaxHeight - cy : cy;
	gport->view.y0 -= gport->view.width / 2;

	/* make GL-context "current" */
	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext)) {
		return NULL;
	}
	gport->render_priv->in_context = pcb_true;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, width, height);

	glEnable(GL_SCISSOR_TEST);
	glScissor(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -Z_NEAR);

	glClearColor(priv->bg_color.red / 65535., priv->bg_color.green / 65535., priv->bg_color.blue / 65535., 1.);
	glStencilMask(~0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	hidgl_reset_stencil_usage();

	/* call the drawing routine */
	hidgl_init_triangle_array(&buffer);
	ghid_gl_invalidate_current_gc();
	glPushMatrix();
	glScalef((conf_core.editor.view.flip_x ? -1. : 1.) / gport->view.coord_per_px,
					 (conf_core.editor.view.flip_y ? -1. : 1.) / gport->view.coord_per_px, 1);
	glTranslatef(conf_core.editor.view.flip_x ? gport->view.x0 - PCB->MaxWidth :
		     -gport->view.x0,
		     conf_core.editor.view.flip_y ? gport->view.y0 - PCB->MaxHeight : -gport->view.y0, 0);

	ctx.view.X1 = MIN(Px(0), Px(gport->view.canvas_width + 1));
	ctx.view.Y1 = MIN(Py(0), Py(gport->view.canvas_height + 1));
	ctx.view.X2 = MAX(Px(0), Px(gport->view.canvas_width + 1));
	ctx.view.Y2 = MAX(Py(0), Py(gport->view.canvas_height + 1));

	ctx.view.X1 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X1));
	ctx.view.X2 = MAX(0, MIN(PCB->MaxWidth, ctx.view.X2));
	ctx.view.Y1 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y1));
	ctx.view.Y2 = MAX(0, MIN(PCB->MaxHeight, ctx.view.Y2));

	pcb_hid_expose_all(&gtk2_gl_hid, &ctx);
	hidgl_flush_triangles(&buffer);
	glPopMatrix();

	glFlush();

	/* end drawing to current GL-context */
	gport->render_priv->in_context = pcb_false;
	gdk_gl_drawable_gl_end(gldrawable);

	gdk_pixmap_unset_gl_capability(pixmap);

	g_object_unref(glconfig);
	g_object_unref(glcontext);

	gport->view = save_view;
	gport->view.canvas_width = save_width;
	gport->view.canvas_height = save_height;

	return pixmap;
}

pcb_hid_t *ghid_gl_request_debug_draw(void)
{
	GHidPort *port = gport;
	GtkWidget *widget = port->drawing_area;
	GtkAllocation allocation;

	gtk_widget_get_allocation(widget, &allocation);

	ghid_gl_start_drawing(port);

	glViewport(0, 0, allocation.width, allocation.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, allocation.width, allocation.height, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -Z_NEAR);

	hidgl_init_triangle_array(&buffer);
	ghid_gl_invalidate_current_gc();

	/* Setup stenciling */
	glDisable(GL_STENCIL_TEST);

	glPushMatrix();
	glScalef((conf_core.editor.view.flip_x ? -1. : 1.) / port->view.coord_per_px,
					 (conf_core.editor.view.flip_y ? -1. : 1.) / port->view.coord_per_px, (conf_core.editor.view.flip_x == conf_core.editor.view.flip_y) ? 1. : -1.);
	glTranslatef(conf_core.editor.view.flip_x ? port->view.x0 - PCB->MaxWidth :
							 -port->view.x0, conf_core.editor.view.flip_y ? port->view.y0 - PCB->MaxHeight : -port->view.y0, 0);

	return &gtk2_gl_hid;
}

void ghid_gl_flush_debug_draw(void)
{
	GtkWidget *widget = gport->drawing_area;
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);

	hidgl_flush_triangles(&buffer);

	if (gdk_gl_drawable_is_double_buffered(pGlDrawable))
		gdk_gl_drawable_swap_buffers(pGlDrawable);
	else
		glFlush();
}

void ghid_gl_finish_debug_draw(void)
{
	hidgl_flush_triangles(&buffer);
	glPopMatrix();

	ghid_gl_end_drawing(gport);
}

static void draw_lead_user(render_priv_t * priv)
{
	int i;
	pcb_lead_user_t *lead_user = &gport->lead_user;
	double radius = lead_user->radius;
	double width = PCB_MM_TO_COORD(LEAD_USER_WIDTH);
	double separation = PCB_MM_TO_COORD(LEAD_USER_ARC_SEPARATION);

	if (!lead_user->lead_user)
		return;

	glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);
	glColor3f(LEAD_USER_COLOR_R, LEAD_USER_COLOR_G, LEAD_USER_COLOR_B);


	/* arcs at the approrpriate radii */

	for (i = 0; i < LEAD_USER_ARC_COUNT; i++, radius -= separation) {
		if (radius < width)
			radius += PCB_MM_TO_COORD(LEAD_USER_INITIAL_RADIUS);

		/* Draw an arc at radius */
		hidgl_draw_arc(width, lead_user->x, lead_user->y, radius, radius, 0, 360, gport->view.coord_per_px);
	}

	hidgl_flush_triangles(&buffer);
	glPopAttrib();
}

void ghid_gl_install(pcb_gtk_common_t *common, pcb_hid_t *hid)
{
	if (common != NULL) {
	common->render_pixmap = ghid_gl_render_pixmap;
	common->init_drawing_widget = ghid_gl_init_drawing_widget;
	common->drawing_realize = ghid_gl_port_drawing_realize_cb;
	common->drawing_area_expose = ghid_gl_drawing_area_expose_cb;
	common->preview_expose = ghid_gl_preview_expose;
	common->invalidate_all = ghid_gl_invalidate_all;
	common->set_special_colors = ghid_gl_set_special_colors;
	common->init_renderer = ghid_gl_init_renderer;
	common->screen_update = ghid_gl_screen_update;
	common->draw_grid_local = ghid_gl_draw_grid_local;
	common->drawing_area_configure_hook = ghid_gl_drawing_area_configure_hook;
	common->shutdown_renderer = ghid_gl_shutdown_renderer;
	common->get_color_name = get_color_name;
	common->map_color_string = map_color_string;
	}

	if (hid != NULL) {
	hid->invalidate_lr = ghid_gl_invalidate_lr;
	hid->invalidate_all = ghid_gl_invalidate_all;
	hid->notify_crosshair_change = ghid_gl_notify_crosshair_change;
	hid->notify_mark_change = ghid_gl_notify_mark_change;
	hid->set_layer_group = ghid_gl_set_layer_group;
	hid->make_gc = ghid_gl_make_gc;
	hid->destroy_gc = ghid_gl_destroy_gc;
	hid->use_mask = ghid_gl_use_mask;
	hid->set_color = ghid_gl_set_color;
	hid->set_line_cap = ghid_gl_set_line_cap;
	hid->set_line_width = ghid_gl_set_line_width;
	hid->set_draw_xor = ghid_gl_set_draw_xor;
	hid->draw_line = ghid_gl_draw_line;
	hid->draw_arc = ghid_gl_draw_arc;
	hid->draw_rect = ghid_gl_draw_rect;
	hid->fill_circle = ghid_gl_fill_circle;
	hid->fill_polygon = ghid_gl_fill_polygon;
	hid->fill_rect = ghid_gl_fill_rect;

	hid->request_debug_draw = ghid_gl_request_debug_draw;
	hid->flush_debug_draw = ghid_gl_flush_debug_draw;
	hid->finish_debug_draw = ghid_gl_finish_debug_draw;
	}
}


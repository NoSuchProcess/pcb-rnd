#include "config.h"

#include <stdio.h>

#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "layer.h"
#include "hid_draw_helpers.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hidlib_conf.h"
#include "funchash_core.h"

#include "../src_plugins/lib_hid_common/clip.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

#include "../src_plugins/lib_gtk_hid/gui.h"
#include "../src_plugins/lib_gtk_hid/coord_conv.h"
#include "../src_plugins/lib_gtk_hid/render.h"
#include "../src_plugins/lib_gtk_hid/preview_helper.h"

#include "../src_plugins/lib_hid_gl/opengl.h"
#include "../src_plugins/lib_hid_gl/draw_gl.h"
#include <gtk/gtkgl.h>
#include "../src_plugins/lib_hid_gl/hidgl.h"
#include "hid_draw_helpers.h"
#include "../src_plugins/lib_hid_gl/stencil_gl.h"

#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"

#define Z_NEAR 3.0
extern pcb_hid_t gtk2_gl_hid;

static pcb_hid_gc_t current_gc = NULL;
/* Sets gport->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static pcb_coord_t grid_local_x = 0, grid_local_y = 0, grid_local_radius = 0;

/*static int cur_mask = -1;*/

typedef struct render_priv_s {
	GdkGLConfig *glconfig;
	pcb_gtk_color_t bg_color;
	pcb_gtk_color_t offlimits_color;
	pcb_gtk_color_t grid_color;
	pcb_bool trans_lines;
	pcb_bool in_context;
	int subcomposite_stencil_bit;
	char *current_colorname;
	double current_alpha_mult;
} render_priv_t;


typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	pcb_hid_t *me_pointer;

	const pcb_color_t *pcolor;
	double alpha_mult;
	pcb_coord_t width;
	gchar xor;
} hid_gc_s;

void ghid_gl_render_burst(pcb_burst_op_t op, const pcb_box_t *screen)
{
	pcb_gui->coord_per_pix = gport->view.coord_per_px;
}

static const gchar *get_color_name(pcb_gtk_color_t *color)
{
	static char tmp[16];

	if (!color)
		return "#000000";

	sprintf(tmp, "#%2.2x%2.2x%2.2x", (color->red >> 8) & 0xff, (color->green >> 8) & 0xff, (color->blue >> 8) & 0xff);
	return tmp;
}

/* Returns TRUE only if color_string has been allocated to color. */
static pcb_bool map_color_string(const char *color_string, pcb_gtk_color_t *color)
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


#if 0
static void start_subcomposite(void)
{
	render_priv_t *priv = gport->render_priv;
	int stencil_bit;

	/* Flush out any existing geometry to be rendered */
	hidgl_flush_triangles();

	glEnable(GL_STENCIL_TEST); /* Enable Stencil test */
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); /* Stencil pass => replace stencil value (with 1) */

	stencil_bit = hidgl_assign_clear_stencil_bit(); /* Get a new (clean) bitplane to stencil with */
	glStencilMask(stencil_bit); /* Only write to our subcompositing stencil bitplane */
	glStencilFunc(GL_GREATER, stencil_bit, stencil_bit); /* Pass stencil test if our assigned bit is clear */

	priv->subcomposite_stencil_bit = stencil_bit;
}

static void end_subcomposite(void)
{
	render_priv_t *priv = gport->render_priv;

	/* Flush out any existing geometry to be rendered */
	hidgl_flush_triangles();

	hidgl_return_stencil_bit(priv->subcomposite_stencil_bit); /* Relinquish any bitplane we previously used */

	glStencilMask(0);
	glStencilFunc(GL_ALWAYS, 0, 0); /* Always pass stencil test */
	glDisable(GL_STENCIL_TEST); /* Disable Stencil test */

	priv->subcomposite_stencil_bit = 0;
}
#endif

int ghid_gl_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	render_priv_t *priv = gport->render_priv;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -Z_NEAR);

	glScalef((pcbhl_conf.editor.view.flip_x ? -1. : 1.) / gport->view.coord_per_px, (pcbhl_conf.editor.view.flip_y ? -1. : 1.) / gport->view.coord_per_px, ((pcbhl_conf.editor.view.flip_x == pcbhl_conf.editor.view.flip_y) ? 1. : -1.) / gport->view.coord_per_px);
	glTranslatef(pcbhl_conf.editor.view.flip_x ? gport->view.x0 - PCB->hidlib.size_x : -gport->view.x0, pcbhl_conf.editor.view.flip_y ? gport->view.y0 - PCB->hidlib.size_y : -gport->view.y0, 0);

	/* Put the renderer into a good state so that any drawing is done in standard mode */

	drawgl_flush();
	drawgl_reset();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	priv->trans_lines = pcb_true;
	return 1;
}

static void ghid_gl_end_layer(void)
{
	drawgl_flush();
	drawgl_reset();
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
	rv->pcolor = &pcbhl_conf.appearance.color.background;
	rv->alpha_mult = 1.0;
	return rv;
}

void ghid_gl_draw_grid_local(pcb_coord_t cx, pcb_coord_t cy)
{
	/* cx and cy are the actual cursor snapped to wherever - round them to the nearest real grid point */
	grid_local_x = (cx / PCB->hidlib.grid) * PCB->hidlib.grid + PCB->hidlib.grid_ox;
	grid_local_y = (cy / PCB->hidlib.grid) * PCB->hidlib.grid + PCB->hidlib.grid_oy;
	grid_local_radius = conf_hid_gtk.plugins.hid_gtk.local_grid.radius;
}

static void ghid_gl_draw_grid(pcb_box_t *drawn_area)
{
	render_priv_t *priv = gport->render_priv;

	if ((Vz(PCB->hidlib.grid) < PCB_MIN_GRID_DISTANCE) || (!pcbhl_conf.editor.draw_grid))
		return;

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);

	glColor3f(priv->grid_color.red / 65535., priv->grid_color.green / 65535., priv->grid_color.blue / 65535.);

	if (conf_hid_gtk.plugins.hid_gtk.local_grid.enable)
		hidgl_draw_local_grid(grid_local_x, grid_local_y, grid_local_radius);
	else
		hidgl_draw_grid(drawn_area);

	glDisable(GL_COLOR_LOGIC_OP);
}

void pcb_gl_draw_texture(GLuint texture_handle)
{
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
	glVertex3i(PCB->hidlib.size_x, 0, 0);
	glTexCoord2d(1., 1.);
	glVertex3i(PCB->hidlib.size_x, PCB->hidlib.size_y, 0);
	glTexCoord2d(0., 1.);
	glVertex3i(0, PCB->hidlib.size_y, 0);
	glEnd();

	glDisable(GL_TEXTURE_2D);
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

		/* XXX: We should probably determine what the maximum texture supported is,
		 *      and if our image is larger, shrink it down using GDK pixbuf routines
		 *      rather than having it fail below.
		 */

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);
	}

	pcb_gl_draw_texture(texture_handle);
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

	if (((CFT_COLOR *) cfg->val.color == &pcbhl_conf.appearance.color.background)) {
		if (map_color_string(cfg->val.color[0].str, &priv->bg_color))
			set_special_grid_color();
	}
	else if (((CFT_COLOR *) cfg->val.color == &pcbhl_conf.appearance.color.grid)) {
		if (map_color_string(cfg->val.color[0].str, &priv->grid_color))
			set_special_grid_color();
	}
TODO("set the offlimits color or place a comment here why it is not set; see the gdk version");
}

typedef struct {
	int color_set;
	pcb_gtk_color_t color;
	int xor_set;
	pcb_gtk_color_t xor_color;
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

	if (gc->pcolor == NULL) {
		fprintf(stderr, "set_gl_color_for_gc:  gc->colorname = NULL, setting to magenta\n");
		gc->pcolor = pcb_color_magenta;
	}

	if (priv->current_colorname != NULL && strcmp(priv->current_colorname, gc->pcolor->str) == 0 && priv->current_alpha_mult == gc->alpha_mult)
		return;

	free(priv->current_colorname);
	priv->current_colorname = pcb_strdup(gc->pcolor->str);
	priv->current_alpha_mult = gc->alpha_mult;

	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(gport->top_window);
	TODO("color: Do not depend on manual strcmp here - use pcb_color_is_drill()");
	if (pcb_color_is_drill(gc->pcolor)) {
		r = priv->offlimits_color.red / 65535.;
		g = priv->offlimits_color.green / 65535.;
		b = priv->offlimits_color.blue / 65535.;
		a = pcbhl_conf.appearance.drill_alpha;
	}
	else {
		if (pcb_hid_cache_color(0, gc->pcolor->str, &cval, &cache))
			cc = (ColorCache *) cval.ptr;
		else {
			cc = (ColorCache *) malloc(sizeof(ColorCache));
			memset(cc, 0, sizeof(*cc));
			cval.ptr = cc;
			pcb_hid_cache_color(1, gc->pcolor->str, &cval, &cache);
		}

		if (!cc->color_set) {
			if (gdk_color_parse(gc->pcolor->str, &cc->color))
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
		a = pcbhl_conf.appearance.layer_alpha;
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


	/* We need to flush the draw buffer when changing colour so that new primitives
	 * don't get merged. This actually isn't a problem with the colour but due to 
	 * way that the final render pass iterates through the primitive buffer in 
	 * reverse order. If the new primitives are merged with previous ones then they
	 * will be drawn in the wrong order.
	 */
	drawgl_flush();

	drawgl_set_colour(r, g, b, a);
}

void ghid_gl_set_color(pcb_hid_gc_t gc, const pcb_color_t *color)
{
	if (color == NULL) {
		fprintf(stderr, "ghid_gl_set_color():  name = NULL, setting to magenta\n");
		color = pcb_color_magenta;
	}

	gc->pcolor = color;
	set_gl_color_for_gc(gc);
}

void ghid_gl_set_alpha_mult(pcb_hid_gc_t gc, double alpha_mult)
{
	gc->alpha_mult = alpha_mult;
	set_gl_color_for_gc(gc);
}

void ghid_gl_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
}

void ghid_gl_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width < 0 ? (-width) * gport->view.coord_per_px : width;
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


static void ghid_gl_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	USE_GC(gc);

	hidgl_draw_line(gc->core_gc.cap, gc->width, x1, y1, x2, y2, gport->view.coord_per_px);
}

static void ghid_gl_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	USE_GC(gc);

	hidgl_draw_arc(gc->width, cx, cy, xradius, yradius, start_angle, delta_angle, gport->view.coord_per_px);
}

static void ghid_gl_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	USE_GC(gc);

	hidgl_draw_rect(x1, y1, x2, y2);
}


static void ghid_gl_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	USE_GC(gc);

	hidgl_fill_circle(cx, cy, radius, gport->view.coord_per_px);
}


static void ghid_gl_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	USE_GC(gc);

	hidgl_fill_polygon(n_coords, x, y);
}

static void ghid_gl_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	USE_GC(gc);

	hidgl_fill_polygon_offs(n_coords, x, y, dx, dy);
}

static void ghid_gl_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t *poly, const pcb_box_t *clip_box)
{
	USE_GC(gc);

	hidgl_fill_pcb_polygon(poly, clip_box, gport->view.coord_per_px);
}

static void ghid_gl_thindraw_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t *poly, const pcb_box_t *clip_box)
{
	pcb_dhlp_thindraw_pcb_polygon(gc, poly, clip_box);
	/* Disable thindraw poly filling until it is fixed. The poly fill overwrites lines and
	 * arcs that are drawn underneath.
	 * 
	 ghid_gl_set_alpha_mult(gc, 0.25);
	 ghid_gl_fill_pcb_polygon(gc, poly, clip_box);
	 ghid_gl_set_alpha_mult(gc, 1.0);
	 */
}

static void ghid_gl_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
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

static void ghid_gl_notify_crosshair_change(pcb_bool changes_complete)
{
	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	/* FIXME: We could just invalidate the bounds of the crosshair attached objects? */
	ghid_gl_invalidate_all();
}

static void ghid_gl_notify_mark_change(pcb_bool changes_complete)
{
	/* We sometimes get called before the GUI is up */
	if (gport->drawing_area == NULL)
		return;

	/* FIXME: We could just invalidate the bounds of the mark? */
	ghid_gl_invalidate_all();
}

static void pcb_gl_draw_right_cross(GLint x, GLint y, GLint z)
{
	glVertex3i(x, 0, z);
	glVertex3i(x, PCB->hidlib.size_y, z);
	glVertex3i(0, y, z);
	glVertex3i(PCB->hidlib.size_x, y, z);
}

static void pcb_gl_draw_slanted_cross(GLint x, GLint y, GLint z)
{
	GLint x0, y0, x1, y1;

	x0 = x + (PCB->hidlib.size_y - y);
	x0 = MAX(0, MIN(x0, PCB->hidlib.size_x));
	x1 = x - y;
	x1 = MAX(0, MIN(x1, PCB->hidlib.size_x));
	y0 = y + (PCB->hidlib.size_x - x);
	y0 = MAX(0, MIN(y0, PCB->hidlib.size_y));
	y1 = y - x;
	y1 = MAX(0, MIN(y1, PCB->hidlib.size_y));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x - (PCB->hidlib.size_y - y);
	x0 = MAX(0, MIN(x0, PCB->hidlib.size_x));
	x1 = x + y;
	x1 = MAX(0, MIN(x1, PCB->hidlib.size_x));
	y0 = y + x;
	y0 = MAX(0, MIN(y0, PCB->hidlib.size_y));
	y1 = y - (PCB->hidlib.size_x - x);
	y1 = MAX(0, MIN(y1, PCB->hidlib.size_y));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);
}

static void pcb_gl_draw_dozen_cross(GLint x, GLint y, GLint z)
{
	GLint x0, y0, x1, y1;
	gdouble tan60 = sqrt(3);

	x0 = x + (PCB->hidlib.size_y - y) / tan60;
	x0 = MAX(0, MIN(x0, PCB->hidlib.size_x));
	x1 = x - y / tan60;
	x1 = MAX(0, MIN(x1, PCB->hidlib.size_x));
	y0 = y + (PCB->hidlib.size_x - x) * tan60;
	y0 = MAX(0, MIN(y0, PCB->hidlib.size_y));
	y1 = y - x * tan60;
	y1 = MAX(0, MIN(y1, PCB->hidlib.size_y));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x + (PCB->hidlib.size_y - y) * tan60;
	x0 = MAX(0, MIN(x0, PCB->hidlib.size_x));
	x1 = x - y * tan60;
	x1 = MAX(0, MIN(x1, PCB->hidlib.size_x));
	y0 = y + (PCB->hidlib.size_x - x) / tan60;
	y0 = MAX(0, MIN(y0, PCB->hidlib.size_y));
	y1 = y - x / tan60;
	y1 = MAX(0, MIN(y1, PCB->hidlib.size_y));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x - (PCB->hidlib.size_y - y) / tan60;
	x0 = MAX(0, MIN(x0, PCB->hidlib.size_x));
	x1 = x + y / tan60;
	x1 = MAX(0, MIN(x1, PCB->hidlib.size_x));
	y0 = y + x * tan60;
	y0 = MAX(0, MIN(y0, PCB->hidlib.size_y));
	y1 = y - (PCB->hidlib.size_x - x) * tan60;
	y1 = MAX(0, MIN(y1, PCB->hidlib.size_y));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);

	x0 = x - (PCB->hidlib.size_y - y) * tan60;
	x0 = MAX(0, MIN(x0, PCB->hidlib.size_x));
	x1 = x + y * tan60;
	x1 = MAX(0, MIN(x1, PCB->hidlib.size_x));
	y0 = y + x / tan60;
	y0 = MAX(0, MIN(y0, PCB->hidlib.size_y));
	y1 = y - (PCB->hidlib.size_x - x) / tan60;
	y1 = MAX(0, MIN(y1, PCB->hidlib.size_y));
	glVertex3i(x0, y0, z);
	glVertex3i(x1, y1, z);
}

void pcb_gl_draw_crosshair(GLint x, GLint y, GLint z)
{
	static enum pcb_crosshair_shape_e prev = pcb_ch_shape_basic;

	if (gport->view.crosshair_x < 0 || !ghidgui->topwin.active || !gport->view.has_entered)
		return;

	pcb_gl_draw_right_cross(x, y, z);
	if (prev == pcb_ch_shape_union_jack)
		pcb_gl_draw_slanted_cross(x, y, z);
	if (prev == pcb_ch_shape_dozen)
		pcb_gl_draw_dozen_cross(x, y, z);
	prev = pcb_crosshair.shape;
}

static void ghid_gl_show_crosshair(gboolean paint_new_location)
{
	GLint x, y, z;
	static int done_once = 0;
	static pcb_gtk_color_t cross_color;

	if (!paint_new_location)
		return;

	if (!done_once) {
		done_once = 1;
		/* FIXME: when CrossColor changed from config */
		map_color_string(pcbhl_conf.appearance.color.cross.str, &cross_color);
	}
	x = gport->view.crosshair_x;
	y = gport->view.crosshair_y;
	z = 0;

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);

	glColor3f(cross_color.red / 65535., cross_color.green / 65535., cross_color.blue / 65535.);

	if (x >= 0 && paint_new_location) {
		glBegin(GL_LINES);
		pcb_gl_draw_crosshair(x, y, z);
		glEnd();
	}

	glDisable(GL_COLOR_LOGIC_OP);
}

static void ghid_gl_init_renderer(int *argc, char ***argv, void *vport)
{
	GHidPort *port = vport;
	render_priv_t *priv;

	port->render_priv = priv = g_new0(render_priv_t, 1);

	gtk_gl_init(argc, argv);

	/* setup GL-context */
	priv->glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_STENCIL | GDK_GL_MODE_DOUBLE);
	if (!priv->glconfig) {
		printf("Could not setup GL-context!\n");
		return; /* Should we abort? */
	}

	/* Setup HID function pointers specific to the GL renderer */
	gtk2_gl_hid.end_layer = ghid_gl_end_layer;
	gtk2_gl_hid.fill_pcb_polygon = ghid_gl_fill_pcb_polygon;
	gtk2_gl_hid.thindraw_pcb_polygon = ghid_gl_thindraw_pcb_polygon;
}

static void ghid_gl_shutdown_renderer(void *p)
{
	GHidPort *port = p;

	g_free(port->render_priv);
	port->render_priv = NULL;
}

static void ghid_gl_init_drawing_widget(GtkWidget *widget, void *port_)
{
	GHidPort *port = port_;
	render_priv_t *priv = port->render_priv;

	gtk_widget_set_gl_capability(widget, priv->glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);
}

static void ghid_gl_drawing_area_configure_hook(void *port)
{
	static int done_once = 0;
	GHidPort *p = port;
	render_priv_t *priv = p->render_priv;

	gport->drawing_allowed = pcb_true;

	if (!done_once) {
		if (!map_color_string(pcbhl_conf.appearance.color.background.str, &priv->bg_color))
			map_color_string("white", &priv->bg_color);

		if (!map_color_string(pcbhl_conf.appearance.color.off_limit.str, &priv->offlimits_color))
			map_color_string("white", &priv->offlimits_color);

		if (!map_color_string(pcbhl_conf.appearance.color.grid.str, &priv->grid_color))
			map_color_string("blue", &priv->grid_color);
		set_special_grid_color();

		done_once = 1;
	}
}

static gboolean ghid_gl_start_drawing(GHidPort *port)
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

static void ghid_gl_end_drawing(GHidPort *port)
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

static void ghid_gl_screen_update(void)
{
}

/* Settles background color + inital GL configuration, to allow further drawing in GL area.
    (w, h) describes the total area concerned, while (xr, yr, wr, hr) describes area requested by an expose event.
    The color structure holds the wanted solid back-ground color, used to first paint the exposed drawing area.
 */
static void pcb_gl_draw_expose_init(pcb_hid_t *hid, int w, int h, int xr, int yr, int wr, int hr, pcb_gl_color_t *bg_c)
{
	hidgl_init();

	/* If we don't have any stencil bits available,
	   we can't use the hidgl polygon drawing routine */
	/* TODO: We could use the GLU tessellator though */
	if (stencilgl_bit_count() == 0)
		hid->fill_pcb_polygon = pcb_dhlp_fill_pcb_polygon;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, w, h);

	glEnable(GL_SCISSOR_TEST);
	glScissor(xr, yr, wr, hr);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -Z_NEAR);

	glEnable(GL_STENCIL_TEST);
	glClearColor(bg_c->red, bg_c->green, bg_c->blue, 1.);
	glStencilMask(~0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	stencilgl_reset_stencil_usage();

	/* Disable the stencil test until we need it - otherwise it gets dirty */
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0);
	glStencilFunc(GL_ALWAYS, 0, 0);
}

/* alpha component is not part of GdkColor structure. Can't derive it from GTK2 pcb_gtk_color_t */
static void gtk2gl_color(pcb_gl_color_t *gl_c, pcb_gtk_color_t *gtk_c)
{
	gl_c->red = gtk_c->red / 65535.;
	gl_c->green = gtk_c->green / 65535.;
	gl_c->blue = gtk_c->blue / 65535.;
}

static gboolean ghid_gl_drawing_area_expose_cb(GtkWidget *widget, pcb_gtk_expose_t *ev, void *vport)
{
	GHidPort *port = vport;
	render_priv_t *priv = port->render_priv;
	GtkAllocation allocation;
	pcb_hid_expose_ctx_t ctx;
	pcb_gl_color_t off_c, bg_c;

	gtk_widget_get_allocation(widget, &allocation);

	ghid_gl_start_drawing(port);

	ctx.view.X1 = MIN(Px(ev->area.x), Px(ev->area.x + ev->area.width + 1));
	ctx.view.X2 = MAX(Px(ev->area.x), Px(ev->area.x + ev->area.width + 1));
	ctx.view.Y1 = MIN(Py(ev->area.y), Py(ev->area.y + ev->area.height + 1));
	ctx.view.Y2 = MAX(Py(ev->area.y), Py(ev->area.y + ev->area.height + 1));

	ctx.view.X1 = MAX(0, MIN(PCB->hidlib.size_x, ctx.view.X1));
	ctx.view.X2 = MAX(0, MIN(PCB->hidlib.size_x, ctx.view.X2));
	ctx.view.Y1 = MAX(0, MIN(PCB->hidlib.size_y, ctx.view.Y1));
	ctx.view.Y2 = MAX(0, MIN(PCB->hidlib.size_y, ctx.view.Y2));

	gtk2gl_color(&off_c, &priv->offlimits_color);
	gtk2gl_color(&bg_c, &priv->bg_color);

	pcb_gl_draw_expose_init(&gtk2_gl_hid, allocation.width, allocation.height, ev->area.x, allocation.height - ev->area.height - ev->area.y, ev->area.width, ev->area.height, &off_c);

	glScalef((pcbhl_conf.editor.view.flip_x ? -1. : 1.) / port->view.coord_per_px, (pcbhl_conf.editor.view.flip_y ? -1. : 1.) / port->view.coord_per_px, ((pcbhl_conf.editor.view.flip_x == pcbhl_conf.editor.view.flip_y) ? 1. : -1.) / port->view.coord_per_px);
	glTranslatef(pcbhl_conf.editor.view.flip_x ? port->view.x0 - PCB->hidlib.size_x : -port->view.x0, pcbhl_conf.editor.view.flip_y ? port->view.y0 - PCB->hidlib.size_y : -port->view.y0, 0);

	/* Draw PCB background, before PCB primitives */
	glColor3f(bg_c.red, bg_c.green, bg_c.blue);
	glBegin(GL_QUADS);
	glVertex3i(0, 0, 0);
	glVertex3i(PCB->hidlib.size_x, 0, 0);
	glVertex3i(PCB->hidlib.size_x, PCB->hidlib.size_y, 0);
	glVertex3i(0, PCB->hidlib.size_y, 0);
	glEnd();

	ghid_gl_draw_bg_image();

	ghid_gl_invalidate_current_gc();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	pcb_hid_expose_all(&gtk2_gl_hid, &ctx, NULL);
	drawgl_flush();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	ghid_gl_draw_grid(&ctx.view);

	ghid_gl_invalidate_current_gc();

	pcb_draw_attached(0);
	pcb_draw_mark(0);
	drawgl_flush();

	ghid_gl_show_crosshair(TRUE);

	drawgl_flush();

	ghid_gl_end_drawing(port);

	return FALSE;
}

/* This realize callback is used to work around a crash bug in some mesa
 * versions (observed on a machine running the intel i965 driver. It isn't
 * obvious why it helps, but somehow fiddling with the GL context here solves
 * the issue. The problem appears to have been fixed in recent mesa versions.
 */
static void ghid_gl_port_drawing_realize_cb(GtkWidget *widget, gpointer data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		return;

	gdk_gl_drawable_gl_end(gldrawable);
	return;
}

static gboolean ghid_gl_preview_expose(GtkWidget *widget, pcb_gtk_expose_t *ev, pcb_hid_expose_t expcall, pcb_hid_expose_ctx_t *ctx)
{
/*	GdkWindow *window = gtk_widget_get_window(widget);*/
	GdkGLContext *pGlContext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);
	GtkAllocation allocation;
	render_priv_t *priv = gport->render_priv;
	pcb_gtk_view_t save_view;
	int save_width, save_height;
	double xz, yz, vw, vh;
	double xr, yr, wr, hr;
	pcb_coord_t ox1 = ctx->view.X1, oy1 = ctx->view.Y1, ox2 = ctx->view.X2, oy2 = ctx->view.Y2;
	pcb_gl_color_t bg_c;

	vw = ctx->view.X2 - ctx->view.X1;
	vh = ctx->view.Y2 - ctx->view.Y1;

	save_view = gport->view;
	save_width = gport->view.canvas_width;
	save_height = gport->view.canvas_height;

	/* Setup zoom factor for drawing routines */

	gtk_widget_get_allocation(widget, &allocation);
	xz = vw / (double)allocation.width;
	yz = vh / (double)allocation.height;

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

	PCB_GTK_PREVIEW_TUNE_EXTENT(ctx, allocation);

	/* make GL-context "current" */
	if (!gdk_gl_drawable_gl_begin(pGlDrawable, pGlContext)) {
		return FALSE;
	}
	gport->render_priv->in_context = pcb_true;

	if (ev) {
		xr = ev->area.x;
		yr = allocation.height - ev->area.height - ev->area.y;
		wr = ev->area.width;
		hr = ev->area.height;
	}
	else {
		xr = yr = 0;
		wr = allocation.width;
		hr = allocation.height;
	}

	gtk2gl_color(&bg_c, &priv->bg_color);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, allocation.width, allocation.height);

	glEnable(GL_SCISSOR_TEST);
	glScissor(xr, yr, wr, hr);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, allocation.width, allocation.height, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -Z_NEAR);

	glEnable(GL_STENCIL_TEST);
	glClearColor(bg_c.red, bg_c.green, bg_c.blue, 1.);
	glStencilMask(~0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	stencilgl_reset_stencil_usage();
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0);
	glStencilFunc(GL_ALWAYS, 0, 0);

	/* call the drawing routine */
	ghid_gl_invalidate_current_gc();
	glPushMatrix();
	glScalef((pcbhl_conf.editor.view.flip_x ? -1. : 1.) / gport->view.coord_per_px, (pcbhl_conf.editor.view.flip_y ? -1. : 1.) / gport->view.coord_per_px, 1);
	glTranslatef(pcbhl_conf.editor.view.flip_x ? gport->view.x0 - PCB->hidlib.size_x : -gport->view.x0, pcbhl_conf.editor.view.flip_y ? gport->view.y0 - PCB->hidlib.size_y : -gport->view.y0, 0);

	expcall(&gtk2_gl_hid, ctx);

	drawgl_flush();
	glPopMatrix();

	if (gdk_gl_drawable_is_double_buffered(pGlDrawable))
		gdk_gl_drawable_swap_buffers(pGlDrawable);
	else
		glFlush();

	/* end drawing to current GL-context */
	gport->render_priv->in_context = pcb_false;
	gdk_gl_drawable_gl_end(pGlDrawable);

	/* restore the original context and priv */
	ctx->view.X1 = ox1;
	ctx->view.X2 = ox2;
	ctx->view.Y1 = oy1;
	ctx->view.Y2 = oy2;
	gport->view = save_view;
	gport->view.canvas_width = save_width;
	gport->view.canvas_height = save_height;

	return FALSE;
}

static GtkWidget *ghid_gl_new_drawing_widget(pcb_gtk_common_t *common)
{
	GtkWidget *w = gtk_drawing_area_new();

	g_signal_connect(G_OBJECT(w), "expose_event", G_CALLBACK(common->drawing_area_expose), common->gport);

	return w;
}


void ghid_gl_install(pcb_gtk_common_t *common, pcb_hid_t *hid)
{

	if (common != NULL) {
		common->new_drawing_widget = ghid_gl_new_drawing_widget;
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
		hid->set_color = ghid_gl_set_color;
		hid->set_line_cap = ghid_gl_set_line_cap;
		hid->set_line_width = ghid_gl_set_line_width;
		hid->set_draw_xor = ghid_gl_set_draw_xor;
		hid->draw_line = ghid_gl_draw_line;
		hid->draw_arc = ghid_gl_draw_arc;
		hid->draw_rect = ghid_gl_draw_rect;
		hid->fill_circle = ghid_gl_fill_circle;
		hid->fill_polygon = ghid_gl_fill_polygon;
		hid->fill_polygon_offs = ghid_gl_fill_polygon_offs;
		hid->fill_rect = ghid_gl_fill_rect;

		hid->set_drawing_mode = hidgl_set_drawing_mode;
		hid->render_burst = ghid_gl_render_burst;
	}
}

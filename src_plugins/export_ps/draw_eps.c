/*
   This file is part of pcb-rnd and was part of gEDA/PCB but lacked proper
   copyright banner at the fork. It probably has the same copyright as
   gEDA/PCB as a whole in 2011.
*/

#include <librnd/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <librnd/core/math_helper.h>
#include <librnd/core/color.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>

#include "draw_eps.h"

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_cap_style_t cap;
	rnd_coord_t width;
	unsigned long color;
	int erase;
} rnd_hid_gc_s;

void rnd_eps_print_header(rnd_eps_t *pctx, const char *outfn, int ymirror)
{
	pctx->linewidth = -1;
	pctx->lastcap = -1;
	pctx->lastcolor = -1;
	pctx->drawn_objs = 0;

	fprintf(pctx->outf, "%%!PS-Adobe-3.0 EPSF-3.0\n");

#define pcb2em(x) 1 + RND_COORD_TO_INCH (x) * 72.0 * pctx->scale
	fprintf(pctx->outf, "%%%%BoundingBox: 0 0 %f %f\n", pcb2em(pctx->bounds.X2 - pctx->bounds.X1), pcb2em(pctx->bounds.Y2 - pctx->bounds.Y1));
#undef pcb2em
	fprintf(pctx->outf, "%%%%Pages: 1\n");
	fprintf(pctx->outf, "save countdictstack mark newpath /showpage {} def /setpagedevice {pop} def\n");
	fprintf(pctx->outf, "%%%%EndProlog\n");
	fprintf(pctx->outf, "%%%%Page: 1 1\n");

	fprintf(pctx->outf, "%%%%BeginDocument: %s\n\n", outfn);

	fprintf(pctx->outf, "72 72 scale\n");
	fprintf(pctx->outf, "1 dup neg scale\n");
	fprintf(pctx->outf, "%g dup scale\n", pctx->scale);
	rnd_fprintf(pctx->outf, "%mi %mi translate\n", -pctx->bounds.X1, -pctx->bounds.Y2);
	if (ymirror)
		rnd_fprintf(pctx->outf, "-1 1 scale %mi 0 translate\n", pctx->bounds.X1 - pctx->bounds.X2);

#define Q (rnd_coord_t) RND_MIL_TO_COORD(10)
	rnd_fprintf(pctx->outf,
							"/nclip { %mi %mi moveto %mi %mi lineto %mi %mi lineto %mi %mi lineto %mi %mi lineto eoclip newpath } def\n",
							pctx->bounds.X1 - Q, pctx->bounds.Y1 - Q, pctx->bounds.X1 - Q, pctx->bounds.Y2 + Q,
							pctx->bounds.X2 + Q, pctx->bounds.Y2 + Q, pctx->bounds.X2 + Q, pctx->bounds.Y1 - Q, pctx->bounds.X1 - Q, pctx->bounds.Y1 - Q);
#undef Q
	fprintf(pctx->outf, "/t { moveto lineto stroke } bind def\n");
	fprintf(pctx->outf, "/tc { moveto lineto strokepath nclip } bind def\n");
	fprintf(pctx->outf, "/r { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n");
	fprintf(pctx->outf, "     x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath fill } bind def\n");
	fprintf(pctx->outf, "/c { 0 360 arc fill } bind def\n");
	fprintf(pctx->outf, "/cc { 0 360 arc nclip } bind def\n");
	fprintf(pctx->outf, "/a { gsave setlinewidth translate scale 0 0 1 5 3 roll arc stroke grestore} bind def\n");
}

void rnd_eps_print_footer(rnd_eps_t *pctx)
{
	fprintf(pctx->outf, "showpage\n");

	fprintf(pctx->outf, "%%%%EndDocument\n");
	fprintf(pctx->outf, "%%%%Trailer\n");
	fprintf(pctx->outf, "cleartomark countdictstack exch sub { end } repeat restore\n");
	fprintf(pctx->outf, "%%%%EOF\n");
}

void rnd_eps_init(rnd_eps_t *pctx, FILE *f, rnd_box_t bounds, double scale, int in_mono, int as_shown)
{
	memset(pctx, 0, sizeof(rnd_eps_t));
	pctx->outf = f;
	pctx->linewidth = -1;
	pctx->lastcap = -1;
	pctx->lastcolor = -1;
	pctx->bounds = bounds;
	pctx->scale = scale;
	pctx->in_mono = in_mono;
	pctx->as_shown = as_shown;
	pctx->drawn_objs = 0;
}

rnd_hid_gc_t rnd_eps_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) malloc(sizeof(rnd_hid_gc_s));
	rv->cap = rnd_cap_round;
	rv->width = 0;
	rv->color = 0;
	return rv;
}

void rnd_eps_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

void rnd_eps_set_drawing_mode(rnd_eps_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	if (direct)
		return;
	pctx->drawing_mode = op;
	switch(op) {
		case RND_HID_COMP_RESET:
			fprintf(pctx->outf, "gsave\n");
			break;

		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_POSITIVE_XOR:
		case RND_HID_COMP_NEGATIVE:
			break;

		case RND_HID_COMP_FLUSH:
			fprintf(pctx->outf, "grestore\n");
			pctx->lastcolor = -1;
			break;
	}
}

void rnd_eps_set_color(rnd_eps_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color)
{
	if (pctx->drawing_mode == RND_HID_COMP_NEGATIVE) {
		gc->color = 0xffffff;
		gc->erase = 1;
		return;
	}
	if (rnd_color_is_drill(color)) {
		gc->color = 0xffffff;
		gc->erase = 0;
		return;
	}
	gc->erase = 0;
	if (pctx->in_mono)
		gc->color = 0;
	else if (color->str[0] == '#')
		gc->color = (color->r << 16) + (color->g << 8) + color->b;
	else
		gc->color = 0;
}

void rnd_eps_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

void rnd_eps_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}

void rnd_eps_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

static void use_gc(rnd_eps_t *pctx, rnd_hid_gc_t gc)
{
	pctx->drawn_objs++;
	if (pctx->linewidth != gc->width) {
		rnd_fprintf(pctx->outf, "%mi setlinewidth\n", gc->width);
		pctx->linewidth = gc->width;
	}
	if (pctx->lastcap != gc->cap) {
		int c;
		switch (gc->cap) {
		case rnd_cap_round:
			c = 1;
			break;
		case rnd_cap_square:
			c = 2;
			break;
		default:
			assert(!"unhandled cap");
			c = 1;
		}
		fprintf(pctx->outf, "%d setlinecap\n", c);
		pctx->lastcap = gc->cap;
	}
	if (pctx->lastcolor != gc->color) {
		int c = gc->color;
#define CV(x,b) (((x>>b)&0xff)/255.0)
		fprintf(pctx->outf, "%g %g %g setrgbcolor\n", CV(c, 16), CV(c, 8), CV(c, 0));
		pctx->lastcolor = gc->color;
	}
}

void rnd_eps_draw_rect(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(pctx, gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi %mi r\n", x1, y1, x2, y2);
}

void rnd_eps_draw_line(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_coord_t w = gc->width / 2;
	if (x1 == x2 && y1 == y2) {
		if (gc->cap == rnd_cap_square)
			rnd_eps_fill_rect(pctx, gc, x1 - w, y1 - w, x1 + w, y1 + w);
		else
			rnd_eps_fill_circle(pctx, gc, x1, y1, w);
		return;
	}
	use_gc(pctx, gc);
	if (gc->erase && gc->cap != rnd_cap_square) {
		double ang = atan2(y2 - y1, x2 - x1);
		double dx = w * sin(ang);
		double dy = -w * cos(ang);
		double deg = ang * 180.0 / M_PI;
		rnd_coord_t vx1 = x1 + dx;
		rnd_coord_t vy1 = y1 + dy;

		rnd_fprintf(pctx->outf, "%mi %mi moveto ", vx1, vy1);
		rnd_fprintf(pctx->outf, "%mi %mi %mi %g %g arc\n", x2, y2, w, deg - 90, deg + 90);
		rnd_fprintf(pctx->outf, "%mi %mi %mi %g %g arc\n", x1, y1, w, deg + 90, deg + 270);
		fprintf(pctx->outf, "nclip\n");

		return;
	}
	rnd_fprintf(pctx->outf, "%mi %mi %mi %mi %s\n", x1, y1, x2, y2, gc->erase ? "tc" : "t");
}

void rnd_eps_draw_arc(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_angle_t sa, ea;
	double w;

	if ((width == 0) && (height == 0)) {
		/* degenerate case, draw dot */
		rnd_eps_draw_line(pctx, gc, cx, cy, cx, cy);
		return;
	}

	if (delta_angle > 0) {
		sa = start_angle;
		ea = start_angle + delta_angle;
	}
	else {
		sa = start_angle + delta_angle;
		ea = start_angle;
	}
#if 0
	printf("draw_arc %d,%d %dx%d %d..%d %d..%d\n", cx, cy, width, height, start_angle, delta_angle, sa, ea);
#endif
	use_gc(pctx, gc);
	w = width;
	if (w == 0) /* make sure not to div by zero; this hack will have very similar effect */
		w = 0.0001;
	rnd_fprintf(pctx->outf, "%ma %ma %mi %mi %mi %mi %f a\n", sa, ea, -width, height, cx, cy, (double)pctx->linewidth / w);
}

void rnd_eps_fill_circle(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	use_gc(pctx,gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi %s\n", cx, cy, radius, gc->erase ? "cc" : "c");
}

void rnd_eps_fill_polygon_offs(rnd_eps_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int i;
	const char *op = "moveto";
	use_gc(pctx, gc);
	for (i = 0; i < n_coords; i++) {
		rnd_fprintf(pctx->outf, "%mi %mi %s\n", x[i] + dx, y[i] + dy, op);
		op = "lineto";
	}

	fprintf(pctx->outf, "fill\n");
}

void rnd_eps_fill_rect(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(pctx, gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi %mi r\n", x1, y1, x2, y2);
}

void rnd_eps_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
}


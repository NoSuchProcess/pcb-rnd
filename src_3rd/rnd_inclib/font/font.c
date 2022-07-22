#include <librnd/core/config.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/hid.h>
#include <rnd_inclib/font/font.h>
#include <rnd_inclib/font/xform_mx.h>

#define MAX_SIMPLE_POLY_POINTS 256

rnd_coord_t rnd_font_string_width(rnd_font_t *font, double scx, const unsigned char *string)
{
	rnd_coord_t w = 0;
	const rnd_box_t *unk_gl;

	if (string == NULL)
		return 0;

	unk_gl = &font->unknown_glyph;

	for(; *string != '\0'; string++) {
		if ((*string <= RND_FONT_MAX_GLYPHS) && font->glyph[*string].valid)
			w += (font->glyph[*string].width + font->glyph[*string].xdelta);
		else
			w += (unk_gl->X2 - unk_gl->X1) * 6 / 5;
	}
	return rnd_round((double)w * scx);
}

rnd_coord_t rnd_font_string_height(rnd_font_t *font, double scy, const unsigned char *string)
{
	rnd_coord_t h;

	if (string == NULL)
		return 0;

	h = font->max_height;

	for(; *string != '\0'; string++)
		if (*string == '\n')
			h += font->max_height;

	return rnd_round((double)h * scy);
}

RND_INLINE void cheap_text_line(rnd_xform_mx_t mx, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_glyph_atom_t a;

	a.type = RND_GLYPH_LINE;
	a.line.x1 = rnd_round(rnd_xform_x(mx, x1, y1));
	a.line.y1 = rnd_round(rnd_xform_y(mx, x1, y1));
	a.line.x2 = rnd_round(rnd_xform_x(mx, x2, y2));
	a.line.y2 = rnd_round(rnd_xform_y(mx, x2, y2));
	a.line.thickness = -1;

	cb(cb_ctx, &a);
}

/* Decreased level-of-detail: draw only a few lines for the whole text */
RND_INLINE int draw_text_cheap(rnd_font_t *font, rnd_xform_mx_t mx, const unsigned char *string, double scx, double scy, rnd_font_tiny_t tiny, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_coord_t w, h = rnd_font_string_height(font, scy, string);

	if (tiny == RND_FONT_TINY_HIDE) {
		if (h <= rnd_render->coord_per_pix*6) /* <= 6 pixel high: unreadable */
			return 1;
	}
	else if (tiny == RND_FONT_TINY_CHEAP) {
		if (h <= rnd_render->coord_per_pix*2) { /* <= 1 pixel high: draw a single line in the middle */
			w = rnd_font_string_width(font, scx, string);
			cheap_text_line(mx, 0, h/2, w, h/2, cb, cb_ctx);
			return 1;
		}
		else if (h <= rnd_render->coord_per_pix*4) { /* <= 4 pixel high: draw a mirrored Z-shape */
			w = rnd_font_string_width(font, scx, string);
			h /= 4;
			cheap_text_line(mx, 0, h,   w, h,   cb, cb_ctx);
			cheap_text_line(mx, 0, h,   w, h*3, cb, cb_ctx);
			cheap_text_line(mx, 0, h*3, w, h*3, cb, cb_ctx);
			return 1;
		}
	}

	return 0;
}


RND_INLINE void draw_atom(const rnd_glyph_atom_t *a, rnd_xform_mx_t mx, rnd_coord_t dx, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int poly_thin, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	long nx, ny, h;
	rnd_glyph_atom_t res;
	rnd_coord_t tmp[2*MAX_SIMPLE_POLY_POINTS];

	res.type = a->type;
	switch(a->type) {
		case RND_GLYPH_LINE:
			res.line.x1 = rnd_round(rnd_xform_x(mx, a->line.x1+dx, a->line.y1));
			res.line.y1 = rnd_round(rnd_xform_y(mx, a->line.x1+dx, a->line.y1));
			res.line.x2 = rnd_round(rnd_xform_x(mx, a->line.x2+dx, a->line.y2));
			res.line.y2 = rnd_round(rnd_xform_y(mx, a->line.x2+dx, a->line.y2));
			res.line.thickness = rnd_round(a->line.thickness * (scx+scy) / 4.0);

			if (res.line.thickness < min_line_width)
				res.line.thickness = min_line_width;
			if (thickness > 0)
				res.line.thickness = thickness;

			break;

		case RND_GLYPH_ARC:
			res.arc.cx = rnd_round(rnd_xform_x(mx, a->arc.cx + dx, a->arc.cy));
			res.arc.cy = rnd_round(rnd_xform_y(mx, a->arc.cx + dx, a->arc.cy));
			res.arc.r  = rnd_round(a->arc.r * scx);
			res.arc.start = a->arc.start + rotdeg;
			res.arc.delta = a->arc.delta;
			res.arc.thickness = rnd_round(a->arc.thickness * (scx+scy) / 4.0);
			if (mirror) {
				res.arc.start = RND_SWAP_ANGLE(res.arc.start);
				res.arc.delta = RND_SWAP_DELTA(res.arc.delta);
			}
			if (res.arc.thickness < min_line_width)
				res.arc.thickness = min_line_width;
			if (thickness > 0)
				res.arc.thickness = thickness;
			break;

		case RND_GLYPH_POLY:
			h = a->poly.pts.used/2;

			if (poly_thin) {
				rnd_coord_t lpx, lpy, tx, ty;

				res.line.thickness = -1;
				res.line.type = RND_GLYPH_LINE;

				tx = a->poly.pts.array[h-1];
				ty = a->poly.pts.array[a->poly.pts.used-1];
				lpx = rnd_round(rnd_xform_x(mx, tx + dx, ty));
				lpy = rnd_round(rnd_xform_y(mx, tx + dx, ty));

				for(nx = 0, ny = h; nx < h; nx++,ny++) {
					rnd_coord_t npx, npy, px = a->poly.pts.array[nx], py = a->poly.pts.array[ny];
					npx = rnd_round(rnd_xform_x(mx, px + dx, py));
					npy = rnd_round(rnd_xform_y(mx, px + dx, py));
					res.line.x1 = lpx; res.line.y1 = lpy;
					res.line.x2 = npx; res.line.y2 = npy;
					cb(cb_ctx, &res);
					lpx = npx;
					lpy = npy;
				}
				return; /* don't let the original cb() at the end run */
			}
			res.poly.pts.used = res.poly.pts.alloced = a->poly.pts.used;
			res.poly.pts.array = tmp;
			for(nx = 0, ny = h; nx < h; nx++,ny++) {
				rnd_coord_t px = a->poly.pts.array[nx], py = a->poly.pts.array[ny];
				res.poly.pts.array[nx] = rnd_round(rnd_xform_x(mx, px + dx, py));
				res.poly.pts.array[ny] = rnd_round(rnd_xform_y(mx, px + dx, py));
			}
			break;
	}

	cb(cb_ctx, &res);
}

#define rnd_font_mx(mx, x0, y0, scx, scy, rotdeg, mirror) \
do { \
	rnd_xform_mx_translate(mx, x0, y0); \
	if (mirror != RND_FONT_MIRROR_NO) \
		rnd_xform_mx_scale(mx, (mirror & RND_FONT_MIRROR_X) ? -1 : 1, (mirror & RND_FONT_MIRROR_Y) ? -1 : 1); \
	rnd_xform_mx_rotate(mx, rotdeg); \
	rnd_xform_mx_scale(mx, scx, scy); \
} while(0)

RND_FONT_DRAW_API void rnd_font_draw_string(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int poly_thin, rnd_font_tiny_t tiny, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_xform_mx_t mx = RND_XFORM_MX_IDENT;
	const unsigned char *s;
	rnd_coord_t x = 0;

	rnd_font_mx(mx, x0, y0, scx, scy, rotdeg, mirror);

	/* Text too small at this zoom level: cheap draw */
	if (tiny != RND_FONT_TINY_ACCURATE) {
		if (draw_text_cheap(font, mx, string, scx, scy, tiny, cb, cb_ctx))
			return;
	}

	for(s = string; *s != '\0'; s++) {
		/* draw atoms if symbol is valid and data is present */
		if ((*s <= RND_FONT_MAX_GLYPHS) && font->glyph[*s].valid) {
			long n;
			rnd_glyph_t *g = &font->glyph[*s];
			
			for(n = 0; n < g->atoms.used; n++)
				draw_atom(&g->atoms.array[n], mx, x,  scx, scy, rotdeg, mirror, thickness, min_line_width, poly_thin, cb, cb_ctx);

			/* move on to next cursor position */
			x += (g->width + g->xdelta);
		}
		else {
			/* the default symbol is a filled box */
			rnd_coord_t size = (font->unknown_glyph.X2 - font->unknown_glyph.X1) * 6 / 5;
			rnd_coord_t p[8];
			rnd_glyph_atom_t tmp;

			p[0+0] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y1));
			p[4+0] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y1));
			p[0+1] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y1));
			p[4+1] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y1));
			p[0+2] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y2));
			p[4+2] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y2));
			p[0+3] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y2));
			p[4+3] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y2));

			/* draw move on to next cursor position */
			if (poly_thin) {
				tmp.line.type = RND_GLYPH_LINE;
				tmp.line.thickness = -1;

				tmp.line.x1 = p[0+0]; tmp.line.y1 = p[4+0];
				tmp.line.x2 = p[0+1]; tmp.line.y2 = p[4+1];
				cb(cb_ctx, &tmp);

				tmp.line.x1 = p[0+1]; tmp.line.y1 = p[4+1];
				tmp.line.x2 = p[0+2]; tmp.line.y2 = p[4+2];
				cb(cb_ctx, &tmp);

				tmp.line.x1 = p[0+2]; tmp.line.y1 = p[4+2];
				tmp.line.x2 = p[0+3]; tmp.line.y2 = p[4+3];
				cb(cb_ctx, &tmp);

				tmp.line.x1 = p[0+3]; tmp.line.y1 = p[4+3];
				tmp.line.x2 = p[0+0]; tmp.line.y2 = p[4+0];
				cb(cb_ctx, &tmp);
			}
			else {
				tmp.poly.type = RND_GLYPH_POLY;
				tmp.poly.pts.used = tmp.poly.pts.alloced = 8;
				tmp.poly.pts.array = p;
				cb(cb_ctx, &tmp);
			}
			x += size;
		}
	}
}

RND_INLINE void font_arc_bbox(rnd_box_t *dst, rnd_glyph_arc_t *a)
{
	double ca1, ca2, sa1, sa2, minx, maxx, miny, maxy, delta;
	rnd_angle_t ang1, ang2;
	rnd_coord_t width;

	/* first put angles into standard form: ang1 < ang2, both angles between 0 and 720 */
	delta = RND_CLAMP(a->delta, -360, 360);

	if (delta > 0) {
		ang1 = rnd_normalize_angle(a->start);
		ang2 = rnd_normalize_angle(a->start + delta);
	}
	else {
		ang1 = rnd_normalize_angle(a->start + delta);
		ang2 = rnd_normalize_angle(a->start);
	}

	if (ang1 > ang2)
		ang2 += 360;

	/* Make sure full circles aren't treated as zero-length arcs */
	if (delta == 360 || delta == -360)
		ang2 = ang1 + 360;

	/* calculate sines, cosines */
	sa1 = sin(RND_M180 * ang1); ca1 = cos(RND_M180 * ang1);
	sa2 = sin(RND_M180 * ang2); ca2 = cos(RND_M180 * ang2);

	minx = MIN(ca1, ca2); miny = MIN(sa1, sa2);
	maxx = MAX(ca1, ca2); maxy = MAX(sa1, sa2);

	/* Check for extreme angles */
	if ((ang1 <= 0 && ang2 >= 0) || (ang1 <= 360 && ang2 >= 360))
		maxx = 1;
	if ((ang1 <= 90 && ang2 >= 90) || (ang1 <= 450 && ang2 >= 450))
		maxy = 1;
	if ((ang1 <= 180 && ang2 >= 180) || (ang1 <= 540 && ang2 >= 540))
		minx = -1;
	if ((ang1 <= 270 && ang2 >= 270) || (ang1 <= 630 && ang2 >= 630))
		miny = -1;

	/* Finally, calculate bounds, converting sane geometry into rnd geometry */
	dst->X1 = a->cx - a->r * maxx;
	dst->X2 = a->cx - a->r * minx;
	dst->Y1 = a->cy + a->r * miny;
	dst->Y2 = a->cy + a->r * maxy;
}

void rnd_font_string_bbox(rnd_coord_t cx[4], rnd_coord_t cy[4], rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width)
{
	const unsigned char *s;
	int space, width;
	rnd_coord_t minx, miny, maxx, maxy, tx, min_unscaled_radius;
	rnd_bool first_time = rnd_true;
	rnd_xform_mx_t mx = RND_XFORM_MX_IDENT;

	if (string == NULL) {
		int n;
		for(n = 0; n < 4; n++) {
			cx[n] = x0;
			cy[n] = y0;
		}
		return;
	}

	rnd_font_mx(mx, x0, y0, scx, scy, rotdeg, mirror);

	minx = miny = maxx = maxy = tx = 0;

	/* We are initially computing the bbox of the un-scaled text but
	   min_line_width is interpreted on the scaled text. */
	min_unscaled_radius = (min_line_width * (scx+scy)/2) / 2;

	/* calculate size of the bounding box */
	for (s = string; *s != '\0'; s++) {
		if ((*s <= RND_FONT_MAX_GLYPHS) && font->glyph[*s].valid) {
			long n;
			rnd_glyph_t *g = &font->glyph[*s];

			width = g->width;
			for(n = 0; n < g->atoms.used; n++) {
				rnd_glyph_atom_t *a = &g->atoms.array[n];
				rnd_coord_t unscaled_radius;
				rnd_box_t b;

				switch(a->type) {
					case RND_GLYPH_LINE:
							/* Clamp the width of text lines at the minimum thickness.
							   NB: Divide 4 in thickness calculation is comprised of a factor
							       of 1/2 to get a radius from the center-line, and a factor
							       of 1/2 because some stupid reason we render our glyphs
							       at half their defined stroke-width. */
						unscaled_radius = RND_MAX(min_unscaled_radius, a->line.thickness / 4);

						if (first_time) {
							minx = maxx = a->line.x1;
							miny = maxy = a->line.y1;
							first_time = rnd_false;
						}

						minx = MIN(minx, a->line.x1 - unscaled_radius + tx);
						miny = MIN(miny, a->line.y1 - unscaled_radius);
						minx = MIN(minx, a->line.x2 - unscaled_radius + tx);
						miny = MIN(miny, a->line.y2 - unscaled_radius);
						maxx = MAX(maxx, a->line.x1 + unscaled_radius + tx);
						maxy = MAX(maxy, a->line.y1 + unscaled_radius);
						maxx = MAX(maxx, a->line.x2 + unscaled_radius + tx);
						maxy = MAX(maxy, a->line.y2 + unscaled_radius);
					break;

					case RND_GLYPH_ARC:
						font_arc_bbox(&b, a);
						unscaled_radius = RND_MAX(min_unscaled_radius, a->arc.thickness / 4);
						minx = MIN(minx, b.X1 + tx - unscaled_radius);
						miny = MIN(miny, b.Y1 - unscaled_radius);
						maxx = MAX(maxx, b.X2 + tx + unscaled_radius);
						maxy = MAX(maxy, b.Y2 + unscaled_radius);
						break;

					case RND_GLYPH_POLY:
						{
							int i, h = a->poly.pts.used/2;
							rnd_coord_t *px = &a->poly.pts.array[0], *py = &a->poly.pts.array[h];

							for(i = 0; i < h; i++,px++,py++) {
								minx = MIN(minx, *px + tx);
								miny = MIN(miny, *py);
								maxx = MAX(maxx, *px + tx);
								maxy = MAX(maxy, *py);
							}
						}
						break;
				}
			}
			space = g->xdelta;
		}
		else {
			rnd_box_t *ds = &font->unknown_glyph;
			rnd_coord_t w = ds->X2 - ds->X1;

			minx = MIN(minx, ds->X1 + tx);
			miny = MIN(miny, ds->Y1);
			minx = MIN(minx, ds->X2 + tx);
			miny = MIN(miny, ds->Y2);
			maxx = MAX(maxx, ds->X1 + tx);
			maxy = MAX(maxy, ds->Y1);
			maxx = MAX(maxx, ds->X2 + tx);
			maxy = MAX(maxy, ds->Y2);

			space = w / 5;
		}
		tx += width + space;
	}

	/* it is enough to do the transformations only once, on the raw bounding box:
	   calculate the transformed coordinates of all 4 corners of the raw
	   (non-axis-aligned) bounding box */
	cx[0] = rnd_round(rnd_xform_x(mx, minx, miny));
	cy[0] = rnd_round(rnd_xform_y(mx, minx, miny));
	cx[1] = rnd_round(rnd_xform_x(mx, maxx, miny));
	cy[1] = rnd_round(rnd_xform_y(mx, maxx, miny));
	cx[2] = rnd_round(rnd_xform_x(mx, maxx, maxy));
	cy[2] = rnd_round(rnd_xform_y(mx, maxx, maxy));
	cx[3] = rnd_round(rnd_xform_x(mx, minx, maxy));
	cy[3] = rnd_round(rnd_xform_y(mx, minx, maxy));
}

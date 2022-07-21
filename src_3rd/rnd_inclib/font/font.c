#include <librnd/core/config.h>
#include <librnd/core/compat_misc.h>
#include <rnd_inclib/font/font.h>
#include <rnd_inclib/font/xform_mx.h>

#define MAX_SIMPLE_POLY_POINTS 256

RND_INLINE void draw_atom(const rnd_glyph_atom_t *a, rnd_xform_mx_t mx, rnd_coord_t dx, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int poly_thin, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	long n;
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
			res.poly.xy.used = res.poly.xy.alloced = a->poly.xy.used;
			res.poly.xy.array = tmp;
			for(n = 0; n < a->poly.xy.used; n+=2) {
				rnd_coord_t px = a->poly.xy.array[n], py = a->poly.xy.array[n+1];
				res.poly.xy.array[n]   = rnd_round(rnd_xform_x(mx, px + dx, py));
				res.poly.xy.array[n+1] = rnd_round(rnd_xform_y(mx, px + dx, py));
			}
			break;
	}

	cb(cb_ctx, &res);
}

RND_FONT_DRAW_API void rnd_font_draw_string(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int poly_thin, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_xform_mx_t mx = RND_XFORM_MX_IDENT;
	const unsigned char *s;
	rnd_coord_t x = 0;

	rnd_xform_mx_translate(mx, x0, y0);
	if (mirror != RND_TXT_MIRROR_NO)
		rnd_xform_mx_scale(mx, (mirror & RND_TXT_MIRROR_X) ? -1 : 1, (mirror & RND_TXT_MIRROR_Y) ? -1 : 1);
	rnd_xform_mx_rotate(mx, rotdeg);
	rnd_xform_mx_scale(mx, scx, scy);

	for(s = string; *s != '\0'; s++) {
		/* draw atoms if symbol is valid and data is present */
		if ((*s <= RND_FONT_MAX_GLYPHS) && font->glyph[*s].valid) {
			long n;
			rnd_glyph_t *g = &font->glyph[*s];
			
			for(n = 0; n < g->atoms.used; g++)
				draw_atom(&g->atoms.array[n], mx, x,  scx, scy, rotdeg, mirror, thickness, min_line_width, poly_thin, cb, cb_ctx);

			/* move on to next cursor position */
			x += (g->width + g->xdelta);
		}
		else {
			/* the default symbol is a filled box */
			rnd_coord_t size = (font->unknown_glyph.X2 - font->unknown_glyph.X1) * 6 / 5;
			rnd_coord_t p[8];
			rnd_glyph_atom_t tmp;

			p[0] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y1));
			p[1] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y1));
			p[2] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y1));
			p[3] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y1));
			p[4] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y2));
			p[5] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y2));
			p[6] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y2));
			p[7] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y2));

			/* draw move on to next cursor position */
			if (poly_thin) {
				tmp.line.type = RND_GLYPH_LINE;
				tmp.line.thickness = -1;

				tmp.line.x1 = p[0]; tmp.line.y1 = p[1];
				tmp.line.x2 = p[2]; tmp.line.y2 = p[3];
				cb(cb_ctx, &tmp);

				tmp.line.x1 = p[2]; tmp.line.y1 = p[3];
				tmp.line.x2 = p[4]; tmp.line.y2 = p[5];
				cb(cb_ctx, &tmp);

				tmp.line.x1 = p[4]; tmp.line.y1 = p[5];
				tmp.line.x2 = p[6]; tmp.line.y2 = p[7];
				cb(cb_ctx, &tmp);

				tmp.line.x1 = p[6]; tmp.line.y1 = p[7];
				tmp.line.x2 = p[0]; tmp.line.y2 = p[1];
				cb(cb_ctx, &tmp);
			}
			else {
				tmp.poly.type = RND_GLYPH_POLY;
				tmp.poly.xy.used = tmp.poly.xy.alloced = 8;
				tmp.poly.xy.array = p;
				cb(cb_ctx, &tmp);
			}
			x += size;
		}
	}
}

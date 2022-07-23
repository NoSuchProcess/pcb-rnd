/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2018,2019,2020,2021 Tibor 'Igor2' Palinkas
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
 *
 */

/* Drawing primitive: text - old font rendering (will be replaced by
   rnd_font_*() */

static void draw_text_poly(pcb_draw_info_t *info, pcb_poly_t *poly, pcb_xform_mx_t mx, rnd_coord_t xo, int xordraw, int thindraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_draw_text_cb cb, void *cb_ctx);

/* creates the bounding box of a text object */
static void pcb_text_bbox_orig(pcb_font_t *FontPtr, pcb_text_t *Text)
{
	pcb_symbol_t *symbol;
	unsigned char *s, *rendered = pcb_text_render_str(Text);
	int i;
	int space;
	rnd_coord_t minx, miny, maxx, maxy, tx;
	rnd_coord_t min_final_radius;
	rnd_coord_t min_unscaled_radius;
	rnd_bool first_time = rnd_true;
	pcb_poly_t *poly;
	double scx, scy;
	pcb_xform_mx_t mx = PCB_XFORM_MX_IDENT;
	rnd_coord_t cx[4], cy[4];
	pcb_text_mirror_t mirror;

	pcb_text_get_scale_xy(Text, &scx, &scy);

	s = rendered;

	if (FontPtr == NULL)
		FontPtr = pcb_font(PCB, Text->fid, 1);

	symbol = FontPtr->Symbol;

	minx = miny = maxx = maxy = tx = 0;

	/* Calculate the bounding box based on the larger of the thicknesses
	 * the text might clamped at on silk or copper layers.
	 */
	min_final_radius = MAX(conf_core.design.min_wid, conf_core.design.min_slk) / 2;

	/* Pre-adjust the line radius for the fact we are initially computing the
	 * bounds of the un-scaled text, and the thickness clamping applies to
	 * scaled text.
	 */
	min_unscaled_radius = PCB_UNPCB_SCALE_TEXT(min_final_radius, Text->Scale);

	/* calculate size of the bounding box */
	for (; s && *s; s++) {
		if (*s <= PCB_MAX_FONTPOSITION && symbol[*s].Valid) {
			pcb_line_t *line = symbol[*s].Line;
			pcb_arc_t *arc;
			for (i = 0; i < symbol[*s].LineN; line++, i++) {
				/* Clamp the width of text lines at the minimum thickness.
				 * NB: Divide 4 in thickness calculation is comprised of a factor
				 *     of 1/2 to get a radius from the center-line, and a factor
				 *     of 1/2 because some stupid reason we render our glyphs
				 *     at half their defined stroke-width.
				 */
				rnd_coord_t unscaled_radius = MAX(min_unscaled_radius, line->Thickness / 4);

				if (first_time) {
					minx = maxx = line->Point1.X;
					miny = maxy = line->Point1.Y;
					first_time = rnd_false;
				}

				minx = MIN(minx, line->Point1.X - unscaled_radius + tx);
				miny = MIN(miny, line->Point1.Y - unscaled_radius);
				minx = MIN(minx, line->Point2.X - unscaled_radius + tx);
				miny = MIN(miny, line->Point2.Y - unscaled_radius);
				maxx = MAX(maxx, line->Point1.X + unscaled_radius + tx);
				maxy = MAX(maxy, line->Point1.Y + unscaled_radius);
				maxx = MAX(maxx, line->Point2.X + unscaled_radius + tx);
				maxy = MAX(maxy, line->Point2.Y + unscaled_radius);
			}

			for(arc = arclist_first(&symbol[*s].arcs); arc != NULL; arc = arclist_next(arc)) {
				pcb_arc_bbox(arc);
				minx = MIN(minx, arc->bbox_naked.X1 + tx);
				miny = MIN(miny, arc->bbox_naked.Y1);
				maxx = MAX(maxx, arc->bbox_naked.X2 + tx);
				maxy = MAX(maxy, arc->bbox_naked.Y2);
			}

			for(poly = polylist_first(&symbol[*s].polys); poly != NULL; poly = polylist_next(poly)) {
				int n;
				rnd_point_t *pnt;
				for(n = 0, pnt = poly->Points; n < poly->PointN; n++,pnt++) {
					minx = MIN(minx, pnt->X + tx);
					miny = MIN(miny, pnt->Y);
					maxx = MAX(maxx, pnt->X + tx);
					maxy = MAX(maxy, pnt->Y);
				}
			}

			space = symbol[*s].Delta;
		}
		else {
			rnd_box_t *ds = &FontPtr->DefaultSymbol;
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
		tx += symbol[*s].Width + space;
	}

	mirror = text_mirror_bits(Text);

	/* it is enough to do the transformations only once, on the raw bounding box */
	pcb_xform_mx_translate(mx, Text->X, Text->Y);
	if (mirror != PCB_TXT_MIRROR_NO)
		pcb_xform_mx_scale(mx, (mirror & PCB_TXT_MIRROR_X) ? -1 : 1, (mirror & PCB_TXT_MIRROR_Y) ? -1 : 1);
	pcb_xform_mx_rotate(mx, Text->rot);
	pcb_xform_mx_scale(mx, scx, scy);

	/* calculate the transformed coordinates of all 4 corners of the raw
	   (non-axis-aligned) bounding box */
	cx[0] = rnd_round(pcb_xform_x(mx, minx, miny));
	cy[0] = rnd_round(pcb_xform_y(mx, minx, miny));
	cx[1] = rnd_round(pcb_xform_x(mx, maxx, miny));
	cy[1] = rnd_round(pcb_xform_y(mx, maxx, miny));
	cx[2] = rnd_round(pcb_xform_x(mx, maxx, maxy));
	cy[2] = rnd_round(pcb_xform_y(mx, maxx, maxy));
	cx[3] = rnd_round(pcb_xform_x(mx, minx, maxy));
	cy[3] = rnd_round(pcb_xform_y(mx, minx, maxy));

	/* calculate the axis-aligned version */
	Text->bbox_naked.X1 = Text->bbox_naked.X2 = cx[0];
	Text->bbox_naked.Y1 = Text->bbox_naked.Y2 = cy[0];
	rnd_box_bump_point(&Text->bbox_naked, cx[1], cy[1]);
	rnd_box_bump_point(&Text->bbox_naked, cx[2], cy[2]);
	rnd_box_bump_point(&Text->bbox_naked, cx[3], cy[3]);

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	Text->BoundingBox.X1 = Text->bbox_naked.X1 - conf_core.design.bloat;
	Text->BoundingBox.Y1 = Text->bbox_naked.Y1 - conf_core.design.bloat;
	Text->BoundingBox.X2 = Text->bbox_naked.X2 + conf_core.design.bloat;
	Text->BoundingBox.Y2 = Text->bbox_naked.Y2 + conf_core.design.bloat;
	rnd_close_box(&Text->bbox_naked);
	rnd_close_box(&Text->BoundingBox);
	pcb_text_free_str(Text, rendered);
}

/* Very rough estimation on the full width of the text */
rnd_coord_t pcb_text_width(pcb_font_t *font, double scx, const unsigned char *string)
{
	rnd_coord_t w = 0;
	const rnd_box_t *defaultsymbol;
	if (string == NULL)
		return 0;
	defaultsymbol = &font->DefaultSymbol;
	while(*string) {
		/* draw lines if symbol is valid and data is present */
		if (*string <= PCB_MAX_FONTPOSITION && font->Symbol[*string].Valid)
			w += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
		else
			w += (defaultsymbol->X2 - defaultsymbol->X1) * 6 / 5;
		string++;
	}
	return rnd_round((double)w * scx);
}

rnd_coord_t pcb_text_height(pcb_font_t *font, double scy, const unsigned char *string)
{
	rnd_coord_t h;
	if (string == NULL)
		return 0;
	h = font->MaxHeight;
	while(*string != '\0') {
		if (*string == '\n')
			h += font->MaxHeight;
		string++;
	}
	return rnd_round((double)h * scy);
}

int pcb_text_invalid_chars(pcb_board_t *pcb, pcb_font_t *FontPtr, pcb_text_t *Text)
{
	unsigned char *s, *rendered;
	int ctr = 0;
	pcb_symbol_t *symbol;

	if (FontPtr == NULL)
		FontPtr = pcb_font(pcb, Text->fid, 1);

	symbol = FontPtr->Symbol;

	rendered = pcb_text_render_str(Text);
	if (rendered == NULL)
		return 0;

	for(s = rendered; *s != '\0'; s++)
		if ((*s > PCB_MAX_FONTPOSITION) || (!symbol[*s].Valid))
			ctr++;

	pcb_text_free_str(Text, rendered);

	return ctr;
}

RND_INLINE void cheap_text_line(rnd_hid_gc_t gc, pcb_xform_mx_t mx, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t xordx, rnd_coord_t xordy)
{
	rnd_coord_t tx1, ty1, tx2, ty2;

	tx1 = rnd_round(pcb_xform_x(mx, x1, y1));
	ty1 = rnd_round(pcb_xform_y(mx, x1, y1));
	tx2 = rnd_round(pcb_xform_x(mx, x2, y2));
	ty2 = rnd_round(pcb_xform_y(mx, x2, y2));

	tx1 += xordx;
	ty1 += xordy;
	tx2 += xordx;
	ty2 += xordy;

	rnd_render->draw_line(gc, tx1, ty1, tx2, ty2);
}


/* Decreased level-of-detail: draw only a few lines for the whole text */
RND_INLINE int draw_text_cheap(pcb_font_t *font, pcb_xform_mx_t mx, const unsigned char *string, double scx, double scy, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_text_tiny_t tiny)
{
	rnd_coord_t w, h = rnd_round((double)font->MaxHeight * scy);
	if (tiny == PCB_TXT_TINY_HIDE) {
		if (h <= rnd_render->coord_per_pix*6) /* <= 6 pixel high: unreadable */
			return 1;
	}
	else if (tiny == PCB_TXT_TINY_CHEAP) {
		if (h <= rnd_render->coord_per_pix*2) { /* <= 1 pixel high: draw a single line in the middle */
			w = pcb_text_width(font, scx, string);
			if (xordraw) {
				cheap_text_line(pcb_crosshair.GC, mx, 0, h/2, w, h/2, xordx, xordy);
			}
			else {
				rnd_hid_set_line_width(pcb_draw_out.fgGC, -1);
				rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_square);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h/2, w, h/2, 0, 0);
			}
			return 1;
		}
		else if (h <= rnd_render->coord_per_pix*4) { /* <= 4 pixel high: draw a mirrored Z-shape */
			w = pcb_text_width(font, scx, string);
			if (xordraw) {
				h /= 4;
				cheap_text_line(pcb_crosshair.GC, mx, 0, h,   w, h,   xordx, xordy);
				cheap_text_line(pcb_crosshair.GC, mx, 0, h,   w, h*3, xordx, xordy);
				cheap_text_line(pcb_crosshair.GC, mx, 0, h*3, w, h*3, xordx, xordy);
			}
			else {
				h /= 4;
				rnd_hid_set_line_width(pcb_draw_out.fgGC, -1);
				rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_square);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h,   w, h,   0, 0);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h,   w, h*3, 0, 0);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h*3, w, h*3, 0, 0);
			}
			return 1;
		}
	}
	return 0;
}

RND_INLINE void pcb_text_draw_string_orig(pcb_draw_info_t *info, pcb_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, pcb_text_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_text_tiny_t tiny, pcb_draw_text_cb cb, void *cb_ctx)
{
	rnd_coord_t x = 0;
	rnd_cardinal_t n;
	pcb_xform_mx_t mx = PCB_XFORM_MX_IDENT;

	pcb_xform_mx_translate(mx, x0, y0);
	if (mirror != PCB_TXT_MIRROR_NO)
		pcb_xform_mx_scale(mx, (mirror & PCB_TXT_MIRROR_X) ? -1 : 1, (mirror & PCB_TXT_MIRROR_Y) ? -1 : 1);
	pcb_xform_mx_rotate(mx, rotdeg);
	pcb_xform_mx_scale(mx, scx, scy);

	/* Text too small at this zoom level: cheap draw */
	if ((tiny != PCB_TXT_TINY_ACCURATE) && (cb == NULL)) {
		if (draw_text_cheap(font, mx, string, scx, scy, xordraw, xordx, xordy, tiny))
			return;
	}

	/* normal draw */
	while (string && *string) {
		/* draw lines if symbol is valid and data is present */
		if (*string <= PCB_MAX_FONTPOSITION && font->Symbol[*string].Valid) {
			pcb_line_t *line = font->Symbol[*string].Line;
			pcb_line_t newline;
			pcb_poly_t *p;
			pcb_arc_t *a, newarc;
			int poly_thin;

			for (n = font->Symbol[*string].LineN; n; n--, line++) {
				/* create one line, scale, move, rotate and swap it */
				newline = *line;
				newline.Point1.X = rnd_round(pcb_xform_x(mx, line->Point1.X+x, line->Point1.Y));
				newline.Point1.Y = rnd_round(pcb_xform_y(mx, line->Point1.X+x, line->Point1.Y));
				newline.Point2.X = rnd_round(pcb_xform_x(mx, line->Point2.X+x, line->Point2.Y));
				newline.Point2.Y = rnd_round(pcb_xform_y(mx, line->Point2.X+x, line->Point2.Y));
				newline.Thickness = rnd_round(newline.Thickness * (scx+scy) / 4.0);

				if (newline.Thickness < min_line_width)
					newline.Thickness = min_line_width;
				if (thickness > 0)
					newline.Thickness = thickness;
				if (cb != NULL) {
					newline.type = PCB_OBJ_LINE;
					cb(cb_ctx, (pcb_any_obj_t *)&newline);
				}
				else if (xordraw)
					rnd_render->draw_line(pcb_crosshair.GC, xordx + newline.Point1.X, xordy + newline.Point1.Y, xordx + newline.Point2.X, xordy + newline.Point2.Y);
				else
					pcb_line_draw_(info, &newline, 0);
			}

			/* draw the arcs */
			for(a = arclist_first(&font->Symbol[*string].arcs); a != NULL; a = arclist_next(a)) {
				newarc = *a;

				newarc.X = rnd_round(pcb_xform_x(mx, a->X + x, a->Y));
				newarc.Y = rnd_round(pcb_xform_y(mx, a->X + x, a->Y));
				newarc.Height = newarc.Width = rnd_round(newarc.Height * scx);
				newarc.Thickness = rnd_round(newarc.Thickness * (scx+scy) / 4.0);
				newarc.StartAngle += rotdeg;
				if (mirror) {
					newarc.StartAngle = RND_SWAP_ANGLE(newarc.StartAngle);
					newarc.Delta = RND_SWAP_DELTA(newarc.Delta);
				}
				if (newarc.Thickness < min_line_width)
					newarc.Thickness = min_line_width;
				if (thickness > 0)
					newarc.Thickness = thickness;
				if (cb != NULL) {
					newarc.type = PCB_OBJ_ARC;
					cb(cb_ctx, (pcb_any_obj_t *)&newarc);
				}
				else if (xordraw)
					rnd_render->draw_arc(pcb_crosshair.GC, xordx + newarc.X, xordy + newarc.Y, newarc.Width, newarc.Height, newarc.StartAngle, newarc.Delta);
				else
					pcb_arc_draw_(info, &newarc, 0);
			}

			/* draw the polygons */
			poly_thin = info->xform->thin_draw || info->xform->wireframe;
			for(p = polylist_first(&font->Symbol[*string].polys); p != NULL; p = polylist_next(p))
				draw_text_poly(info, p, mx, x, xordraw, poly_thin, xordx, xordy, cb, cb_ctx);

			/* move on to next cursor position */
			x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
		}
		else {
			/* the default symbol is a filled box */
			rnd_coord_t size = (font->DefaultSymbol.X2 - font->DefaultSymbol.X1) * 6 / 5;
			rnd_coord_t px[4], py[4];

			px[0] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y1));
			py[0] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y1));
			px[1] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y1));
			py[1] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y1));
			px[2] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y2));
			py[2] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y2));
			px[3] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y2));
			py[3] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y2));

			/* draw move on to next cursor position */
			if ((cb == NULL) && (xordraw || (info->xform->thin_draw || info->xform->wireframe))) {
				if (xordraw) {
					rnd_render->draw_line(pcb_crosshair.GC, px[0] + xordx, py[0] + xordy, px[1] + xordx, py[1] + xordy);
					rnd_render->draw_line(pcb_crosshair.GC, px[1] + xordx, py[1] + xordy, px[2] + xordx, py[2] + xordy);
					rnd_render->draw_line(pcb_crosshair.GC, px[2] + xordx, py[2] + xordy, px[3] + xordx, py[3] + xordy);
					rnd_render->draw_line(pcb_crosshair.GC, px[3] + xordx, py[3] + xordy, px[0] + xordx, py[0] + xordy);
				}
				else {
					rnd_render->draw_line(pcb_draw_out.fgGC, px[0], py[0], px[1], py[1]);
					rnd_render->draw_line(pcb_draw_out.fgGC, px[1], py[1], px[2], py[2]);
					rnd_render->draw_line(pcb_draw_out.fgGC, px[2], py[2], px[3], py[3]);
					rnd_render->draw_line(pcb_draw_out.fgGC, px[3], py[3], px[0], py[0]);
				}
			}
			else {
				if (cb != NULL) {
					pcb_poly_t po;
					rnd_point_t pt[4], *p;

					memset(&po, 0, sizeof(po));
					po.type = PCB_OBJ_POLY;
					po.PointN = 4;
					po.Points = pt;
					for(n = 0, p = po.Points; n < po.PointN; n++,p++) {
						p->X = px[n];
						p->Y = py[n];
					}
					cb(cb_ctx, (pcb_any_obj_t *)&po);
				}
				else
					rnd_render->fill_polygon(pcb_draw_out.fgGC, 4, px, py);
			}
			x += size;
		}
		string++;
	}
}

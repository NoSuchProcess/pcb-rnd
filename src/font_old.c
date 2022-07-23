/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

/* Old font support. Will be replaced by rnd_font_*().
   Note: glyphs are called symbols here */

#define STEP_SYMBOLLINE 10

static void pcb_font_load_internal(pcb_font_t *font)
{
	int n, l;
	memset(font, 0, sizeof(pcb_font_t));
	font->MaxWidth  = embf_maxx - embf_minx;
	font->MaxHeight = embf_maxy - embf_miny;
	for(n = 0; n < sizeof(embf_font) / sizeof(embf_font[0]); n++) {
		if (embf_font[n].delta != 0) {
			pcb_symbol_t *s = font->Symbol + n;
			embf_line_t *lines = embf_font[n].lines;

			for(l = 0; l < embf_font[n].num_lines; l++) {
				rnd_coord_t x1 = RND_MIL_TO_COORD(lines[l].x1);
				rnd_coord_t y1 = RND_MIL_TO_COORD(lines[l].y1);
				rnd_coord_t x2 = RND_MIL_TO_COORD(lines[l].x2);
				rnd_coord_t y2 = RND_MIL_TO_COORD(lines[l].y2);
				rnd_coord_t th = RND_MIL_TO_COORD(lines[l].th);
				pcb_font_new_line_in_sym(s, x1, y1, x2, y2, th);
			}

			s->Valid = 1;
			s->Delta = RND_MIL_TO_COORD(embf_font[n].delta);
		}
	}
	pcb_font_set_info(font);
}

/* transforms symbol coordinates so that the left edge of each symbol
   is at the zero position. The y coordinates are moved so that min(y) = 0 */
void pcb_font_set_info(pcb_font_t *Ptr)
{
	rnd_cardinal_t i, j;
	pcb_symbol_t *symbol;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_poly_t *poly;
	rnd_coord_t totalminy = RND_MAX_COORD;

	/* calculate cell with and height (is at least PCB_DEFAULT_CELLSIZE)
	   maximum cell width and height
	   minimum x and y position of all lines */
	Ptr->MaxWidth = PCB_DEFAULT_CELLSIZE;
	Ptr->MaxHeight = PCB_DEFAULT_CELLSIZE;
	for (i = 0, symbol = Ptr->Symbol; i <= PCB_MAX_FONTPOSITION; i++, symbol++) {
		rnd_coord_t minx, miny, maxx, maxy;

		/* next one if the index isn't used or symbol is empty (SPACE) */
		if (!symbol->Valid || !symbol->LineN)
			continue;

		minx = miny = RND_MAX_COORD;
		maxx = maxy = 0;
		for (line = symbol->Line, j = symbol->LineN; j; j--, line++) {
			minx = MIN(minx, line->Point1.X);
			miny = MIN(miny, line->Point1.Y);
			minx = MIN(minx, line->Point2.X);
			miny = MIN(miny, line->Point2.Y);
			maxx = MAX(maxx, line->Point1.X);
			maxy = MAX(maxy, line->Point1.Y);
			maxx = MAX(maxx, line->Point2.X);
			maxy = MAX(maxy, line->Point2.Y);
		}

		for(arc = arclist_first(&symbol->arcs); arc != NULL; arc = arclist_next(arc)) {
			pcb_arc_bbox(arc);
			minx = MIN(minx, arc->BoundingBox.X1);
			miny = MIN(miny, arc->BoundingBox.Y1);
			maxx = MAX(maxx, arc->BoundingBox.X2);
			maxy = MAX(maxy, arc->BoundingBox.Y2);
		}

		for(poly = polylist_first(&symbol->polys); poly != NULL; poly = polylist_next(poly)) {
			pcb_poly_bbox(poly);
			minx = MIN(minx, poly->BoundingBox.X1);
			miny = MIN(miny, poly->BoundingBox.Y1);
			maxx = MAX(maxx, poly->BoundingBox.X2);
			maxy = MAX(maxy, poly->BoundingBox.Y2);
		}


		/* move symbol to left edge */
		for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
			pcb_line_move(line, -minx, 0);

		for(arc = arclist_first(&symbol->arcs); arc != NULL; arc = arclist_next(arc))
			pcb_arc_move(arc, -minx, 0);

		for(poly = polylist_first(&symbol->polys); poly != NULL; poly = polylist_next(poly))
			pcb_poly_move(poly, -minx, 0);

		/* set symbol bounding box with a minimum cell size of (1,1) */
		symbol->Width = maxx - minx + 1;
		symbol->Height = maxy + 1;

		/* check total min/max  */
		Ptr->MaxWidth = MAX(Ptr->MaxWidth, symbol->Width);
		Ptr->MaxHeight = MAX(Ptr->MaxHeight, symbol->Height);
		totalminy = MIN(totalminy, miny);
	}

	/* move coordinate system to the upper edge (lowest y on screen) */
	for (i = 0, symbol = Ptr->Symbol; i <= PCB_MAX_FONTPOSITION; i++, symbol++) {
		if (symbol->Valid) {
			symbol->Height -= totalminy;
			for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
				pcb_line_move(line, 0, -totalminy);

			for(arc = arclist_first(&symbol->arcs); arc != NULL; arc = arclist_next(arc))
				pcb_arc_move(arc, 0, -totalminy);

			for(poly = polylist_first(&symbol->polys); poly != NULL; poly = polylist_next(poly))
				pcb_poly_move(poly, 0, -totalminy);
		}
	}

	/* setup the box for the default symbol */
	Ptr->DefaultSymbol.X1 = Ptr->DefaultSymbol.Y1 = 0;
	Ptr->DefaultSymbol.X2 = Ptr->DefaultSymbol.X1 + Ptr->MaxWidth;
	Ptr->DefaultSymbol.Y2 = Ptr->DefaultSymbol.Y1 + Ptr->MaxHeight;
}

/* creates a new line in a symbol */
pcb_line_t *pcb_font_new_line_in_sym(pcb_symbol_t *Symbol, rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, rnd_coord_t Thickness)
{
	pcb_line_t *line = Symbol->Line;

	/* realloc new memory if necessary and clear it */
	if (Symbol->LineN >= Symbol->LineMax) {
		Symbol->LineMax += STEP_SYMBOLLINE;
		line = (pcb_line_t *) realloc(line, Symbol->LineMax * sizeof(pcb_line_t));
		Symbol->Line = line;
		memset(line + Symbol->LineN, 0, STEP_SYMBOLLINE * sizeof(pcb_line_t));
	}

	/* copy values */
	line = line + Symbol->LineN++;
	line->Point1.X = X1;
	line->Point1.Y = Y1;
	line->Point2.X = X2;
	line->Point2.Y = Y2;
	line->Thickness = Thickness;
	return line;
}

pcb_poly_t *pcb_font_new_poly_in_sym(pcb_symbol_t *Symbol, int num_points)
{
	pcb_poly_t *p = calloc(sizeof(pcb_poly_t), 1);
	if (num_points > 0) {
		p->PointN = p->PointMax = num_points;
		p->Points = malloc(sizeof(rnd_point_t) * num_points);
	}
	polylist_insert(&Symbol->polys, p);
	return p;
}

pcb_arc_t *pcb_font_new_arc_in_sym(pcb_symbol_t *Symbol, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, rnd_angle_t start, rnd_angle_t delta, rnd_coord_t thickness)
{
	pcb_arc_t *a = calloc(sizeof(pcb_arc_t), 1);
	a->X = cx;
	a->Y = cy;
	a->Height = a->Width = r;
	a->StartAngle = start;
	a->Delta = delta;
	a->Thickness = thickness;
	arclist_insert(&Symbol->arcs, a);
	return a;
}

void pcb_font_clear_symbol(pcb_symbol_t *s)
{
	pcb_poly_t *p;
	pcb_arc_t *a;


	s->Valid = 0;
	s->Width = 0;
	s->Delta = 0;

	s->LineN = 0;

	for(p = polylist_first(&s->polys); p != NULL; p = polylist_first(&s->polys)) {
		polylist_remove(p);
		pcb_poly_free_fields(p);
		free(p);
	}

	for(a = arclist_first(&s->arcs); a != NULL; a = arclist_first(&s->arcs)) {
		arclist_remove(a);
		free(a);
	}

}

void pcb_font_free_symbol(pcb_symbol_t *s)
{

	pcb_font_clear_symbol(s);

	free(s->Line);
	memset (s, 0, sizeof(pcb_symbol_t));
}

void pcb_font_free(pcb_font_t *f)
{
	int i;
	for (i = 0; i <= PCB_MAX_FONTPOSITION; i++)
		pcb_font_free_symbol(&f->Symbol[i]);

	free(f->name);
	f->name = NULL;
	f->id = -1;
}

static void copy_font(pcb_font_t *dst, pcb_font_t *src)
{
	int i;
	
	memcpy(dst, src, sizeof(pcb_font_t));
	for (i = 0; i <= PCB_MAX_FONTPOSITION; i++) {
		pcb_poly_t *p_src;
		pcb_arc_t *a_src;
		
		if (src->Symbol[i].Line) {
			dst->Symbol[i].Line = malloc(sizeof(pcb_line_t) * src->Symbol[i].LineMax);
			memcpy(dst->Symbol[i].Line, src->Symbol[i].Line, sizeof(pcb_line_t) * src->Symbol[i].LineN);
		}

		memset(&dst->Symbol[i].polys, 0, sizeof(polylist_t));
		for(p_src = polylist_first(&src->Symbol[i].polys); p_src != NULL; p_src = polylist_next(p_src)) {
			pcb_poly_t *p_dst = pcb_font_new_poly_in_sym(&dst->Symbol[i], p_src->PointN);
			memcpy(p_dst->Points, p_src->Points, p_src->PointN * sizeof(rnd_point_t));
		}

		memset(&dst->Symbol[i].arcs, 0, sizeof(arclist_t));
		for(a_src = arclist_first(&src->Symbol[i].arcs); a_src != NULL; a_src = arclist_next(a_src)) {
			pcb_font_new_arc_in_sym(&dst->Symbol[i], a_src->X, a_src->Y, a_src->Width,
				a_src->StartAngle, a_src->Delta, a_src->Thickness);
		}
	}
	if (src->name != NULL)
		dst->name = rnd_strdup(src->name);
}

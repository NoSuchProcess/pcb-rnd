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

/* font support. Note: glyphs are called symbols here */

#include "config.h"

#include <string.h>
#include <genht/hash.h>

#include "font.h"
#include "board.h"
#include "conf_core.h"
#include "error.h"
#include "plug_io.h"
#include "paths.h"
#include "compat_misc.h"
#include "event.h"
#include "file_loaded.h"

#define STEP_SYMBOLLINE 10

typedef struct embf_line_s {
	int x1, y1, x2, y2, th;
} embf_line_t;

typedef struct embf_font_s {
	int delta;
	embf_line_t *lines;
	int num_lines;
} embf_font_t;

#include "font_internal.c"

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
				pcb_coord_t x1 = PCB_MIL_TO_COORD(lines[l].x1);
				pcb_coord_t y1 = PCB_MIL_TO_COORD(lines[l].y1);
				pcb_coord_t x2 = PCB_MIL_TO_COORD(lines[l].x2);
				pcb_coord_t y2 = PCB_MIL_TO_COORD(lines[l].y2);
				pcb_coord_t th = PCB_MIL_TO_COORD(lines[l].th);
				pcb_font_new_line_in_sym(s, x1, y1, x2, y2, th);
			}

			s->Valid = 1;
			s->Delta = PCB_MIL_TO_COORD(embf_font[n].delta);
		}
	}
	pcb_font_set_info(font);
}

static int pcb_parse_font_default(pcb_font_t *ptr, const char *filename)
{
	int res = pcb_parse_font(ptr, filename);
	if (res == 0)
		pcb_file_loaded_set_at("font", "default", filename, "original default font");
	return res;
}

/* parses a file with font information and installs it into the provided PCB
 * checks directories given as colon separated list by resource fontPath
 * if the fonts filename doesn't contain a directory component */
void pcb_font_create_default(pcb_board_t *pcb)
{
	int res = -1;
	pcb_io_err_inhibit_inc();
	conf_list_foreach_path_first(res, &conf_core.rc.default_font_file, pcb_parse_font_default(&pcb->fontkit.dflt, __path__));
	pcb_io_err_inhibit_dec();

	if (res != 0) {
		const char *s;
		gds_t buff;
		s = conf_concat_strlist(&conf_core.rc.default_font_file, &buff, NULL, ':');
		pcb_message(PCB_MSG_WARNING, "Can't find font-symbol-file. Searched: '%s'; falling back to the embedded default font\n", s);
		pcb_font_load_internal(&pcb->fontkit.dflt);
		pcb_file_loaded_set_at("font", "default", "<internal>", "original default font");
		gds_uninit(&buff);
	}
}

/* transforms symbol coordinates so that the left edge of each symbol
 * is at the zero position. The y coordinates are moved so that min(y) = 0 */
void pcb_font_set_info(pcb_font_t *Ptr)
{
	pcb_cardinal_t i, j;
	pcb_symbol_t *symbol;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_poly_t *poly;
	pcb_coord_t totalminy = PCB_MAX_COORD;

	/* calculate cell with and height (is at least PCB_DEFAULT_CELLSIZE)
	 * maximum cell width and height
	 * minimum x and y position of all lines
	 */
	Ptr->MaxWidth = PCB_DEFAULT_CELLSIZE;
	Ptr->MaxHeight = PCB_DEFAULT_CELLSIZE;
	for (i = 0, symbol = Ptr->Symbol; i <= PCB_MAX_FONTPOSITION; i++, symbol++) {
		pcb_coord_t minx, miny, maxx, maxy;

		/* next one if the index isn't used or symbol is empty (SPACE) */
		if (!symbol->Valid || !symbol->LineN)
			continue;

		minx = miny = PCB_MAX_COORD;
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
pcb_line_t *pcb_font_new_line_in_sym(pcb_symbol_t *Symbol, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness)
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
		p->Points = malloc(sizeof(pcb_point_t) * num_points);
	}
	polylist_insert(&Symbol->polys, p);
	return p;
}

pcb_arc_t *pcb_font_new_arc_in_sym(pcb_symbol_t *Symbol, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r, pcb_angle_t start, pcb_angle_t delta, pcb_coord_t thickness)
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


static pcb_font_t *pcb_font_(pcb_board_t *pcb, pcb_font_id_t id, int fallback, int unlink)
{
	if (id <= 0) {
		do_default:;
		if (unlink) {
			pcb_font_t *f = malloc(sizeof(pcb_font_t));
			memcpy(f, &pcb->fontkit.dflt, sizeof(pcb_font_t));
			memset(&pcb->fontkit.dflt, 0, sizeof(pcb_font_t));
			return f;
		}
		else
			return &pcb->fontkit.dflt;
	}

	if (pcb->fontkit.hash_inited) {
		pcb_font_t *f = htip_get(&pcb->fontkit.fonts, id);
		if (f != NULL) {
			if (unlink)
				htip_popentry(&pcb->fontkit.fonts, id);
			return f;
		}
	}

	if (fallback)
		goto do_default;

	return NULL;
}

pcb_font_t *pcb_font(pcb_board_t *pcb, pcb_font_id_t id, int fallback)
{
	return pcb_font_(pcb, id, fallback, 0);
}

pcb_font_t *pcb_font_unlink(pcb_board_t *pcb, pcb_font_id_t id)
{
	return pcb_font_(pcb, id, 0, 1);
}


static void hash_setup(pcb_fontkit_t *fk)
{
	if (fk->hash_inited)
		return;

	htip_init(&fk->fonts, longhash, longkeyeq);
	fk->hash_inited = 1;
}

pcb_font_t *pcb_new_font(pcb_fontkit_t *fk, pcb_font_id_t id, const char *name)
{
	pcb_font_t *f;

	if (id == 0)
		return NULL;

	if (id < 0)
		id = fk->last_id + 1;

	hash_setup(fk);

	/* do not attempt to overwrite/reuse existing font of the same ID, rather report error */
	f = htip_get(&fk->fonts, id);
	if (f != NULL)
		return NULL;

	f = calloc(sizeof(pcb_font_t), 1);
	htip_set(&fk->fonts, id, f);
	if (name != NULL)
		f->name = pcb_strdup(name);
	f->id = id;

	if (f->id > fk->last_id)
		fk->last_id = f->id;

	return f;
}

void pcb_font_free(pcb_font_t *f)
{
	int i;
	for (i = 0; i <= PCB_MAX_FONTPOSITION; i++) {
		pcb_poly_t *p;
		pcb_arc_t *a;

		free(f->Symbol[i].Line);

		for(p = polylist_first(&f->Symbol[i].polys); p != NULL; p = polylist_first(&f->Symbol[i].polys)) {
			polylist_remove(p);
			pcb_poly_free_fields(p);
			free(p);
		}

		for(a = arclist_first(&f->Symbol[i].arcs); a != NULL; a = arclist_first(&f->Symbol[i].arcs)) {
			arclist_remove(a);
			free(a);
		}
		
		memset (&f->Symbol[i], 0, sizeof(f->Symbol[0]));
	}
	free(f->name);
	f->name = NULL;
	f->id = -1;
}

void pcb_fontkit_free(pcb_fontkit_t *fk)
{
	pcb_font_free(&fk->dflt);
	if (fk->hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&fk->fonts); e; e = htip_next(&fk->fonts, e))
			pcb_font_free(e->value);
		htip_uninit(&fk->fonts);
		fk->hash_inited = 0;
	}
	fk->last_id = 0;
}

void pcb_fontkit_reset(pcb_fontkit_t *fk)
{
	if (fk->hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&fk->fonts); e; e = htip_first(&fk->fonts)) {
			pcb_font_free(e->value);
			htip_delentry(&fk->fonts, e);
		}
	}
	fk->last_id = 0;
}

static void update_last_id(pcb_fontkit_t *fk)
{
	pcb_font_id_t id;
	for (id = fk->last_id; id > 0; id--) {
		if (htip_has(&fk->fonts, id))
			break;
	}
	fk->last_id = id;
}

int pcb_del_font(pcb_fontkit_t *fk, pcb_font_id_t id)
{
	htip_entry_t *e;

	if ((id == 0) || (!fk->hash_inited) || (htip_get(&fk->fonts, id) == NULL))
		return -1;

	e = htip_popentry(&fk->fonts, id);
	pcb_font_free(e->value);
	pcb_event(&PCB->hidlib, PCB_EVENT_FONT_CHANGED, "i", id);
	if (id == fk->last_id)
		update_last_id(fk);
	return 0;
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
			memcpy(p_dst->Points, p_src->Points, p_src->PointN * sizeof(pcb_point_t));
		}

		memset(&dst->Symbol[i].arcs, 0, sizeof(arclist_t));
		for(a_src = arclist_first(&src->Symbol[i].arcs); a_src != NULL; a_src = arclist_next(a_src)) {
			pcb_font_new_arc_in_sym(&dst->Symbol[i], a_src->X, a_src->Y, a_src->Width,
															a_src->StartAngle, a_src->Delta, a_src->Thickness);
		}
	}
	if (src->name != NULL)
		dst->name = pcb_strdup(src->name);
}

int pcb_move_font(pcb_fontkit_t *fk, pcb_font_id_t src, pcb_font_id_t dst)
{
	htip_entry_t *e;
	pcb_font_t *src_font;
	
	if ((!fk->hash_inited) || (htip_get(&fk->fonts, src) == NULL))
		return -1;
	
	pcb_del_font(fk, dst);
	
	e = htip_popentry(&fk->fonts, src);
	src_font = e->value;
	if (dst == 0) {
		pcb_font_free (&fk->dflt);
		copy_font (&fk->dflt, src_font);
		pcb_font_free (src_font);
		fk->dflt.id = 0;
	} else {
		htip_set(&fk->fonts, dst, src_font);
		src_font->id = dst;
	}
	if (src == fk->last_id)
		update_last_id (fk);
	fk->last_id = MAX(fk->last_id, dst);
	pcb_event(&PCB->hidlib, PCB_EVENT_FONT_CHANGED, "i", src);
	pcb_event(&PCB->hidlib, PCB_EVENT_FONT_CHANGED, "i", dst);

	return 0;
}


/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - footprint import/export
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
 */

#include "config.h"
#include <stdio.h>
#include <math.h>
#include <genht/htsp.h>
#include <genht/htip.h>
#include <genht/hash.h>

#include "footprint.h"
#include "parse.h"

#include "unit.h"
#include "attrib.h"
#include "error.h"
#include "data.h"
#include "board.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "obj_line.h"
#include "obj_arc.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "../src_plugins/lib_compat_help/subc_help.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"

static void print_sqpad_coords(FILE *f, pcb_any_line_t *Pad, pcb_coord_t cx, pcb_coord_t cy)
{
	pcb_coord_t x[4], y[4];

	pcb_sqline_to_rect((pcb_line_t *)Pad, x, y);
	pcb_fprintf(f, " %.9mm %.9mm", x[0] - cx, y[0] - cy);
	pcb_fprintf(f, " %.9mm %.9mm", x[1] - cx, y[1] - cy);
	pcb_fprintf(f, " %.9mm %.9mm", x[2] - cx, y[2] - cy);
	pcb_fprintf(f, " %.9mm %.9mm", x[3] - cx, y[3] - cy);
}

#define elem_layer(elem, obj) \
	((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (elem)) == PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (obj))) ? "primary" : "secondary")

#define safe_term_num(out, obj, buff) \
do { \
	out = obj->Number; \
	if ((out == NULL) || (*out == '\0')) { \
		sprintf(buff, "%p", (void *)obj); \
		out = buff; \
		if (out[1] == 'x') \
			out+=2; \
	} \
} while(0)

#define print_term(pnum, obj) \
do { \
	if (htsp_get(&terms, pnum) == NULL) { \
		htsp_set(&terms, pcb_strdup(pnum), obj); \
		fprintf(f, "	term %s %s - %s\n", pnum, pnum, obj->Name); \
	} \
} while(0)

#define print_terma(terms, pnum, obj) \
do { \
	if (htsp_get(terms, pnum) == NULL) { \
		htsp_set(terms, pcb_strdup(pnum), obj); \
		fprintf(f, "	term %s %s - %s\n", pnum, pnum, pnum); \
	} \
} while(0)

#define TERM_NAME(term) ((term == NULL) ? "-" : term)

#define PIN_PLATED(obj) (PCB_FLAG_TEST(PCB_FLAG_HOLE, (obj)) ? "unplated" : "-")

#define get_layer_props(lyt, lloc, ltyp, invalid) \
	ltyp = lloc = NULL; \
	if (lyt & PCB_LYT_LOGICAL) { invalid; } \
	else if (lyt & PCB_LYT_TOP) lloc = "primary"; \
	else if (lyt & PCB_LYT_BOTTOM) lloc = "secondary"; \
	else if (lyt & PCB_LYT_INTERN) lloc = "inner"; \
	else if ((lyt & PCB_LYT_ANYWHERE) == 0) lloc = "all"; \
	if (lyt & PCB_LYT_COPPER) ltyp = "copper"; \
	else if (lyt & PCB_LYT_SILK) ltyp = "silk"; \
	else if (lyt & PCB_LYT_MASK) ltyp = "mask"; \
	else if (lyt & PCB_LYT_PASTE) ltyp = "paste"; \
	else { invalid; }

int tedax_pstk_fsave(pcb_pstk_t *padstack, pcb_coord_t ox, pcb_coord_t oy, FILE *f)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(padstack);
	pcb_pstk_tshape_t *tshp;
	pcb_pstk_shape_t *shp;
	int n, i;

	if (proto == NULL) {
		pcb_message(PCB_MSG_ERROR, "tEDAx footprint export: omitting subc padstack with invalid prototype\n");
		return 1;
	}
	if (proto->hdia > 0)
		pcb_fprintf(f, "	hole %s %mm %mm %mm %s\n", TERM_NAME(padstack->term), padstack->x - ox, padstack->y - oy, proto->hdia, proto->hplated ? "-" : "unplated");

	tshp = pcb_pstk_get_tshape(padstack);
	for(n = 0, shp = tshp->shape; n < tshp->len; n++,shp++) {
		const char *lloc, *ltyp;
		pcb_coord_t clr;
		
		get_layer_props(shp->layer_mask, lloc, ltyp, continue);
		clr = padstack->Clearance > 0 ? padstack->Clearance : shp->clearance;
		switch(shp->shape) {
			case PCB_PSSH_HSHADOW:
				break;
			case PCB_PSSH_CIRC:
				pcb_fprintf(f, "	fillcircle %s %s %s %mm %mm %mm %mm\n", lloc, ltyp, TERM_NAME(padstack->term),
					padstack->x + shp->data.circ.x - ox, padstack->y + shp->data.circ.y - oy,
					shp->data.circ.dia/2, clr);
				break;
			case PCB_PSSH_LINE:
				if (shp->data.line.square) {
					pcb_any_line_t tmp;
					tmp.Point1.X = shp->data.line.x1 + padstack->x;
					tmp.Point1.Y = shp->data.line.y1 + padstack->y;
					tmp.Point2.X = shp->data.line.x2 + padstack->x;
					tmp.Point2.Y = shp->data.line.y2 + padstack->y;
					tmp.Thickness = shp->data.line.thickness;
					pcb_fprintf(f, "	polygon %s %s %s %mm 4", lloc, ltyp, TERM_NAME(padstack->term), clr);
					print_sqpad_coords(f, &tmp, ox, oy);
					pcb_fprintf(f, "\n");
				}
				else {
					pcb_fprintf(f, "	line %s %s %s %mm %mm %mm %mm %mm %mm\n", lloc, ltyp, TERM_NAME(padstack->term),
						shp->data.line.x1 + padstack->x - ox, shp->data.line.y1 + padstack->y - oy,
						shp->data.line.x2 + padstack->x - ox, shp->data.line.y2 + padstack->y - oy,
						shp->data.line.thickness, clr);
				}
				break;
			case PCB_PSSH_POLY:
				pcb_fprintf(f, "	polygon %s %s %s %.06mm %ld", lloc, ltyp, TERM_NAME(padstack->term),
					clr, shp->data.poly.len);
				for(i = 0; i < shp->data.poly.len; i++)
					pcb_fprintf(f, " %.06mm %.06mm",
						shp->data.poly.x[i] + padstack->x - ox,
						shp->data.poly.y[i] + padstack->y - oy);
				pcb_fprintf(f, "\n");
				break;
		}
	}
	return 0;
}

int tedax_fp_fsave(pcb_data_t *data, FILE *f)
{
	htsp_t terms;
	htsp_entry_t *e;

	htsp_init(&terms, strhash, strkeyeq);

	fprintf(f, "tEDAx v1\n");

	PCB_SUBC_LOOP(data)
	{
		pcb_coord_t ox = 0, oy = 0;
		const char *fpname = NULL;
		int l;

		fpname = pcb_attribute_get(&subc->Attributes, "tedax::footprint");
		if (fpname == NULL)
			fpname = pcb_attribute_get(&subc->Attributes, "visible_footprint");
		if (fpname == NULL)
			fpname = pcb_attribute_get(&subc->Attributes, "footprint");
		if ((fpname == NULL) && (subc->refdes != NULL))
			fpname = subc->refdes;
		if (fpname == NULL)
			fpname = "-";

		pcb_subc_get_origin(subc, &ox, &oy);

		fprintf(f, "\nbegin footprint v1 %s\n", fpname);

		for(l = 0; l < subc->data->LayerN; l++) {
			pcb_layer_t *ly = &subc->data->Layer[l];
			pcb_layer_type_t lyt = pcb_layer_flags_(ly);
			const char *lloc, *ltyp;

			get_layer_props(lyt, lloc, ltyp, continue);

			PCB_LINE_LOOP(ly)
			{
				if (line->term != NULL) print_terma(&terms, line->term, line);
				pcb_fprintf(f, "	line %s %s %s %mm %mm %mm %mm %mm %mm\n", lloc, ltyp, TERM_NAME(line->term),
					line->Point1.X - ox, line->Point1.Y - oy, line->Point2.X - ox, line->Point2.Y - oy,
					line->Thickness, line->Clearance);
			}
			PCB_END_LOOP;

			PCB_ARC_LOOP(ly)
			{
				if (arc->term != NULL) print_terma(&terms, arc->term, arc);
				pcb_fprintf(f, "	arc %s %s %s %mm %mm %mm %f %f %mm %mm\n", lloc, ltyp, TERM_NAME(arc->term),
					arc->X - ox, arc->Y - oy, (arc->Width + arc->Height)/2, arc->StartAngle, arc->Delta,
					arc->Thickness, arc->Clearance);
			}
			PCB_END_LOOP;


			PCB_POLY_LOOP(ly)
			{
				int go;
				long numpt = 0;
				pcb_coord_t x, y;
				pcb_poly_it_t it;

				pcb_poly_iterate_polyarea(polygon->Clipped, &it);
				if (pcb_poly_contour(&it) == NULL) {
					pcb_message(PCB_MSG_ERROR, "tEDAx footprint export: omitting subc polygon with no clipped contour\n");
					continue;
				}

				/* iterate over the vectors of the contour */
				for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y))
					numpt++;

				if (pcb_poly_hole_first(&it) != NULL)
					pcb_message(PCB_MSG_ERROR, "tEDAx footprint export: omitting subc polygon holes\n");

				if (numpt == 0) {
					pcb_message(PCB_MSG_ERROR, "tEDAx footprint export: omitting subc polygon with no points\n");
					continue;
				}

				if (polygon->term != NULL) print_terma(&terms, polygon->term, polygon);
				pcb_fprintf(f, "	polygon %s %s %s %.06mm %ld", lloc, ltyp, TERM_NAME(polygon->term),
					(PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, polygon) ? 0 : polygon->Clearance),
					numpt);

				/* rewind the iterator*/
				pcb_poly_iterate_polyarea(polygon->Clipped, &it);
				pcb_poly_contour(&it);
				for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y))
					pcb_fprintf(f, " %.06mm %.06mm", x - ox, y - oy);
				pcb_fprintf(f, "\n");
			}
			PCB_END_LOOP;
		}

		PCB_PADSTACK_LOOP(subc->data)
		{
			if (padstack->term != NULL) print_terma(&terms, padstack->term, padstack);
			tedax_pstk_fsave(padstack, ox, oy, f);
		}
		PCB_END_LOOP;

		fprintf(f, "end footprint\n");

		for (e = htsp_first(&terms); e; e = htsp_next(&terms, e)) {
			free(e->key);
			htsp_delentry(&terms, e);
		}

	}
	PCB_END_LOOP;

	htsp_uninit(&terms);

	return 0;
}

int tedax_fp_save(pcb_data_t *data, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_fp_save(): can't open %s for writing\n", fn);
		return -1;
	}
	res = tedax_fp_fsave(data, f);
	fclose(f);
	return res;
}


/*******************************/

typedef struct {
	char *pinid;
	/* type can be ignored in pcb-rnd */
	char *name;

	/* put all objects on this per terminal list so the padstack converter can pick them up */
	vtp0_t objs;
} term_t;

static term_t *term_new(const char *pinid, const char *name)
{
	term_t *t = calloc(sizeof(term_t), 1);
	t->pinid = pcb_strdup(pinid);
	t->name = pcb_strdup(name);
	return t;
}

static void term_destroy(term_t *t)
{
	free(t->pinid);
	free(t->name);
	vtp0_uninit(&t->objs);
	free(t);
}

#define load_int(dst, src, msg) \
do { \
	char *end; \
	dst = strtol(src, &end, 10); \
	if (*end != '\0') { \
		pcb_message(PCB_MSG_ERROR, msg, src); \
		return -1; \
	} \
} while(0)

#define load_dbl(dst, src, msg) \
do { \
	char *end; \
	dst = strtod(src, &end); \
	if (*end != '\0') { \
		pcb_message(PCB_MSG_ERROR, msg, src); \
		return -1; \
	} \
} while(0)

#define load_val(dst, src, msg) \
do { \
	pcb_bool succ; \
	dst = pcb_get_value_ex(src, NULL, NULL, NULL, "mm", &succ); \
	if (!succ) { \
		pcb_message(PCB_MSG_ERROR, msg, src); \
		return -1; \
	} \
} while(0)

#define load_term(obj, src, msg) \
do { \
	int termid; \
	term_t *term; \
	if ((src[0] == '-') && (src[1] == '\0')) break; \
	load_int(termid, src, msg); \
	term = htip_get(&terms, termid); \
	if (term == NULL) { \
		pcb_message(PCB_MSG_ERROR, "undefined terminal %s - skipping footprint\n", src); \
		return -1; \
	} \
	pcb_attribute_put(&obj->Attributes, "term", term->name); \
	vtp0_append(&term->objs, obj); \
} while(0)

#define load_lloc(dst, src, msg) \
do { \
	if (strcmp(src, "secondary") == 0) \
		dst = 1; \
	else if (strcmp(src, "primary") == 0) \
		dst = 0; \
	else { \
		pcb_message(PCB_MSG_ERROR, msg, lloc); \
		return -1; \
	} \
} while(0)


static int load_poly(pcb_coord_t *px, pcb_coord_t *py, int maxpt, int argc, char *argv[])
{
	int max, n;
	load_int(max, argv[0], "invalid number of points '%s' in poly, skipping footprint\n");
	argc--;
	argv++;
	if (max*2 != argc) {
		pcb_message(PCB_MSG_ERROR, "invalid number of polygon points: expected %d coords got %d skipping footprint\n", max*2, argc);
		return -1;
	}
	for(n = 0; n < max; n++) {
		load_val(px[n], argv[n*2+0], "invalid X '%s' in poly, skipping footprint\n");
		load_val(py[n], argv[n*2+1], "invalid Y '%s' in poly, skipping footprint\n");
	}
	return max;
}

/* Returns a NULL terminated list of layers to operate on or NULL on error;
   not reentrant! */
static pcb_layer_t **subc_get_layer(pcb_subc_t *subc, const char *lloc, const char *ltyp)
{
	pcb_layer_type_t lyt = 0;
	static pcb_layer_t *layers[4];
	char name[128];

	memset(layers, 0, sizeof(layers));

	if (strcmp(ltyp, "copper") == 0) lyt |= PCB_LYT_COPPER;
	else if (strcmp(ltyp, "silk") == 0) lyt |= PCB_LYT_SILK;
	else if (strcmp(ltyp, "mask") == 0) lyt |= PCB_LYT_MASK;
	else if (strcmp(ltyp, "paste") == 0) lyt |= PCB_LYT_PASTE;
	else {
		pcb_message(PCB_MSG_ERROR, "tEDAx footprint load: invalid layer type %s\n", ltyp);
		return NULL;
	}

	if (strcmp(lloc, "all") == 0) {
		sprintf(name, "top_%s", ltyp);
		layers[0] = pcb_subc_get_layer(subc, lyt | PCB_LYT_TOP, -1, pcb_true, name, pcb_false);
		sprintf(name, "bottom_%s", ltyp);
		layers[1] = pcb_subc_get_layer(subc, lyt | PCB_LYT_BOTTOM, -1, pcb_true, name, pcb_false);
		if (lyt == PCB_LYT_COPPER) {
			sprintf(name, "intern_%s", ltyp);
			layers[2] = pcb_subc_get_layer(subc, lyt | PCB_LYT_INTERN, -1, pcb_true, name, pcb_false);
		}
		return layers;
	}

	if (strcmp(lloc, "primary") == 0) lyt |= PCB_LYT_TOP;
	else if (strcmp(lloc, "secondary") == 0) lyt |= PCB_LYT_BOTTOM;
	else if (strcmp(lloc, "inner") == 0) lyt |= PCB_LYT_INTERN;
	else {
		pcb_message(PCB_MSG_ERROR, "tEDAx footprint load: invalid layer location %s\n", lloc);
		return NULL;
	}

	sprintf(name, "%s_%s", lloc, ltyp);
	layers[0] = pcb_subc_get_layer(subc, lyt, -1, pcb_true, name, pcb_false);
	return layers;
}

/* Parse one footprint block */
static int tedax_parse_1fp_(pcb_subc_t *subc, FILE *fn, char *buff, int buff_size, char *argv[], int argv_size)
{
	int argc, termid, numpt, res = -1;
	htip_entry_t *ei;
	htip_t terms;
	term_t *term;
	pcb_coord_t px[256], py[256], clr;
	pcb_layer_t **ly;

	htip_init(&terms, longhash, longkeyeq);
	while((argc = tedax_getline(fn, buff, buff_size, argv, argv_size)) >= 0) {
		if ((argc == 5) && (strcmp(argv[0], "term") == 0)) {
			load_int(termid, argv[1], "invalid term ID '%s', skipping footprint\n");
			term = term_new(argv[2], argv[4]);
			htip_set(&terms, termid, term);
		}
		else if ((argc > 12) && (strcmp(argv[0], "polygon") == 0)) {
			pcb_poly_t *p;
			ly = subc_get_layer(subc, argv[1], argv[2]);
			if (ly == NULL) return -1;

			numpt = load_poly(px, py, (sizeof(px) / sizeof(px[0])), argc-5, argv+5);
			if (numpt < 0) {
				pcb_message(PCB_MSG_ERROR, "tEDAx footprint load: failed to load polygon\n");
				return -1;
			}
			load_val(clr, argv[4], "invalid clearance");

			for(; *ly != NULL; ly++) {
				int n;
				p = pcb_poly_new(*ly, clr, pcb_no_flags());
				if (p == NULL) {
					pcb_message(PCB_MSG_ERROR, "tEDAx footprint load: failed to create poly, skipping footprint\n");
					return -1;
				}
				for(n = 0; n < numpt; n++)
					pcb_poly_point_new(p, px[n], py[n]);

				load_term(p, argv[3], "invalid term ID for polygon: '%s', skipping footprint\n");

				pcb_add_poly_on_layer(*ly, p);
			}
		}
		else if ((argc == 10) && (strcmp(argv[0], "line") == 0)) {
			pcb_coord_t x1, y1, x2, y2, w, clr;
			pcb_line_t *l;

			ly = subc_get_layer(subc, argv[1], argv[2]);

			load_val(x1, argv[4], "invalid line x1");
			load_val(y1, argv[5], "invalid line y1");
			load_val(x2, argv[6], "invalid line x2");
			load_val(y2, argv[7], "invalid line y2");
			load_val(w, argv[8], "invalid line width");
			load_val(clr, argv[9], "invalid line clearance");

			for(; *ly != NULL; ly++) {
				l = pcb_line_new(*ly, x1, y1, x2, y2, w, clr, pcb_flag_make(PCB_FLAG_CLEARLINE));
				load_term(l, argv[3], "invalid term ID for line: '%s', skipping footprint\n");
			}
		}
		else if ((argc == 11) && (strcmp(argv[0], "arc") == 0)) {
			pcb_coord_t cx, cy, r, w;
			double sa, da;
			pcb_arc_t *a;

			ly = subc_get_layer(subc, argv[1], argv[2]);

			load_val(cx, argv[4], "invalid arc cx");
			load_val(cy, argv[5], "invalid arc cy");
			load_val(r, argv[6], "invalid arc radius");
			load_dbl(sa, argv[7], "invalid arc start angle");
			load_dbl(da, argv[8], "invalid arc delta angle");
			load_val(w, argv[9], "invalid arc width");

			for(; *ly != NULL; ly++) {
				a = pcb_arc_new(*ly, cx, cy, r, r, sa, da, w, clr, pcb_flag_make(PCB_FLAG_CLEARLINE), pcb_true);
				load_term(a, argv[3], "invalid term ID for arc: '%s', skipping footprint\n");
			}
		}
		else if ((argc == 6) && (strcmp(argv[0], "hole") == 0)) {
			pcb_coord_t cx, cy, d;
			int plated;
			pcb_pstk_t *ps;

			load_val(cx, argv[2], "invalid hole cx");
			load_val(cy, argv[3], "invalid hole cy");
			load_val(d, argv[4], "invalid hole radius");
			plated = !(strcmp(argv[5], "unplated") == 0);

			ps = pcb_pstk_new_hole(subc->data, cx, cy, d, plated);
			load_term(ps, argv[1], "invalid term ID for hole: '%s', skipping footprint\n");
		}
		else if ((argc == 8) && (strcmp(argv[0], "fillcircle") == 0)) {
			pcb_coord_t cx, cy, r, clr;
			pcb_line_t *l;

			ly = subc_get_layer(subc, argv[1], argv[2]);
			load_val(cx, argv[4], "invalid fillcircle cx");
			load_val(cy, argv[5], "invalid fillcircle cy");
			load_val(r, argv[6], "invalid fillcircle radius");
			load_val(clr, argv[7], "invalid fillcircle clearance");

			for(; *ly != NULL; ly++) {
				l = pcb_line_new(*ly, cx, cy, cx, cy, r*2, clr, pcb_flag_make(PCB_FLAG_CLEARLINE));
				load_term(l, argv[3], "invalid term ID for fillcircle: '%s', skipping footprint\n");
			}
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "footprint") == 0)) {
			res = 0;
			break;
		}
	}

	/* By now we have heavy terminals and a list of object for each terminal in
	   the terms hash. Iterate over the terminals and try to convert the objects
	   into one or more padstacks */
	for (ei = htip_first(&terms); ei; ei = htip_next(&terms, ei)) {
		term = ei->value;
		pcb_pstk_vect2pstk(subc->data, &term->objs, pcb_true);
		term_destroy(term);
	}
	htip_uninit(&terms);

	pcb_attribute_put(&subc->Attributes, "refdes", "X1");
	pcb_subc_add_refdes_text(subc, 0, 0, 0, 100, pcb_false);
	pcb_subc_create_aux(subc, 0, 0, 0.0, pcb_false);

	return res;
}

static int tedax_parse_1fp(pcb_data_t *data, FILE *fn, char *buff, int buff_size, char *argv[], int argv_size)
{
	pcb_subc_t *sc = pcb_subc_alloc();
	pcb_subc_reg(data, sc);

	if (tedax_parse_1fp_(sc, fn, buff, buff_size, argv, argv_size) != 0) {
		pcb_subc_free(sc);
		return -1;
	}

	return 0;
}


/* parse one or more footprint blocks */
static int tedax_parse_fp(pcb_data_t *data, FILE *fn, int multi)
{
	char line[520];
	char *argv[16];
	int found = 0;

	if (tedax_seek_hdr(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	do {
		if (tedax_seek_block(fn, "footprint", "v1", (found > 0), line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
			break;

		if (tedax_parse_1fp(data, fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
			return -1;
		found++;
	} while(multi);
	return 0;
}

int tedax_fp_load(pcb_data_t *data, const char *fn, int multi)
{
	FILE *f;
	int ret = 0;

	f = pcb_fopen(fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fn);
		return -1;
	}

	ret = tedax_parse_fp(data, f, multi);

	fclose(f);
	return ret;
}

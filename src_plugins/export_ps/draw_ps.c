/*
   This file is part of pcb-rnd and was part of gEDA/PCB but lacked proper
   copyright banner at the fork. It probably has the same copyright as
   gEDA/PCB as a whole in 2011.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include <librnd/core/math_helper.h>
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>

TODO("this needs to be moved too")
#include "../lib_compat_help/media.h"

#include "draw_ps.h"

static void use_gc(rnd_ps_t *pctx, rnd_hid_gc_t gc);

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;
	rnd_cap_style_t cap;
	rnd_coord_t width;
	unsigned char r, g, b;
	int erase;
	int faded;
} rnd_hid_gc_s;

void rnd_ps_start_file(rnd_ps_t *pctx, const char *swver)
{
	FILE *f = pctx->outf;
	time_t currenttime = time(NULL);

	fprintf(f, "%%!PS-Adobe-3.0\n");

	/* Document Structuring Conventions (DCS): */

	/* Start General Header Comments: */

	/* %%Title DCS provides text title for the document that is useful
	   for printing banner pages. */
	fprintf(f, "%%%%Title: %s\n", rnd_hid_export_fn(pctx->hidlib->filename));

	/* %%CreationDate DCS indicates the date and time the document was
	   created. Neither the date nor time need be in any standard
	   format. This comment is meant to be used purely for informational
	   purposes, such as printing on banner pages. */
	fprintf(f, "%%%%CreationDate: %s", asctime(localtime(&currenttime)));

	/* %%Creator DCS indicates the document creator, usually the name of
	   the document composition software. */
	fprintf(f, "%%%%Creator: %s\n", swver);

	/* %%Version DCS comment can be used to note the version and
	   revision number of a document or resource. A document manager may
	   wish to provide version control services, or allow substitution
	   of compatible versions/revisions of a resource or document.
	
	   The format should be in the form of 'procname':
	    <procname>::= < name> < version> < revision>
	    < name> ::= < text>
	    < version> ::= < real>
	    < revision> ::= < uint>
	
	   If a version numbering scheme is not used, these fields should
	   still be filled with a dummy value of 0.
	
	   There is currently no code in PCB to manage this revision number. */
	fprintf(f, "%%%%Version: (%s) 0.0 0\n", swver);

	/* %%PageOrder DCS is intended to help document managers determine
	   the order of pages in the document file, which in turn enables a
	   document manager optionally to reorder the pages.  'Ascend'-The
	   pages are in ascending order for example, 1-2-3-4-5-6. */
	fprintf(f, "%%%%PageOrder: Ascend\n");

	/* %%Pages: < numpages> | (atend) < numpages> ::= < uint> (Total
	   %%number of pages)
	
	   %%Pages DCS defines the number of virtual pages that a document
	   will image.  (atend) defers the count until the end of the file,
	   which is useful for dynamically generated contents. */
	fprintf(f, "%%%%Pages: (atend)\n");

	/* %%DocumentMedia: <name> <width> <height> <weight> <color> <type>
	   Substitute 0 or "" for N/A.  Width and height are in points (1/72").
	   Media sizes are in PCB units */
	rnd_fprintf(f, "%%%%DocumentMedia: %s %f %f 0 \"\" \"\"\n",
		pcb_media_data[pctx->media_idx].name,
		72 * RND_COORD_TO_INCH(pcb_media_data[pctx->media_idx].width),
		72 * RND_COORD_TO_INCH(pcb_media_data[pctx->media_idx].height));
	rnd_fprintf(f, "%%%%DocumentPaperSizes: %s\n", pcb_media_data[pctx->media_idx].name);

	/* End General Header Comments. */

	/* General Body Comments go here. Currently there are none. */

	/* %%EndComments DCS indicates an explicit end to the header
	   comments of the document.  All global DCS's must preceded
	   this.  A blank line gives an implicit end to the comments. */
	fprintf(f, "%%%%EndComments\n\n");
}

void rnd_ps_end_file(rnd_ps_t *pctx)
{
	FILE *f = pctx->outf;

	/* %%Trailer DCS must only occur once at the end of the document
	   script.  Any post-processing or cleanup should be contained in
	   the trailer of the document, which is anything that follows the
	   %%Trailer comment. Any of the document level structure comments
	   that were deferred by using the (atend) convention must be
	   mentioned in the trailer of the document after the %%Trailer
	   comment. */
	fprintf(f, "%%%%Trailer\n");

	/* %%Pages was deferred until the end of the document via the
	   (atend) mentioned, in the General Header section. */
	fprintf(f, "%%%%Pages: %d\n", pctx->pagecount);

	/* %%EOF DCS signifies the end of the document. When the document
	   manager sees this comment, it issues an end-of-file signal to the
	   PostScript interpreter.  This is done so system-dependent file
	   endings, such as Control-D and end-of-file packets, do not
	   confuse the PostScript interpreter. */
	fprintf(f, "%%%%EOF\n");
}

void rnd_ps_init(rnd_ps_t *pctx, rnd_hidlib_t *hidlib, FILE *f, int media_idx, int fillpage, double scale_factor)
{
	memset(pctx, 0, sizeof(rnd_ps_t));

	pctx->hidlib = hidlib;
	pctx->outf = f;

	pctx->media_idx = media_idx;
	pctx->media_width = pcb_media_data[media_idx].width;
	pctx->media_height = pcb_media_data[media_idx].height;
	pctx->ps_width = pctx->media_width - 2.0 * pcb_media_data[media_idx].margin_x;
	pctx->ps_height = pctx->media_height - 2.0 * pcb_media_data[media_idx].margin_y;

	pctx->fillpage = fillpage;
	pctx->scale_factor = scale_factor;
	if (pctx->fillpage) {
		double zx, zy;
		if (hidlib->size_x > hidlib->size_y) {
			zx = pctx->ps_height / hidlib->size_x;
			zy = pctx->ps_width / hidlib->size_y;
		}
		else {
			zx = pctx->ps_height / hidlib->size_y;
			zy = pctx->ps_width / hidlib->size_x;
		}
		pctx->scale_factor *= MIN(zx, zy);
	}

	/* cache */
	pctx->linewidth = -1;
	pctx->pagecount = 1;
	pctx->lastcap = -1;
	pctx->lastcolor = -1;
}

void rnd_ps_begin_toc(rnd_ps_t *pctx)
{
	/* %%Page DSC requires both a label and an ordinal */
	fprintf(pctx->outf, "%%%%Page: TableOfContents 1\n");
	fprintf(pctx->outf, "/Times-Roman findfont 24 scalefont setfont\n");
	fprintf(pctx->outf, "/rightshow { /s exch def s stringwidth pop -1 mul 0 rmoveto s show } def\n");
	fprintf(pctx->outf, "/y 72 9 mul def /toc { 100 y moveto show /y y 24 sub def } bind def\n");
	fprintf(pctx->outf, "/tocp { /y y 12 sub def 90 y moveto rightshow } bind def\n");

	pctx->doing_toc = 1;
	pctx->pagecount = 1;
	pctx->lastgroup = -1;
}

void rnd_ps_end_toc(rnd_ps_t *pctx)
{
}

void rnd_ps_begin_pages(rnd_ps_t *pctx)
{
	pctx->doing_toc = 0;
	pctx->pagecount = 1; /* Reset 'pagecount' if single file */
	pctx->lastgroup = -1;
}

void rnd_ps_end_pages(rnd_ps_t *pctx)
{
	if (pctx->outf != NULL)
		fprintf(pctx->outf, "showpage\n");
}

static void corner(FILE * fh, rnd_coord_t x, rnd_coord_t y, int dx, int dy)
{
	rnd_coord_t len = RND_MIL_TO_COORD(2000);
	rnd_coord_t len2 = RND_MIL_TO_COORD(200);
	rnd_coord_t thick = 0;

	/* Originally 'thick' used thicker lines.  Currently is uses
	   Postscript's "device thin" line - i.e. zero width means one
	   device pixel.  The code remains in case you want to make them
	   thicker - it needs to offset everything so that the *edge* of the
	   thick line lines up with the edge of the board, not the *center*
	   of the thick line. */

	rnd_fprintf(fh, "gsave %mi setlinewidth %mi %mi translate %d %d scale\n", thick * 2, x, y, dx, dy);
	rnd_fprintf(fh, "%mi %mi moveto %mi %mi %mi 0 90 arc %mi %mi lineto\n", len, thick, thick, thick, len2 + thick, thick, len);
	if (dx < 0 && dy < 0)
		rnd_fprintf(fh, "%mi %mi moveto 0 %mi rlineto\n", len2 * 2 + thick, thick, -len2);
	fprintf(fh, "stroke grestore\n");
}

int rnd_ps_printed_toc(rnd_ps_t *pctx, int group, const char *name)
{
	if (pctx->doing_toc) {
		if ((group < 0) || (group != pctx->lastgroup)) {
			if (pctx->pagecount == 1) {
				time_t currenttime = time(NULL);
				fprintf(pctx->outf, "30 30 moveto (%s) show\n", rnd_hid_export_fn(pctx->hidlib->filename));

				fprintf(pctx->outf, "(%d.) tocp\n", pctx->pagecount);
				fprintf(pctx->outf, "(Table of Contents \\(This Page\\)) toc\n");

				fprintf(pctx->outf, "(Created on %s) toc\n", asctime(localtime(&currenttime)));
				fprintf(pctx->outf, "( ) tocp\n");
			}

			pctx->pagecount++;
			pctx->lastgroup = group;
			fprintf(pctx->outf, "(%d.) tocp\n", pctx->single_page ? 2 : pctx->pagecount);
			fprintf(pctx->outf, "(%s) toc\n", name);
		}
		return 1;
	}
	return 0;
}

int rnd_ps_is_new_page(rnd_ps_t *pctx, int group)
{
	int newpage = 0;

	if ((group < 0) || (group != pctx->lastgroup))
		newpage = 1;
	if ((pctx->pagecount > 1) && pctx->single_page)
		newpage = 0;

	if (newpage) {
		pctx->lastgroup = group;
		pctx->pagecount++;
	}

	return newpage;
}

int rnd_ps_new_file(rnd_ps_t *pctx, FILE *new_f, const char *fn)
{
	int ern = errno;

	if (pctx->outf != NULL) {
		rnd_ps_end_file(pctx);
		fclose(pctx->outf);
	}
	pctx->outf = new_f;
	if (pctx->outf == NULL) {
		rnd_message(RND_MSG_ERROR, "rnd_ps_new_file(): failed to open %s: %s\n", fn, strerror(ern));
		return -1;
	}
	return 0;
}

double rnd_ps_page_frame(rnd_ps_t *pctx, int mirror_this, const char *layer_fn, int noscale)
{
	double boffset;

	/* %%Page DSC comment marks the beginning of the PostScript
	   language instructions that describe a particular
	   page. %%Page: requires two arguments: a page label and a
	   sequential page number. The label may be anything, but the
	   ordinal page number must reflect the position of that page in
	   the body of the PostScript file and must start with 1, not 0. */
	{
		gds_t tmp;
		gds_init(&tmp);
		fprintf(pctx->outf, "%%%%Page: %s %d\n", layer_fn, pctx->pagecount);
		gds_uninit(&tmp);
	}

	fprintf(pctx->outf, "/Helvetica findfont 10 scalefont setfont\n");
	if (pctx->legend) {
		gds_t tmp;
		fprintf(pctx->outf, "30 30 moveto (%s) show\n", rnd_hid_export_fn(pctx->hidlib->filename));

		gds_init(&tmp);
		if (pctx->hidlib->name)
			fprintf(pctx->outf, "30 41 moveto (%s, %s) show\n", pctx->hidlib->name, layer_fn);
		else
			fprintf(pctx->outf, "30 41 moveto (%s) show\n", layer_fn);
		gds_uninit(&tmp);

		if (mirror_this)
			fprintf(pctx->outf, "( \\(mirrored\\)) show\n");

		if (pctx->fillpage)
			fprintf(pctx->outf, "(, not to scale) show\n");
		else
			fprintf(pctx->outf, "(, scale = 1:%.3f) show\n", pctx->scale_factor);
	}
	fprintf(pctx->outf, "newpath\n");

	rnd_fprintf(pctx->outf, "72 72 scale %mi %mi translate\n", pctx->media_width / 2, pctx->media_height / 2);

	boffset = pctx->media_height / 2;
	if (pctx->hidlib->size_x > pctx->hidlib->size_y) {
		fprintf(pctx->outf, "90 rotate\n");
		boffset = pctx->media_width / 2;
		fprintf(pctx->outf, "%g %g scale %% calibration\n", pctx->calibration_y, pctx->calibration_x);
	}
	else
		fprintf(pctx->outf, "%g %g scale %% calibration\n", pctx->calibration_x, pctx->calibration_y);

	if (mirror_this)
		fprintf(pctx->outf, "1 -1 scale\n");


	fprintf(pctx->outf, "%g dup neg scale\n", noscale ? 1.0 : pctx->scale_factor);
	rnd_fprintf(pctx->outf, "%mi %mi translate\n", -pctx->hidlib->size_x / 2, -pctx->hidlib->size_y / 2);

	return boffset;
}

void rnd_ps_page_background(rnd_ps_t *pctx, int has_outline, int page_is_route, rnd_coord_t min_wid)
{
	if (pctx->invert) {
		fprintf(pctx->outf, "/gray { 1 exch sub setgray } bind def\n");
		fprintf(pctx->outf, "/rgb { 1 1 3 { pop 1 exch sub 3 1 roll } for setrgbcolor } bind def\n");
	}
	else {
		fprintf(pctx->outf, "/gray { setgray } bind def\n");
		fprintf(pctx->outf, "/rgb { setrgbcolor } bind def\n");
	}

	if (!has_outline || pctx->invert) {
		if (page_is_route) {
			rnd_fprintf(pctx->outf,
								"0 setgray %mi setlinewidth 0 0 moveto 0 "
								"%mi lineto %mi %mi lineto %mi 0 lineto closepath %s\n",
								min_wid,
								pctx->hidlib->size_y, pctx->hidlib->size_x, pctx->hidlib->size_y, pctx->hidlib->size_x, pctx->invert ? "fill" : "stroke");
		}
	}

	if (pctx->align_marks) {
		corner(pctx->outf, 0, 0, -1, -1);
		corner(pctx->outf, pctx->hidlib->size_x, 0, 1, -1);
		corner(pctx->outf, pctx->hidlib->size_x, pctx->hidlib->size_y, 1, 1);
		corner(pctx->outf, 0, pctx->hidlib->size_y, -1, 1);
	}

	pctx->linewidth = -1;
	use_gc(pctx, NULL);  /* reset static vars */

	fprintf(pctx->outf,
					"/ts 1 def\n"
					"/ty ts neg def /tx 0 def /Helvetica findfont ts scalefont setfont\n"
					"/t { moveto lineto stroke } bind def\n"
					"/dr { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n"
					"      x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath stroke } bind def\n");
	fprintf(pctx->outf,"/r { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n"
					"     x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath fill } bind def\n"
					"/c { 0 360 arc fill } bind def\n"
					"/a { gsave setlinewidth translate scale 0 0 1 5 3 roll arc stroke grestore} bind def\n");
}

static int rnd_ps_me;
rnd_hid_gc_t rnd_ps_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(1, sizeof(rnd_hid_gc_s));
	rv->me_pointer = (rnd_hid_t *)&rnd_ps_me;
	rv->cap = rnd_cap_round;
	return rv;
}

void rnd_ps_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

void rnd_ps_set_drawing_mode(rnd_ps_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	pctx->drawing_mode = op;
}

void rnd_ps_set_color(rnd_ps_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color)
{
	if (pctx->drawing_mode == RND_HID_COMP_NEGATIVE) {
		gc->r = gc->g = gc->b = 255;
		gc->erase = 0;
	}
	else if (rnd_color_is_drill(color)) {
		gc->r = gc->g = gc->b = 255;
		gc->erase = 1;
	}
	else if (pctx->incolor) {
		gc->r = color->r;
		gc->g = color->g;
		gc->b = color->b;
		gc->erase = 0;
	}
	else {
		gc->r = gc->g = gc->b = 0;
		gc->erase = 0;
	}
}

void rnd_ps_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

void rnd_ps_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}

void rnd_ps_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

void rnd_ps_set_draw_faded(rnd_hid_gc_t gc, int faded)
{
	gc->faded = faded;
}

static void use_gc(rnd_ps_t *pctx, rnd_hid_gc_t gc)
{
	pctx->drawn_objs++;
	if (gc == NULL) {
		pctx->lastcap = pctx->lastcolor = -1;
		return;
	}
	if (gc->me_pointer != (rnd_hid_t *)&rnd_ps_me) {
		fprintf(stderr, "Fatal: GC from another HID passed to ps HID\n");
		abort();
	}
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
		fprintf(pctx->outf, "%d setlinecap %d setlinejoin\n", c, c);
		pctx->lastcap = gc->cap;
	}
#define CBLEND(gc) (((gc->r)<<24)|((gc->g)<<16)|((gc->b)<<8)|(gc->faded))
	if (pctx->lastcolor != CBLEND(gc)) {
		if (pctx->is_drill || pctx->is_mask) {
			fprintf(pctx->outf, "%d gray\n", (gc->erase || pctx->is_mask) ? 0 : 1);
			pctx->lastcolor = 0;
		}
		else {
			double r, g, b;
			r = gc->r;
			g = gc->g;
			b = gc->b;
			if (gc->faded) {
				r = (1 - pctx->fade_ratio) *255 + pctx->fade_ratio * r;
				g = (1 - pctx->fade_ratio) *255 + pctx->fade_ratio * g;
				b = (1 - pctx->fade_ratio) *255 + pctx->fade_ratio * b;
			}
			if (gc->r == gc->g && gc->g == gc->b)
				fprintf(pctx->outf, "%g gray\n", r / 255.0);
			else
				fprintf(pctx->outf, "%g %g %g rgb\n", r / 255.0, g / 255.0, b / 255.0);
			pctx->lastcolor = CBLEND(gc);
		}
	}
}

void rnd_ps_use_gc(rnd_ps_t *pctx, rnd_hid_gc_t gc)
{
	use_gc(pctx, gc);
}


void rnd_ps_draw_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(pctx, gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi %mi dr\n", x1, y1, x2, y2);
}

void rnd_ps_draw_line(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	if (x1 == x2 && y1 == y2) {
		rnd_coord_t w = gc->width / 2;
		if (gc->cap == rnd_cap_square)
			rnd_ps_fill_rect(pctx, gc, x1 - w, y1 - w, x1 + w, y1 + w);
		else
			rnd_ps_fill_circle(pctx, gc, x1, y1, w);
		return;
	}
	use_gc(pctx, gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi %mi t\n", x1, y1, x2, y2);
}

void rnd_ps_draw_arc(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_angle_t sa, ea;
	double w;

	if ((width == 0) && (height == 0)) {
		/* degenerate case, draw dot */
		rnd_ps_draw_line(pctx, gc, cx, cy, cx, cy);
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
	rnd_fprintf(pctx->outf, "%ma %ma %mi %mi %mi %mi %f a\n",
		sa, ea, -width, height, cx, cy, (double)(pctx->linewidth) / w);
}

void rnd_ps_fill_circle(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	use_gc(pctx, gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi c\n", cx, cy, radius);
}

void rnd_ps_fill_polygon_offs(rnd_ps_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int i;
	const char *op = "moveto";
	use_gc(pctx, gc);
	for (i = 0; i < n_coords; i++) {
		rnd_fprintf(pctx->outf, "%mi %mi %s\n", x[i]+dx, y[i]+dy, op);
		op = "lineto";
	}
	fprintf(pctx->outf, "fill\n");
}

typedef struct {
	rnd_coord_t x1, y1, x2, y2;
} lseg_t;

typedef struct {
	rnd_coord_t x, y;
} lpoint_t;

#define minmax(val, min, max) \
do { \
	if (val < min) min = val; \
	if (val > max) max = val; \
} while(0)

#define lsegs_append(x1_, y1_, x2_, y2_) \
do { \
	if (y1_ < y2_) { \
		lsegs[lsegs_used].x1 = x1_; \
		lsegs[lsegs_used].y1 = y1_; \
		lsegs[lsegs_used].x2 = x2_; \
		lsegs[lsegs_used].y2 = y2_; \
	} \
	else { \
		lsegs[lsegs_used].x2 = x1_; \
		lsegs[lsegs_used].y2 = y1_; \
		lsegs[lsegs_used].x1 = x2_; \
		lsegs[lsegs_used].y1 = y2_; \
	} \
	lsegs_used++; \
	minmax(y1_, lsegs_ymin, lsegs_ymax); \
	minmax(y2_, lsegs_ymin, lsegs_ymax); \
	minmax(x1_, lsegs_xmin, lsegs_xmax); \
	minmax(x2_, lsegs_xmin, lsegs_xmax); \
} while(0)

#define lseg_line(x1_, y1_, x2_, y2_) \
	do { \
			fprintf(global.f, "newpath\n"); \
			rnd_fprintf(global.f, "%mi %mi moveto\n", x1_, y1_); \
			rnd_fprintf(global.f, "%mi %mi lineto\n", x2_, y2_); \
			fprintf (global.f, "stroke\n"); \
	} while(0)

int coord_comp(const void *c1_, const void *c2_)
{
	const rnd_coord_t *c1 = c1_, *c2 = c2_;
	return *c1 < *c2;
}

void rnd_ps_fill_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(pctx, gc);
	if (x1 > x2) {
		rnd_coord_t t = x1;
		x1 = x2;
		x2 = t;
	}
	if (y1 > y2) {
		rnd_coord_t t = y1;
		y1 = y2;
		y2 = t;
	}
	rnd_fprintf(pctx->outf, "%mi %mi %mi %mi r\n", x1, y1, x2, y2);
}


void rnd_ps_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
}

int rnd_ps_gc_get_erase(rnd_hid_gc_t gc)
{
	return gc->erase;
}

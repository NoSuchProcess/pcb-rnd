#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <librnd/core/math_helper.h>
#include "board.h"
#include "data.h"
#include "layer.h"
#include <librnd/core/error.h>
#include "draw.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include "hid_cam.h"
#include <librnd/core/safe_fs.h>
#include "funchash_core.h"
#include "layer_vis.h"

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include "ps.h"
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>
#include "conf_core.h"
#include <librnd/core/compat_misc.h>
#include "stub_draw.h"
#include "../src_plugins/lib_compat_help/media.h"

#include "draw_ps.h"

const char *ps_cookie = "ps HID";

static int ps_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform);
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

static pcb_cam_t ps_cam;

static const rnd_export_opt_t ps_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --psfile <string>
Name of the postscript output file. Can contain a path.
@end ftable
%end-doc
*/
	{"psfile", "Postscript output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_psfile 0

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex drill-helper
@item --drill-helper
Draw small holes to center drill but in copper
@end ftable
%end-doc
*/
	{"drill-helper", "Draw small holes to center drill but in copper",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_drillhelper 1

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex align-marks
@item --align-marks
Print alignment marks on each sheet. This is meant to ease alignment during exposure.
@end ftable
%end-doc
*/
	{"align-marks", "Print alignment marks on each sheet",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_alignmarks 2

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --outline
Print the contents of the outline layer on each sheet.
@end ftable
%end-doc
*/
	{"outline", "Print outline on each sheet",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_outline 3
/* %start-doc options "91 Postscript Export"
@ftable @code
@item --mirror
Print mirror image.
@end ftable
%end-doc
*/
	{"mirror", "Print mirror image of every page",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_mirror 4

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --fill-page
Scale output to make the board fit the page.
@end ftable
%end-doc
*/
	{"fill-page", "Scale board to fill page",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_fillpage 5

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --auto-mirror
Print mirror image of appropriate layers.
@end ftable
%end-doc
*/
	{"auto-mirror", "Print mirror image of appropriate layers",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_automirror 6

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --ps-color
Postscript output in color.
@end ftable
%end-doc
*/
	{"ps-color", "Prints in color",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_color 7

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex ps-invert
@item --ps-invert
Draw objects as white-on-black.
@end ftable
%end-doc
*/
	{"ps-invert", "Draw objects as white-on-black",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_psinvert 8

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --media <media-name>
Size of the media, the postscript is fitted to. The parameter
@code{<media-name>} can be any of the standard names for paper size: @samp{A0}
to @samp{A10}, @samp{B0} to @samp{B10}, @samp{Letter}, @samp{11x17},
@samp{Ledger}, @samp{Legal}, @samp{Executive}, @samp{A-Size}, @samp{B-size},
@samp{C-Size}, @samp{D-size}, @samp{E-size}, @samp{US-Business_Card},
@samp{Intl-Business_Card}.
@end ftable
%end-doc
*/
	{"media", "media type",
	 RND_HATT_ENUM, 0, 0, {22, 0, 0}, pcb_medias},
#define HA_media 9

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex psfade
@item --psfade <num>
Fade amount for assembly drawings (0.0=missing, 1.0=solid).
@end ftable
%end-doc
*/
	{"psfade", "Fade amount for assembly drawings (0.0=missing, 1.0=solid)",
	 RND_HATT_REAL, 0, 1, {0, 0, 0.40}, 0},
#define HA_psfade 10

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --scale <num>
Scale value to compensate for printer sizing errors (1.0 = full scale).
@end ftable
%end-doc
*/
	{"scale", "Scale value to compensate for printer sizing errors (1.0 = full scale)",
	 RND_HATT_REAL, 0.01, 4, {0, 0, 1.00}, 0},
#define HA_scale 11

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex multi-file
@item --multi-file
Produce multiple files, one per page, instead of a single multi page file.
@end ftable
%end-doc
*/
	{"multi-file", "Produce multiple files, one per page, instead of a single file",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0.40}, 0},
#define HA_multifile 12

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --xcalib <num>
Paper width. Used for x-Axis calibration.
@end ftable
%end-doc
*/
	{"xcalib", "Paper width. Used for x-Axis calibration",
	 RND_HATT_REAL, 0, 2, {0, 0, 1.0}, 0},
#define HA_xcalib 13

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --ycalib <num>
Paper height. Used for y-Axis calibration.
@end ftable
%end-doc
*/
	{"ycalib", "Paper height. Used for y-Axis calibration",
	 RND_HATT_REAL, 0, 2, {0, 0, 1.0}, 0},
#define HA_ycalib 14

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --drill-copper
Draw drill holes in pins / vias, instead of leaving solid copper.
@end ftable
%end-doc
*/
	{"drill-copper", "Draw drill holes in pins / vias, instead of leaving solid copper",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_drillcopper 15

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex show-legend
@item --show-legend
Print file name and scale on printout.
@end ftable
%end-doc
*/
	{"show-legend", "Print file name and scale on printout",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_legend 16

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex show-toc
@item --show-toc
Generate Table of Contents
@end ftable
%end-doc
*/
	{"show-toc", "Print Table of Content",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_toc 17


/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex single-page
@item --single-page
Merge all drawings on a single page
@end ftable
%end-doc
*/
	{"single-page", "Merge all drawings on a single page",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_single_page 18

/* %start-doc options "91 Postscript Export"
@ftable @code
@cindex drill-helper-size
@item --drill-helper-size
Diameter of the small hole when drill-helper is on
@end ftable
%end-doc
*/
	{"drill-helper-size", "Diameter of the small hole when drill-helper is on",
	 RND_HATT_COORD, 0, RND_MM_TO_COORD(10), {0, 0, 0, RND_MIL_TO_COORD(4)}, 0},
#define HA_drillhelpersize 19


	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 20

};

#define NUM_OPTIONS (sizeof(ps_attribute_list)/sizeof(ps_attribute_list[0]))

/* All file-scope data is in global struct */
static struct {
	rnd_ps_t ps;

	rnd_bool multi_file;
	rnd_bool multi_file_cam;

	const char *filename;
	rnd_bool drill_helper;
	rnd_coord_t drill_helper_size;
	rnd_bool outline;
	rnd_bool mirror;
	rnd_bool automirror;
	rnd_bool drillcopper;
	int has_outline;

	rnd_hid_expose_ctx_t exps;

	rnd_hid_attr_val_t ps_values[NUM_OPTIONS];

	rnd_bool is_mask;
	rnd_bool is_drill;
	rnd_bool is_assy;
	rnd_bool is_copper;
	rnd_bool is_paste;

	int ovr_all;
} global;

static const rnd_export_opt_t *ps_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *val = global.ps_values[HA_psfile].str;

	if ((PCB != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &global.ps_values[HA_psfile], ".ps");

	if (n)
		*n = NUM_OPTIONS;
	return ps_attribute_list;
}

static rnd_layergrp_id_t group_for_layer(int l)
{
	if (l < pcb_max_layer(PCB) && l >= 0)
		return pcb_layer_get_group(PCB, l);
	/* else something unique */
	return pcb_max_group(PCB) + 3 + l;
}

static int layer_sort(const void *va, const void *vb)
{
	int a = *(int *) va;
	int b = *(int *) vb;
	int d = group_for_layer(b) - group_for_layer(a);
	if (d)
		return d;
	return b - a;
}

void rnd_ps_start_file(rnd_ps_t *pctx, const char *swver)
{
	FILE *f = pctx->outf;
	time_t currenttime = time(NULL);

	fprintf(f, "%%!PS-Adobe-3.0\n");

	/* Document Structuring Conventions (DCS): */

	/* Start General Header Comments: */

	/*
	 * %%Title DCS provides text title for the document that is useful
	 * for printing banner pages.
	 */
	fprintf(f, "%%%%Title: %s\n", rnd_hid_export_fn(PCB->hidlib.filename));

	/*
	 * %%CreationDate DCS indicates the date and time the document was
	 * created. Neither the date nor time need be in any standard
	 * format. This comment is meant to be used purely for informational
	 * purposes, such as printing on banner pages.
	 */
	fprintf(f, "%%%%CreationDate: %s", asctime(localtime(&currenttime)));

	/*
	 * %%Creator DCS indicates the document creator, usually the name of
	 * the document composition software.
	 */
	fprintf(f, "%%%%Creator: %s\n", swver);

	/*
	 * %%Version DCS comment can be used to note the version and
	 * revision number of a document or resource. A document manager may
	 * wish to provide version control services, or allow substitution
	 * of compatible versions/revisions of a resource or document.
	 *
	 * The format should be in the form of 'procname':
	 *  <procname>::= < name> < version> < revision>
	 *  < name> ::= < text>
	 *  < version> ::= < real>
	 *  < revision> ::= < uint>
	 *
	 * If a version numbering scheme is not used, these fields should
	 * still be filled with a dummy value of 0.
	 *
	 * There is currently no code in PCB to manage this revision number.
	 *
	 */
	fprintf(f, "%%%%Version: (%s) 0.0 0\n", swver);


	/*
	 * %%PageOrder DCS is intended to help document managers determine
	 * the order of pages in the document file, which in turn enables a
	 * document manager optionally to reorder the pages.  'Ascend'-The
	 * pages are in ascending order for example, 1-2-3-4-5-6.
	 */
	fprintf(f, "%%%%PageOrder: Ascend\n");

	/*
	 * %%Pages: < numpages> | (atend) < numpages> ::= < uint> (Total
	 * %%number of pages)
	 *
	 * %%Pages DCS defines the number of virtual pages that a document
	 * will image.  (atend) defers the count until the end of the file,
	 * which is useful for dynamically generated contents.
	 */
	fprintf(f, "%%%%Pages: (atend)\n");

	/*
	 * %%DocumentMedia: <name> <width> <height> <weight> <color> <type>
	 *
	 * Substitute 0 or "" for N/A.  Width and height are in points
	 * (1/72").
	 *
	 * Media sizes are in PCB units
	 */
	rnd_fprintf(f, "%%%%DocumentMedia: %s %f %f 0 \"\" \"\"\n",
							pcb_media_data[global.ps.media_idx].name,
							72 * RND_COORD_TO_INCH(pcb_media_data[global.ps.media_idx].width),
							72 * RND_COORD_TO_INCH(pcb_media_data[global.ps.media_idx].height));
	rnd_fprintf(f, "%%%%DocumentPaperSizes: %s\n", pcb_media_data[global.ps.media_idx].name);

	/* End General Header Comments. */

	/* General Body Comments go here. Currently there are none. */

	/*
	 * %%EndComments DCS indicates an explicit end to the header
	 * comments of the document.  All global DCS's must preceded
	 * this.  A blank line gives an implicit end to the comments.
	 */
	fprintf(f, "%%%%EndComments\n\n");
}

void rnd_ps_end_file(rnd_ps_t *pctx)
{
	FILE *f = pctx->outf;

	/*
	 * %%Trailer DCS must only occur once at the end of the document
	 * script.  Any post-processing or cleanup should be contained in
	 * the trailer of the document, which is anything that follows the
	 * %%Trailer comment. Any of the document level structure comments
	 * that were deferred by using the (atend) convention must be
	 * mentioned in the trailer of the document after the %%Trailer
	 * comment.
	 */
	fprintf(f, "%%%%Trailer\n");

	/*
	 * %%Pages was deferred until the end of the document via the
	 * (atend) mentioned, in the General Header section.
	 */
	fprintf(f, "%%%%Pages: %d\n", pctx->pagecount);

	/*
	 * %%EOF DCS signifies the end of the document. When the document
	 * manager sees this comment, it issues an end-of-file signal to the
	 * PostScript interpreter.  This is done so system-dependent file
	 * endings, such as Control-D and end-of-file packets, do not
	 * confuse the PostScript interpreter.
	 */
	fprintf(f, "%%%%EOF\n");
}

static FILE *psopen(const char *base, const char *which)
{
	FILE *ps_open_file;
	char *buf, *suff, *buf2;

	if (base == NULL) /* cam, file name template case */
		return NULL;

	if (!global.multi_file)
		return rnd_fopen_askovr(&PCB->hidlib, base, "w", NULL);

	buf = (char *) malloc(strlen(base) + strlen(which) + 5);

	suff = (char *) strrchr(base, '.');
	if (suff) {
		strcpy(buf, base);
		buf2 = strrchr(buf, '.');
		sprintf(buf2, ".%s.%s", which, suff + 1);
	}
	else {
		sprintf(buf, "%s.%s.ps", base, which);
	}
	ps_open_file = rnd_fopen_askovr(&PCB->hidlib, buf, "w", &global.ovr_all);
	free(buf);
	return ps_open_file;
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

/* This is used by other HIDs that use a postscript format, like lpr or eps.  */
void ps_hid_export_to_file(FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];

	rnd_ps_init(&global.ps, &PCB->hidlib, the_file, options[HA_media].lng, options[HA_fillpage].lng, options[HA_scale].dbl);

	/* pcb-rnd-specific config from export params */
	global.drill_helper = options[HA_drillhelper].lng;
	global.drill_helper_size = options[HA_drillhelpersize].crd;
	global.outline = options[HA_outline].lng;
	global.mirror = options[HA_mirror].lng;
	global.automirror = options[HA_automirror].lng;
	global.drillcopper = options[HA_drillcopper].lng;

	/* generic ps config: extra conf from export params */
	global.ps.align_marks = options[HA_alignmarks].lng;
	global.ps.incolor = options[HA_color].lng;
	global.ps.invert = options[HA_psinvert].lng;
	global.ps.fade_ratio = RND_CLAMP(options[HA_psfade].dbl, 0, 1);
	global.ps.calibration_x = options[HA_xcalib].dbl;
	global.ps.calibration_y = options[HA_ycalib].dbl;
	global.ps.legend = options[HA_legend].lng;
	global.ps.single_page = options[HA_single_page].lng;


	if (the_file)
		rnd_ps_start_file(&global.ps, "PCB release: pcb-rnd " PCB_VERSION);

	global.has_outline = pcb_has_explicit_outline(PCB);
	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));
	qsort(pcb_layer_stack, pcb_max_layer(PCB), sizeof(pcb_layer_stack[0]), layer_sort);

	/* reset static vars */
	use_gc(&global.ps, NULL);

	global.exps.view.X1 = 0;
	global.exps.view.Y1 = 0;
	global.exps.view.X2 = PCB->hidlib.size_x;
	global.exps.view.Y2 = PCB->hidlib.size_y;

	/* print ToC */
	if ((!global.multi_file && !global.multi_file_cam) && (options[HA_toc].lng)) {
		rnd_ps_begin_toc(&global.ps);
		rnd_app.expose_main(&ps_hid, &global.exps, xform);
		rnd_ps_end_toc(&global.ps);
	}

	/* print page(s) */
	rnd_ps_begin_pages(&global.ps);
	rnd_app.expose_main(&ps_hid, &global.exps, xform);
	rnd_ps_end_pages(&global.ps);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));
}

static void ps_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	FILE *fh;
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;

	global.ovr_all = 0;

	if (!options) {
		ps_get_export_options(hid, 0);
		options = global.ps_values;
	}

	global.ps.drawn_objs = 0;
	pcb_cam_begin(PCB, &ps_cam, &xform, options[HA_cam].str, ps_attribute_list, NUM_OPTIONS, options);

	global.filename = options[HA_psfile].str;
	if (!global.filename)
		global.filename = "pcb-rnd-out.ps";

	/* cam mode shall result in a single file, no matter what other attributes say */
	if (ps_cam.active) {
		global.multi_file = options[HA_multifile].lng;
		global.multi_file_cam = (ps_cam.fn_template != NULL); /* template means multiple files potentially */
	}
	else {
		global.multi_file = options[HA_multifile].lng;
		global.multi_file_cam = 0;
	}

	if (global.multi_file || global.multi_file_cam)
			fh = 0;
	else {
		const char *fn = ps_cam.active ? ps_cam.fn : global.filename;
		fh = psopen(fn, "toc");
		if (!fh) {
			perror(fn);
			return;
		}
	}

	if (!ps_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);
	ps_hid_export_to_file(fh, options, &xform);
	if (!ps_cam.active)
		pcb_hid_restore_layer_ons(save_ons);

	global.multi_file = 0;
	if (fh) {
		rnd_ps_end_file(&global.ps);
		fclose(fh);
	}

	if (!ps_cam.active) ps_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&ps_cam) == 0) {
		if (!ps_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "ps cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (global.ps.drawn_objs == 0) {
		if (!ps_cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "ps cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}

}

static int ps_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, ps_attribute_list, NUM_OPTIONS, ps_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static void corner(FILE * fh, rnd_coord_t x, rnd_coord_t y, int dx, int dy)
{
	rnd_coord_t len = RND_MIL_TO_COORD(2000);
	rnd_coord_t len2 = RND_MIL_TO_COORD(200);
	rnd_coord_t thick = 0;
	/*
	 * Originally 'thick' used thicker lines.  Currently is uses
	 * Postscript's "device thin" line - i.e. zero width means one
	 * device pixel.  The code remains in case you want to make them
	 * thicker - it needs to offset everything so that the *edge* of the
	 * thick line lines up with the edge of the board, not the *center*
	 * of the thick line.
	 */

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
				fprintf(pctx->outf, "30 30 moveto (%s) show\n", rnd_hid_export_fn(PCB->hidlib.filename));

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

int rnd_ps_new_file(rnd_ps_t *pctx, FILE *new_f)
{
	if (pctx->outf != NULL) {
		rnd_ps_end_file(pctx);
		fclose(pctx->outf);
	}
	pctx->outf = new_f;
	if (pctx->outf == NULL) {
		TODO("use rnd_message() and strerror instead");
		perror(global.filename);
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
			fprintf(global.ps.outf, "%%%%Page: %s %d\n", layer_fn, global.ps.pagecount);
			gds_uninit(&tmp);
		}

		fprintf(global.ps.outf, "/Helvetica findfont 10 scalefont setfont\n");
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

			if (global.ps.fillpage)
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
			fprintf(global.ps.outf, "%g %g scale %% calibration\n", pctx->calibration_x, pctx->calibration_y);

		if (mirror_this)
			fprintf(pctx->outf, "1 -1 scale\n");


		fprintf(pctx->outf, "%g dup neg scale\n", noscale ? 1.0 : global.ps.scale_factor);
		rnd_fprintf(pctx->outf, "%mi %mi translate\n", -pctx->hidlib->size_x / 2, -pctx->hidlib->size_y / 2);

	return boffset;
}

void rnd_ps_page_background(rnd_ps_t *pctx, int has_outline, int page_is_route)
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
									conf_core.design.min_wid,
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

static int ps_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	gds_t tmp_ln;
	const char *name;
	int newpage;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&ps_cam, group, purpose, purpi, flags, xform);

	if (!ps_cam.active) {
		if (flags & PCB_LYT_NOEXPORT)
			return 0;

		if (!PCB_LAYER_IS_OUTLINE(flags, purpi)) { /* outline layer can never be empty, because of the implicit outline */
			if (is_empty)
				return 0;

			if ((group >= 0) && pcb_layergrp_is_empty(PCB, group))
				return 0;
		}

		if (flags & PCB_LYT_INVIS)
			return 0;

		if (PCB_LAYER_IS_CSECT(flags, purpi)) /* not yet finished */
			return 0;
	}


	gds_init(&tmp_ln);
	name = pcb_layer_to_file_name(&tmp_ln, layer, flags, purpose, purpi, PCB_FNS_fixed);

	global.is_drill = PCB_LAYER_IS_DRILL(flags, purpi);
	global.is_mask = !!(flags & PCB_LYT_MASK);
	global.is_assy = PCB_LAYER_IS_ASSY(flags, purpi);
	global.is_copper = !!(flags & PCB_LYT_COPPER);
	global.is_paste = !!(flags & PCB_LYT_PASTE);

#if 0
	printf("Layer %s group %d drill %d mask %d\n", name, group, global.is_drill, global.is_mask);
#endif

	if (rnd_ps_printed_toc(&global.ps, group, name)) {
		gds_uninit(&tmp_ln);
		return 0;
	}

	if (ps_cam.active)
		newpage = ps_cam.fn_changed || (global.ps.pagecount == 1);
	else
		newpage = rnd_ps_is_new_page(&global.ps, group);

	if (newpage) {
		double boffset;
		int mirror_this = 0;

		if ((!ps_cam.active) && (global.ps.pagecount != 0)) {
			rnd_fprintf(global.ps.outf, "showpage\n");
		}
		if ((!ps_cam.active && global.multi_file) || (ps_cam.active && ps_cam.fn_changed)) {
			int nr;
			const char *fn;
			gds_t tmp;

			gds_init(&tmp);
			fn = ps_cam.active ? ps_cam.fn : pcb_layer_to_file_name(&tmp, layer, flags, purpose, purpi, PCB_FNS_fixed);
			nr = rnd_ps_new_file(&global.ps, psopen(ps_cam.active ? fn : global.filename, fn));
			gds_uninit(&tmp);
			if (nr != 0)
				return 0;

			rnd_ps_start_file(&global.ps, "PCB release: pcb-rnd " PCB_VERSION);
		}

		if (global.mirror)
			mirror_this = !mirror_this;
		if (global.automirror && (flags & PCB_LYT_BOTTOM))
			mirror_this = !mirror_this;

		{
			gds_t tmp = {0};
			const char *layer_fn = pcb_layer_to_file_name(&tmp, layer, flags, purpose, purpi, PCB_FNS_fixed);
			int noscale = PCB_LAYER_IS_FAB(flags, purpi);
			boffset = rnd_ps_page_frame(&global.ps, mirror_this, layer_fn, noscale);
			gds_uninit(&tmp);
		}

		/* Keep the drill list from falling off the left edge of the paper,
		 * even if it means some of the board falls off the right edge.
		 * If users don't want to make smaller boards, or use fewer drill
		 * sizes, they can always ignore this sheet. */
		if (PCB_LAYER_IS_FAB(flags, purpi)) {
			rnd_coord_t natural = boffset - RND_MIL_TO_COORD(500) - PCB->hidlib.size_y / 2;
			rnd_coord_t needed = pcb_stub_draw_fab_overhang();
			rnd_fprintf(global.ps.outf, "%% PrintFab overhang natural %mi, needed %mi\n", natural, needed);
			if (needed > natural)
				rnd_fprintf(global.ps.outf, "0 %mi translate\n", needed - natural);
		}

		rnd_ps_page_background(&global.ps, global.has_outline, ((PCB_LAYER_IS_ROUTE(flags, purpi)) || (global.outline)));

		if (global.drill_helper)
			rnd_fprintf(global.ps.outf,
									"/dh { gsave %mi setlinewidth 0 gray %mi 0 360 arc stroke grestore} bind def\n",
									(rnd_coord_t) global.drill_helper_size, (rnd_coord_t) (global.drill_helper_size * 3 / 2));
	}
#if 0
	/* Try to outsmart ps2pdf's heuristics for page rotation, by putting
	 * text on all pages -- even if that text is blank */
	if (!(PCB_LAYER_IS_FAB(flags, purpi)))
		fprintf(global.ps.outf, "gsave tx ty translate 1 -1 scale 0 0 moveto (Layer %s) show grestore newpath /ty ty ts sub def\n", name);
	else
		fprintf(global.ps.outf, "gsave tx ty translate 1 -1 scale 0 0 moveto ( ) show grestore newpath /ty ty ts sub def\n");
#endif

	/* If we're printing a layer other than an outline layer, and
	   we want to "print outlines", and we have an outline layer,
	   print the outline layer on this layer also.  */
	if (global.outline &&
			global.has_outline &&
			!(PCB_LAYER_IS_ROUTE(flags, purpi))) {
		int save_drill = global.is_drill;
		global.is_drill = 0;
		pcb_draw_groups(hid, PCB, PCB_LYT_BOUNDARY, F_proute, NULL, &global.exps.view, rnd_color_black, PCB_LYT_MECH, 0, 0);
		pcb_draw_groups(hid, PCB, PCB_LYT_BOUNDARY, F_uroute, NULL, &global.exps.view, rnd_color_black, PCB_LYT_MECH, 0, 0);
		global.is_drill = save_drill;
	}

	gds_uninit(&tmp_ln);
	return 1;
}

static rnd_hid_gc_t ps_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(1, sizeof(rnd_hid_gc_s));
	rv->me_pointer = &ps_hid;
	rv->cap = rnd_cap_round;
	return rv;
}

static void ps_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

void rnd_ps_set_drawing_mode(rnd_ps_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	pctx->drawing_mode = op;
}

static void ps_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	rnd_ps_set_drawing_mode(&global.ps, hid, op, direct, screen);
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


static void ps_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	rnd_ps_set_color(&global.ps, gc, color);
}

static void ps_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

static void ps_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}

static void ps_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

static void ps_set_draw_faded(rnd_hid_gc_t gc, int faded)
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
	if (gc->me_pointer != &ps_hid) {
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
		if (global.is_drill || global.is_mask) {
			fprintf(pctx->outf, "%d gray\n", (gc->erase || global.is_mask) ? 0 : 1);
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


void rnd_ps_draw_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(pctx, gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi %mi dr\n", x1, y1, x2, y2);
}

static void ps_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_ps_draw_rect(&global.ps, gc, x1, y1, x2, y2);
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

static void ps_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_ps_draw_line(&global.ps, gc, x1, y1, x2, y2);
}


void rnd_ps_draw_arc(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_angle_t sa, ea;
	double w;

	if ((width == 0) && (height == 0)) {
		/* degenerate case, draw dot */
		ps_draw_line(gc, cx, cy, cx, cy);
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

static void ps_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_ps_draw_arc(&global.ps, gc, cx, cy, width, height, start_angle, delta_angle);
}


void rnd_ps_fill_circle(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	use_gc(pctx, gc);
	rnd_fprintf(pctx->outf, "%mi %mi %mi c\n", cx, cy, radius);
}

static void ps_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	if (!gc->erase || !global.is_copper || global.drillcopper) {
		if (gc->erase && global.is_copper && global.drill_helper && radius >= global.drill_helper_size)
			radius = global.drill_helper_size;
		rnd_ps_fill_circle(&global.ps, gc, cx, cy, radius);
	}
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

static void ps_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_ps_fill_polygon_offs(&global.ps, gc, n_coords, x, y, dx, dy);
}


static void ps_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	ps_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
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

static void ps_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_ps_fill_rect(&global.ps, gc, x1, y1, x2, y2);
}


static void ps_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
}

rnd_hid_t ps_hid;


static int ps_inited = 0;
void ps_ps_init(rnd_hid_t * hid)
{
	if (ps_inited)
		return;

	hid->get_export_options = ps_get_export_options;
	hid->do_export = ps_do_export;
	hid->parse_arguments = ps_parse_arguments;
	hid->set_layer_group = ps_set_layer_group;
	hid->make_gc = ps_make_gc;
	hid->destroy_gc = ps_destroy_gc;
	hid->set_drawing_mode = ps_set_drawing_mode;
	hid->set_color = ps_set_color;
	hid->set_line_cap = ps_set_line_cap;
	hid->set_line_width = ps_set_line_width;
	hid->set_draw_xor = ps_set_draw_xor;
	hid->set_draw_faded = ps_set_draw_faded;
	hid->draw_line = ps_draw_line;
	hid->draw_arc = ps_draw_arc;
	hid->draw_rect = ps_draw_rect;
	hid->fill_circle = ps_fill_circle;
	hid->fill_polygon_offs = ps_fill_polygon_offs;
	hid->fill_polygon = ps_fill_polygon;
	hid->fill_rect = ps_fill_rect;
	hid->set_crosshair = ps_set_crosshair;

	ps_inited = 1;
}

static int ps_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nps exporter command line arguments:\n\n");
	rnd_hid_usage(ps_attribute_list, sizeof(ps_attribute_list) / sizeof(ps_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x ps [ps options] foo.pcb\n\n");
	return 0;
}

static void plugin_ps_uninit(void)
{
	rnd_remove_actions_by_cookie(ps_cookie);
	rnd_export_remove_opts_by_cookie(ps_cookie);
	ps_inited = 0;
}


int pplg_check_ver_export_ps(int ver_needed) { return 0; }

void pplg_uninit_export_ps(void)
{
	plugin_ps_uninit();
	rnd_hid_remove_hid(&ps_hid);
	hid_eps_uninit();
}

int pplg_init_export_ps(void)
{
	RND_API_CHK_VER;
	memset(&ps_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&ps_hid);
	ps_ps_init(&ps_hid);

	ps_hid.struct_size = sizeof(rnd_hid_t);
	ps_hid.name = "ps";
	ps_hid.description = "Postscript export";
	ps_hid.exporter = 1;
	ps_hid.mask_invert = 1;
	ps_hid.argument_array = global.ps_values;

	ps_hid.usage = ps_usage;

	rnd_hid_register_hid(&ps_hid);
	rnd_hid_load_defaults(&ps_hid, ps_attribute_list, NUM_OPTIONS);

	hid_eps_init();
	return 0;
}

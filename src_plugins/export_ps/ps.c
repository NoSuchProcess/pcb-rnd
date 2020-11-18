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

const char *ps_cookie = "ps HID";

static int ps_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform);
static void use_gc(rnd_hid_gc_t gc);

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

rnd_export_opt_t ps_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --psfile <string>
Name of the postscript output file. Can contain a path.
@end ftable
%end-doc
*/
	{"psfile", "Postscript output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_alignmarks 2

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --outline
Print the contents of the outline layer on each sheet.
@end ftable
%end-doc
*/
	{"outline", "Print outline on each sheet",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_outline 3
/* %start-doc options "91 Postscript Export"
@ftable @code
@item --mirror
Print mirror image.
@end ftable
%end-doc
*/
	{"mirror", "Print mirror image of every page",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mirror 4

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --fill-page
Scale output to make the board fit the page.
@end ftable
%end-doc
*/
	{"fill-page", "Scale board to fill page",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_fillpage 5

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --auto-mirror
Print mirror image of appropriate layers.
@end ftable
%end-doc
*/
	{"auto-mirror", "Print mirror image of appropriate layers",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_automirror 6

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --ps-color
Postscript output in color.
@end ftable
%end-doc
*/
	{"ps-color", "Prints in color",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_ENUM, 0, 0, {22, 0, 0}, pcb_medias, 0},
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
	 RND_HATT_REAL, 0, 1, {0, 0, 0.40}, 0, 0},
#define HA_psfade 10

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --scale <num>
Scale value to compensate for printer sizing errors (1.0 = full scale).
@end ftable
%end-doc
*/
	{"scale", "Scale value to compensate for printer sizing errors (1.0 = full scale)",
	 RND_HATT_REAL, 0.01, 4, {0, 0, 1.00}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {0, 0, 0.40}, 0, 0},
#define HA_multifile 12

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --xcalib <num>
Paper width. Used for x-Axis calibration.
@end ftable
%end-doc
*/
	{"xcalib", "Paper width. Used for x-Axis calibration",
	 RND_HATT_REAL, 0, 2, {0, 0, 1.0}, 0, 0},
#define HA_xcalib 13

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --ycalib <num>
Paper height. Used for y-Axis calibration.
@end ftable
%end-doc
*/
	{"ycalib", "Paper height. Used for y-Axis calibration",
	 RND_HATT_REAL, 0, 2, {0, 0, 1.0}, 0, 0},
#define HA_ycalib 14

/* %start-doc options "91 Postscript Export"
@ftable @code
@item --drill-copper
Draw drill holes in pins / vias, instead of leaving solid copper.
@end ftable
%end-doc
*/
	{"drill-copper", "Draw drill holes in pins / vias, instead of leaving solid copper",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_COORD, 0, RND_MM_TO_COORD(10), {0, 0, 0, PCB_MIN_PINORVIAHOLE}, 0, 0},
#define HA_drillhelpersize 19


	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 20

};

#define NUM_OPTIONS (sizeof(ps_attribute_list)/sizeof(ps_attribute_list[0]))

/* All file-scope data is in global struct */
static struct {
	double calibration_x, calibration_y;

	FILE *f;
	int pagecount;
	rnd_coord_t linewidth;
	double fade_ratio;
	rnd_bool multi_file;
	rnd_bool multi_file_cam;
	rnd_coord_t media_width, media_height, ps_width, ps_height;

	const char *filename;
	rnd_bool drill_helper;
	rnd_coord_t drill_helper_size;
	rnd_bool align_marks;
	rnd_bool outline;
	rnd_bool mirror;
	rnd_bool fillpage;
	rnd_bool automirror;
	rnd_bool incolor;
	rnd_bool doing_toc;
	rnd_bool invert;
	int media_idx;
	rnd_bool drillcopper;
	rnd_bool legend;
	rnd_bool single_page;

	int has_outline;
	double scale_factor;

	rnd_hid_expose_ctx_t exps;

	rnd_hid_attr_val_t ps_values[NUM_OPTIONS];

	rnd_bool is_mask;
	rnd_bool is_drill;
	rnd_bool is_assy;
	rnd_bool is_copper;
	rnd_bool is_paste;

	rnd_composite_op_t drawing_mode;
	int ovr_all;
	long drawn_objs;
} global;

static rnd_export_opt_t *ps_get_export_options(rnd_hid_t *hid, int *n)
{
	char **val = ps_attribute_list[HA_psfile].value;

	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &ps_attribute_list[HA_psfile], ".ps");

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

void ps_start_file(FILE * f)
{
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
	fprintf(f, "%%%%Creator: PCB release: pcb-rnd " PCB_VERSION "\n");

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
	fprintf(f, "%%%%Version: (PCB pcb-rnd " PCB_VERSION ") 0.0 0\n");


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
							pcb_media_data[global.media_idx].name,
							72 * RND_COORD_TO_INCH(pcb_media_data[global.media_idx].width),
							72 * RND_COORD_TO_INCH(pcb_media_data[global.media_idx].height));
	rnd_fprintf(f, "%%%%DocumentPaperSizes: %s\n", pcb_media_data[global.media_idx].name);

	/* End General Header Comments. */

	/* General Body Comments go here. Currently there are none. */

	/*
	 * %%EndComments DCS indicates an explicit end to the header
	 * comments of the document.  All global DCS's must preceded
	 * this.  A blank line gives an implicit end to the comments.
	 */
	fprintf(f, "%%%%EndComments\n\n");
}

static void ps_end_file(FILE * f)
{
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
	fprintf(f, "%%%%Pages: %d\n", global.pagecount);

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

/* This is used by other HIDs that use a postscript format, like lpr
   or eps.  */
void ps_hid_export_to_file(FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];

	global.f = the_file;
	global.drill_helper = options[HA_drillhelper].lng;
	global.drill_helper_size = options[HA_drillhelpersize].crd;
	global.align_marks = options[HA_alignmarks].lng;
	global.outline = options[HA_outline].lng;
	global.mirror = options[HA_mirror].lng;
	global.fillpage = options[HA_fillpage].lng;
	global.automirror = options[HA_automirror].lng;
	global.incolor = options[HA_color].lng;
	global.invert = options[HA_psinvert].lng;
	global.fade_ratio = RND_CLAMP(options[HA_psfade].dbl, 0, 1);
	global.media_idx = options[HA_media].lng;
	global.media_width = pcb_media_data[global.media_idx].width;
	global.media_height = pcb_media_data[global.media_idx].height;
	global.ps_width = global.media_width - 2.0 * pcb_media_data[global.media_idx].margin_x;
	global.ps_height = global.media_height - 2.0 * pcb_media_data[global.media_idx].margin_y;
	global.scale_factor = options[HA_scale].dbl;
	global.calibration_x = options[HA_xcalib].dbl;
	global.calibration_y = options[HA_ycalib].dbl;
	global.drillcopper = options[HA_drillcopper].lng;
	global.legend = options[HA_legend].lng;
	global.single_page = options[HA_single_page].lng;

	if (the_file)
		ps_start_file(the_file);

	if (global.fillpage) {
		double zx, zy;
		if (PCB->hidlib.size_x > PCB->hidlib.size_y) {
			zx = global.ps_height / PCB->hidlib.size_x;
			zy = global.ps_width / PCB->hidlib.size_y;
		}
		else {
			zx = global.ps_height / PCB->hidlib.size_y;
			zy = global.ps_width / PCB->hidlib.size_x;
		}
		global.scale_factor *= MIN(zx, zy);
	}

	global.has_outline = pcb_has_explicit_outline(PCB);
	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));
	qsort(pcb_layer_stack, pcb_max_layer(PCB), sizeof(pcb_layer_stack[0]), layer_sort);

	global.linewidth = -1;
	/* reset static vars */
	ps_set_layer_group(rnd_render, -1, NULL, -1, -1, 0, -1, NULL);
	use_gc(NULL);

	global.exps.view.X1 = 0;
	global.exps.view.Y1 = 0;
	global.exps.view.X2 = PCB->hidlib.size_x;
	global.exps.view.Y2 = PCB->hidlib.size_y;

	if ((!global.multi_file && !global.multi_file_cam) && (options[HA_toc].lng)) {
		/* %%Page DSC requires both a label and an ordinal */
		fprintf(the_file, "%%%%Page: TableOfContents 1\n");
		fprintf(the_file, "/Times-Roman findfont 24 scalefont setfont\n");
		fprintf(the_file, "/rightshow { /s exch def s stringwidth pop -1 mul 0 rmoveto s show } def\n");
		fprintf(the_file, "/y 72 9 mul def /toc { 100 y moveto show /y y 24 sub def } bind def\n");
		fprintf(the_file, "/tocp { /y y 12 sub def 90 y moveto rightshow } bind def\n");

		global.doing_toc = 1;
		global.pagecount = 1;				/* 'pagecount' is modified by rnd_expose_main() call */
		rnd_expose_main(&ps_hid, &global.exps, xform);
	}

	global.pagecount = 1;					/* Reset 'pagecount' if single file */
	global.doing_toc = 0;
	ps_set_layer_group(rnd_render, -1, NULL, -1, -1, 0, -1, NULL); /* reset static vars */
	rnd_expose_main(&ps_hid, &global.exps, xform);

	if (the_file)
		fprintf(the_file, "showpage\n");

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

	global.drawn_objs = 0;
	pcb_cam_begin(PCB, &ps_cam, &xform, options[HA_cam].str, ps_attribute_list, NUM_OPTIONS, options);

	global.filename = options[HA_psfile].str;
	if (!global.filename)
		global.filename = "pcb-out.ps";

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
		ps_end_file(fh);
		fclose(fh);
	}

	if (!ps_cam.active) ps_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&ps_cam) == 0) {
		if (!ps_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "ps cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (global.drawn_objs == 0) {
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

static int ps_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	gds_t tmp_ln;
	static int lastgroup = -1;
	time_t currenttime;
	const char *name;
	int newpage;

	if (is_empty == -1) {
		lastgroup = -1;
		return 0;
	}


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

	if (global.doing_toc) {
		if (group < 0 || group != lastgroup) {
			if (global.pagecount == 1) {
				currenttime = time(NULL);
				fprintf(global.f, "30 30 moveto (%s) show\n", rnd_hid_export_fn(PCB->hidlib.filename));

				fprintf(global.f, "(%d.) tocp\n", global.pagecount);
				fprintf(global.f, "(Table of Contents \\(This Page\\)) toc\n");

				fprintf(global.f, "(Created on %s) toc\n", asctime(localtime(&currenttime)));
				fprintf(global.f, "( ) tocp\n");
			}

			global.pagecount++;
			lastgroup = group;
			fprintf(global.f, "(%d.) tocp\n", global.single_page ? 2 : global.pagecount);
			fprintf(global.f, "(%s) toc\n", name);
		}
		gds_uninit(&tmp_ln);
		return 0;
	}

	if (ps_cam.active)
		newpage = ps_cam.fn_changed || (global.pagecount == 1);
	else
		newpage = (group < 0 || group != lastgroup);
	if ((global.pagecount > 1) && global.single_page)
		newpage = 0;

	if (newpage) {
		double boffset;
		int mirror_this = 0;
		lastgroup = group;

		if ((!ps_cam.active) && (global.pagecount != 0)) {
			rnd_fprintf(global.f, "showpage\n");
		}
		global.pagecount++;
		if ((!ps_cam.active && global.multi_file) || (ps_cam.active && ps_cam.fn_changed)) {
			const char *fn;
			gds_t tmp;

			gds_init(&tmp);
			fn = ps_cam.active ? ps_cam.fn : pcb_layer_to_file_name(&tmp, layer, flags, purpose, purpi, PCB_FNS_fixed);
			if (global.f) {
				ps_end_file(global.f);
				fclose(global.f);
			}
			global.f = psopen(ps_cam.active ? fn : global.filename, fn);
			gds_uninit(&tmp);

			if (!global.f) {
				perror(global.filename);
				gds_uninit(&tmp_ln);
				return 0;
			}

			ps_start_file(global.f);
		}

		/*
		 * %%Page DSC comment marks the beginning of the PostScript
		 * language instructions that describe a particular
		 * page. %%Page: requires two arguments: a page label and a
		 * sequential page number. The label may be anything, but the
		 * ordinal page number must reflect the position of that page in
		 * the body of the PostScript file and must start with 1, not 0.
		 */
		{
			gds_t tmp;
			gds_init(&tmp);
			fprintf(global.f, "%%%%Page: %s %d\n", pcb_layer_to_file_name(&tmp, layer, flags, purpose, purpi, PCB_FNS_fixed), global.pagecount);
			gds_uninit(&tmp);
		}

		if (global.mirror)
			mirror_this = !mirror_this;
		if (global.automirror && (flags & PCB_LYT_BOTTOM))
			mirror_this = !mirror_this;

		fprintf(global.f, "/Helvetica findfont 10 scalefont setfont\n");
		if (global.legend) {
			gds_t tmp;
			fprintf(global.f, "30 30 moveto (%s) show\n", rnd_hid_export_fn(PCB->hidlib.filename));

			gds_init(&tmp);
			if (PCB->hidlib.name)
				fprintf(global.f, "30 41 moveto (%s, %s) show\n", PCB->hidlib.name, pcb_layer_to_file_name(&tmp, layer, flags, purpose, purpi, PCB_FNS_fixed));
			else
				fprintf(global.f, "30 41 moveto (%s) show\n", pcb_layer_to_file_name(&tmp, layer, flags, purpose, purpi, PCB_FNS_fixed));
			gds_uninit(&tmp);

			if (mirror_this)
				fprintf(global.f, "( \\(mirrored\\)) show\n");

			if (global.fillpage)
				fprintf(global.f, "(, not to scale) show\n");
			else
				fprintf(global.f, "(, scale = 1:%.3f) show\n", global.scale_factor);
		}
		fprintf(global.f, "newpath\n");

		rnd_fprintf(global.f, "72 72 scale %mi %mi translate\n", global.media_width / 2, global.media_height / 2);

		boffset = global.media_height / 2;
		if (PCB->hidlib.size_x > PCB->hidlib.size_y) {
			fprintf(global.f, "90 rotate\n");
			boffset = global.media_width / 2;
			fprintf(global.f, "%g %g scale %% calibration\n", global.calibration_y, global.calibration_x);
		}
		else
			fprintf(global.f, "%g %g scale %% calibration\n", global.calibration_x, global.calibration_y);

		if (mirror_this)
			fprintf(global.f, "1 -1 scale\n");

		fprintf(global.f, "%g dup neg scale\n", PCB_LAYER_IS_FAB(flags, purpi) ? 1.0 : global.scale_factor);
		rnd_fprintf(global.f, "%mi %mi translate\n", -PCB->hidlib.size_x / 2, -PCB->hidlib.size_y / 2);

		/* Keep the drill list from falling off the left edge of the paper,
		 * even if it means some of the board falls off the right edge.
		 * If users don't want to make smaller boards, or use fewer drill
		 * sizes, they can always ignore this sheet. */
		if (PCB_LAYER_IS_FAB(flags, purpi)) {
			rnd_coord_t natural = boffset - RND_MIL_TO_COORD(500) - PCB->hidlib.size_y / 2;
			rnd_coord_t needed = pcb_stub_draw_fab_overhang();
			rnd_fprintf(global.f, "%% PrintFab overhang natural %mi, needed %mi\n", natural, needed);
			if (needed > natural)
				rnd_fprintf(global.f, "0 %mi translate\n", needed - natural);
		}

		if (global.invert) {
			fprintf(global.f, "/gray { 1 exch sub setgray } bind def\n");
			fprintf(global.f, "/rgb { 1 1 3 { pop 1 exch sub 3 1 roll } for setrgbcolor } bind def\n");
		}
		else {
			fprintf(global.f, "/gray { setgray } bind def\n");
			fprintf(global.f, "/rgb { setrgbcolor } bind def\n");
		}

		if (!global.has_outline || global.invert) {
			if ((PCB_LAYER_IS_ROUTE(flags, purpi)) || (global.outline)) {
				rnd_fprintf(global.f,
									"0 setgray %mi setlinewidth 0 0 moveto 0 "
									"%mi lineto %mi %mi lineto %mi 0 lineto closepath %s\n",
									conf_core.design.min_wid,
									PCB->hidlib.size_y, PCB->hidlib.size_x, PCB->hidlib.size_y, PCB->hidlib.size_x, global.invert ? "fill" : "stroke");
			}
		}

		if (global.align_marks) {
			corner(global.f, 0, 0, -1, -1);
			corner(global.f, PCB->hidlib.size_x, 0, 1, -1);
			corner(global.f, PCB->hidlib.size_x, PCB->hidlib.size_y, 1, 1);
			corner(global.f, 0, PCB->hidlib.size_y, -1, 1);
		}

		global.linewidth = -1;
		use_gc(NULL);								/* reset static vars */

		fprintf(global.f,
						"/ts 1 def\n"
						"/ty ts neg def /tx 0 def /Helvetica findfont ts scalefont setfont\n"
						"/t { moveto lineto stroke } bind def\n"
						"/dr { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n"
						"      x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath stroke } bind def\n");
		fprintf(global.f,"/r { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n"
						"     x1 y1 moveto x1 y2 lineto x2 y2 lineto x2 y1 lineto closepath fill } bind def\n"
						"/c { 0 360 arc fill } bind def\n"
						"/a { gsave setlinewidth translate scale 0 0 1 5 3 roll arc stroke grestore} bind def\n");
		if (global.drill_helper)
			rnd_fprintf(global.f,
									"/dh { gsave %mi setlinewidth 0 gray %mi 0 360 arc stroke grestore} bind def\n",
									(rnd_coord_t) global.drill_helper_size, (rnd_coord_t) (global.drill_helper_size * 3 / 2));
	}
#if 0
	/* Try to outsmart ps2pdf's heuristics for page rotation, by putting
	 * text on all pages -- even if that text is blank */
	if (!(PCB_LAYER_IS_FAB(flags, purpi)))
		fprintf(global.f, "gsave tx ty translate 1 -1 scale 0 0 moveto (Layer %s) show grestore newpath /ty ty ts sub def\n", name);
	else
		fprintf(global.f, "gsave tx ty translate 1 -1 scale 0 0 moveto ( ) show grestore newpath /ty ty ts sub def\n");
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

static void ps_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	global.drawing_mode = op;
}


static void ps_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	if (global.drawing_mode == RND_HID_COMP_NEGATIVE) {
		gc->r = gc->g = gc->b = 255;
		gc->erase = 0;
	}
	else if (rnd_color_is_drill(color)) {
		gc->r = gc->g = gc->b = 255;
		gc->erase = 1;
	}
	else if (global.incolor) {
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

static void use_gc(rnd_hid_gc_t gc)
{
	static int lastcap = -1;
	static int lastcolor = -1;

	global.drawn_objs++;
	if (gc == NULL) {
		lastcap = lastcolor = -1;
		return;
	}
	if (gc->me_pointer != &ps_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to ps HID\n");
		abort();
	}
	if (global.linewidth != gc->width) {
		rnd_fprintf(global.f, "%mi setlinewidth\n", gc->width);
		global.linewidth = gc->width;
	}
	if (lastcap != gc->cap) {
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
		fprintf(global.f, "%d setlinecap %d setlinejoin\n", c, c);
		lastcap = gc->cap;
	}
#define CBLEND(gc) (((gc->r)<<24)|((gc->g)<<16)|((gc->b)<<8)|(gc->faded))
	if (lastcolor != CBLEND(gc)) {
		if (global.is_drill || global.is_mask) {
			fprintf(global.f, "%d gray\n", (gc->erase || global.is_mask) ? 0 : 1);
			lastcolor = 0;
		}
		else {
			double r, g, b;
			r = gc->r;
			g = gc->g;
			b = gc->b;
			if (gc->faded) {
				r = (1 - global.fade_ratio) *255 + global.fade_ratio * r;
				g = (1 - global.fade_ratio) *255 + global.fade_ratio * g;
				b = (1 - global.fade_ratio) *255 + global.fade_ratio * b;
			}
			if (gc->r == gc->g && gc->g == gc->b)
				fprintf(global.f, "%g gray\n", r / 255.0);
			else
				fprintf(global.f, "%g %g %g rgb\n", r / 255.0, g / 255.0, b / 255.0);
			lastcolor = CBLEND(gc);
		}
	}
}

static void ps_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(gc);
	rnd_fprintf(global.f, "%mi %mi %mi %mi dr\n", x1, y1, x2, y2);
}

static void ps_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
static void ps_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);

static void ps_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
#if 0
	/* If you're etching your own paste mask, this will reduce the
	   amount of brass you need to etch by drawing outlines for large
	   pads.  See also ps_fill_rect.  */
	if (is_paste && gc->width > 2500 && gc->cap == rnd_cap_square && (x1 == x2 || y1 == y2)) {
		rnd_coord_t t, w;
		if (x1 > x2) {
			t = x1;
			x1 = x2;
			x2 = t;
		}
		if (y1 > y2) {
			t = y1;
			y1 = y2;
			y2 = t;
		}
		w = gc->width / 2;
		ps_fill_rect(gc, x1 - w, y1 - w, x2 + w, y2 + w);
		return;
	}
#endif
	if (x1 == x2 && y1 == y2) {
		rnd_coord_t w = gc->width / 2;
		if (gc->cap == rnd_cap_square)
			ps_fill_rect(gc, x1 - w, y1 - w, x1 + w, y1 + w);
		else
			ps_fill_circle(gc, x1, y1, w);
		return;
	}
	use_gc(gc);
	rnd_fprintf(global.f, "%mi %mi %mi %mi t\n", x1, y1, x2, y2);
}

static void ps_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
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
	use_gc(gc);
	w = width;
	if (w == 0) /* make sure not to div by zero; this hack will have very similar effect */
		w = 0.0001;
	rnd_fprintf(global.f, "%ma %ma %mi %mi %mi %mi %f a\n",
							sa, ea, -width, height, cx, cy, (double)(global.linewidth) / w);
}

static void ps_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	use_gc(gc);
	if (!gc->erase || !global.is_copper || global.drillcopper) {
		if (gc->erase && global.is_copper && global.drill_helper && radius >= global.drill_helper_size)
			radius = global.drill_helper_size;
		rnd_fprintf(global.f, "%mi %mi %mi c\n", cx, cy, radius);
	}
}

static void ps_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int i;
	const char *op = "moveto";
	use_gc(gc);
	for (i = 0; i < n_coords; i++) {
		rnd_fprintf(global.f, "%mi %mi %s\n", x[i]+dx, y[i]+dy, op);
		op = "lineto";
	}
	fprintf(global.f, "fill\n");
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

static void ps_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(gc);
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
#if 0
	/* See comment in ps_draw_line.  */
	if (is_paste && (x2 - x1) > 2500 && (y2 - y1) > 2500) {
		linewidth = 1000;
		lastcap = rnd_cap_round;
		fprintf(f, "1000 setlinewidth 1 setlinecap 1 setlinejoin\n");
		fprintf(f, "%d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath stroke\n",
						x1 + 500 - bloat, y1 + 500 - bloat,
						x1 + 500 - bloat, y2 - 500 + bloat, x2 - 500 + bloat, y2 - 500 + bloat, x2 - 500 + bloat, y1 + 500 - bloat);
		return;
	}
#endif
	rnd_fprintf(global.f, "%mi %mi %mi %mi r\n", x1, y1, x2, y2);
}

rnd_hid_attribute_t ps_calib_attribute_list[] = {
	{"lprcommand", "Command to print",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
};

static const char *const calib_lines[] = {
	"%!PS-Adobe-3.0\n",
	"%%Title: Calibration Page\n",
	"%%PageOrder: Ascend\n",
	"%%Pages: 1\n",
	"%%EndComments\n",
	"\n",
	"%%Page: Calibrate 1\n",
	"72 72 scale\n",
	"\n",
	"0 setlinewidth\n",
	"0.375 0.375 moveto\n",
	"8.125 0.375 lineto\n",
	"8.125 10.625 lineto\n",
	"0.375 10.625 lineto\n",
	"closepath stroke\n",
	"\n",
	"0.5 0.5 translate\n",
	"0.001 setlinewidth\n",
	"\n",
	"/Times-Roman findfont 0.2 scalefont setfont\n",
	"\n",
	"/sign {\n",
	"    0 lt { -1 } { 1 } ifelse\n",
	"} def\n",
	"\n",
	"/cbar {\n",
	"    /units exch def\n",
	"    /x exch def\n",
	"    /y exch def  \n",
	"\n",
	"    /x x sign 0.5 mul def\n",
	"\n",
	"    0 setlinewidth\n",
	"    newpath x y 0.25 0 180 arc gsave 0.85 setgray fill grestore closepath stroke\n",
	"    newpath x 0 0.25 180 360 arc gsave 0.85 setgray fill grestore closepath stroke\n",
	"    0.001 setlinewidth\n",
	"\n",
	"    x 0 moveto\n",
	"    x y lineto\n",
	"%    -0.07 -0.2 rlineto 0.14 0 rmoveto -0.07 0.2 rlineto\n",
	"    x y lineto\n",
	"    -0.1 0 rlineto 0.2 0 rlineto\n",
	"    stroke\n",
	"    x 0 moveto\n",
	"%    -0.07 0.2 rlineto 0.14 0 rmoveto -0.07 -0.2 rlineto\n",
	"    x 0 moveto\n",
	"    -0.1 0 rlineto 0.2 0 rlineto\n",
	"     stroke\n",
	"\n",
	"    x 0.1 add\n",
	"    y 0.2 sub moveto\n",
	"    units show\n",
	"} bind def\n",
	"\n",
	"/y 9 def\n",
	"/t {\n",
	"    /str exch def\n",
	"    1.5 y moveto str show\n",
	"    /y y 0.25 sub def\n",
	"} bind def\n",
	"\n",
	"(Please measure ONE of the horizontal lines, in the units indicated for)t\n",
	"(that line, and enter that value as X.  Similarly, measure ONE of the)t\n",
	"(vertical lines and enter that value as Y.  Measurements should be)t\n",
	"(between the flat faces of the semicircles.)t\n",
	"()t\n",
	"(The large box is 10.25 by 7.75 inches)t\n",
	"\n",
	"/in { } bind def\n",
	"/cm { 2.54 div } bind def\n",
	"/mm { 25.4 div } bind def\n",
	"\n",
	0
};

static int guess(double val, double close_to, double *calib)
{
	if (val >= close_to * 0.9 && val <= close_to * 1.1) {
		*calib = close_to / val;
		return 0;
	}
	return 1;
}

void ps_calibrate_1(rnd_hid_t *hid, double xval, double yval, int use_command)
{
	FILE *ps_cal_file;
	int used_popen = 0, c;

	if (xval > 0 && yval > 0) {
		if (guess(xval, 4, &global.calibration_x))
			if (guess(xval, 15, &global.calibration_x))
				if (guess(xval, 7.5, &global.calibration_x)) {
					if (xval < 2)
						ps_attribute_list[HA_xcalib].default_val.dbl = global.calibration_x = xval;
					else
						rnd_message(RND_MSG_ERROR, "X value of %g is too far off.\n" "Expecting it near: 1.0, 4.0, 15.0, 7.5\n", xval);
				}
		if (guess(yval, 4, &global.calibration_y))
			if (guess(yval, 20, &global.calibration_y))
				if (guess(yval, 10, &global.calibration_y)) {
					if (yval < 2)
						ps_attribute_list[HA_ycalib].default_val.dbl = global.calibration_y = yval;
					else
						rnd_message(RND_MSG_ERROR, "Y value of %g is too far off.\n" "Expecting it near: 1.0, 4.0, 20.0, 10.0\n", yval);
				}
		return;
	}

	if (ps_calib_attribute_list[0].val.str == NULL) {
		ps_calib_attribute_list[0].val.str = rnd_strdup("lpr");
	}

	if (rnd_attribute_dialog("ps_calibrate", ps_calib_attribute_list, 1, "Print Calibration Page", NULL))
		return;

	if (ps_calib_attribute_list[0].val.str == NULL)
		return;

	if (use_command || strchr(ps_calib_attribute_list[0].val.str, '|')) {
		const char *cmd = ps_calib_attribute_list[0].val.str;
		while (*cmd == ' ' || *cmd == '|')
			cmd++;
		ps_cal_file = rnd_popen(&PCB->hidlib, cmd, "w");
		used_popen = 1;
	}
	else
		ps_cal_file = rnd_fopen(&PCB->hidlib, ps_calib_attribute_list[0].val.str, "w");

	for (c = 0; calib_lines[c]; c++)
		fputs(calib_lines[c], ps_cal_file);

	fprintf(ps_cal_file, "4 in 0.5 (Y in) cbar\n");
	fprintf(ps_cal_file, "20 cm 1.5 (Y cm) cbar\n");
	fprintf(ps_cal_file, "10 in 2.5 (Y in) cbar\n");
	fprintf(ps_cal_file, "-90 rotate\n");
	fprintf(ps_cal_file, "4 in -0.5 (X in) cbar\n");
	fprintf(ps_cal_file, "15 cm -1.5 (X cm) cbar\n");
	fprintf(ps_cal_file, "7.5 in -2.5 (X in) cbar\n");

	fprintf(ps_cal_file, "showpage\n");

	fprintf(ps_cal_file, "%%%%EOF\n");

	if (used_popen)
		rnd_pclose(ps_cal_file);
	else
		fclose(ps_cal_file);
}

static void ps_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	ps_calibrate_1(hid, xval, yval, 0);
}

static void ps_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
}

rnd_hid_t ps_hid;

static fgw_error_t pcb_act_PSCalib(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	ps_calibrate(&ps_hid, 0.0, 0.0);
	return 0;
}

rnd_action_t hidps_action_list[] = {
	{"pscalib", pcb_act_PSCalib, NULL, NULL}
};

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
	hid->calibrate = ps_calibrate;
	hid->set_crosshair = ps_set_crosshair;

	RND_REGISTER_ACTIONS(hidps_action_list, ps_cookie)

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

	hid_eps_init();
	return 0;
}

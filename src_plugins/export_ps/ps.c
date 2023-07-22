/*
   This file is part of pcb-rnd and was part of gEDA/PCB but lacked proper
   copyright banner at the fork. It probably has the same copyright as
   gEDA/PCB as a whole in 2011.
*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>
#include <librnd/plugins/lib_exp_text/draw_ps.h>

#include "ps.h"
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>
#include <librnd/core/actions.h>
#include "conf_core.h"
#include <librnd/core/compat_misc.h>
#include "stub_draw.h"
#include "../src_plugins/lib_compat_help/media.h"


const char *ps_cookie = "ps HID";

static int ps_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform);

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

	rnd_bool is_assy;
	rnd_bool is_copper;
	rnd_bool is_paste;

	int ovr_all;
	int had_page; /* 1 if we ever wrote a page */
} global;

static const rnd_export_opt_t *ps_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *val = global.ps_values[HA_psfile].str;

	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &global.ps_values[HA_psfile], ".ps");

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


/* This is used by other HIDs that use a postscript format, like lpr or eps.  */
void ps_hid_export_to_file(rnd_design_t *dsg, FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform, void *appspec_)
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
	rnd_ps_use_gc(&global.ps, NULL);

	global.exps.design = dsg;
	global.exps.view.X1 = dsg->dwg.X1;
	global.exps.view.Y1 = dsg->dwg.Y1;
	global.exps.view.X2 = dsg->dwg.X2;
	global.exps.view.Y2 = dsg->dwg.Y2;

	global.had_page = 0;

	/* print ToC */
	if ((!global.multi_file && !global.multi_file_cam) && (options[HA_toc].lng)) {
		rnd_ps_begin_toc(&global.ps);
		rnd_app.expose_main(&ps_hid, &global.exps, xform);
		rnd_ps_end_toc(&global.ps);
		global.had_page = 1;
	}

	/* print page(s) */
	rnd_ps_begin_pages(&global.ps);
	rnd_app.expose_main(&ps_hid, &global.exps, xform);
	rnd_ps_end_pages(&global.ps);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void ps_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	FILE *fh;
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;

	global.ovr_all = 0;

	if (!options) {
		ps_get_export_options(hid, 0, design, appspec);
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
	ps_hid_export_to_file(design, fh, options, &xform, appspec);
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

extern int rnd_ps_faded;

static int ps_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
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

			if ((group >= 0) && pcb_cam_layergrp_is_empty(&ps_cam, PCB, group))
				return 0;
		}

		if (flags & PCB_LYT_INVIS)
			return 0;

		if (PCB_LAYER_IS_CSECT(flags, purpi)) /* not yet finished */
			return 0;
	}


	gds_init(&tmp_ln);
	name = pcb_layer_to_file_name(&tmp_ln, layer, flags, purpose, purpi, PCB_FNS_fixed);

	global.ps.is_drill = PCB_LAYER_IS_DRILL(flags, purpi);
	global.ps.is_mask = !!(flags & PCB_LYT_MASK);
	global.is_assy = PCB_LAYER_IS_ASSY(flags, purpi);
	global.is_copper = !!(flags & PCB_LYT_COPPER);
	global.is_paste = !!(flags & PCB_LYT_PASTE);
	rnd_ps_faded = ((xform != NULL) && (*xform != NULL) && (*xform)->layer_faded);

#if 0
	printf("Layer %s group %d drill %d mask %d\n", name, group, global.ps.is_drill, global.ps.is_mask);
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

		if ((!ps_cam.active) && (global.ps.pagecount != 0) && global.had_page) {
			rnd_fprintf(global.ps.outf, "showpage\n");
		}
		if ((!ps_cam.active && global.multi_file) || (ps_cam.active && ps_cam.fn_changed)) {
			int nr;
			const char *fn;
			gds_t tmp;

			gds_init(&tmp);
			fn = ps_cam.active ? ps_cam.fn : pcb_layer_to_file_name(&tmp, layer, flags, purpose, purpi, PCB_FNS_fixed);

			rnd_ps_end_pages(&global.ps);
			nr = rnd_ps_new_file(&global.ps, psopen(ps_cam.active ? fn : global.filename, fn), fn);
			gds_uninit(&tmp);
			if (nr != 0)
				return 0;

			rnd_ps_start_file(&global.ps, "PCB release: pcb-rnd " PCB_VERSION);
		}
		else
			global.had_page = 1;

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
		   even if it means some of the board falls off the right edge.
		   If users don't want to make smaller boards, or use fewer drill
		   sizes, they can always ignore this sheet. */
		if (PCB_LAYER_IS_FAB(flags, purpi)) {
			rnd_coord_t natural = boffset - RND_MIL_TO_COORD(500) - (PCB->hidlib.dwg.Y1 + PCB->hidlib.dwg.Y2) / 2;
			rnd_coord_t needed = pcb_stub_draw_fab_overhang();
			rnd_fprintf(global.ps.outf, "%% PrintFab overhang natural %mi, needed %mi\n", natural, needed);
			if (needed > natural)
				rnd_fprintf(global.ps.outf, "0 %mi translate\n", needed - natural);
		}

		rnd_ps_page_background(&global.ps,
			global.has_outline,
			((PCB_LAYER_IS_ROUTE(flags, purpi)) || (global.outline)),
			conf_core.design.min_wid);

		if (global.drill_helper)
			rnd_fprintf(global.ps.outf,
									"/dh { gsave %mi setlinewidth 0 gray %mi 0 360 arc stroke grestore} bind def\n",
									(rnd_coord_t) global.drill_helper_size, (rnd_coord_t) (global.drill_helper_size * 3 / 2));
	}
#if 0
	/* Try to outsmart ps2pdf's heuristics for page rotation, by putting
	   text on all pages -- even if that text is blank */
	if (!(PCB_LAYER_IS_FAB(flags, purpi)))
		fprintf(global.ps.outf, "gsave tx ty translate 1 -1 scale 0 0 moveto (Layer %s) show grestore newpath /ty ty ts sub def\n", name);
	else
		fprintf(global.ps.outf, "gsave tx ty translate 1 -1 scale 0 0 moveto ( ) show grestore newpath /ty ty ts sub def\n");
#endif

	/* If we're printing a layer other than an outline layer, and
	   we want to "print outlines", and we have an outline layer,
	   print the outline layer on this layer also. */
	if (global.outline &&
			global.has_outline &&
			!(PCB_LAYER_IS_ROUTE(flags, purpi))) {
		int save_drill = global.ps.is_drill;
		global.ps.is_drill = 0;
		pcb_draw_groups(hid, PCB, PCB_LYT_BOUNDARY, F_proute, NULL, &global.exps.view, rnd_color_black, PCB_LYT_MECH, 0, 0);
		pcb_draw_groups(hid, PCB, PCB_LYT_BOUNDARY, F_uroute, NULL, &global.exps.view, rnd_color_black, PCB_LYT_MECH, 0, 0);
		global.ps.is_drill = save_drill;
	}

	gds_uninit(&tmp_ln);
	return 1;
}

static void ps_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	rnd_ps_set_drawing_mode(&global.ps, hid, op, direct, screen);
}

static void ps_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	rnd_ps_set_color(&global.ps, gc, color);
}

static void ps_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_ps_draw_rect(&global.ps, gc, x1, y1, x2, y2);
}

static void ps_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_ps_draw_line(&global.ps, gc, x1, y1, x2, y2);
}

static void ps_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_ps_draw_arc(&global.ps, gc, cx, cy, width, height, start_angle, delta_angle);
}

static void ps_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	int gc_erase = rnd_ps_gc_get_erase(gc);
	if (!gc_erase || !global.is_copper || global.drillcopper) {
		if (gc_erase && global.is_copper && global.drill_helper && radius >= global.drill_helper_size)
			radius = global.drill_helper_size;
		rnd_ps_fill_circle(&global.ps, gc, cx, cy, radius);
	}
}

static void ps_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_ps_fill_polygon_offs(&global.ps, gc, n_coords, x, y, dx, dy);
}


static void ps_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	ps_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void ps_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_ps_fill_rect(&global.ps, gc, x1, y1, x2, y2);
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
	hid->make_gc = rnd_ps_make_gc;
	hid->destroy_gc = rnd_ps_destroy_gc;
	hid->set_drawing_mode = ps_set_drawing_mode;
	hid->set_color = ps_set_color;
	hid->set_line_cap = rnd_ps_set_line_cap;
	hid->set_line_width = rnd_ps_set_line_width;
	hid->set_draw_xor = rnd_ps_set_draw_xor;
	hid->draw_line = ps_draw_line;
	hid->draw_arc = ps_draw_arc;
	hid->draw_rect = ps_draw_rect;
	hid->fill_circle = ps_fill_circle;
	hid->fill_polygon_offs = ps_fill_polygon_offs;
	hid->fill_polygon = ps_fill_polygon;
	hid->fill_rect = ps_fill_rect;
	hid->set_crosshair = rnd_ps_set_crosshair;

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

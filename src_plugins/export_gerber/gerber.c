#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <time.h>

#include <librnd/core/math_helper.h>
#include "board.h"
#include "build_run.h"
#include "config.h"
#include "data.h"
#include <librnd/core/error.h>
#include "draw.h"
#include "layer.h"
#include "layer_vis.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include "hid_cam.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include "funchash_core.h"
#include "gerber_conf.h"
#include <librnd/core/event.h>

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_inlines.h>
#include <librnd/core/hid_dad.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/actions.h>

#include "../src_plugins/export_excellon/aperture.h"

#include <librnd/plugins/lib_hid_common/xpm.h>


const char *gerber_cookie = "gerber HID";

conf_gerber_t conf_gerber;

#define CRASH(func) fprintf(stderr, "HID error: pcb called unimplemented Gerber function %s.\n", func); abort()

#define SUFF_LEN 256

#if SUFF_LEN < PCB_DERIVE_FN_SUFF_LEN
#	error SUFF_LEN needs at least PCB_DERIVE_FN_SUFF_LEN
#endif

static pcb_cam_t gerber_cam;

/* These are for films */
#define gerberX(pcb, x) ((rnd_coord_t) (x))
#define gerberY(pcb, y) ((rnd_coord_t) ((pcb)->hidlib.size_y - (y)))
#define gerberXOffset(pcb, x) ((rnd_coord_t) (x))
#define gerberYOffset(pcb, y) ((rnd_coord_t) (-(y)))

static int verbose;
static int all_layers;
static int is_mask, was_drill;
static int is_drill, is_plated;
static rnd_composite_op_t gerber_drawing_mode, drawing_mode_issued;
static int flash_drills, line_slots;
static int copy_outline_mode;
static int want_cross_sect;
static int want_per_file_apertures;
static int has_outline;
static int gerber_debug;
static int gerber_ovr;
static int gerber_global_aperture_cnt;
static long gerber_drawn_objs;

static aperture_list_t *layer_aptr_list;
static aperture_list_t *curr_aptr_list;
static int layer_list_max;
static int layer_list_idx;

static void reset_apertures(void)
{
	int i;
	for (i = 0; i < layer_list_max; ++i)
		uninit_aperture_list(&layer_aptr_list[i]);
	free(layer_aptr_list);
	layer_aptr_list = NULL;
	curr_aptr_list = NULL;
	layer_list_max = 0;
	layer_list_idx = 0;
}

static void fprint_aperture(FILE *f, aperture_t *aptr)
{
	switch (aptr->shape) {
	case ROUND:
		rnd_fprintf(f, "%%ADD%dC,%[5]*%%\r\n", aptr->dCode, aptr->width);
		break;
	case SQUARE:
		rnd_fprintf(f, "%%ADD%dR,%[5]X%[5]*%%\r\n", aptr->dCode, aptr->width, aptr->width);
		break;
	case OCTAGON:
		rnd_fprintf(f, "%%AMOCT%d*5,0,8,0,0,%[5],22.5*%%\r\n"
								"%%ADD%dOCT%d*%%\r\n", aptr->dCode, (rnd_coord_t) ((double) aptr->width / RND_COS_22_5_DEGREE), aptr->dCode, aptr->dCode);
		break;
	}
}

#define AUTO_OUTLINE_WIDTH RND_MIL_TO_COORD(8)	/* Auto-geneated outline width of 8 mils */

/* Set the aperture list for the current layer,
 * expanding the list buffer if needed  */
static aperture_list_t *set_layer_aperture_list(int layer_idx, int aper_per_file)
{
	if (layer_idx >= layer_list_max) {
		int i = layer_list_max;
		layer_list_max = 2 * (layer_idx + 1);
		layer_aptr_list = (aperture_list_t *)
			realloc(layer_aptr_list, layer_list_max * sizeof(*layer_aptr_list));
		for (; i < layer_list_max; ++i)
			init_aperture_list(&layer_aptr_list[i]);
	}
	curr_aptr_list = &layer_aptr_list[layer_idx];
	if (aper_per_file)
		curr_aptr_list->aperture_count = &curr_aptr_list->aperture_count_default;
	else
		curr_aptr_list->aperture_count = &gerber_global_aperture_cnt;
	return curr_aptr_list;
}

/* --------------------------------------------------------------------------- */

static rnd_hid_t gerber_hid;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_cap_style_t cap;
	int width;
	int color;
	int erase;
	int drill;
} rnd_hid_gc_s;

static FILE *f = NULL;
static gds_t fn_gds;
static int fn_baselen = 0;
static char *filename = NULL;
static char *filesuff = NULL;
static char *layername = NULL;
static int lncount = 0;

static int finding_apertures = 0;
static int pagecount = 0;
static int linewidth = -1;
static rnd_layergrp_id_t lastgroup = -1;
static int lastcap = -1;
static int lastcolor = -1;
static int lastX, lastY;				/* the last X and Y coordinate */

static const char *copy_outline_names[] = {
#define COPY_OUTLINE_NONE 0
	"none",
#define COPY_OUTLINE_MASK 1
	"mask",
#define COPY_OUTLINE_SILK 2
	"silk",
#define COPY_OUTLINE_ALL 3
	"all",
	NULL
};

typedef struct {
	const char *hdr1;
	const char *cfmt; /* drawing coordinate format */
	const char *afmt; /* aperture description format */
} coord_format_t;

static coord_format_t coord_format[] = {
	{"%MOIN*%\r\n%FSLAX25Y25*%\r\n", "%.0mc", "%.4mi"}, /* centimil:   inch, leading zero suppression, Absolute Data, 2.5 format */
	{"%MOMM*%\r\n%FSLAX43Y43*%\r\n", "%.0mu", "%.3mm"}, /* micrometer: mm,   leading zero suppression, Absolute Data, 4.3 format */
	{"%MOMM*%\r\n%FSLAX46Y46*%\r\n", "%.0mn", "%.6mm"}  /* nanometer:  mm,   leading zero suppression, Absolute Data, 4.6 format */
};
#define NUM_COORD_FORMATS (sizeof(coord_format)/sizeof(coord_format[0]))

static coord_format_t *gerber_cfmt;

static const char *coord_format_names[NUM_COORD_FORMATS+1] = {
	"centimil",
	"micrometer",
	"nanometer",
	NULL
};

static void gerber_warning(rnd_hid_export_opt_func_action_t act, void *call_ctx, rnd_export_opt_t *opt);

static rnd_export_opt_t gerber_options[] = {
	{"", "WARNING",
	 RND_HATT_BEGIN_VBOX, 0, 0, {0, 0, 0, 0, {0}, gerber_warning}, 0, 0},
#define HA_warning 0

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --gerberfile <string>
Gerber output file prefix. Can include a path.
@end ftable
%end-doc
*/
	{"gerberfile", "Gerber output file base",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_gerberfile 1

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --all-layers
Output contains all layers, even empty ones.
@end ftable
%end-doc
*/
	{"all-layers", "Output all layers, even empty ones",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_all_layers 2

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --verbose
Print file names and aperture counts on stdout.
@end ftable
%end-doc
*/
	{"verbose", "Print file names and aperture counts on stdout",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_verbose 3
	{"copy-outline", "Copy outline onto other layers",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, copy_outline_names, 0},
#define HA_copy_outline 4
	{"cross-sect", "Export the cross section layer",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cross_sect 5

	{"coord-format", "Coordinate format (resolution)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, coord_format_names, 0},
#define HA_coord_format 6

	{"aperture-per-file", "Restart aperture numbering in each new file",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_apeture_per_file 7

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 8
};

#define NUM_OPTIONS (sizeof(gerber_options)/sizeof(gerber_options[0]))

static rnd_hid_attr_val_t gerber_values[NUM_OPTIONS];

static rnd_export_opt_t *gerber_get_export_options(rnd_hid_t *hid, int *n)
{
	char **val = gerber_options[HA_gerberfile].value;
	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &gerber_options[HA_gerberfile], "");

	if (n)
		*n = NUM_OPTIONS;
	return gerber_options;
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

static void maybe_close_f(FILE * f)
{
	if (f) {
		fprintf(f, "M02*\r\n");
		fclose(f);
	}
}

static rnd_box_t region;

static void append_file_suffix(gds_t *dst, rnd_layergrp_id_t gid, rnd_layer_id_t lid, unsigned int flags, const char *purpose, int purpi, int drill, int *merge_same)
{
	const char *sext = ".gbr";

	fn_gds.used = fn_baselen;
	if (merge_same != NULL) *merge_same = 0;

	pcb_layer_to_file_name_append(dst, lid, flags, purpose, purpi, PCB_FNS_pcb_rnd);
	gds_append_str(dst, sext);

	filename = fn_gds.array;
	filesuff = fn_gds.array + fn_baselen;
}

static void gerber_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *fnbase;
	int i;
	static int saved_layer_stack[PCB_MAX_LAYER];
	int save_ons[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;
	rnd_xform_t xform;

	gerber_ovr = 0;

	drawing_mode_issued = RND_HID_COMP_POSITIVE;

	if (!options) {
		gerber_get_export_options(hid, NULL);
		options = gerber_values;
	}

	/* set up the coord format */
	i = options[HA_coord_format].lng;
	if ((i < 0) || (i >= NUM_COORD_FORMATS)) {
		rnd_message(RND_MSG_ERROR, "Invalid coordinate format (out of bounds)\n");
		return;
	}
	gerber_cfmt = &coord_format[i];
	rnd_printf_slot[4] = gerber_cfmt->cfmt;
	rnd_printf_slot[5] = gerber_cfmt->afmt;

	gerber_drawn_objs = 0;
	pcb_cam_begin(PCB, &gerber_cam, &xform, options[HA_cam].str, gerber_options, NUM_OPTIONS, options);

	fnbase = options[HA_gerberfile].str;
	if (!fnbase)
		fnbase = "pcb-out";

	verbose = options[HA_verbose].lng || rnd_conf.rc.verbose;
	all_layers = options[HA_all_layers].lng;

	copy_outline_mode = options[HA_copy_outline].lng;

	want_cross_sect = options[HA_cross_sect].lng;
	want_per_file_apertures = options[HA_apeture_per_file].lng;

	has_outline = pcb_has_explicit_outline(PCB);

	i = strlen(fnbase);
	gds_init(&fn_gds);
	gds_append_str(&fn_gds, fnbase);
	gds_append(&fn_gds, '.');
	fn_baselen = fn_gds.used;
	filename = fn_gds.array;

	if (!gerber_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));
	qsort(pcb_layer_stack, pcb_max_layer(PCB), sizeof(pcb_layer_stack[0]), layer_sort);
	linewidth = -1;
	lastcap = -1;
	lastgroup = -1;
	lastcolor = -1;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

	pagecount = 1;
	reset_apertures();

	xform.no_slot_in_nonmech = 1;

	lastgroup = -1;
	layer_list_idx = 0;
	finding_apertures = 1;
	rnd_expose_main(&gerber_hid, &ctx, &xform);

	lastgroup = -2;
	layer_list_idx = 0;
	finding_apertures = 0;
	rnd_expose_main(&gerber_hid, &ctx, &xform);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));

	maybe_close_f(f);
	f = NULL;
	if (!gerber_cam.active)
		pcb_hid_restore_layer_ons(save_ons);
	rnd_conf_update(NULL, -1); /* resotre forced sets */

	if (!gerber_cam.active) gerber_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&gerber_cam) == 0) {
		if (!gerber_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "gerber cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (gerber_drawn_objs == 0) {
		if (!gerber_cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "gerber cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}

	/* in cam mode we have f still open */
	maybe_close_f(f);
	f = NULL;
	gds_uninit(&fn_gds);
}

static int gerber_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, gerber_options, NUM_OPTIONS, gerber_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}



static int gerber_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	int want_outline;
	char *cp;
	const char *group_name;

	pcb_cam_set_layer_group(&gerber_cam, group, purpose, purpi, flags, xform);

	if ((!gerber_cam.active) && (PCB_LAYER_IS_ASSY(flags, purpi)))
		return 0;

	if ((!gerber_cam.active) && (flags & PCB_LYT_DOC))
		return 0;

	if (flags & PCB_LYT_UI)
		return 0;

#if 0
	printf(" Layer %s group %lx drill %d mask %d flags=%lx\n", pcb_layer_name(PCB, layer), group, is_drill, is_mask, flags);
#endif


	if (!all_layers) {
		int stay = 0;
		if ((group >= 0) && pcb_layergrp_is_empty(PCB, group) && !(flags & PCB_LYT_SILK)) {
			/* layer is empty and the user didn't want to have empty layers; however;
			   if the user wants to copy the outline to specific layers, those
			   layers will become non-empty: even an empty outline would bring
			   the implicit outline rectangle at board extents! */

			if (PCB_LAYER_IS_OUTLINE(flags, purpi)) stay = 1; /* outline layer can never be empty, because of the implicit outline */

			if (copy_outline_mode == COPY_OUTLINE_MASK && (flags & PCB_LYT_MASK)) stay = 1;
			if (copy_outline_mode == COPY_OUTLINE_SILK && (flags & PCB_LYT_SILK)) stay = 1;
			if (copy_outline_mode == COPY_OUTLINE_ALL && \
				((flags & PCB_LYT_SILK) || (flags & PCB_LYT_MASK) ||
				PCB_LAYER_IS_FAB(flags, purpi) ||
				PCB_LAYER_IS_ASSY(flags, purpi))) stay = 1;

			if (!stay) return 0;
		}
	}

	if (flags & PCB_LYT_INVIS) {
/*		printf("  nope: invis %d or assy %d\n", (flags & PCB_LYT_INVIS), (flags & PCB_LYT_ASSY));*/
		return 0;
	}

	if (PCB_LAYER_IS_CSECT(flags, purpi) && (!want_cross_sect))
		return 0;

	if ((group >= 0) && (group < pcb_max_group(PCB))) {
		group_name = PCB->LayerGroups.grp[group].name;
		if (group_name == NULL)
			group_name = "<unknown>";
	}
	else
		group_name = "<virtual group>";

	flash_drills = 0;
	line_slots = 0;
	if ((flags & PCB_LYT_MECH) && PCB_LAYER_IS_ROUTE(flags, purpi)) {
		flash_drills = 1;
		line_slots = 1;
	}

	is_drill = PCB_LAYER_IS_DRILL(flags, purpi) || ((flags & PCB_LYT_MECH) && PCB_LAYER_IS_ROUTE(flags, purpi));
	is_plated = PCB_LAYER_IS_PROUTE(flags, purpi) || PCB_LAYER_IS_PDRILL(flags, purpi);
	is_mask = !!(flags & PCB_LYT_MASK);
	if (group < 0 || group != lastgroup) {
		char utcTime[64];
		aperture_list_t *aptr_list;
		aperture_t *search;

		lastgroup = group;
		lastX = -1;
		lastY = -1;
		lastcolor = 0;
		linewidth = -1;
		lastcap = -1;

		aptr_list = set_layer_aperture_list(layer_list_idx++, want_per_file_apertures);

		if (finding_apertures)
			goto emit_outline;

		if (aptr_list->count == 0 && !all_layers && !is_drill)
			return 0;

		if ((!gerber_cam.active) || (gerber_cam.fn_changed)) {
			/* in cam mode we reuse f */
			maybe_close_f(f);
			f = NULL;
		}

		pagecount++;
		append_file_suffix(&fn_gds, group, layer, flags, purpose, purpi, 0, NULL);
		if (f == NULL) { /* open a new file if we closed the previous (cam mode: only one file) */
			f = rnd_fopen_askovr(&PCB->hidlib, gerber_cam.active ? gerber_cam.fn : filename, "wb", &gerber_ovr); /* Binary needed to force CR-LF */
			if (f == NULL) {
				rnd_message(RND_MSG_ERROR, "Error:  Could not open %s for writing.\n", filename);
				return 1;
			}
		}

		was_drill = is_drill;

		if (verbose) {
			int c = aptr_list->count;
			fprintf(stderr, "Gerber: %d aperture%s in %s\n", c, c == 1 ? "" : "s", filename);
		}

		fprintf(f, "G04 start of page %d for group %ld layer_idx %ld *\r\n", pagecount, group, layer);

		/* Create a portable timestamp. */
		rnd_print_utc(utcTime, sizeof(utcTime), 0);

		/* Print a cute file header at the beginning of each file. */
		fprintf(f, "G04 Title: %s, %s *\r\n", RND_UNKNOWN(PCB->hidlib.name), RND_UNKNOWN(group_name));
		fprintf(f, "G04 Creator: pcb-rnd " PCB_VERSION " *\r\n");
		fprintf(f, "G04 CreationDate: %s *\r\n", utcTime);

		/* ID the user. */
		fprintf(f, "G04 For: %s *\r\n", pcb_author());

		fprintf(f, "G04 Format: Gerber/RS-274X *\r\n");
		rnd_fprintf(f, "G04 PCB-Dimensions: %[4] %[4] *\r\n", PCB->hidlib.size_x, PCB->hidlib.size_y);
		fprintf(f, "G04 PCB-Coordinate-Origin: lower left *\r\n");

		/* Unit and coord format */
		fprintf(f, "%s", gerber_cfmt->hdr1);

		/* build a legal identifier. */
		if (layername)
			free(layername);
		layername = rnd_strdup(filesuff);
		if (strrchr(layername, '.'))
			*strrchr(layername, '.') = 0;

		for (cp = layername; *cp; cp++) {
			if (isalnum((int) *cp))
				*cp = toupper((int) *cp);
			else
				*cp = '_';
		}
		fprintf(f, "%%LN%s*%%\r\n", layername);
		lncount = 1;

		for (search = aptr_list->data; search; search = search->next)
			fprint_aperture(f, search);
		if (aptr_list->count == 0)
			/* We need to put *something* in the file to make it be parsed
			   as RS-274X instead of RS-274D. */
			fprintf(f, "%%ADD11C,0.0100*%%\r\n");
	}

emit_outline:
	/* If we're printing a copper layer other than the outline layer,
	   and we want to "print outlines", and we have an outline layer,
	   print the outline layer on this layer also.  */
	want_outline = 0;
	if (copy_outline_mode == COPY_OUTLINE_MASK && (flags & PCB_LYT_MASK))
		want_outline = 1;
	if (copy_outline_mode == COPY_OUTLINE_SILK && (flags & PCB_LYT_SILK))
		want_outline = 1;

	if (copy_outline_mode == COPY_OUTLINE_ALL && ((flags & PCB_LYT_SILK) || (flags & PCB_LYT_MASK) || PCB_LAYER_IS_FAB(flags, purpi) || PCB_LAYER_IS_ASSY(flags, purpi)))
		want_outline = 1;

	if (want_outline && !(PCB_LAYER_IS_ROUTE(flags, purpi))) {
		if (has_outline) {
			pcb_draw_groups(hid, PCB, PCB_LYT_BOUNDARY, F_proute, NULL, &region, rnd_color_black, PCB_LYT_MECH, 0, 0);
			pcb_draw_groups(hid, PCB, PCB_LYT_BOUNDARY, F_uroute, NULL, &region, rnd_color_black, PCB_LYT_MECH, 0, 0);
		}
		else {
			rnd_hid_gc_t gc = rnd_hid_make_gc();
			if (flags & PCB_LYT_SILK)
				rnd_hid_set_line_width(gc, conf_core.design.min_slk);
			else if (group >= 0)
				rnd_hid_set_line_width(gc, conf_core.design.min_wid);
			else
				rnd_hid_set_line_width(gc, AUTO_OUTLINE_WIDTH);
			rnd_render->draw_line(gc, 0, 0, PCB->hidlib.size_x, 0);
			rnd_render->draw_line(gc, 0, 0, 0, PCB->hidlib.size_y);
			rnd_render->draw_line(gc, PCB->hidlib.size_x, 0, PCB->hidlib.size_x, PCB->hidlib.size_y);
			rnd_render->draw_line(gc, 0, PCB->hidlib.size_y, PCB->hidlib.size_x, PCB->hidlib.size_y);
			rnd_hid_destroy_gc(gc);
		}
	}
	return 1;
}

static rnd_hid_gc_t gerber_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(1, sizeof(*rv));
	rv->cap = rnd_cap_round;
	return rv;
}

static void gerber_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void gerber_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *drw_screen)
{
	gerber_drawing_mode = op;
	if ((f != NULL) && (gerber_debug))
		fprintf(f, "G04 hid debug composite: %d*\r\n", op);
}

static void gerber_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	if (rnd_color_is_drill(color)) {
		gc->color = 1;
		gc->erase = 0;
		gc->drill = 1;
	}
	else {
		gc->color = 0;
		gc->erase = 0;
		gc->drill = 0;
	}
}

static void gerber_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

static void gerber_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}

static void gerber_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

static void use_gc(rnd_hid_gc_t gc, int radius)
{
	gerber_drawn_objs++;
	if ((f != NULL) && (gerber_drawing_mode != drawing_mode_issued)) {
		if ((gerber_drawing_mode == RND_HID_COMP_POSITIVE) || (gerber_drawing_mode == RND_HID_COMP_POSITIVE_XOR)) {
			fprintf(f, "%%LPD*%%\r\n");
			drawing_mode_issued = gerber_drawing_mode;
		}
		else if (gerber_drawing_mode == RND_HID_COMP_NEGATIVE) {
			fprintf(f, "%%LPC*%%\r\n");
			drawing_mode_issued = gerber_drawing_mode;
		}
	}

	if (radius) {
		radius *= 2;
		if (radius != linewidth || lastcap != rnd_cap_round) {
			aperture_t *aptr = find_aperture(curr_aptr_list, radius, ROUND);
			if (aptr == NULL)
				rnd_fprintf(stderr, "error: aperture for radius %$mS type ROUND is null\n", radius);
			else if (f != NULL)
				fprintf(f, "G54D%d*", aptr->dCode);
			linewidth = radius;
			lastcap = rnd_cap_round;
		}
	}
	else if (linewidth != gc->width || lastcap != gc->cap) {
		aperture_t *aptr;
		aperture_shape_t shape;

		linewidth = gc->width;
		lastcap = gc->cap;
		switch (gc->cap) {
		case rnd_cap_round:
			shape = ROUND;
			break;
		case rnd_cap_square:
			shape = SQUARE;
			break;
		default:
			assert(!"unhandled cap");
			shape = ROUND;
		}
		aptr = find_aperture(curr_aptr_list, linewidth, shape);
		if (aptr == NULL)
			rnd_fprintf(stderr, "error: aperture for width %$mS type %s is null\n", linewidth, shape == ROUND ? "ROUND" : "SQUARE");
		if (f && aptr)
			fprintf(f, "G54D%d*", aptr->dCode);
	}
}

static void gerber_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_bool m = rnd_false;
	int i;
	int firstTime = 1;
	rnd_coord_t startX = 0, startY = 0;

	if (is_mask && (gerber_drawing_mode != RND_HID_COMP_POSITIVE) && (gerber_drawing_mode != RND_HID_COMP_POSITIVE_XOR) && (gerber_drawing_mode != RND_HID_COMP_NEGATIVE))
		return;

	use_gc(gc, 10 * 100);
	if (!f)
		return;
	fprintf(f, "G36*\r\n");
	for (i = 0; i < n_coords; i++) {
		if (x[i]+dx != lastX) {
			m = rnd_true;
			lastX = x[i]+dx;
			rnd_fprintf(f, "X%[4]", gerberX(PCB, lastX));
		}
		if (y[i]+dy != lastY) {
			m = rnd_true;
			lastY = y[i]+dy;
			rnd_fprintf(f, "Y%[4]", gerberY(PCB, lastY));
		}
		if (firstTime) {
			firstTime = 0;
			startX = x[i]+dx;
			startY = y[i]+dy;
			if (m)
				fprintf(f, "D02*");
		}
		else if (m)
			fprintf(f, "D01*\r\n");
		m = rnd_false;
	}
	if (startX != lastX) {
		m = rnd_true;
		lastX = startX;
		rnd_fprintf(f, "X%[4]", gerberX(PCB, startX));
	}
	if (startY != lastY) {
		m = rnd_true;
		lastY = startY;
		rnd_fprintf(f, "Y%[4]", gerberY(PCB, lastY));
	}
	if (m)
		fprintf(f, "D01*\r\n");
	fprintf(f, "G37*\r\n");
}

static void gerber_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	gerber_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void gerber_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_bool m = rnd_false;

	if (line_slots) {
		rnd_coord_t dia = gc->width/2;
		find_aperture(curr_aptr_list, dia*2, ROUND); /* for a real gerber export of the BOUNDARY group: place aperture on the per layer aperture list */

		if (finding_apertures)
			return;
	}

	if (x1 != x2 && y1 != y2 && gc->cap == rnd_cap_square) {
		rnd_coord_t x[5], y[5];
		double tx, ty, theta;

		theta = atan2(y2 - y1, x2 - x1);

		/* T is a vector half a thickness long, in the direction of
		   one of the corners.  */
		tx = gc->width / 2.0 * cos(theta + M_PI / 4) * sqrt(2.0);
		ty = gc->width / 2.0 * sin(theta + M_PI / 4) * sqrt(2.0);

		x[0] = x1 - tx;
		y[0] = y1 - ty;
		x[1] = x2 + ty;
		y[1] = y2 - tx;
		x[2] = x2 + tx;
		y[2] = y2 + ty;
		x[3] = x1 - ty;
		y[3] = y1 + tx;

		x[4] = x[0];
		y[4] = y[0];
		gerber_fill_polygon(gc, 5, x, y);
		return;
	}

	use_gc(gc, 0);
	if (!f)
		return;

	if (x1 != lastX) {
		m = rnd_true;
		lastX = x1;
		rnd_fprintf(f, "X%[4]", gerberX(PCB, lastX));
	}
	if (y1 != lastY) {
		m = rnd_true;
		lastY = y1;
		rnd_fprintf(f, "Y%[4]", gerberY(PCB, lastY));
	}
	if ((x1 == x2) && (y1 == y2))
		fprintf(f, "D03*\r\n");
	else {
		if (m)
			fprintf(f, "D02*");
		if (x2 != lastX) {
			lastX = x2;
			rnd_fprintf(f, "X%[4]", gerberX(PCB, lastX));
		}
		if (y2 != lastY) {
			lastY = y2;
			rnd_fprintf(f, "Y%[4]", gerberY(PCB, lastY));

		}
		fprintf(f, "D01*\r\n");
	}
}

static void gerber_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	gerber_draw_line(gc, x1, y1, x1, y2);
	gerber_draw_line(gc, x1, y1, x2, y1);
	gerber_draw_line(gc, x1, y2, x2, y2);
	gerber_draw_line(gc, x2, y1, x2, y2);
}

static void gerber_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_bool m = rnd_false;
	double arcStartX, arcStopX, arcStartY, arcStopY;

	/* we never draw zero-width lines */
	if (gc->width == 0)
		return;

	use_gc(gc, 0);
	if (!f)
		return;

	/* full circle is full.... truncate so that the arc split code never needs to
	   do more than 180 deg */
	if (delta_angle < -360.0)
		delta_angle = -360.0;
	if (delta_angle > +360.0)
		delta_angle = +360.0;


	/* some gerber interpreters (gerbv for one) have hard time dealing with
	   full-circle arcs - split large arcs up into 2 smaller ones */
	if (delta_angle < -180.0) {
		gerber_draw_arc(gc, cx, cy, width, height, start_angle, -180.0);
		gerber_draw_arc(gc, cx, cy, width, height, start_angle-180, delta_angle+180.0);
		return;
	}
	if (delta_angle > +180.0) {
		gerber_draw_arc(gc, cx, cy, width, height, start_angle, 180.0);
		gerber_draw_arc(gc, cx, cy, width, height, start_angle+180, delta_angle-180.0);
		return;
	}


	arcStartX = cx - width * cos(RND_TO_RADIANS(start_angle));
	arcStartY = cy + height * sin(RND_TO_RADIANS(start_angle));

	if (fabs(delta_angle) < 0.01) {
		gerber_draw_line(gc, arcStartX, arcStartY, arcStartX, arcStartY);
		return;
	}

	/* I checked three different gerber viewers, and they all disagreed
	   on how ellipses should be drawn.  The spec just calls G74/G75
	   "circular interpolation" so there's a chance it just doesn't
	   support ellipses at all.  Thus, we draw them out with line
	   segments.  Note that most arcs in pcb are circles anyway.  */
	if (width != height) {
		double step, angle;
		rnd_coord_t max = width > height ? width : height;
		rnd_coord_t minr = max - gc->width / 10;
		int nsteps;
		rnd_coord_t x0, y0, x1, y1;

		if (minr >= max)
			minr = max - 1;
		step = acos((double) minr / (double) max) * 180.0 / M_PI;
		if (step > 5)
			step = 5;
		nsteps = fabs(delta_angle) / step + 1;
		step = (double) delta_angle / nsteps;

		x0 = arcStartX;
		y0 = arcStartY;
		angle = start_angle;
		while (nsteps > 0) {
			nsteps--;
			x1 = cx - width * cos(RND_TO_RADIANS(angle + step));
			y1 = cy + height * sin(RND_TO_RADIANS(angle + step));
			gerber_draw_line(gc, x0, y0, x1, y1);
			x0 = x1;
			y0 = y1;
			angle += step;
		}
		return;
	}

	arcStopX = cx - width * cos(RND_TO_RADIANS(start_angle + delta_angle));
	arcStopY = cy + height * sin(RND_TO_RADIANS(start_angle + delta_angle));
	if (arcStartX != lastX) {
		m = rnd_true;
		lastX = arcStartX;
		rnd_fprintf(f, "X%[4]", gerberX(PCB, lastX));
	}
	if (arcStartY != lastY) {
		m = rnd_true;
		lastY = arcStartY;
		rnd_fprintf(f, "Y%[4]", gerberY(PCB, lastY));
	}
	if (m)
		fprintf(f, "D02*");
	rnd_fprintf(f,
							"G75*G0%1dX%[4]Y%[4]I%[4]J%[4]D01*G01*\r\n",
							(delta_angle < 0) ? 2 : 3,
							gerberX(PCB, arcStopX), gerberY(PCB, arcStopY),
							gerberXOffset(PCB, cx - arcStartX), gerberYOffset(PCB, cy - arcStartY));
	lastX = arcStopX;
	lastY = arcStopY;
}

static void gerber_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	if (radius <= 0)
		return;
	if (is_drill)
		radius = 50 * rnd_round(radius / 50.0);
	use_gc(gc, radius);
	if (!f)
		return;
	if (is_drill) {
		if (finding_apertures)
			return;
	}
	else if (gc->drill && !flash_drills)
		return;
	if (cx != lastX) {
		lastX = cx;
		rnd_fprintf(f, "X%[4]", gerberX(PCB, lastX));
	}
	if (cy != lastY) {
		lastY = cy;
		rnd_fprintf(f, "Y%[4]", gerberY(PCB, lastY));
	}
	fprintf(f, "D03*\r\n");
}

static void gerber_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_coord_t x[5];
	rnd_coord_t y[5];
	x[0] = x[4] = x1;
	y[0] = y[4] = y1;
	x[1] = x1;
	y[1] = y2;
	x[2] = x2;
	y[2] = y2;
	x[3] = x2;
	y[3] = y1;
	gerber_fill_polygon(gc, 5, x, y);
}

static void gerber_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	CRASH("gerber_calibrate");
}

static int gerber_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\ngerber exporter command line arguments:\n\n");
	rnd_hid_usage(gerber_options, sizeof(gerber_options) / sizeof(gerber_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x gerber [gerber options] foo.pcb\n\n");
	return 0;
}

static void gerber_go_to_cam_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_dad_retovr_t retovr;
	rnd_hid_dad_close(hid_ctx, &retovr, -1);
	rnd_actionva(&PCB->hidlib, "cam", NULL);
}


static void gerber_warning(rnd_hid_export_opt_func_action_t act, void *call_ctx, rnd_export_opt_t *opt)
{
	const char warn_txt[] =
		"WARNING: direct gerber export is most probably NOT what\n"
		"you want, especially if you are exporting to fab the\n"
		"board. Please use the cam export instead!\n"
		"For more info please read:\n"
		"http://repo.hu/cgi-bin/pool.cgi?cmd=show&node=cam_switch\n";
	rnd_hid_export_opt_func_dad_t *dad = call_ctx;

	switch(act) {
		case RND_HIDEOF_USAGE:
			fprintf((FILE *)call_ctx, "******************************************************************************\n");
			fprintf((FILE *)call_ctx, warn_txt);
			fprintf((FILE *)call_ctx, "For the cam export, try --help cam\n");
			fprintf((FILE *)call_ctx, "Command line would look like pcb-rnd -x cam gerber:fixed filename\n");
			fprintf((FILE *)call_ctx, "******************************************************************************\n\n");
			break;
		case RND_HIDEOF_DAD:
			RND_DAD_BEGIN_VBOX(dad->dlg);
				RND_DAD_COMPFLAG(dad->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME);
				RND_DAD_BEGIN_HBOX(dad->dlg);
					RND_DAD_PICTURE(dad->dlg, pcp_dlg_xpm_by_name("warning"));
					RND_DAD_BEGIN_VBOX(dad->dlg);
						RND_DAD_LABEL(dad->dlg, warn_txt);
						RND_DAD_BUTTON(dad->dlg, "Get me to the cam export dialog");
							RND_DAD_CHANGE_CB(dad->dlg, gerber_go_to_cam_cb);
						RND_DAD_LABEL(dad->dlg, "");
					RND_DAD_END(dad->dlg);
				RND_DAD_END(dad->dlg);
			RND_DAD_END(dad->dlg);
			break;
	}
}


static void gerber_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
}

static void gerber_session_begin(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	gerber_global_aperture_cnt = 0;
}

int pplg_check_ver_export_gerber(int ver_needed) { return 0; }

void pplg_uninit_export_gerber(void)
{
	rnd_export_remove_opts_by_cookie(gerber_cookie);
	rnd_conf_unreg_fields("plugins/export_gerber/");
	rnd_event_unbind_allcookie(gerber_cookie);
	rnd_hid_remove_hid(&gerber_hid);
}

int pplg_init_export_gerber(void)
{
	RND_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_gerber, field,isarray,type_name,cpath,cname,desc,flags);
#include "gerber_conf_fields.h"

	memset(&gerber_hid, 0, sizeof(gerber_hid));

	rnd_hid_nogui_init(&gerber_hid);

	gerber_hid.struct_size = sizeof(gerber_hid);
	gerber_hid.name = "gerber";
	gerber_hid.description = "RS-274X (Gerber) export";
	gerber_hid.exporter = 1;

	gerber_hid.mask_invert = 1;

	gerber_hid.get_export_options = gerber_get_export_options;
	gerber_hid.do_export = gerber_do_export;
	gerber_hid.parse_arguments = gerber_parse_arguments;
	gerber_hid.set_layer_group = gerber_set_layer_group;
	gerber_hid.make_gc = gerber_make_gc;
	gerber_hid.destroy_gc = gerber_destroy_gc;
	gerber_hid.set_drawing_mode = gerber_set_drawing_mode;
	gerber_hid.set_color = gerber_set_color;
	gerber_hid.set_line_cap = gerber_set_line_cap;
	gerber_hid.set_line_width = gerber_set_line_width;
	gerber_hid.set_draw_xor = gerber_set_draw_xor;
	gerber_hid.draw_line = gerber_draw_line;
	gerber_hid.draw_arc = gerber_draw_arc;
	gerber_hid.draw_rect = gerber_draw_rect;
	gerber_hid.fill_circle = gerber_fill_circle;
	gerber_hid.fill_polygon = gerber_fill_polygon;
	gerber_hid.fill_polygon_offs = gerber_fill_polygon_offs;
	gerber_hid.fill_rect = gerber_fill_rect;
	gerber_hid.calibrate = gerber_calibrate;
	gerber_hid.set_crosshair = gerber_set_crosshair;
	gerber_hid.usage = gerber_usage;
	gerber_hid.argument_array = gerber_values;

	rnd_hid_register_hid(&gerber_hid);

	rnd_event_bind(RND_EVENT_EXPORT_SESSION_BEGIN, gerber_session_begin, NULL, gerber_cookie);

	return 0;
}


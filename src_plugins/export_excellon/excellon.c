#include "config.h"

#include "board.h"
#include <librnd/core/global_typedefs.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/error.h>
#include "draw.h"
#include <librnd/core/compat_misc.h>
#include "layer.h"
#include "layer_vis.h"
#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/plugins.h>
#include "funchash_core.h"
#include "conf_core.h"
#include <librnd/core/event.h>
#include "excellon_conf.h"

#include "excellon.h"

conf_excellon_t conf_excellon;

static int exc_aperture_cnt;

#define excellonDrX(pcb, x) ((rnd_coord_t) (x))
#define excellonDrY(pcb, y) ((rnd_coord_t) ((pcb)->hidlib.size_y - (y)))

typedef struct {
	const char *hdr1;
	const char *cfmt; /* drawing coordinate format */
	const char *afmt; /* aperture description format */
} coord_format_t;

static coord_format_t coord_format[] = {
	{"INCH",            "%06.0mk", "%.3mi"}, /* decimil: inch in 00.0000 format, implicit leading-zero (LZ); max board size: 99 inch (2514mm) */
	{"METRIC,000.000",  "%03.3mm", "%.6mm"}, /* micron: mm in 000.0000 format, implicit leading-zero (LZ); max board size: 999mm */
	{"METRIC,0000.00",  "%04.2mm", "%.5mm"}, /* micron: mm in 000.0000 format, implicit leading-zero (LZ); max board size: 9999mm */
};
#define NUM_COORD_FORMATS (sizeof(coord_format)/sizeof(coord_format[0]))

static const char *coord_format_names[NUM_COORD_FORMATS+1] = {
	"decimil",
	"um",
	"10um",
	NULL
};


static rnd_cardinal_t drill_print_objs(pcb_board_t *pcb, FILE *f, pcb_drill_ctx_t *ctx, int force_g85, int slots, rnd_coord_t *excellon_last_tool_dia)
{
	rnd_cardinal_t i, cnt = 0;
	int first = 1;

	for (i = 0; i < ctx->obj.used; i++) {
		pcb_pending_drill_t *pd = &ctx->obj.array[i];
		if (slots != (!!pd->is_slot))
			continue;
		if (i == 0 || pd->diam != *excellon_last_tool_dia) {
			aperture_t *ap = find_aperture(&ctx->apr, pd->diam, ROUND);
			if (ap == NULL) {
				rnd_message(RND_MSG_ERROR, "excellon: internal error: can't register ROUND aperture of dia %$mm\n", pd->diam);
				continue;
			}
			fprintf(f, "T%02d\r\n", ap->dCode);
			*excellon_last_tool_dia = pd->diam;
		}
		if (pd->is_slot) {
			if (first) {
				rnd_fprintf(f, "G00");
				first = 0;
			}
			if (force_g85)
				rnd_fprintf(f, "X%[3]Y%[3]G85X%[3]Y%[3]\r\n", excellonDrX(pcb, pd->x), excellonDrY(PCB, pd->y), excellonDrX(pcb, pd->x2), excellonDrY(PCB, pd->y2));
			else
				rnd_fprintf(f, "X%[3]Y%[3]\r\nM15\r\nG01X%[3]Y%[3]\r\nM17\r\n", excellonDrX(pcb, pd->x), excellonDrY(PCB, pd->y), excellonDrX(pcb, pd->x2), excellonDrY(PCB, pd->y2));
			first = 1; /* each new slot will need a G00 for some fabs that ignore M17 and M15 */
		}
		else {
			if (first) {
				rnd_fprintf(f, "G05\r\n");
				first = 0;
			}
			rnd_fprintf(f, "X%[3]Y%[3]\r\n", excellonDrX(pcb, pd->x), excellonDrY(pcb, pd->y));
		}
		cnt++;
	}
	return cnt;
}

static rnd_cardinal_t drill_print_holes(pcb_board_t *pcb, FILE *f, pcb_drill_ctx_t *ctx, int force_g85, const char *coord_fmt_hdr)
{
	aperture_t *search;
	rnd_cardinal_t cnt = 0;
	rnd_coord_t excellon_last_tool_dia = 0;

	/* We omit the ,TZ here because we are not omitting trailing zeros.  Our format is
	   always six-digit 0.1 mil resolution (i.e. 001100 = 0.11") */
	fprintf(f, "M48\r\n%s\r\n", coord_fmt_hdr);
	for (search = ctx->apr.data; search; search = search->next)
		rnd_fprintf(f, "T%02dC%[2]\r\n", search->dCode, search->width);
	fprintf(f, "%%\r\n");

	/* dump pending drills in sequence */
	pcb_drill_sort(ctx);
	cnt += drill_print_objs(pcb, f, ctx, force_g85, 0, &excellon_last_tool_dia);
	cnt += drill_print_objs(pcb, f, ctx, force_g85, 1, &excellon_last_tool_dia);
	return cnt;
}

void pcb_drill_export_excellon(pcb_board_t *pcb, pcb_drill_ctx_t *ctx, int force_g85, int coord_fmt_idx, const char *fn)
{
	FILE *f = rnd_fopen_askovr(&PCB->hidlib, fn, "wb", NULL); /* Binary needed to force CR-LF */
	coord_format_t *cfmt;

	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Error:  Could not open %s for writing the excellon file.\n", fn);
		return;
	}

	if ((coord_fmt_idx < 0) || (coord_fmt_idx >= NUM_COORD_FORMATS)) {
		rnd_message(RND_MSG_ERROR, "Error: Invalid excellon coordinate format idx %d.\n", coord_fmt_idx);
		return;
	}

	cfmt = &coord_format[coord_fmt_idx];
	rnd_printf_slot[3] = cfmt->cfmt;
	rnd_printf_slot[2] = cfmt->afmt;

	if (ctx->obj.used > 0)
		drill_print_holes(pcb, f, ctx, force_g85, cfmt->hdr1);

	fprintf(f, "M30\r\n");
	fclose(f);
}

/*** HID ***/
static rnd_hid_t excellon_hid;
static const char *excellon_cookie = "excellon drill/cnc exporter";

#define SUFF_LEN 32

/* global exporter states */
static int is_plated, finding_apertures, exc_aperture_cnt;
static pcb_drill_ctx_t pdrills, udrills;
static pcb_cam_t excellon_cam;
static rnd_coord_t lastwidth;
static char *filename = NULL;
static struct {
	unsigned nonround:1;
	unsigned arc:1;
	unsigned poly:1;
	unsigned comp:1;
} warn;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_cap_style_t style;
	rnd_coord_t width;
} rnd_hid_gc_s;

static long exc_drawn_objs;

static rnd_export_opt_t excellon_options[] = {

/* %start-doc options "90 excellon Export"
@ftable @code
@item --excellonfile <string>
excellon output file prefix. Can include a path.
@end ftable
%end-doc
*/
	{"filename", "excellon output file base - used to generate default plated and/or unplated file name in case they are not specified",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_excellonfile 0

	{"filename-plated", "excellon output file name for plated features",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_excellonfile_plated 1

	{"filename-unplated", "excellon output file name for unplated features",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_excellonfile_unplated 2

	{"coord-format", "Coordinate format (resolution)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, coord_format_names, 0},
#define HA_excellonfile_coordfmt 3

	{"aperture-per-file", "Restart aperture numbering in each new file",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_apeture_per_file 4

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 5
};

#define NUM_OPTIONS (sizeof(excellon_options)/sizeof(excellon_options[0]))

static rnd_hid_attr_val_t excellon_values[NUM_OPTIONS];

static rnd_export_opt_t *excellon_get_export_options(rnd_hid_t *hid, int *n)
{
	char **val = excellon_options[HA_excellonfile].value;
	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &excellon_options[HA_excellonfile], "");

	if (n)
		*n = NUM_OPTIONS;
	return excellon_options;
}

static void excellon_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *fnbase, *fn;
	char *filesuff;
	int i;
	int save_ons[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;
	rnd_xform_t xform;

	if (!options) {
		excellon_get_export_options(hid, NULL);
		options = excellon_values;
	}
	pcb_drill_init(&pdrills, options[HA_apeture_per_file].lng ? NULL : &exc_aperture_cnt);
	pcb_drill_init(&udrills, options[HA_apeture_per_file].lng ? NULL : &exc_aperture_cnt);
	memset(&warn, 0, sizeof(warn));

	exc_drawn_objs = 0;
	pcb_cam_begin(PCB, &excellon_cam, &xform, options[HA_cam].str, excellon_options, NUM_OPTIONS, options);

	fnbase = options[HA_excellonfile].str;
	if (!fnbase)
		fnbase = "pcb-out";

	i = strlen(fnbase);
	filename = (char *) realloc(filename, i + SUFF_LEN);
	strcpy(filename, fnbase);
	filesuff = filename + strlen(filename);

	if (!excellon_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

	lastwidth = -1;
	finding_apertures = 1;
	rnd_expose_main(&excellon_hid, &ctx, &xform);

	lastwidth = -1;
	finding_apertures = 0;
	rnd_expose_main(&excellon_hid, &ctx, &xform);
	rnd_conf_update(NULL, -1); /* resotre forced sets */


	if (excellon_cam.active) {
		fn = excellon_cam.fn;
		pcb_drill_export_excellon(PCB, &pdrills, conf_excellon.plugins.export_excellon.plated_g85_slot, options[HA_excellonfile_coordfmt].lng, fn);
	}
	else {
		if (options[HA_excellonfile_plated].str == NULL) {
			strcpy(filesuff, ".plated.cnc");
			fn = filename;
		}
		else
			fn = options[HA_excellonfile_plated].str;
		pcb_drill_export_excellon(PCB, &pdrills, conf_excellon.plugins.export_excellon.plated_g85_slot, options[HA_excellonfile_coordfmt].lng, fn);

		if (options[HA_excellonfile_unplated].str == NULL) {
			strcpy(filesuff, ".unplated.cnc");
			fn = filename;
		}
		else
			fn = options[HA_excellonfile_unplated].str;
		pcb_drill_export_excellon(PCB, &udrills, conf_excellon.plugins.export_excellon.unplated_g85_slot, options[HA_excellonfile_coordfmt].lng, fn);
	}

	if (!excellon_cam.active) excellon_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&excellon_cam) == 0) {
		if (!excellon_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "excellon cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (exc_drawn_objs == 0) {
		if (!excellon_cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "excellon cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}

	pcb_drill_uninit(&pdrills);
	pcb_drill_uninit(&udrills);
}


static int excellon_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, excellon_options, NUM_OPTIONS, excellon_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int excellon_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	int is_drill;

	/* before cam lets this happen... */
	if (PCB_LAYER_IS_ASSY(flags, purpi))
		return 0;

	pcb_cam_set_layer_group(&excellon_cam, group, purpose, purpi, flags, xform);

	if (flags & PCB_LYT_UI)
		return 0;

	is_drill = PCB_LAYER_IS_DRILL(flags, purpi) || ((flags & PCB_LYT_MECH) && PCB_LAYER_IS_ROUTE(flags, purpi));
	if (!is_drill)
		return 0;

	is_plated = PCB_LAYER_IS_PROUTE(flags, purpi) || PCB_LAYER_IS_PDRILL(flags, purpi);
	return 1;
}

static rnd_hid_gc_t excellon_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = calloc(1, sizeof(*rv));
	return rv;
}

static void excellon_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void excellon_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *drw_screen)
{
	switch(op) {
		case RND_HID_COMP_RESET:
		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_FLUSH:
			break;

		case RND_HID_COMP_POSITIVE_XOR:
		case RND_HID_COMP_NEGATIVE:
			if (!warn.comp) {
				warn.comp = 1;
				rnd_message(RND_MSG_ERROR, "Excellon: can not draw composite layers (some features may be missing from the export)\n");
			}
	}
}

static void excellon_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
}

static void excellon_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->style = style;
}

static void excellon_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}

static void excellon_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
}

pcb_drill_ctx_t *get_drill_ctx(void)
{
	if (excellon_cam.active)
		return &pdrills;

	return (is_plated ? &pdrills : &udrills);
}

static void use_gc(rnd_hid_gc_t gc, rnd_coord_t radius)
{
	exc_drawn_objs++;
	if ((gc->style != rnd_cap_round) && (!warn.nonround)) {
		warn.nonround = 1;
		rnd_message(RND_MSG_ERROR, "Excellon: can not set non-round aperture (some features may be missing from the export)\n");
	}

	if (radius == 0)
		radius = gc->width;
	else
		radius *= 2;

	if (radius != lastwidth) {
		aperture_t *aptr = find_aperture(&(get_drill_ctx()->apr), radius, ROUND);
		if (aptr == NULL)
			rnd_fprintf(stderr, "error: aperture for radius %$mS type ROUND is null\n", radius);
		lastwidth = radius;
	}
}


static void excellon_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_coord_t dia = gc->width/2;

	find_aperture(&(get_drill_ctx()->apr), dia*2, ROUND);

	if (!finding_apertures)
		pcb_drill_new_pending(get_drill_ctx(), x1, y1, x2, y2, dia*2);
}

static void excellon_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	excellon_draw_line(gc, x1, y1, x1, y2);
	excellon_draw_line(gc, x1, y1, x2, y1);
	excellon_draw_line(gc, x1, y2, x2, y2);
	excellon_draw_line(gc, x2, y1, x2, y2);
}

static void excellon_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	if (!warn.arc) {
		warn.arc = 1;
		rnd_message(RND_MSG_ERROR, "Excellon: can not export arcs (some features may be missing from the export)\n");
	}
}

static void excellon_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	if (radius <= 0)
		return;

	radius = 50 * rnd_round(radius / 50.0);
	use_gc(gc, radius);
	if (!finding_apertures)
		pcb_drill_new_pending(get_drill_ctx(), cx, cy, cx, cy, radius * 2);
}

static void excellon_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	if (!warn.poly) {
		warn.poly = 1;
		rnd_message(RND_MSG_ERROR, "Excellon: can not export polygons (some features may be missing from the export)\n");
	}
}

static void excellon_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	excellon_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}


static void excellon_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	excellon_fill_polygon(gc, 0, NULL, NULL);
}

static void excellon_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	rnd_message(RND_MSG_ERROR, "Excellon internal error: can not calibrate()\n");
}

static int excellon_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nexcellon exporter command line arguments:\n\n");
	rnd_hid_usage(excellon_options, sizeof(excellon_options) / sizeof(excellon_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x excellon [excellon options] foo.pcb\n\n");
	return 0;
}

static void excellon_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
}

static void exc_session_begin(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	exc_aperture_cnt = 0;
}

int pplg_check_ver_export_excellon(int ver_needed) { return 0; }

void pplg_uninit_export_excellon(void)
{
	rnd_export_remove_opts_by_cookie(excellon_cookie);
	free(filename);
	rnd_conf_unreg_fields("plugins/export_excellon/");
	rnd_event_unbind_allcookie(excellon_cookie);
	rnd_hid_remove_hid(&excellon_hid);
}

int pplg_init_export_excellon(void)
{
	RND_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_excellon, field,isarray,type_name,cpath,cname,desc,flags);
#include "excellon_conf_fields.h"

	memset(&excellon_hid, 0, sizeof(excellon_hid));

	rnd_hid_nogui_init(&excellon_hid);

	excellon_hid.struct_size = sizeof(excellon_hid);
	excellon_hid.name = "excellon";
	excellon_hid.description = "excellon drill/boundary export";
	excellon_hid.exporter = 1;

	excellon_hid.get_export_options = excellon_get_export_options;
	excellon_hid.do_export = excellon_do_export;
	excellon_hid.parse_arguments = excellon_parse_arguments;
	excellon_hid.set_layer_group = excellon_set_layer_group;
	excellon_hid.make_gc = excellon_make_gc;
	excellon_hid.destroy_gc = excellon_destroy_gc;
	excellon_hid.set_drawing_mode = excellon_set_drawing_mode;
	excellon_hid.set_color = excellon_set_color;
	excellon_hid.set_line_cap = excellon_set_line_cap;
	excellon_hid.set_line_width = excellon_set_line_width;
	excellon_hid.set_draw_xor = excellon_set_draw_xor;
	excellon_hid.draw_line = excellon_draw_line;
	excellon_hid.draw_arc = excellon_draw_arc;
	excellon_hid.draw_rect = excellon_draw_rect;
	excellon_hid.fill_circle = excellon_fill_circle;
	excellon_hid.fill_polygon = excellon_fill_polygon;
	excellon_hid.fill_polygon_offs = excellon_fill_polygon_offs;
	excellon_hid.fill_rect = excellon_fill_rect;
	excellon_hid.calibrate = excellon_calibrate;
	excellon_hid.set_crosshair = excellon_set_crosshair;
	excellon_hid.usage = excellon_usage;
	excellon_hid.argument_array = excellon_values;

	rnd_hid_register_hid(&excellon_hid);

	rnd_event_bind(RND_EVENT_EXPORT_SESSION_BEGIN, exc_session_begin, NULL, excellon_cookie);
	return 0;
}

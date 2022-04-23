/*
   This file is part of pcb-rnd and was part of gEDA/PCB but lacked proper
   copyright banner at the fork. It probably has the same copyright as
   gEDA/PCB as a whole in 2011.
*/

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <librnd/core/math_helper.h>
#include "board.h"
#include <librnd/core/color.h>
#include "data.h"
#include "draw.h"
#include "layer.h"
#include "layer_vis.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/safe_fs.h>

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include "ps.h"
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"
#include "funchash_core.h"

#include "draw_eps.h"

static pcb_cam_t eps_cam;

static rnd_hid_t eps_hid;

static rnd_eps_t pctx_, *pctx = &pctx_;
static int print_group[PCB_MAX_LAYERGRP];
static int print_layer[PCB_MAX_LAYER];
static int fast_erase = -1;

static const rnd_export_opt_t eps_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@item --eps-file <string>
Name of the encapsulated postscript output file. Can contain a path.
@end ftable
%end-doc
*/
	{"eps-file", "Encapsulated Postscript output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_psfile 0

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@item --eps-scale <num>
Scale EPS output by the parameter @samp{num}.
@end ftable
%end-doc
*/
	{"eps-scale", "EPS scale",
	 RND_HATT_REAL, 0, 100, {0, 0, 1.0}, 0},
#define HA_scale 1

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@cindex as-shown (EPS)
@item --as-shown
Export layers as shown on screen.
@end ftable
%end-doc
*/
	{"as-shown", "Export layers as shown on screen",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_as_shown 2

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@item --monochrome
Convert output to monochrome.
@end ftable
%end-doc
*/
	{"monochrome", "Convert to monochrome",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_mono 3

/* %start-doc options "92 Encapsulated Postscript Export"
@ftable @code
@cindex only-visible
@item --only-visible
Limit the bounds of the EPS file to the visible items.
@end ftable
%end-doc
*/
	{"only-visible", "Limit the bounds of the EPS file to the visible items",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_only_visible 4

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 5

};

#define NUM_OPTIONS (sizeof(eps_attribute_list)/sizeof(eps_attribute_list[0]))

static rnd_hid_attr_val_t eps_values[NUM_OPTIONS];

static const rnd_export_opt_t *eps_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *val = eps_values[HA_psfile].str;

	if ((PCB != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &eps_values[HA_psfile], ".eps");

	if (n)
		*n = NUM_OPTIONS;
	return eps_attribute_list;
}

static rnd_layergrp_id_t group_for_layer(int l)
{
	if (l < pcb_max_layer(PCB) && l >= 0)
		return pcb_layer_get_group(PCB, l);
	/* else something unique */
	return pcb_max_group(PCB) + 3 + l;
}

static int is_solder(rnd_layergrp_id_t grp)     { return pcb_layergrp_flags(PCB, grp) & PCB_LYT_BOTTOM; }
static int is_component(rnd_layergrp_id_t grp)  { return pcb_layergrp_flags(PCB, grp) & PCB_LYT_TOP; }

static int layer_sort(const void *va, const void *vb)
{
	int a = *(int *) va;
	int b = *(int *) vb;
	rnd_layergrp_id_t al = group_for_layer(a);
	rnd_layergrp_id_t bl = group_for_layer(b);
	int d = bl - al;

	if (a >= 0 && a < pcb_max_layer(PCB)) {
		int aside = (is_solder(al) ? 0 : is_component(al) ? 2 : 1);
		int bside = (is_solder(bl) ? 0 : is_component(bl) ? 2 : 1);
		if (bside != aside)
			return bside - aside;
	}
	if (d)
		return d;
	return b - a;
}

static const char *filename;

static rnd_hid_attr_val_t *options_;

void eps_hid_export_to_file(FILE * the_file, rnd_hid_attr_val_t *options, rnd_xform_t *xform)
{
	int i;
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_box_t tmp, region, *bnds;
	rnd_hid_expose_ctx_t ctx;

	options_ = options;

	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->hidlib.size_x;
	region.Y2 = PCB->hidlib.size_y;

	if (options[HA_only_visible].lng)
		bnds = pcb_data_bbox(&tmp, PCB->Data, rnd_false);
	else
		bnds = &region;

	memset(print_group, 0, sizeof(print_group));
	memset(print_layer, 0, sizeof(print_layer));

	/* Figure out which layers actually have stuff on them.  */
	for (i = 0; i < pcb_max_layer(PCB); i++) {
		pcb_layer_t *layer = PCB->Data->Layer + i;
		if (pcb_layer_flags(PCB, i) & PCB_LYT_SILK)
			continue;
		if (layer->meta.real.vis)
			if (!pcb_layer_is_empty_(PCB, layer))
				print_group[pcb_layer_get_group(PCB, i)] = 1;
	}

	/* Now, if only one layer has real stuff on it, we can use the fast
	   erase logic.  Otherwise, we have to use the expensive multi-mask
	   erase.  */
	fast_erase = 0;
	for (i = 0; i < pcb_max_group(PCB); i++)
		if (print_group[i])
			fast_erase++;

	/* If NO layers had anything on them, at least print the component
	   layer to get the pins.  */
	if (fast_erase == 0) {
		rnd_layergrp_id_t comp_copp;
		if (pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp_copp, 1) > 0) {
			print_group[pcb_layer_get_group(PCB, comp_copp)] = 1;
			fast_erase = 1;
		}
	}

	/* "fast_erase" is 1 if we can just paint white to erase.  */
	fast_erase = fast_erase == 1 ? 1 : 0;

	/* Now, for each group we're printing, mark its layers for
	   printing.  */
	for (i = 0; i < pcb_max_layer(PCB); i++) {
		if (pcb_layer_flags(PCB, i) & PCB_LYT_SILK)
			continue;
		if (print_group[pcb_layer_get_group(PCB, i)])
			print_layer[i] = 1;
	}

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));
	if (options[HA_as_shown].lng)
		pcb_draw_setup_default_gui_xform(xform);
	if (!options[HA_as_shown].lng) {
		qsort(pcb_layer_stack, pcb_max_layer(PCB), sizeof(pcb_layer_stack[0]), layer_sort);
	}

	rnd_eps_init(pctx, the_file, *bnds, options_[HA_scale].dbl, options[HA_mono].lng, options[HA_as_shown].lng);

	if (pctx->outf != NULL)
		rnd_eps_print_header(pctx, rnd_hid_export_fn(filename), pctx->as_shown && conf_core.editor.show_solder_side);

	if (pctx->as_shown) {
		/* disable (exporter default) hiding overlay in as_shown */
		xform->omit_overlay = 0;
		xform->add_gui_xform = 1;
		xform->enable_silk_invis_clr = 1;
	}

	ctx.view = *bnds;
	rnd_app.expose_main(&eps_hid, &ctx, xform);

	rnd_eps_print_footer(pctx);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));
	options_ = NULL;
}

static void eps_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;

	if (!options) {
		eps_get_export_options(hid, 0);
		options = eps_values;
	}

	
	pcb_cam_begin(PCB, &eps_cam, &xform, options[HA_cam].str, eps_attribute_list, NUM_OPTIONS, options);

	filename = options[HA_psfile].str;
	if (!filename)
		filename = "pcb-rnd-out.eps";

	if (eps_cam.fn_template == NULL) {
		pctx->outf = rnd_fopen_askovr(&PCB->hidlib, eps_cam.active ? eps_cam.fn : filename, "w", NULL);
		if (pctx->outf == NULL) {
			perror(filename);
			return;
		}
	}
	else
		pctx->outf = NULL;

	if ((!eps_cam.active) && (!options[HA_as_shown].lng))
		pcb_hid_save_and_show_layer_ons(save_ons);
	eps_hid_export_to_file(pctx->outf, options, &xform);
	if ((!eps_cam.active) && (!options[HA_as_shown].lng))
		pcb_hid_restore_layer_ons(save_ons);

	fclose(pctx->outf);

	if (!eps_cam.active) eps_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&eps_cam) == 0) {
		if (!eps_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "eps cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (pctx->drawn_objs == 0) {
		if (!eps_cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "eps cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}
}

static int eps_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, eps_attribute_list, sizeof(eps_attribute_list) / sizeof(eps_attribute_list[0]), ps_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int is_mask;
static int is_paste;
static int is_drill;

static int eps_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	gds_t tmp_ln;
	const char *name;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&eps_cam, group, purpose, purpi, flags, xform);

	if (eps_cam.fn_changed) {
		if (pctx->outf != NULL) {
			rnd_eps_print_footer(pctx);
			fclose(pctx->outf);
		}
		pctx->outf = rnd_fopen_askovr(&PCB->hidlib, eps_cam.fn, "w", NULL);
		rnd_eps_print_header(pctx, eps_cam.fn, pctx->as_shown && conf_core.editor.show_solder_side);
	}

	if (!eps_cam.active) {
		if (flags & PCB_LYT_NOEXPORT)
			return 0;

		if (PCB_LAYER_IS_ASSY(flags, purpi) || PCB_LAYER_IS_FAB(flags, purpi) || PCB_LAYER_IS_CSECT(flags, purpi) || (flags & PCB_LYT_INVIS))
			return 0;

		if ((group >= 0) && pcb_layergrp_is_empty(PCB, group) && PCB_LAYER_IS_ROUTE(flags, purpi))
			return 0;
	}

	is_drill = PCB_LAYER_IS_DRILL(flags, purpi);
	is_mask = (flags & PCB_LYT_MASK);
	is_paste = !!(flags & PCB_LYT_PASTE);

	if (is_mask || is_paste)
		return 0;

	gds_init(&tmp_ln);
	name = pcb_layer_to_file_name(&tmp_ln, layer, flags, purpose, purpi, PCB_FNS_fixed);

#if 0
	printf("Layer %s group %d drill %d mask %d\n", name, group, is_drill, is_mask);
#endif
	fprintf(pctx->outf, "%% Layer %s group %ld drill %d mask %d\n", name, group, is_drill, is_mask);
	gds_uninit(&tmp_ln);

	if (eps_cam.active) /* CAM has decided already */
		return 1;

	if (pctx->as_shown) {
		if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
			return pcb_silk_on(PCB);
		else
			return 0;
	}
	else {
		if (((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK) && (flags & PCB_LYT_TOP))
			return 1;
		if (((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK) && (flags & PCB_LYT_BOTTOM))
			return 0;
	}

	return 1;
}

static void eps_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	rnd_eps_set_drawing_mode(pctx, hid, op, direct, screen);
}

static void eps_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	rnd_eps_set_color(pctx, gc, color);
}

static void eps_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_eps_draw_rect(pctx, gc, x1, y1, x2, y2);
}

static void eps_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_eps_draw_line(pctx, gc, x1, y1, x2, y2);
}

static void eps_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_eps_draw_arc(pctx, gc, cx, cy, width, height, start_angle, delta_angle);
}

static void eps_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	rnd_eps_fill_circle(pctx, gc, cx, cy, radius);
}

static void eps_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_eps_fill_polygon_offs(pctx, gc, n_coords, x, y, dx, dy);
}

static void eps_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	rnd_eps_fill_polygon_offs(pctx, gc, n_coords, x, y, 0, 0);
}

static void eps_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_eps_fill_rect(pctx, gc, x1, y1, x2, y2);
}

static int eps_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\neps exporter command line arguments:\n\n");
	rnd_hid_usage(eps_attribute_list, sizeof(eps_attribute_list) / sizeof(eps_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x eps [eps options] foo.pcb\n\n");
	return 0;
}

void hid_eps_uninit()
{
	rnd_hid_remove_hid(&eps_hid);
}

void hid_eps_init()
{
	memset(&eps_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&eps_hid);

	eps_hid.struct_size = sizeof(rnd_hid_t);
	eps_hid.name = "eps";
	eps_hid.description = "Encapsulated Postscript";
	eps_hid.exporter = 1;

	eps_hid.get_export_options = eps_get_export_options;
	eps_hid.do_export = eps_do_export;
	eps_hid.parse_arguments = eps_parse_arguments;
	eps_hid.set_layer_group = eps_set_layer_group;
	eps_hid.make_gc = rnd_eps_make_gc;
	eps_hid.destroy_gc = rnd_eps_destroy_gc;
	eps_hid.set_drawing_mode = eps_set_drawing_mode;
	eps_hid.set_color = eps_set_color;
	eps_hid.set_line_cap = rnd_eps_set_line_cap;
	eps_hid.set_line_width = rnd_eps_set_line_width;
	eps_hid.set_draw_xor = rnd_eps_set_draw_xor;
	eps_hid.draw_line = eps_draw_line;
	eps_hid.draw_arc = eps_draw_arc;
	eps_hid.draw_rect = eps_draw_rect;
	eps_hid.fill_circle = eps_fill_circle;
	eps_hid.fill_polygon = eps_fill_polygon;
	eps_hid.fill_polygon_offs = eps_fill_polygon_offs;
	eps_hid.fill_rect = eps_fill_rect;
	eps_hid.set_crosshair = rnd_eps_set_crosshair;
	eps_hid.argument_array = eps_values;

	eps_hid.usage = eps_usage;

	rnd_hid_register_hid(&eps_hid);
	rnd_hid_load_defaults(&eps_hid, eps_attribute_list, NUM_OPTIONS);
}

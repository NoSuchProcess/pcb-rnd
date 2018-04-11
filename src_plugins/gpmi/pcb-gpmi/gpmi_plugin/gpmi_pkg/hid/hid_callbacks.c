#include <stdlib.h>
#include "src/board.h"
#include "src/global_typedefs.h"
#include "src/hid.h"
#include "src/data.h"
#define FROM_PKG
#include "hid.h"
#include "hid_events.h"
#include "hid_callbacks.h"
#include "src/hid_flags.h"
#include "src/hid_init.h"

pcb_hid_attribute_t *gpmi_hid_get_export_options(int *num)
{
	gpmi_hid_t *h;

	h = hid_gpmi_data_get(pcb_exporter);

	if (h == NULL)
		return NULL;

	gpmi_event(h->module, HIDE_get_export_options, h);

	if (num != NULL)
		*num = h->attr_num;
	return h->attr;
}

static char *gcs = "abcdefghijklmnopqrstuvxyz";
pcb_hid_gc_t gpmi_hid_make_gc(void)
{
	pcb_hid_gc_t ret;
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);

	/* TODO: fix gc handling... */
	h->new_gc = (void *)(gcs++);
	gpmi_event(h->module, HIDE_make_gc, h, h->new_gc);
	ret = h->new_gc;
	h->new_gc = NULL;
	return ret;
}

void gpmi_hid_destroy_gc(pcb_hid_gc_t gc)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_destroy_gc, h, gc);
}

void gpmi_hid_do_export(pcb_hid_attr_val_t * options)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	int save_ons[PCB_MAX_LAYER + 2];
	pcb_hid_expose_ctx_t ctx;

	h->result = options;
	gpmi_event(h->module, HIDE_do_export_start, h);

	pcb_hid_save_and_show_layer_ons(save_ons);

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->MaxWidth;
	ctx.view.Y2 = PCB->MaxHeight;

	pcb_hid_expose_all(h->hid, &ctx);
	pcb_hid_restore_layer_ons(save_ons);
	gpmi_event(h->module, HIDE_do_export_finish, h);
	h->result = NULL;
}

int gpmi_hid_parse_arguments(int *pcbargc, char ***pcbargv)
{
	/* Do nothing for now */
	return pcb_hid_parse_command_line(pcbargc, pcbargv);
}

void gpmi_hid_set_crosshair(int x, int y, int cursor_action)
{
	/* Do nothing */
}

int gpmi_hid_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_set_layer_group, h, group, layer, flags, is_empty);
	return 1;
}

void gpmi_hid_set_color(pcb_hid_gc_t gc, const char *name)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_set_color, h, gc, name);
}

void gpmi_hid_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_set_line_cap, h, gc, style);
}

void gpmi_hid_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_set_line_width, h, gc, width);
}

void gpmi_hid_set_draw_xor(pcb_hid_gc_t gc, int xor)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_set_draw_xor, h, gc, xor);
}

void gpmi_hid_set_draw_faded(pcb_hid_gc_t gc, int faded)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_set_draw_faded, h, gc, faded);
}

void gpmi_hid_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_draw_line, h, gc, x1, y1, x2, y2);
}

void gpmi_hid_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_draw_arc, h, gc, cx, cy, xradius, yradius, start_angle, delta_angle);
}

void gpmi_hid_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_draw_rect, h, gc, x1, y1, x2, y2);
}

void gpmi_hid_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_fill_circle, h, gc, cx, cy, radius);
}

void gpmi_hid_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	/* TODO: need accessor for these */
	gpmi_event(h->module, HIDE_fill_polygon, h, gc, x, y);
}

void gpmi_hid_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	/* TODO: need accessor for these */
	gpmi_event(h->module, HIDE_fill_polygon_offs, h, gc, x, y, dx, dy);
}

void gpmi_hid_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t *poly, const pcb_box_t *clip_box)
{
	/* TODO */
}

void gpmi_hid_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gpmi_hid_t *h = hid_gpmi_data_get(pcb_exporter);
	gpmi_event(h->module, HIDE_fill_rect, h, gc, x1, y1, x2, y2);
}


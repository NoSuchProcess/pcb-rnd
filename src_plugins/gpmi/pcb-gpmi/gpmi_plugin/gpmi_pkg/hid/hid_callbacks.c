#include <stdlib.h>
#include "src/global.h"
#include "src/hid.h"
#include "src/data.h"
#define FROM_PKG
#include "hid.h"
#include "hid_events.h"
#include "hid_callbacks.h"
#include "src/hid_flags.h"
#include "src/hid_init.h"


/* TODO */
#define MAX_LAYER 16

HID_Attribute *gpmi_hid_get_export_options(int *num)
{
	hid_t *h;

	h = hid_gpmi_data_get(exporter);

	if (h == NULL)
		return NULL;

	gpmi_event(h->module, HIDE_get_export_options, h);

	if (num != NULL)
		*num = h->attr_num;
	return h->attr;
}

static char *gcs = "abcdefghijklmnopqrstuvxyz";
hidGC gpmi_hid_make_gc(void)
{
	hidGC ret;
	hid_t *h = hid_gpmi_data_get(exporter);

	/* TODO: fix gc handling... */
	h->new_gc = (void *)(gcs++);
	gpmi_event(h->module, HIDE_make_gc, h, h->new_gc);
	ret = h->new_gc;
	h->new_gc = NULL;
	return ret;
}

void gpmi_hid_destroy_gc(hidGC gc)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_destroy_gc, h, gc);
}

void gpmi_hid_do_export(HID_Attr_Val * options)
{
	hid_t *h = hid_gpmi_data_get(exporter);
  int save_ons[MAX_LAYER + 2];
  BoxType region;

	h->result = options;
	gpmi_event(h->module, HIDE_do_export_start, h);

	hid_save_and_show_layer_ons(save_ons);

  region.X1 = 0;
  region.Y1 = 0;
  region.X2 = PCB->MaxWidth;
  region.Y2 = PCB->MaxHeight;

	hid_expose_callback(h->hid, &region, 0);
	hid_restore_layer_ons(save_ons);
	gpmi_event(h->module, HIDE_do_export_finish, h);
	h->result = NULL;
}

void gpmi_hid_parse_arguments(int *pcbargc, char ***pcbargv)
{
	/* Do nothing for now */
	hid_parse_command_line(pcbargc, pcbargv);
}

void gpmi_hid_set_crosshair(int x, int y, int cursor_action)
{
	/* Do nothing */
}

int gpmi_hid_set_layer(const char *name, int group, int empty)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_set_layer, h, name, group, empty);
	return 1;
}

void gpmi_hid_set_color(hidGC gc, const char *name)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_set_color, h, gc, name);
}

void gpmi_hid_set_line_cap(hidGC gc, EndCapStyle style)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_set_line_cap, h, gc, style);
}

void gpmi_hid_set_line_width(hidGC gc, Coord width)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_set_line_width, h, gc, width);
}

void gpmi_hid_set_draw_xor(hidGC gc, int xor)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_set_draw_xor, h, gc, xor);
}

void gpmi_hid_set_draw_faded(hidGC gc, int faded)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_set_draw_faded, h, gc, faded);
}

void gpmi_hid_draw_line(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_draw_line, h, gc, x1, y1, x2, y2);
}

void gpmi_hid_draw_arc(hidGC gc, Coord cx, Coord cy, Coord xradius, Coord yradius, Angle start_angle, Angle delta_angle)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_draw_arc, h, gc, cx, cy, xradius, yradius, start_angle, delta_angle);
}

void gpmi_hid_draw_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_draw_rect, h, gc, x1, y1, x2, y2);
}

void gpmi_hid_fill_circle(hidGC gc, Coord cx, Coord cy, Coord radius)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_fill_circle, h, gc, cx, cy, radius);
}

void gpmi_hid_fill_polygon(hidGC gc, int n_coords, Coord *x, Coord *y)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	/* TODO: need accessor for these */
	gpmi_event(h->module, HIDE_fill_polygon, h, gc, x, y);
}

void gpmi_hid_fill_pcb_polygon(hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
	/* TODO */
}

void gpmi_hid_fill_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_fill_rect, h, gc, x1, y1, x2, y2);
}

void gpmi_hid_use_mask(int use_it)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_use_mask, h, use_it);
}

void gpmi_hid_fill_pcb_pv(hidGC fg_gc, hidGC bg_gc, PinType *pad, bool drawHole, bool mask)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_fill_pcb_pv, h, fg_gc, bg_gc, pad, drawHole, mask);
}

void gpmi_hid_fill_pcb_pad(hidGC gc, PadType * pad, bool clear, bool mask)
{
	hid_t *h = hid_gpmi_data_get(exporter);
	gpmi_event(h->module, HIDE_fill_pcb_pad, h, gc, pad, clear, mask);
}

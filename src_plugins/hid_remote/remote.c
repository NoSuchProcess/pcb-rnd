#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "hid.h"
#include "data.h"
#include "draw.h"
#include "layer.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "compat_misc.h"
#include "funchash_core.h"
#include "event.h"

#include "proto.h"

#include "hid_nogui.h"
#include "actions.h"
#include "hid_init.h"

static const char *remote_cookie = "remote HID";

static pcb_hid_t remote_hid;

typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	int nop;
} hid_gc_s;

static pcb_hid_attribute_t *remote_get_export_options(pcb_hid_t *hid, int *n_ret)
{
	if (n_ret != NULL)
		*n_ret = 0;
	return 0;
}

/* ----------------------------------------------------------------------------- */

static void ev_pcb_changed(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
}


/*pcb_action_t remote_action_list[] = {
};

PCB_REGISTER_ACTIONS(remote_action_list, remote_cookie)*/


static void remote_send_all_layers()
{
	pcb_layer_id_t arr[128];
	pcb_layergrp_id_t garr[128];
	int n, used;

	used = pcb_layergrp_list_any(PCB, PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL, garr, sizeof(garr)/sizeof(garr[0]));
	for(n = 0; n < used; n++) {
		pcb_layergrp_id_t gid = garr[n];
		pcb_remote_new_layer_group(pcb_layergrp_name(PCB, gid), gid, pcb_layergrp_flags(PCB, gid));
	}

	used = pcb_layer_list_any(PCB, PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL, arr, sizeof(arr)/sizeof(arr[0]));

TODO("layer: remove this temporary hack for virtual layers")
	for(n = 0; n < used; n++) {
		const char *name;
		pcb_layer_id_t layer_id = arr[n];
		pcb_layergrp_id_t gid = pcb_layer_get_group(PCB, layer_id);
		name = pcb_layer_name(PCB->Data, layer_id);
		if ((gid < 0) && (name != NULL)) {
			pcb_remote_new_layer_group(name, layer_id, pcb_layer_flags(PCB, layer_id));
			pcb_remote_new_layer(name, layer_id, layer_id);
		}
	}


	for(n = 0; n < used; n++) {
		pcb_layer_id_t lid = arr[n];
		pcb_layergrp_id_t gid = pcb_layer_get_group(PCB, lid);
		if (gid >= 0)
			pcb_remote_new_layer(pcb_layer_name(PCB->Data, lid), lid, gid);
	}
}


/* ----------------------------------------------------------------------------- */
static int remote_stay;
static void remote_do_export(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_hid_attr_val_t *options)
{
	pcb_hid_expose_ctx_t ctx;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

TODO(": wait for a connection?")
	remote_proto_send_ver();
	remote_proto_send_unit();
	remote_proto_send_brddim(PCB->hidlib.size_x, PCB->hidlib.size_y);
	remote_send_all_layers();
	if (remote_proto_send_ready() != 0)
		exit(1);

	pcbhl_expose_main(&remote_hid, &ctx, NULL);

/* main loop, parser */
	if (remote_proto_parse_all() != 0)
		exit(1);
}

static void remote_do_exit(pcb_hid_t *hid)
{
	remote_stay = 0;
}

static int remote_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	return pcb_hid_parse_command_line(argc, argv);
}

static void remote_invalidate_lr(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_coord_t l, pcb_coord_t r, pcb_coord_t t, pcb_coord_t b)
{
	proto_send_invalidate(l,r, t, b);
}

static void remote_invalidate_all(pcb_hid_t *hid, pcb_hidlib_t *hidlib)
{
	proto_send_invalidate_all(hidlib);
}

static int remote_set_layer_group(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	if (flags & PCB_LYT_UI) /* do not draw UI layers yet, we didn't create them */
		return 0;
	if (flags & PCB_LYT_NOEXPORT)
		return 0;
	if (PCB_LAYER_IS_CSECT(flags, purpi)) /* do not draw cross-sect, we didn't create them */
		return 0;
	if (group >= 0)
		proto_send_set_layer_group(group, purpose, is_empty);
	else {
TODO("layer: remove this temporary hack for virtual layers")
		proto_send_set_layer_group(layer, purpose, is_empty);
	}

	return 1; /* do draw */
}

typedef struct {
	char color[64];
	pcb_coord_t line_width;
	char cap;
} remote_gc_cache_t;
static hid_gc_s remote_gc[32];
static remote_gc_cache_t gc_cache[32];

static pcb_hid_gc_t remote_make_gc(pcb_hid_t *hid)
{
	int gci = proto_send_make_gc();
	int max = sizeof(remote_gc) / sizeof(remote_gc[0]);
	if (gci >= max) {
		pcb_message(PCB_MSG_ERROR, "remote_make_gc(): GC index too high: %d >= %d\n", gci, max);
		proto_send_del_gc(gci);
		return NULL;
	}
	memset(&gc_cache[gci], 0, sizeof(remote_gc_cache_t));
	return remote_gc+gci;
}

static int gc2idx(pcb_hid_gc_t gc)
{
	int idx = gc - remote_gc;
	int max = sizeof(remote_gc) / sizeof(remote_gc[0]);

	if ((idx < 0) || (idx >= max)) {
		pcb_message(PCB_MSG_ERROR, "GC index too high: %d >= %d\n", idx, max);
		return -1;
	}
	return idx;
}

static void remote_destroy_gc(pcb_hid_t *hid, pcb_hid_gc_t gc)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_del_gc(idx);
}

static const char *drawing_mode_names[] = { "reset", "positive", "negative", "flush"};
static void remote_set_drawing_mode(pcb_hid_t *hid, pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *drw_screen)
{
	if ((op >= 0) && (op < sizeof(drawing_mode_names) / sizeof(drawing_mode_names[0])))
		proto_send_set_drawing_mode(drawing_mode_names[op], direct);
	else
		pcb_message(PCB_MSG_ERROR, "Invalid drawing mode %d\n", op);
}

static void remote_set_color(pcb_hid_gc_t gc, const pcb_color_t *color)
{
	int idx = gc2idx(gc);
	if (idx >= 0) {
		if (strcmp(gc_cache[idx].color, color->str) != 0) {
			proto_send_set_color(idx, color->str);
			strcpy(gc_cache[idx].color, color->str);
		}
	}
}

/* r=round, s=square, b=beveled (octagon) */
static const char *cap_style_names = "rsrb";
static void remote_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	int idx = gc2idx(gc);
	int max = strlen(cap_style_names);


	if (style >= max) {
		pcb_message(PCB_MSG_ERROR, "can't set invalid cap style: %d >= %d\n", style, max);
		return;
	}
	if (idx >= 0) {
		char cs = cap_style_names[style];
		if (cs != gc_cache[idx].cap) {
			proto_send_set_line_cap(idx, cs);
			gc_cache[idx].cap = cs;
		}
	}
}

static void remote_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	int idx = gc2idx(gc);
	if (idx >= 0) {
		if (width != gc_cache[idx].line_width) {
			proto_send_set_line_width(idx, width);
			gc_cache[idx].line_width = width;
		}
	}
}

static void remote_set_draw_xor(pcb_hid_gc_t gc, int xor_set)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_set_draw_xor(idx, xor_set);
}

static void remote_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_line(idx, x1, y1, x2, y2);
}

static void remote_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_arc(idx, cx, cy, width, height, start_angle, delta_angle);
}

static void remote_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_rect(idx, x1, y1, x2, y2, 0);
}

static void remote_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_fill_circle(idx, cx, cy, radius);
}

static void remote_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_poly(idx, n_coords, x, y, 0, 0);
}

static void remote_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_poly(idx, n_coords, x, y, dx, dy);
}

static void remote_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_rect(idx, x1, y1, x2, y2, 1);
}

static void remote_calibrate(pcb_hid_t *hid, double xval, double yval)
{
}

static int remote_shift_is_pressed(void)
{
	return 0;
}

static int remote_control_is_pressed(void)
{
	return 0;
}

static int remote_mod1_is_pressed(void)
{
	return 0;
}

static void remote_get_coords(const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force)
{
}

static void remote_set_crosshair(pcb_coord_t x, pcb_coord_t y, int action)
{
}

static pcb_hidval_t remote_add_timer(void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	pcb_hidval_t rv;
	rv.lval = 0;
	return rv;
}

static void remote_stop_timer(pcb_hidval_t timer)
{
}

pcb_hidval_t
remote_watch_file(int fd, unsigned int condition, pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								 pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

void remote_unwatch_file(pcb_hidval_t data)
{
}

static void *remote_attr_dlg_new(const char *id, pcb_hid_attribute_t *attrs_, int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	return NULL;
}

static int remote_attr_dlg_run(void *hid_ctx)
{
	return 0;
}

static void remote_attr_dlg_free(void *hid_ctx)
{
}

static void remote_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val)
{
}

static void remote_create_menu(const char *menu_path, const pcb_menu_prop_t *props)
{
}

#include "dolists.h"


int pplg_check_ver_hid_remote(int ver_needed) { return 0; }

void pplg_uninit_hid_remote(void)
{
/*	pcb_remove_actions_by_cookie(remote_cookie);*/
	pcb_event_unbind_allcookie(remote_cookie);
}

int pplg_init_hid_remote(void)
{
	PCB_API_CHK_VER;

	memset(&remote_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&remote_hid);

	remote_hid.struct_size = sizeof(pcb_hid_t);
	remote_hid.name = "remote";
	remote_hid.description = "remote-mode GUI for non-interactive use.";
	remote_hid.gui = 1;
	remote_hid.heavy_term_layer_ind = 1;

	remote_hid.get_export_options = remote_get_export_options;
	remote_hid.do_export = remote_do_export;
	remote_hid.do_exit = remote_do_exit;
	remote_hid.parse_arguments = remote_parse_arguments;
	remote_hid.invalidate_lr = remote_invalidate_lr;
	remote_hid.invalidate_all = remote_invalidate_all;
	remote_hid.set_layer_group = remote_set_layer_group;
	remote_hid.make_gc = remote_make_gc;
	remote_hid.destroy_gc = remote_destroy_gc;
	remote_hid.set_drawing_mode = remote_set_drawing_mode;
	remote_hid.set_color = remote_set_color;
	remote_hid.set_line_cap = remote_set_line_cap;
	remote_hid.set_line_width = remote_set_line_width;
	remote_hid.set_draw_xor = remote_set_draw_xor;
	remote_hid.draw_line = remote_draw_line;
	remote_hid.draw_arc = remote_draw_arc;
	remote_hid.draw_rect = remote_draw_rect;
	remote_hid.fill_circle = remote_fill_circle;
	remote_hid.fill_polygon = remote_fill_polygon;
	remote_hid.fill_polygon_offs = remote_fill_polygon_offs;
	remote_hid.fill_rect = remote_fill_rect;
	remote_hid.calibrate = remote_calibrate;
	remote_hid.shift_is_pressed = remote_shift_is_pressed;
	remote_hid.control_is_pressed = remote_control_is_pressed;
	remote_hid.mod1_is_pressed = remote_mod1_is_pressed;
	remote_hid.get_coords = remote_get_coords;
	remote_hid.set_crosshair = remote_set_crosshair;
	remote_hid.add_timer = remote_add_timer;
	remote_hid.stop_timer = remote_stop_timer;
	remote_hid.watch_file = remote_watch_file;
	remote_hid.unwatch_file = remote_unwatch_file;
	remote_hid.attr_dlg_new = remote_attr_dlg_new;
	remote_hid.attr_dlg_run = remote_attr_dlg_run;
	remote_hid.attr_dlg_free = remote_attr_dlg_free;
	remote_hid.attr_dlg_property = remote_attr_dlg_property;
	remote_hid.create_menu = remote_create_menu;

/*	PCB_REGISTER_ACTIONS(remote_action_list, remote_cookie)*/

	pcb_hid_register_hid(&remote_hid);

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, ev_pcb_changed, NULL, remote_cookie);

	return 0;
}

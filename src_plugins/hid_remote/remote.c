#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include <librnd/core/hid.h>
#include "data.h"
#include "draw.h"
#include "layer.h"
#include <librnd/core/pcb-printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include "funchash_core.h"
#include <librnd/core/event.h>

#include "proto.h"

#include <librnd/core/hid_nogui.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_init.h>

static const char *remote_cookie = "remote HID";

static rnd_hid_t remote_hid;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	int nop;
} rnd_hid_gc_s;

static rnd_export_opt_t *remote_get_export_options(rnd_hid_t *hid, int *n_ret)
{
	if (n_ret != NULL)
		*n_ret = 0;
	return 0;
}

/* ----------------------------------------------------------------------------- */

static void ev_pcb_changed(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
}


/*rnd_action_t remote_action_list[] = {
};

RND_REGISTER_ACTIONS(remote_action_list, remote_cookie)*/


static void remote_send_all_layers()
{
	rnd_layer_id_t arr[128];
	rnd_layergrp_id_t garr[128];
	int n, used;

	used = pcb_layergrp_list_any(PCB, PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL, garr, sizeof(garr)/sizeof(garr[0]));
	for(n = 0; n < used; n++) {
		rnd_layergrp_id_t gid = garr[n];
		pcb_remote_new_layer_group(pcb_layergrp_name(PCB, gid), gid, pcb_layergrp_flags(PCB, gid));
	}

	used = pcb_layer_list_any(PCB, PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL, arr, sizeof(arr)/sizeof(arr[0]));

TODO("layer: remove this temporary hack for virtual layers")
	for(n = 0; n < used; n++) {
		const char *name;
		rnd_layer_id_t layer_id = arr[n];
		rnd_layergrp_id_t gid = pcb_layer_get_group(PCB, layer_id);
		name = pcb_layer_name(PCB->Data, layer_id);
		if ((gid < 0) && (name != NULL)) {
			pcb_remote_new_layer_group(name, layer_id, pcb_layer_flags(PCB, layer_id));
			pcb_remote_new_layer(name, layer_id, layer_id);
		}
	}


	for(n = 0; n < used; n++) {
		rnd_layer_id_t lid = arr[n];
		rnd_layergrp_id_t gid = pcb_layer_get_group(PCB, lid);
		if (gid >= 0)
			pcb_remote_new_layer(pcb_layer_name(PCB->Data, lid), lid, gid);
	}
}


/* ----------------------------------------------------------------------------- */
static int remote_stay;
static void remote_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	rnd_hid_expose_ctx_t ctx;

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

static void remote_do_exit(rnd_hid_t *hid)
{
	remote_stay = 0;
}

static int remote_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	return pcb_hid_parse_command_line(argc, argv);
}

static void remote_invalidate_lr(rnd_hid_t *hid, rnd_coord_t l, rnd_coord_t r, rnd_coord_t t, rnd_coord_t b)
{
	proto_send_invalidate(l,r, t, b);
}

static void remote_invalidate_all(rnd_hid_t *hid)
{
	proto_send_invalidate_all();
}

static int remote_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
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
	unsigned long int color_packed;
	rnd_coord_t line_width;
	char cap;
} remote_gc_cache_t;
static rnd_hid_gc_s remote_gc[32];
static remote_gc_cache_t gc_cache[32];

static rnd_hid_gc_t remote_make_gc(rnd_hid_t *hid)
{
	int gci = proto_send_make_gc();
	int max = sizeof(remote_gc) / sizeof(remote_gc[0]);
	if (gci >= max) {
		rnd_message(RND_MSG_ERROR, "remote_make_gc(): GC index too high: %d >= %d\n", gci, max);
		proto_send_del_gc(gci);
		return NULL;
	}
	memset(&gc_cache[gci], 0, sizeof(remote_gc_cache_t));
	return remote_gc+gci;
}

static int gc2idx(rnd_hid_gc_t gc)
{
	int idx = gc - remote_gc;
	int max = sizeof(remote_gc) / sizeof(remote_gc[0]);

	if ((idx < 0) || (idx >= max)) {
		rnd_message(RND_MSG_ERROR, "GC index too high: %d >= %d\n", idx, max);
		return -1;
	}
	return idx;
}

static void remote_destroy_gc(rnd_hid_gc_t gc)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_del_gc(idx);
}

static const char *drawing_mode_names[] = { "reset", "positive", "negative", "flush"};
static void remote_set_drawing_mode(rnd_hid_t *hid, pcb_composite_op_t op, rnd_bool direct, const rnd_rnd_box_t *drw_screen)
{
	if ((op >= 0) && (op < sizeof(drawing_mode_names) / sizeof(drawing_mode_names[0])))
		proto_send_set_drawing_mode(drawing_mode_names[op], direct);
	else
		rnd_message(RND_MSG_ERROR, "Invalid drawing mode %d\n", op);
}

static void remote_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	int idx = gc2idx(gc);
	if (idx >= 0) {
		if (gc_cache[idx].color_packed != color->packed) {
			proto_send_set_color(idx, color->str);
			gc_cache[idx].color_packed = color->packed;
		}
	}
}

/* r=round, s=square, b=beveled (octagon) */
static const char *cap_style_names = "rsrb";
static void remote_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	int idx = gc2idx(gc);
	int max = strlen(cap_style_names);


	if (style >= max) {
		rnd_message(RND_MSG_ERROR, "can't set invalid cap style: %d >= %d\n", style, max);
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

static void remote_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	int idx = gc2idx(gc);
	if (idx >= 0) {
		if (width != gc_cache[idx].line_width) {
			proto_send_set_line_width(idx, width);
			gc_cache[idx].line_width = width;
		}
	}
}

static void remote_set_draw_xor(rnd_hid_gc_t gc, int xor_set)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_set_draw_xor(idx, xor_set);
}

static void remote_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_line(idx, x1, y1, x2, y2);
}

static void remote_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_arc(idx, cx, cy, width, height, start_angle, delta_angle);
}

static void remote_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_rect(idx, x1, y1, x2, y2, 0);
}

static void remote_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_fill_circle(idx, cx, cy, radius);
}

static void remote_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t * x, rnd_coord_t * y)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_poly(idx, n_coords, x, y, 0, 0);
}

static void remote_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_poly(idx, n_coords, x, y, dx, dy);
}

static void remote_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_rect(idx, x1, y1, x2, y2, 1);
}

static void remote_calibrate(rnd_hid_t *hid, double xval, double yval)
{
}

static int remote_shift_is_pressed(rnd_hid_t *hid)
{
	return 0;
}

static int remote_control_is_pressed(rnd_hid_t *hid)
{
	return 0;
}

static int remote_mod1_is_pressed(rnd_hid_t *hid)
{
	return 0;
}

static void remote_get_coords(rnd_hid_t *hid, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force)
{
}

static void remote_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
}

static rnd_hidval_t remote_add_timer(rnd_hid_t *hid, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data)
{
	rnd_hidval_t rv;
	rv.lval = 0;
	return rv;
}

static void remote_stop_timer(rnd_hid_t *hid, rnd_hidval_t timer)
{
}

rnd_hidval_t
remote_watch_file(rnd_hid_t *hid, int fd, unsigned int condition, rnd_bool (*func)(rnd_hidval_t watch, int fd, unsigned int condition, rnd_hidval_t user_data),
								 rnd_hidval_t user_data)
{
	rnd_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

void remote_unwatch_file(rnd_hid_t *hid, rnd_hidval_t data)
{
}

static void *remote_attr_dlg_new(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs_, int n_attrs_, const char *title_, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	return NULL;
}

static int remote_attr_dlg_run(void *hid_ctx)
{
	return 0;
}

static void remote_attr_dlg_raise(void *hid_ctx)
{
}

static void remote_attr_dlg_close(void *hid_ctx)
{
}

static void remote_attr_dlg_free(void *hid_ctx)
{
}

static void remote_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val)
{
}

static void remote_create_menu(rnd_hid_t *hid, const char *menu_path, const pcb_menu_prop_t *props)
{
}

int pplg_check_ver_hid_remote(int ver_needed) { return 0; }

void pplg_uninit_hid_remote(void)
{
/*	rnd_remove_actions_by_cookie(remote_cookie);*/
	rnd_event_unbind_allcookie(remote_cookie);
}

int pplg_init_hid_remote(void)
{
	PCB_API_CHK_VER;

	memset(&remote_hid, 0, sizeof(rnd_hid_t));

	pcb_hid_nogui_init(&remote_hid);

	remote_hid.struct_size = sizeof(rnd_hid_t);
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
	remote_hid.attr_dlg_raise = remote_attr_dlg_raise;
	remote_hid.attr_dlg_close = remote_attr_dlg_close;
	remote_hid.attr_dlg_free = remote_attr_dlg_free;
	remote_hid.attr_dlg_property = remote_attr_dlg_property;
	remote_hid.create_menu = remote_create_menu;

/*	RND_REGISTER_ACTIONS(remote_action_list, remote_cookie)*/

	pcb_hid_register_hid(&remote_hid);

	rnd_event_bind(RND_EVENT_BOARD_CHANGED, ev_pcb_changed, NULL, remote_cookie);

	return 0;
}

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "hid.h"
#include "data.h"
#include "layer.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "compat_misc.h"

#include "proto.h"

#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "hid_actions.h"
#include "hid_init.h"

static const char *remote_cookie = "remote HID";

static pcb_hid_t remote_hid;

typedef struct hid_gc_s {
	int nop;
} hid_gc_s;

static pcb_hid_attribute_t *remote_get_export_options(int *n_ret)
{
	return 0;
}

/* ----------------------------------------------------------------------------- */

static int nop(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

static int PCBChanged(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

static int help(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_print_actions();
	return 0;
}

static int info(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

pcb_hid_action_t remote_action_list[] = {
	{"PCBChanged", 0, PCBChanged}
	,
	{"RouteStylesChanged", 0, nop}
	,
	{"NetlistChanged", 0, nop}
	,
	{"LayersChanged", 0, nop}
	,
	{"LibraryChanged", 0, nop}
	,
	{"Busy", 0, nop}
	,
	{"Help", 0, help}
	,
	{"Info", 0, info}
	,
	{"PointCursor", 0, info}
};

PCB_REGISTER_ACTIONS(remote_action_list, remote_cookie)

/* ----------------------------------------------------------------------------- */
static int remote_stay;
static void remote_do_export(pcb_hid_attr_val_t * options)
{
	pcb_box_t region;
	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->MaxWidth;
	region.Y2 = PCB->MaxHeight;

#warning TODO: wait for a connection?
	remote_proto_send_ver();
	remote_proto_send_unit();
	if (remote_proto_send_ready() != 0)
		exit(1);

	pcb_hid_expose_callback(&remote_hid, &region, 0);

/* main loop, parser */
	if (remote_proto_parse_all() != 0)
		exit(1);
}

static void remote_do_exit(pcb_hid_t *hid)
{
	remote_stay = 0;
}

static void remote_parse_arguments(int *argc, char ***argv)
{
	hid_parse_command_line(argc, argv);
}

static void remote_invalidate_lr(int l, int r, int t, int b)
{
	proto_send_invalidate(l,r, t, b);
}

static void remote_invalidate_all(void)
{
	proto_send_invalidate_all();
}

static int remote_set_layer(const char *name, int idx, int empty)
{
	proto_send_set_layer(name, idx, empty);
	return 0;
}

typedef struct {
	char color[64];
	pcb_coord_t line_width;
	char cap;
} remote_gc_cache_t;
static hid_gc_s remote_gc[32];
static remote_gc_cache_t gc_cache[32];

static pcb_hid_gc_t remote_make_gc(void)
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

static void remote_destroy_gc(pcb_hid_gc_t gc)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_del_gc(idx);
}

static const char *mask_names[] = { "off", "before", "clear", "after" };
static void remote_use_mask(int mask)
{
	if ((mask >= 0) && (mask < sizeof(mask_names) / sizeof(mask_names[0])))
		proto_send_use_mask(mask_names[mask]);
	else
		pcb_message(PCB_MSG_ERROR, "Invalid use_mask %d\n", mask);
}

static void remote_set_color(pcb_hid_gc_t gc, const char *name)
{
	int idx = gc2idx(gc);
	if (idx >= 0) {
		if (strcmp(gc_cache[idx].color, name) != 0) {
			proto_send_set_color(idx, name);
			strcpy(gc_cache[idx].color, name);
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

static void remote_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t end_angle)
{
#warning TODO
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
		proto_send_draw_poly(idx, n_coords, x, y);
}

static void remote_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int idx = gc2idx(gc);
	if (idx >= 0)
		proto_send_draw_rect(idx, x1, y1, x2, y2, 1);
}

static void remote_calibrate(double xval, double yval)
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

static void remote_get_coords(const char *msg, pcb_coord_t * x, pcb_coord_t * y)
{
}

static void remote_set_crosshair(int x, int y, int action)
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
remote_watch_file(int fd, unsigned int condition, void (*func) (pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								 pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

void remote_unwatch_file(pcb_hidval_t data)
{
}

static pcb_hidval_t remote_add_block_hook(void (*func) (pcb_hidval_t data), pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

static void remote_stop_block_hook(pcb_hidval_t mlpoll)
{
}

static int
remote_attribute_dialog(pcb_hid_attribute_t * attrs_, int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, const char *descr_)
{
	return 0;
}

static void remote_show_item(void *item)
{
}

static void remote_create_menu(const char *menu, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie)
{
}

static int remote_propedit_start(void *pe, int num_props, const char *(*query)(void *pe, const char *cmd, const char *key, const char *val, int idx))
{
	return 0;
}

static void remote_propedit_end(void *pe)
{
}

static void *remote_propedit_add_prop(void *pe, const char *propname, int is_mutable, int num_vals)
{
	return NULL;
}

static void remote_propedit_add_value(void *pe, const char *propname, void *propctx, const char *value, int repeat_cnt)
{
}

static void remote_propedit_add_stat(void *pe, const char *propname, void *propctx, const char *most_common, const char *min, const char *max, const char *avg)
{
}


#include "dolists.h"


static void hid_hid_remote_uninit()
{
	pcb_hid_remove_actions_by_cookie(remote_cookie);
}

pcb_uninit_t hid_hid_remote_init()
{
	memset(&remote_hid, 0, sizeof(pcb_hid_t));

	common_nogui_init(&remote_hid);
	pcb_dhlp_draw_helpers_init(&remote_hid);

	remote_hid.struct_size = sizeof(pcb_hid_t);
	remote_hid.name = "remote";
	remote_hid.description = "remote-mode GUI for non-interactive use.";
	remote_hid.gui = 1;

	remote_hid.get_export_options = remote_get_export_options;
	remote_hid.do_export = remote_do_export;
	remote_hid.do_exit = remote_do_exit;
	remote_hid.parse_arguments = remote_parse_arguments;
	remote_hid.invalidate_lr = remote_invalidate_lr;
	remote_hid.invalidate_all = remote_invalidate_all;
	remote_hid.set_layer = remote_set_layer;
	remote_hid.make_gc = remote_make_gc;
	remote_hid.destroy_gc = remote_destroy_gc;
	remote_hid.use_mask = remote_use_mask;
	remote_hid.set_color = remote_set_color;
	remote_hid.set_line_cap = remote_set_line_cap;
	remote_hid.set_line_width = remote_set_line_width;
	remote_hid.set_draw_xor = remote_set_draw_xor;
	remote_hid.draw_line = remote_draw_line;
	remote_hid.draw_arc = remote_draw_arc;
	remote_hid.draw_rect = remote_draw_rect;
	remote_hid.fill_circle = remote_fill_circle;
	remote_hid.fill_polygon = remote_fill_polygon;
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
	remote_hid.add_block_hook = remote_add_block_hook;
	remote_hid.stop_block_hook = remote_stop_block_hook;
	remote_hid.attribute_dialog = remote_attribute_dialog;
	remote_hid.show_item = remote_show_item;
	remote_hid.create_menu = remote_create_menu;

	remote_hid.propedit_start = remote_propedit_start;
	remote_hid.propedit_end = remote_propedit_end;
	remote_hid.propedit_add_prop = remote_propedit_add_prop;
	remote_hid.propedit_add_value = remote_propedit_add_value;
	remote_hid.propedit_add_stat = remote_propedit_add_stat;

	PCB_REGISTER_ACTIONS(remote_action_list, remote_cookie)

	hid_register_hid(&remote_hid);


	return hid_hid_remote_uninit;
}

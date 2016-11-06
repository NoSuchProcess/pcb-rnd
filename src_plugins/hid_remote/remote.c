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

#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "hid_actions.h"
#include "hid_init.h"

static const char *remote_cookie = "remote HID";

typedef struct hid_gc_struct {
	int nop;
} hid_gc_struct;

static HID_Attribute *remote_get_export_options(int *n_ret)
{
	return 0;
}

/* ----------------------------------------------------------------------------- */

static int nop(int argc, const char **argv, Coord x, Coord y)
{
	return 0;
}

static int PCBChanged(int argc, const char **argv, Coord x, Coord y)
{
	return 0;
}

static int help(int argc, const char **argv, Coord x, Coord y)
{
	print_actions();
	return 0;
}

static int info(int argc, const char **argv, Coord x, Coord y)
{
	return 0;
}

HID_Action remote_action_list[] = {
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

REGISTER_ACTIONS(remote_action_list, remote_cookie)

/* ----------------------------------------------------------------------------- */
static int remote_stay;
static void remote_do_export(HID_Attr_Val * options)
{
/* main loop, parser */
	remote_proto_send_ver();
	remote_proto_parse();
}

static void remote_do_exit(HID *hid)
{
	remote_stay = 0;
}

static void remote_parse_arguments(int *argc, char ***argv)
{
	hid_parse_command_line(argc, argv);
}

static void remote_invalidate_lr(int l, int r, int t, int b)
{
}

static void remote_invalidate_all(void)
{
}

static int remote_set_layer(const char *name, int idx, int empty)
{
	return 0;
}

static hidGC remote_make_gc(void)
{
	return 0;
}

static void remote_destroy_gc(hidGC gc)
{
}

static void remote_use_mask(int use_it)
{
}

static void remote_set_color(hidGC gc, const char *name)
{
}

static void remote_set_line_cap(hidGC gc, EndCapStyle style)
{
}

static void remote_set_line_width(hidGC gc, Coord width)
{
}

static void remote_set_draw_xor(hidGC gc, int xor_set)
{
}

static void remote_draw_line(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void remote_draw_arc(hidGC gc, Coord cx, Coord cy, Coord width, Coord height, Angle start_angle, Angle end_angle)
{
}

static void remote_draw_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
}

static void remote_fill_circle(hidGC gc, Coord cx, Coord cy, Coord radius)
{
}

static void remote_fill_polygon(hidGC gc, int n_coords, Coord * x, Coord * y)
{
}

static void remote_fill_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
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

static void remote_get_coords(const char *msg, Coord * x, Coord * y)
{
}

static void remote_set_crosshair(int x, int y, int action)
{
}

static hidval remote_add_timer(void (*func) (hidval user_data), unsigned long milliseconds, hidval user_data)
{
	hidval rv;
	rv.lval = 0;
	return rv;
}

static void remote_stop_timer(hidval timer)
{
}

hidval
remote_watch_file(int fd, unsigned int condition, void (*func) (hidval watch, int fd, unsigned int condition, hidval user_data),
								 hidval user_data)
{
	hidval ret;
	ret.ptr = NULL;
	return ret;
}

void remote_unwatch_file(hidval data)
{
}

static hidval remote_add_block_hook(void (*func) (hidval data), hidval user_data)
{
	hidval ret;
	ret.ptr = NULL;
	return ret;
}

static void remote_stop_block_hook(hidval mlpoll)
{
}

static int
remote_attribute_dialog(HID_Attribute * attrs_, int n_attrs_, HID_Attr_Val * results_, const char *title_, const char *descr_)
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

static HID remote_hid;

static void hid_hid_remote_uninit()
{
	hid_remove_actions_by_cookie(remote_cookie);
}

pcb_uninit_t hid_hid_remote_init()
{
	memset(&remote_hid, 0, sizeof(HID));

	common_nogui_init(&remote_hid);
	common_draw_helpers_init(&remote_hid);

	remote_hid.struct_size = sizeof(HID);
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

	REGISTER_ACTIONS(remote_action_list, remote_cookie)

	hid_register_hid(&remote_hid);
	return hid_hid_remote_uninit;
}

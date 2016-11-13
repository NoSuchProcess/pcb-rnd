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

static const char *batch_cookie = "batch HID";

static void batch_begin(void);
static void batch_end(void);

/* This is a text-line "batch" HID, which exists for scripting and
   non-GUI needs.  */

typedef struct hid_gc_s {
	int nothing_interesting_here;
} hid_gc_s;

static pcb_hid_attribute_t *batch_get_export_options(int *n_ret)
{
	return 0;
}

/* ----------------------------------------------------------------------------- */

static char *prompt = NULL;

static void hid_batch_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(batch_cookie);
	if (prompt != NULL)
		free(prompt);
}

static int nop(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

static int PCBChanged(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (prompt != NULL)
		free(prompt);
	if (PCB && PCB->Filename) {
		prompt = strrchr(PCB->Filename, '/');
		if (prompt)
			prompt++;
		else
			prompt = PCB->Filename;
		if (prompt != NULL)
			prompt = pcb_strdup(prompt);
	}
	else
		prompt = pcb_strdup("no-board");
	return 0;
}

static int help(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_print_actions();
	return 0;
}

static int info(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int i, j;
	int cg, sg;
	if (!PCB || !PCB->Data || !PCB->Filename) {
		printf("No PCB loaded.\n");
		return 0;
	}
	printf("Filename: %s\n", PCB->Filename);
	pcb_printf("Size: %ml x %ml mils, %mm x %mm mm\n", PCB->MaxWidth, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
	cg = GetLayerGroupNumberByNumber(component_silk_layer);
	sg = GetLayerGroupNumberByNumber(solder_silk_layer);
	for (i = 0; i < MAX_LAYER; i++) {

		int lg = GetLayerGroupNumberByNumber(i);
		for (j = 0; j < MAX_LAYER; j++)
			putchar(j == lg ? '#' : '-');
		printf(" %c %s\n", lg == cg ? 'c' : lg == sg ? 's' : '-', PCB->Data->Layer[i].Name);
	}
	return 0;
}

pcb_hid_action_t batch_action_list[] = {
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

PCB_REGISTER_ACTIONS(batch_action_list, batch_cookie)

extern int isatty();

/* ----------------------------------------------------------------------------- */
static int batch_stay;
static void batch_do_export(pcb_hid_attr_val_t * options)
{
	int interactive;
	char line[1000];

	batch_begin();

	if (isatty(0))
		interactive = 1;
	else
		interactive = 0;

	if (interactive) {
		printf("Entering %s version %s batch mode.\n", PACKAGE, VERSION);
		printf("See http://repo.hu/projects/pcb-rnd for project information\n");
	}

	batch_stay = 1;
	while (batch_stay) {
		if (interactive) {
			printf("%s> ", prompt);
			fflush(stdout);
		}
		if (fgets(line, sizeof(line) - 1, stdin) == NULL) {
			hid_batch_uninit();
			return;
		}
		pcb_hid_parse_command(line);
	}
	batch_end();
}

static void batch_do_exit(pcb_hid_t *hid)
{
	batch_stay = 0;
}

static void batch_parse_arguments(int *argc, char ***argv)
{
	hid_parse_command_line(argc, argv);
}

static void batch_invalidate_lr(int l, int r, int t, int b)
{
}

static void batch_invalidate_all(void)
{
}

static int batch_set_layer(const char *name, int idx, int empty)
{
	return 0;
}

static pcb_hid_gc_t batch_make_gc(void)
{
	return 0;
}

static void batch_destroy_gc(pcb_hid_gc_t gc)
{
}

static void batch_use_mask(int use_it)
{
}

static void batch_set_color(pcb_hid_gc_t gc, const char *name)
{
}

static void batch_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
}

static void batch_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
}

static void batch_set_draw_xor(pcb_hid_gc_t gc, int xor_set)
{
}

static void batch_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void batch_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t end_angle)
{
}

static void batch_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void batch_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
}

static void batch_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
}

static void batch_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void batch_calibrate(double xval, double yval)
{
}

static int batch_shift_is_pressed(void)
{
	return 0;
}

static int batch_control_is_pressed(void)
{
	return 0;
}

static int batch_mod1_is_pressed(void)
{
	return 0;
}

static void batch_get_coords(const char *msg, pcb_coord_t * x, pcb_coord_t * y)
{
}

static void batch_set_crosshair(int x, int y, int action)
{
}

static pcb_hidval_t batch_add_timer(void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	pcb_hidval_t rv;
	rv.lval = 0;
	return rv;
}

static void batch_stop_timer(pcb_hidval_t timer)
{
}

pcb_hidval_t
batch_watch_file(int fd, unsigned int condition, void (*func) (pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								 pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

void batch_unwatch_file(pcb_hidval_t data)
{
}

static pcb_hidval_t batch_add_block_hook(void (*func) (pcb_hidval_t data), pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

static void batch_stop_block_hook(pcb_hidval_t mlpoll)
{
}

static int
batch_attribute_dialog(pcb_hid_attribute_t * attrs_, int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, const char *descr_)
{
	return 0;
}

static void batch_show_item(void *item)
{
}

static void batch_create_menu(const char *menu, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie)
{
}

static int batch_propedit_start(void *pe, int num_props, const char *(*query)(void *pe, const char *cmd, const char *key, const char *val, int idx))
{
	printf("propedit start %d\n", num_props);
 	return 0;
}

static void batch_propedit_end(void *pe)
{
	printf("propedit end\n");
}

static void *batch_propedit_add_prop(void *pe, const char *propname, int is_mutable, int num_vals)
{
	printf("  %s (%d) %s\n", propname, num_vals, is_mutable ? "mutable" : "immutable");
	return NULL;
}

static void batch_propedit_add_value(void *pe, const char *propname, void *propctx, const char *value, int repeat_cnt)
{
	printf("    %s (%d)\n", value, repeat_cnt);
}

static void batch_propedit_add_stat(void *pe, const char *propname, void *propctx, const char *most_common, const char *min, const char *max, const char *avg)
{
	printf("    [%s|%s|%s|%s]\n", most_common, min, max, avg);
}


#include "dolists.h"

static pcb_hid_t batch_hid;

pcb_uninit_t hid_hid_batch_init()
{
	memset(&batch_hid, 0, sizeof(pcb_hid_t));

	common_nogui_init(&batch_hid);
	common_draw_helpers_init(&batch_hid);

	batch_hid.struct_size = sizeof(pcb_hid_t);
	batch_hid.name = "batch";
	batch_hid.description = "Batch-mode GUI for non-interactive use.";
	batch_hid.gui = 1;

	batch_hid.get_export_options = batch_get_export_options;
	batch_hid.do_export = batch_do_export;
	batch_hid.do_exit = batch_do_exit;
	batch_hid.parse_arguments = batch_parse_arguments;
	batch_hid.invalidate_lr = batch_invalidate_lr;
	batch_hid.invalidate_all = batch_invalidate_all;
	batch_hid.set_layer = batch_set_layer;
	batch_hid.make_gc = batch_make_gc;
	batch_hid.destroy_gc = batch_destroy_gc;
	batch_hid.use_mask = batch_use_mask;
	batch_hid.set_color = batch_set_color;
	batch_hid.set_line_cap = batch_set_line_cap;
	batch_hid.set_line_width = batch_set_line_width;
	batch_hid.set_draw_xor = batch_set_draw_xor;
	batch_hid.draw_line = batch_draw_line;
	batch_hid.draw_arc = batch_draw_arc;
	batch_hid.draw_rect = batch_draw_rect;
	batch_hid.fill_circle = batch_fill_circle;
	batch_hid.fill_polygon = batch_fill_polygon;
	batch_hid.fill_rect = batch_fill_rect;
	batch_hid.calibrate = batch_calibrate;
	batch_hid.shift_is_pressed = batch_shift_is_pressed;
	batch_hid.control_is_pressed = batch_control_is_pressed;
	batch_hid.mod1_is_pressed = batch_mod1_is_pressed;
	batch_hid.get_coords = batch_get_coords;
	batch_hid.set_crosshair = batch_set_crosshair;
	batch_hid.add_timer = batch_add_timer;
	batch_hid.stop_timer = batch_stop_timer;
	batch_hid.watch_file = batch_watch_file;
	batch_hid.unwatch_file = batch_unwatch_file;
	batch_hid.add_block_hook = batch_add_block_hook;
	batch_hid.stop_block_hook = batch_stop_block_hook;
	batch_hid.attribute_dialog = batch_attribute_dialog;
	batch_hid.show_item = batch_show_item;
	batch_hid.create_menu = batch_create_menu;

	batch_hid.propedit_start = batch_propedit_start;
	batch_hid.propedit_end = batch_propedit_end;
	batch_hid.propedit_add_prop = batch_propedit_add_prop;
	batch_hid.propedit_add_value = batch_propedit_add_value;
	batch_hid.propedit_add_stat = batch_propedit_add_stat;


	hid_register_hid(&batch_hid);
	return NULL;
}

static void batch_begin(void)
{
	PCB_REGISTER_ACTIONS(batch_action_list, batch_cookie)
}

static void batch_end(void)
{
	pcb_hid_remove_actions_by_cookie(batch_cookie);
}

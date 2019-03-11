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
#include "event.h"
#include "conf_core.h"

#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "actions.h"
#include "hid_init.h"
#include "hid_attrib.h"

static const char *batch_cookie = "batch HID";

static int batch_active = 0;
static void batch_begin(void);
static void batch_end(void);

/* This is a text-line "batch" HID, which exists for scripting and
   non-GUI needs.  */

typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
} hid_gc_s;

static pcb_hid_attribute_t *batch_get_export_options(int *n_ret)
{
	if (n_ret != NULL)
		*n_ret = 0;
	return NULL;
}

static int batch_usage(const char *topic)
{
	fprintf(stderr, "\nbatch GUI command line arguments: none\n\n");
	fprintf(stderr, "\nInvocation: pcb-rnd --gui batch [options]\n");
	return 0;
}


/* ----------------------------------------------------------------------------- */

static char *prompt = NULL;

static void uninit_batch(void)
{
	pcb_remove_actions_by_cookie(batch_cookie);
	pcb_event_unbind_allcookie(batch_cookie);
	if (prompt != NULL) {
		free(prompt);
		prompt = NULL;
	}
}

static void ev_pcb_changed(void *user_data, int argc, pcb_event_arg_t argv[])
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
}

static void ev_log_append(void *user_data, int argc, pcb_event_arg_t argv[])
{
	const pcb_logline_t *line = argv[1].d.p;

	if (!batch_active)
		return;

	if ((line->prev == NULL) || (line->prev->str[line->prev->len-1] == '\n')) {
		switch(line->level) {
			case PCB_MSG_DEBUG:   printf("D: "); break;
			case PCB_MSG_INFO:    printf("I: "); break;
			case PCB_MSG_WARNING: printf("W: "); break;
			case PCB_MSG_ERROR:   printf("E: "); break;
		}
	}
	printf("%s", line->str);
}

static fgw_error_t pcb_act_help(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_print_actions();
	PCB_ACT_IRES(0);
	return 0;
}

static fgw_error_t pcb_act_info(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int i, j;
	if (!PCB || !PCB->Data || !PCB->Filename) {
		printf("No PCB loaded.\n");
		return 0;
	}
	printf("Filename: %s\n", PCB->Filename);
	pcb_printf("Size: %ml x %ml mils, %mm x %mm mm\n", PCB->MaxWidth, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
	for (i = 0; i < PCB_MAX_LAYER; i++) {
		pcb_layergrp_id_t lg = pcb_layer_get_group(PCB, i);
		unsigned int gflg = pcb_layergrp_flags(PCB, lg);
		for (j = 0; j < PCB_MAX_LAYER; j++)
			putchar(j == lg ? '#' : '-');
		printf(" %c %s\n", (gflg & PCB_LYT_TOP) ? 'c' : (gflg & PCB_LYT_BOTTOM) ? 's' : '-', PCB->Data->Layer[i].name);
	}
	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t batch_action_list[] = {
	{"Help", pcb_act_help},
	{"Info", pcb_act_info}
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

	if ((interactive) && (!conf_core.rc.quiet)) {
		printf("Entering %s version %s batch mode.\n", PCB_PACKAGE, PCB_VERSION);
		printf("See http://repo.hu/projects/pcb-rnd for project information\n");
	}

	batch_stay = 1;
	while (batch_stay) {
		if ((interactive) && (!conf_core.rc.quiet)) {
			printf("%s:%s> ", prompt, pcb_cli_prompt(NULL));
			fflush(stdout);
		}
		if (fgets(line, sizeof(line) - 1, stdin) == NULL) {
			uninit_batch();
			goto quit;
		}
		pcb_parse_command(line, pcb_false);
	}

	quit:;
	batch_end();
}

static void batch_do_exit(pcb_hid_t *hid)
{
	batch_stay = 0;
}

static int batch_parse_arguments(int *argc, char ***argv)
{
	return pcb_hid_parse_command_line(argc, argv);
}

static void batch_invalidate_lr(pcb_coord_t l, pcb_coord_t r, pcb_coord_t t, pcb_coord_t b)
{
}

static void batch_invalidate_all(void)
{
}

static int batch_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	return 0;
}

static pcb_hid_gc_t batch_make_gc(void)
{
	static pcb_core_gc_t hc;
	return (pcb_hid_gc_t)&hc;
}

static void batch_destroy_gc(pcb_hid_gc_t gc)
{
}

static void batch_set_color(pcb_hid_gc_t gc, const pcb_color_t *name)
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

static void batch_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
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

static void batch_get_coords(const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force)
{
}

static void batch_get_view_size(pcb_coord_t *width, pcb_coord_t *height)
{
	*width = PCB->MaxWidth;
	*height = PCB->MaxHeight;
}

static void batch_set_crosshair(pcb_coord_t x, pcb_coord_t y, int action)
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
batch_watch_file(int fd, unsigned int condition, pcb_bool (*func) (pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								 pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

void batch_unwatch_file(pcb_hidval_t data)
{
}

static void batch_create_menu(const char *menu_path, const pcb_menu_prop_t *props)
{
}

#include "dolists.h"

static pcb_hid_t batch_hid;

int pplg_check_ver_hid_batch(int ver_needed) { return 0; }

void pplg_uninit_hid_batch(void)
{
	pcb_event_unbind_allcookie(batch_cookie);
	pcb_hid_remove_attributes_by_cookie(batch_cookie);
}

int pplg_init_hid_batch(void)
{
	PCB_API_CHK_VER;
	memset(&batch_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&batch_hid);
	pcb_dhlp_draw_helpers_init(&batch_hid);

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
	batch_hid.set_layer_group = batch_set_layer_group;
	batch_hid.make_gc = batch_make_gc;
	batch_hid.destroy_gc = batch_destroy_gc;
	batch_hid.set_color = batch_set_color;
	batch_hid.set_line_cap = batch_set_line_cap;
	batch_hid.set_line_width = batch_set_line_width;
	batch_hid.set_draw_xor = batch_set_draw_xor;
	batch_hid.draw_line = batch_draw_line;
	batch_hid.draw_arc = batch_draw_arc;
	batch_hid.draw_rect = batch_draw_rect;
	batch_hid.fill_circle = batch_fill_circle;
	batch_hid.fill_polygon = batch_fill_polygon;
	batch_hid.fill_polygon_offs = batch_fill_polygon_offs;
	batch_hid.fill_rect = batch_fill_rect;
	batch_hid.calibrate = batch_calibrate;
	batch_hid.shift_is_pressed = batch_shift_is_pressed;
	batch_hid.control_is_pressed = batch_control_is_pressed;
	batch_hid.mod1_is_pressed = batch_mod1_is_pressed;
	batch_hid.get_coords = batch_get_coords;
	batch_hid.get_view_size = batch_get_view_size;
	batch_hid.set_crosshair = batch_set_crosshair;
	batch_hid.add_timer = batch_add_timer;
	batch_hid.stop_timer = batch_stop_timer;
	batch_hid.watch_file = batch_watch_file;
	batch_hid.unwatch_file = batch_unwatch_file;
	batch_hid.create_menu = batch_create_menu;
	batch_hid.usage = batch_usage;

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, ev_pcb_changed, NULL, batch_cookie);
	pcb_event_bind(PCB_EVENT_LOG_APPEND, ev_log_append, NULL, batch_cookie);

	pcb_hid_register_hid(&batch_hid);
	return 0;
}


static void batch_begin(void)
{
	PCB_REGISTER_ACTIONS(batch_action_list, batch_cookie)
	batch_active = 1;
}

static void batch_end(void)
{
	pcb_remove_actions_by_cookie(batch_cookie);
	batch_active = 0;
}


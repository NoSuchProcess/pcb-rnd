#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hid.h"
#include "compat_misc.h"
#include "compat_nls.h"

/* This is the "gui" that is installed at startup, and is used when
   there is no other real GUI to use.  For the most part, it just
   stops the application from (1) crashing randomly, and (2) doing
   gui-specific things when it shouldn't.  */

#define CRASH(func) fprintf(stderr, "HID error: pcb called GUI function %s without having a GUI available.\n", func); abort()

typedef struct hid_gc_s {
	int nothing_interesting_here;
} hid_gc_s;

static pcb_hid_attribute_t *nogui_get_export_options(int *n_ret)
{
	CRASH("get_export_options");
	return 0;
}

static void nogui_do_export(pcb_hid_attr_val_t * options)
{
	CRASH("do_export");
}

static void nogui_parse_arguments(int *argc, char ***argv)
{
	CRASH("parse_arguments");
}

static void nogui_invalidate_lr(int l, int r, int t, int b)
{
	CRASH("invalidate_lr");
}

static void nogui_invalidate_all(void)
{
	CRASH("invalidate_all");
}

static int nogui_set_layer(const char *name, int idx, int empty)
{
	CRASH("set_layer");
	return 0;
}

static void nogui_end_layer(void)
{
}

static pcb_hid_gc_t nogui_make_gc(void)
{
	return 0;
}

static void nogui_destroy_gc(pcb_hid_gc_t gc)
{
}

static void nogui_use_mask(int use_it)
{
	CRASH("use_mask");
}

static void nogui_set_color(pcb_hid_gc_t gc, const char *name)
{
	CRASH("set_color");
}

static void nogui_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	CRASH("set_line_cap");
}

static void nogui_set_line_width(pcb_hid_gc_t gc, Coord width)
{
	CRASH("set_line_width");
}

static void nogui_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	CRASH("set_draw_xor");
}

static void nogui_set_draw_faded(pcb_hid_gc_t gc, int faded)
{
}

static void nogui_draw_line(pcb_hid_gc_t gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	CRASH("draw_line");
}

static void nogui_draw_arc(pcb_hid_gc_t gc, Coord cx, Coord cy, Coord width, Coord height, Angle start_angle, Angle end_angle)
{
	CRASH("draw_arc");
}

static void nogui_draw_rect(pcb_hid_gc_t gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	CRASH("draw_rect");
}

static void nogui_fill_circle(pcb_hid_gc_t gc, Coord cx, Coord cy, Coord radius)
{
	CRASH("fill_circle");
}

static void nogui_fill_polygon(pcb_hid_gc_t gc, int n_coords, Coord * x, Coord * y)
{
	CRASH("fill_polygon");
}

static void nogui_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_polygon_t * poly, const pcb_box_t * clip_box)
{
	CRASH("fill_pcb_polygon");
}

static void nogui_fill_pcb_pad(pcb_hid_gc_t gc, pcb_pad_t * pad, pcb_bool clear, pcb_bool mask)
{
	CRASH("fill_pcb_pad");
}

static void nogui_thindraw_pcb_pad(pcb_hid_gc_t gc, pcb_pad_t * pad, pcb_bool clear, pcb_bool mask)
{
	CRASH("thindraw_pcb_pad");
}

static void nogui_fill_pcb_pv(pcb_hid_gc_t fg_gc, pcb_hid_gc_t bg_gc, pcb_pin_t * pad, pcb_bool drawHole, pcb_bool mask)
{
	CRASH("fill_pcb_pv");
}

static void nogui_thindraw_pcb_pv(pcb_hid_gc_t fg_gc, pcb_hid_gc_t bg_gc, pcb_pin_t * pad, pcb_bool drawHole, pcb_bool mask)
{
	CRASH("thindraw_pcb_pv");
}

static void nogui_fill_rect(pcb_hid_gc_t gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	CRASH("fill_rect");
}

static void nogui_calibrate(double xval, double yval)
{
	CRASH("calibrate");
}

static int nogui_shift_is_pressed(void)
{
	/* This is called from FitCrosshairIntoGrid() when the board is loaded.  */
	return 0;
}

static int nogui_control_is_pressed(void)
{
	CRASH("control_is_pressed");
	return 0;
}

static int nogui_mod1_is_pressed(void)
{
	CRASH("mod1_is_pressed");
	return 0;
}

static void nogui_get_coords(const char *msg, Coord * x, Coord * y)
{
	CRASH("get_coords");
}

static void nogui_set_crosshair(int x, int y, int action)
{
}

static pcb_hidval_t nogui_add_timer(void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	pcb_hidval_t rv;
	CRASH("add_timer");
	rv.lval = 0;
	return rv;
}

static void nogui_stop_timer(pcb_hidval_t timer)
{
	CRASH("stop_timer");
}

static pcb_hidval_t nogui_watch_file(int fd, unsigned int condition, void (*func) (pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data), pcb_hidval_t user_data)
{
	pcb_hidval_t rv;
	CRASH("watch_file");
	rv.lval = 0;
	return rv;
}

static void nogui_unwatch_file(pcb_hidval_t watch)
{
	CRASH("unwatch_file");
}

static pcb_hidval_t nogui_add_block_hook(void (*func) (pcb_hidval_t data), pcb_hidval_t data)
{
	pcb_hidval_t rv;
	CRASH("add_block_hook");
	rv.ptr = NULL;
	return rv;
}

static void nogui_stop_block_hook(pcb_hidval_t block_hook)
{
	CRASH("stop_block_hook");
}

static void nogui_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static void nogui_logv(enum pcb_message_level level, const char *fmt, va_list ap)
{
	switch (level) {
	case PCB_MSG_DEBUG:
		printf("D:");
		break;
	case PCB_MSG_INFO:
		printf("I:");
		break;
	case PCB_MSG_WARNING:
		printf("W:");
		break;
	case PCB_MSG_ERROR:
		printf("E:");
		break;
	}
	vprintf(fmt, ap);
}

/* Return a line of user input text, stripped of any newline characters.
 * Returns NULL if the user simply presses enter, or otherwise gives no input.
 */
#define MAX_LINE_LENGTH 1024
static char *read_stdin_line(void)
{
	static char buf[MAX_LINE_LENGTH];
	char *s;
	int i;

	s = fgets(buf, MAX_LINE_LENGTH, stdin);
	if (s == NULL) {
		printf("\n");
		return NULL;
	}

	/* Strip any trailing newline characters */
	for (i = strlen(s) - 1; i >= 0; i--)
		if (s[i] == '\r' || s[i] == '\n')
			s[i] = '\0';

	if (s[0] == '\0')
		return NULL;

	return pcb_strdup(s);
}

#undef MAX_LINE_LENGTH

static int nogui_confirm_dialog(const char *msg, ...)
{
	char *answer;
	int ret = 0;
	pcb_bool valid_answer = pcb_false;
	va_list args;

	do {
		va_start(args, msg);
		vprintf(msg, args);
		va_end(args);

		printf(" ? 0=cancel 1 = ok : ");

		answer = read_stdin_line();

		if (answer == NULL)
			continue;

		if (answer[0] == '0' && answer[1] == '\0') {
			ret = 0;
			valid_answer = pcb_true;
		}

		if (answer[0] == '1' && answer[1] == '\0') {
			ret = 1;
			valid_answer = pcb_true;
		}

		free(answer);
	}
	while (!valid_answer);
	return ret;
}

static int nogui_close_confirm_dialog()
{
	return nogui_confirm_dialog(_("OK to lose data ?"), NULL);
}

static void nogui_report_dialog(const char *title, const char *msg)
{
	printf("--- %s ---\n%s\n", title, msg);
}

static char *nogui_prompt_for(const char *msg, const char *default_string)
{
	char *answer;

	if (default_string)
		printf("%s [%s] : ", msg, default_string);
	else
		printf("%s : ", msg);

	answer = read_stdin_line();
	if (answer == NULL)
		return pcb_strdup((default_string != NULL) ? default_string : "");
	else
		return pcb_strdup(answer);
}

/* FIXME - this could use some enhancement to actually use the other
   args */
static char *nogui_fileselect(const char *title, const char *descr,
															const char *default_file, const char *default_ext, const char *history_tag, int flags)
{
	char *answer;

	if (default_file)
		printf("%s [%s] : ", title, default_file);
	else
		printf("%s : ", title);

	answer = read_stdin_line();
	if (answer == NULL)
		return (default_file != NULL) ? pcb_strdup(default_file) : NULL;
	else
		return pcb_strdup(answer);
}

static int nogui_attribute_dialog(pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr)
{
	CRASH("attribute_dialog");
}

static void nogui_show_item(void *item)
{
	CRASH("show_item");
}

static void nogui_beep(void)
{
	putchar(7);
	fflush(stdout);
}

static int nogui_progress(int so_far, int total, const char *message)
{
	return 0;
}

static pcb_hid_t *nogui_request_debug_draw(void)
{
	return NULL;
}

static void nogui_flush_debug_draw(void)
{
}

static void nogui_finish_debug_draw(void)
{
}

static void nogui_create_menu(const char *menu, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie)
{
}

void common_nogui_init(pcb_hid_t * hid)
{
	hid->get_export_options = nogui_get_export_options;
	hid->do_export = nogui_do_export;
	hid->parse_arguments = nogui_parse_arguments;
	hid->invalidate_lr = nogui_invalidate_lr;
	hid->invalidate_all = nogui_invalidate_all;
	hid->set_layer = nogui_set_layer;
	hid->end_layer = nogui_end_layer;
	hid->make_gc = nogui_make_gc;
	hid->destroy_gc = nogui_destroy_gc;
	hid->use_mask = nogui_use_mask;
	hid->set_color = nogui_set_color;
	hid->set_line_cap = nogui_set_line_cap;
	hid->set_line_width = nogui_set_line_width;
	hid->set_draw_xor = nogui_set_draw_xor;
	hid->set_draw_faded = nogui_set_draw_faded;
	hid->draw_line = nogui_draw_line;
	hid->draw_arc = nogui_draw_arc;
	hid->draw_rect = nogui_draw_rect;
	hid->fill_circle = nogui_fill_circle;
	hid->fill_polygon = nogui_fill_polygon;
	hid->fill_pcb_polygon = nogui_fill_pcb_polygon;
	hid->fill_pcb_pad = nogui_fill_pcb_pad;
	hid->thindraw_pcb_pad = nogui_thindraw_pcb_pad;
	hid->fill_pcb_pv = nogui_fill_pcb_pv;
	hid->thindraw_pcb_pv = nogui_thindraw_pcb_pv;
	hid->fill_rect = nogui_fill_rect;
	hid->calibrate = nogui_calibrate;
	hid->shift_is_pressed = nogui_shift_is_pressed;
	hid->control_is_pressed = nogui_control_is_pressed;
	hid->mod1_is_pressed = nogui_mod1_is_pressed;
	hid->get_coords = nogui_get_coords;
	hid->set_crosshair = nogui_set_crosshair;
	hid->add_timer = nogui_add_timer;
	hid->stop_timer = nogui_stop_timer;
	hid->watch_file = nogui_watch_file;
	hid->unwatch_file = nogui_unwatch_file;
	hid->add_block_hook = nogui_add_block_hook;
	hid->stop_block_hook = nogui_stop_block_hook;
	hid->log = nogui_log;
	hid->logv = nogui_logv;
	hid->confirm_dialog = nogui_confirm_dialog;
	hid->close_confirm_dialog = nogui_close_confirm_dialog;
	hid->report_dialog = nogui_report_dialog;
	hid->prompt_for = nogui_prompt_for;
	hid->fileselect = nogui_fileselect;
	hid->attribute_dialog = nogui_attribute_dialog;
	hid->show_item = nogui_show_item;
	hid->beep = nogui_beep;
	hid->progress = nogui_progress;
	hid->request_debug_draw = nogui_request_debug_draw;
	hid->flush_debug_draw = nogui_flush_debug_draw;
	hid->finish_debug_draw = nogui_finish_debug_draw;
	hid->create_menu = nogui_create_menu;
}

static pcb_hid_t nogui_hid;

pcb_hid_t *hid_nogui_get_hid(void)
{
	memset(&nogui_hid, 0, sizeof(pcb_hid_t));

	nogui_hid.struct_size = sizeof(pcb_hid_t);
	nogui_hid.name = "nogui";
	nogui_hid.description = "Default GUI when no other GUI is present.  " "Does nothing.";

	common_nogui_init(&nogui_hid);

	return &nogui_hid;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hid.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "conf_core.h"

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
	if (n_ret != NULL)
		*n_ret = 0;
	return NULL;
}

static void nogui_do_export(pcb_hid_attr_val_t * options)
{
	CRASH("do_export");
}

static int nogui_parse_arguments(int *argc, char ***argv)
{
	CRASH("parse_arguments");
}

static void nogui_invalidate_lr(pcb_coord_t l, pcb_coord_t r, pcb_coord_t t, pcb_coord_t b)
{
	CRASH("invalidate_lr");
}

static void nogui_invalidate_all(void)
{
	CRASH("invalidate_all");
}

static int nogui_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	CRASH("set_layer_group");
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

static void nogui_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	CRASH("set_drawing_mode");
}

static void nogui_render_burst(pcb_burst_op_t op, const pcb_box_t *screen)
{
	/* the HID may decide to ignore this hook */
}

static void nogui_set_color(pcb_hid_gc_t gc, const char *name)
{
	CRASH("set_color");
}

static void nogui_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	CRASH("set_line_cap");
}

static void nogui_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
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

static void nogui_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	CRASH("draw_line");
}

static void nogui_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t end_angle)
{
	CRASH("draw_arc");
}

static void nogui_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	CRASH("draw_rect");
}

static void nogui_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	CRASH("fill_circle");
}

static void nogui_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	CRASH("fill_polygon");
}

static void nogui_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t * poly, const pcb_box_t * clip_box)
{
	CRASH("fill_pcb_polygon");
}

static void nogui_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	CRASH("fill_rect");
}

static void nogui_calibrate(double xval, double yval)
{
	CRASH("calibrate");
}

static int nogui_shift_is_pressed(void)
{
	/* This is called from pcb_crosshair_grid_fit() when the board is loaded.  */
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

static void nogui_get_coords(const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force)
{
	CRASH("get_coords");
}

static void nogui_set_crosshair(pcb_coord_t x, pcb_coord_t y, int action)
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

static pcb_hidval_t nogui_watch_file(int fd, unsigned int condition, pcb_bool (*func) (pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data), pcb_hidval_t user_data)
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
	pcb_vfprintf(stdout, fmt, ap);
	va_end(ap);
}

static void nogui_logv(enum pcb_message_level level, const char *fmt, va_list ap)
{
	if ((conf_core.rc.quiet) && (level < PCB_MSG_ERROR))
		return;
	if ((!conf_core.rc.verbose) && (level < PCB_MSG_INFO))
		return;

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
	pcb_vfprintf(stdout, fmt, ap);
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

	if (conf_core.rc.quiet)
		return 1;

	do {
		va_start(args, msg);
		pcb_vfprintf(stdout, msg, args);
		va_end(args);

		printf(" ? 0=cancel 1 = ok : ");

		answer = read_stdin_line();

		if ((answer == NULL) || (answer[0] == '1' && answer[1] == '\0')) {
			ret = 1;
			valid_answer = pcb_true;
		}
		else if (answer[0] == '0' && answer[1] == '\0') {
			ret = 0;
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

static char *nogui_prompt_for(const char *msg, const char *default_string)
{
	char *answer;

	if (conf_core.rc.quiet)
		return pcb_strdup("");

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

	if (conf_core.rc.quiet)
		return pcb_strdup("");

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

static void *nogui_attr_dlg_new(pcb_hid_attribute_t *attrs_, int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, const char *descr_, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev))
{
	CRASH("attr_dlg_new");
}

static int nogui_attr_dlg_run(void *hid_ctx)
{
	CRASH("attr_dlg_run");
}

static void nogui_attr_dlg_free(void *hid_ctx)
{
	CRASH("attr_dlg_free");
}

static void nogui_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val)
{
	CRASH("attr_dlg_dlg_property");
}


static int nogui_attribute_dialog(pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr, void *caller_data)
{
	CRASH("attribute_dialog");
}

static void nogui_beep(void)
{
	putchar(7);
	fflush(stdout);
}

static int nogui_progress(int so_far, int total, const char *message)
{
	static int on = 0;
	if (conf_core.rc.quiet)
		return 0;
	if (message == NULL) {
		if ((on) && (conf_core.rc.verbose >= PCB_MSG_INFO))
			fprintf(stderr, "progress: finished\n");
		on = 0;
	}
	else {
		if (conf_core.rc.verbose >= PCB_MSG_INFO)
			fprintf(stderr, "progress: %d/%d %s\n", so_far, total, message);
		on = 1;
	}
	return 0;
}

static void nogui_create_menu(const char *menu_path, const pcb_menu_prop_t *props)
{
}

static int clip_warn(void)
{
	static int warned = 0;
	if (!warned) {
		pcb_message(PCB_MSG_ERROR, "The current GUI HID does not support clipboard.\nClipboard is emulated, not shared withother programs\n");
		warned = 1;
	}
	return 0;
}

static void *clip_data = NULL;
static size_t clip_len;
static pcb_hid_clipfmt_t clip_format;

static int nogui_clip_set(pcb_hid_clipfmt_t format, const void *data, size_t len)
{
	free(clip_data);
	clip_data = malloc(len);
	if (clip_data != NULL) {
		memcpy(clip_data, data, len);
		clip_len = len;
		clip_format = format;
	}
	else
		clip_data = NULL;
	return clip_warn();
}

static int nogui_clip_get(pcb_hid_clipfmt_t *format, const void **data, size_t *len)
{
	if (clip_data == NULL) {
		*data = NULL;
		clip_warn();
		return -1;
	}
	*data = malloc(clip_len);
	if (*data == NULL) {
		*data = NULL;
		return -1;
	}

	memcpy(*data, clip_data, clip_len);
	*format = clip_format;
	*len = clip_len;
	return clip_warn();
}

static int nogui_clip_free(pcb_hid_clipfmt_t format, void *data, size_t len)
{
	free(data);
}


void pcb_hid_nogui_init(pcb_hid_t * hid)
{
	hid->get_export_options = nogui_get_export_options;
	hid->do_export = nogui_do_export;
	hid->parse_arguments = nogui_parse_arguments;
	hid->invalidate_lr = nogui_invalidate_lr;
	hid->invalidate_all = nogui_invalidate_all;
	hid->set_layer_group = nogui_set_layer_group;
	hid->end_layer = nogui_end_layer;
	hid->make_gc = nogui_make_gc;
	hid->destroy_gc = nogui_destroy_gc;
	hid->set_drawing_mode = nogui_set_drawing_mode;
	hid->render_burst = nogui_render_burst;
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
	hid->prompt_for = nogui_prompt_for;
	hid->fileselect = nogui_fileselect;
	hid->attr_dlg_new = nogui_attr_dlg_new;
	hid->attr_dlg_run = nogui_attr_dlg_run;
	hid->attr_dlg_free = nogui_attr_dlg_free;
	hid->attr_dlg_property = nogui_attr_dlg_property;
	hid->attribute_dialog = nogui_attribute_dialog;
	hid->beep = nogui_beep;
	hid->progress = nogui_progress;
	hid->create_menu = nogui_create_menu;
	hid->clip_set = nogui_clip_set;
	hid->clip_get = nogui_clip_get;
	hid->clip_free = nogui_clip_free;
}

static pcb_hid_t nogui_hid;

pcb_hid_t *pcb_hid_nogui_get_hid(void)
{
	memset(&nogui_hid, 0, sizeof(pcb_hid_t));

	nogui_hid.struct_size = sizeof(pcb_hid_t);
	nogui_hid.name = "nogui";
	nogui_hid.description = "Default GUI when no other GUI is present.  " "Does nothing.";

	pcb_hid_nogui_init(&nogui_hid);

	return &nogui_hid;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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

#include <librnd/config.h>

#include <librnd/core/actions.h>
#include <librnd/core/event.h>
#include <librnd/core/hid.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_nogui.h>

static int hid_dlg_gui_inited = 0;

/* Action and wrapper implementation for dialogs. If GUI is available, the
   gui_ prefixed action is executed, else the cli_ prefixed one is used. If
   nothing is available, the effect is equivalent to cancel. */

/* Call the gui_ or the cli_ action; act_name must be all lowercase! */
static fgw_error_t call_dialog(const char *act_name, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char tmp[128];

	strcpy(tmp, "gui_");
	strncpy(tmp+4, act_name, sizeof(tmp)-5);
	if (RND_HAVE_GUI_ATTR_DLG && (fgw_func_lookup(&rnd_fgw, tmp) != NULL))
		return rnd_actionv_bin(RND_ACT_HIDLIB, tmp, res, argc, argv);

	tmp[0] = 'c'; tmp[1] = 'l';
	if (fgw_func_lookup(&rnd_fgw, tmp) != NULL)
		return rnd_actionv_bin(RND_ACT_HIDLIB, tmp, res, argc, argv);

	return FGW_ERR_NOT_FOUND;
}

static const char pcb_acts_PromptFor[] = "PromptFor([message[,default[,title]]])";
static const char pcb_acth_PromptFor[] = "Prompt for a string. Returns the string (or NULL on cancel)";
/* DOC: promptfor.html */
static fgw_error_t pcb_act_PromptFor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return call_dialog("promptfor", res, argc, argv);
}

char *rnd_hid_prompt_for(rnd_hidlib_t *hl, const char *msg, const char *default_string, const char *title)
{
	fgw_arg_t res, argv[4];

	argv[1].type = FGW_STR; argv[1].val.cstr = msg;
	argv[2].type = FGW_STR; argv[2].val.cstr = default_string;
	argv[3].type = FGW_STR; argv[3].val.cstr = title;

	if (rnd_actionv_bin(hl, "PromptFor", &res, 4, argv) != 0)
		return NULL;

	if (res.type == (FGW_STR | FGW_DYN))
		return res.val.str;

	fgw_arg_free(&rnd_fgw, &res);
	return NULL;
}

static const char pcb_acts_MessageBox[] = "MessageBox(icon, title, label, button_txt, button_retval, ...)";
static const char pcb_acth_MessageBox[] = "Open a modal message dialog box with title and label. If icon string is not empty, display the named icon on the left. Present one or more window close buttons with different text and return value.";
/* DOC: messagebox.html */
static fgw_error_t pcb_act_MessageBox(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return call_dialog("messagebox", res, argc, argv);
}

int rnd_hid_message_box(rnd_hidlib_t *hl, const char *icon, const char *title, const char *label, ...)
{
	fgw_arg_t res, argv[128];
	int argc;
	va_list ap;

	argv[1].type = FGW_STR; argv[1].val.cstr = icon;
	argv[2].type = FGW_STR; argv[2].val.cstr = title;
	argv[3].type = FGW_STR; argv[3].val.cstr = label;
	argc = 4;

	va_start(ap, label);
	for(;argc < 126;) {
		argv[argc].type = FGW_STR;
		argv[argc].val.cstr = va_arg(ap, const char *);
		if (argv[argc].val.cstr == NULL)
			break;
		argc++;
		argv[argc].type = FGW_INT;
		argv[argc].val.nat_int = va_arg(ap, int);
		argc++;
	}
	va_end(ap);

	if (rnd_actionv_bin(hl, "MessageBox", &res, argc, argv) != 0)
		return -1;

	if (fgw_arg_conv(&rnd_fgw, &res, FGW_INT) == 0)
		return res.val.nat_int;

	fgw_arg_free(&rnd_fgw, &res);
	return -1;
}

void rnd_hid_iterate(rnd_hid_t *hid)
{
	if (hid->iterate != NULL)
		hid->iterate(hid);
}

static const char *refresh = "progress refresh";
static const char *cancel  = "progress cancel";
#define REFRESH_RATE 100

static void progress_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	rnd_hid_progress(0, 0, NULL);
}

static void progress_close_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_progress(0, 0, cancel);
}

static void progress_refresh_cb(rnd_hidval_t user_data)
{
	rnd_hid_progress(0, 0, refresh);
}

static int pcb_gui_progress(long so_far, long total, const char *message)
{
	double now;
	static rnd_hidval_t timer;
	static int active = 0, cancelled = 0;
	static int wp, have_timer = 0;
	static rnd_hid_attr_val_t val;
	static double last = 0;
	static int closing = 0;
	static struct {
		RND_DAD_DECL_NOINIT(dlg)
	} ctx;

	if (message == refresh) {
		if (active)
			last = rnd_dtime();
		have_timer = 0;
		refresh_now:;
		if (active) {
			rnd_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, wp, &val);
			if (!have_timer) {
				timer = rnd_gui->add_timer(rnd_gui, progress_refresh_cb, REFRESH_RATE, timer);
				have_timer = 1;
			}
			rnd_hid_iterate(rnd_gui);
		}
		return 0;
	}


	if (message == cancel) {
		cancelled = 1;
		message = NULL;
	}


	/* If we are finished, destroy any dialog */
	if (so_far == 0 && total == 0 && message == NULL) {
		if (active) {
			if (have_timer) {
				rnd_gui->stop_timer(rnd_gui, timer);
				have_timer = 0;
			}
			if (!closing) {
				closing = 1;
				RND_DAD_FREE(ctx.dlg);
			}
			active = 0;
		}
		return 1;
	}

	if (cancelled) {
		cancelled = 0;
		return 1;
	}

	if (!active) {
		RND_DAD_BEGIN_VBOX(ctx.dlg);
			RND_DAD_LABEL(ctx.dlg, message);
			RND_DAD_PROGRESS(ctx.dlg);
				wp = RND_DAD_CURRENT(ctx.dlg);

			/* need to have a manual cancel button as it needs to call the close cb before really closing the window */
			RND_DAD_BEGIN_HBOX(ctx.dlg);
				RND_DAD_BEGIN_HBOX(ctx.dlg);
					RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx.dlg);
				RND_DAD_BUTTON(ctx.dlg, "cancel");
					RND_DAD_CHANGE_CB(ctx.dlg, progress_close_btn_cb);
			RND_DAD_END(ctx.dlg);
		RND_DAD_END(ctx.dlg);

		RND_DAD_NEW("progress", ctx.dlg, "pcb-rnd progress", &ctx, rnd_false, progress_close_cb);
		active = 1;
		cancelled = 0;

		timer = rnd_gui->add_timer(rnd_gui, progress_refresh_cb, REFRESH_RATE, timer);
		have_timer = 1;
		closing = 0;
	}

	val.dbl = (double)so_far / (double)total;

	now = rnd_dtime();
	if (now >= (last + (REFRESH_RATE / 1000.0))) {
		last = now;
		goto refresh_now;
	}
	return 0;
}


int rnd_hid_progress(long so_far, long total, const char *message)
{
	if (rnd_gui == NULL)
		return 0;
	if ((rnd_gui->gui) && (RND_HAVE_GUI_ATTR_DLG) && (hid_dlg_gui_inited || rnd_gui->allow_dad_before_init))
		return pcb_gui_progress(so_far, total, message);

	return rnd_nogui_progress(so_far, total, message);
}

static const char pcb_acts_Print[] = "Print()";
static const char pcb_acth_Print[] = "Present the print export dialog for printing the layout from the GUI.";
/* DOC: print.html */
static fgw_error_t pcb_act_Print(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (RND_HAVE_GUI_ATTR_DLG && (fgw_func_lookup(&rnd_fgw, "printgui") != NULL))
		return rnd_actionv_bin(RND_ACT_HIDLIB, "printgui", res, argc, argv);
	rnd_message(RND_MSG_ERROR, "action Print() is available only under a GUI HID. Please use the lpr exporter instead.\n");
	return FGW_ERR_NOT_FOUND;
}


static rnd_action_t hid_dlg_action_list[] = {
	{"PromptFor", pcb_act_PromptFor, pcb_acth_PromptFor, pcb_acts_PromptFor},
	{"MessageBox", pcb_act_MessageBox, pcb_acth_MessageBox, pcb_acts_MessageBox},
	{"Print", pcb_act_Print, pcb_acth_Print, pcb_acts_Print}
};

static const char *event_dlg_cookie = "hid_dlg";

static void hid_dlg_log_gui_init_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	hid_dlg_gui_inited = 1;
}

void pcb_hid_dlg_uninit(void)
{
	rnd_event_unbind_allcookie(event_dlg_cookie);
}

void pcb_hid_dlg_init(void)
{
	rnd_event_bind(RND_EVENT_GUI_INIT, hid_dlg_log_gui_init_ev, NULL, event_dlg_cookie);
}

void pcb_hid_dlg_init2(void)
{
	RND_REGISTER_ACTIONS(hid_dlg_action_list, NULL);
}


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

#include "config.h"

#include "actions.h"
#include "hid.h"
#include "hid_dad.h"
#include "hid_nogui.h"


/* Action and wrapper implementation for dialogs. If GUI is available, the
   gui_ prefixed action is executed, else the cli_ prefixed one is used. If
   nothing is available, the effect is equivalent to cancel. */

#define HAVE_GUI_ATTR_DLG \
	((pcb_gui != NULL) && (pcb_gui->gui) && (pcb_gui->attr_dlg_new != NULL) && (pcb_gui->attr_dlg_new != pcb_nogui_attr_dlg_new))

/* Call the gui_ or the cli_ action; act_name must be all lowercase! */
static fgw_error_t call_dialog(const char *act_name, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char tmp[128];

	strcpy(tmp, "gui_");
	strncpy(tmp+4, act_name, sizeof(tmp)-5);
	if (HAVE_GUI_ATTR_DLG && (fgw_func_lookup(&pcb_fgw, tmp) != NULL))
		return pcb_actionv_bin(tmp, res, argc, argv);

	tmp[0] = 'c'; tmp[1] = 'l';
	if (fgw_func_lookup(&pcb_fgw, tmp) != NULL)
		return pcb_actionv_bin(tmp, res, argc, argv);

	return FGW_ERR_NOT_FOUND;
}

static const char pcb_acts_PromptFor[] = "PromptFor([message[,default[,title]]])";
static const char pcb_acth_PromptFor[] = "Prompt for a string. Returns the string (or NULL on cancel)";
/* DOC: promptfor.html */
static fgw_error_t pcb_act_PromptFor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return call_dialog("promptfor", res, argc, argv);
}

char *pcb_hid_prompt_for(const char *msg, const char *default_string, const char *title)
{
	fgw_arg_t res, argv[4];

	argv[1].type = FGW_STR; argv[1].val.cstr = msg;
	argv[2].type = FGW_STR; argv[2].val.cstr = default_string;
	argv[3].type = FGW_STR; argv[3].val.cstr = title;

	if (pcb_actionv_bin("PromptFor", &res, 4, argv) != 0)
		return NULL;

	if (res.type == (FGW_STR | FGW_DYN))
		return res.val.str;

	fgw_arg_free(&pcb_fgw, &res);
	return NULL;
}

static const char pcb_acts_MessageBox[] = "MessageBox(icon, title, label, button_txt, button_retval, ...)";
static const char pcb_acth_MessageBox[] = "Open a modal message dialog box with title and label. If icon string is not empty, display the named icon on the left. Present one or more window close buttons with different text and return value.";
/* DOC: messagebox.html */
static fgw_error_t pcb_act_MessageBox(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return call_dialog("messagebox", res, argc, argv);
}

int pcb_hid_message_box(const char *icon, const char *title, const char *label, ...)
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

	if (pcb_actionv_bin("MessageBox", &res, argc, argv) != 0)
		return -1;

	if (fgw_arg_conv(&pcb_fgw, &res, FGW_INT) == 0)
		return res.val.nat_int;

	fgw_arg_free(&pcb_fgw, &res);
	return -1;
}


static const char *refresh = "progress refresh";
#define REFRESH_RATE 100

static void progress_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pcb_hid_progress(0, 0, NULL);
}

static void progress_refresh_cb(pcb_hidval_t user_data)
{
	pcb_hid_progress(0, 0, refresh);
}

int pcb_hid_progress(long so_far, long total, const char *message)
{
	double now;
	static pcb_hidval_t timer;
	static int active = 0;
	static pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {NULL, 0}};
	static int wp;
	static pcb_hid_attr_val_t val;
	static double last = 0;
	static struct {
		PCB_DAD_DECL_NOINIT(dlg)
	} ctx;

	if (message == refresh) {
		if (active)
			last = pcb_dtime();
		refresh_now:;
		if (active) {
			pcb_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, wp, &val);
			timer = pcb_gui->add_timer(progress_refresh_cb, REFRESH_RATE, timer);
		}
		return 0;
	}

	/* If we are finished, destroy any dialog */
	if (so_far == 0 && total == 0 && message == NULL) {
		if (active) {
			pcb_gui->stop_timer(timer);
			PCB_DAD_FREE(ctx.dlg);
			active = 0;
		}
		return 1;
	}

	if (!active) {
		PCB_DAD_BEGIN_VBOX(ctx.dlg);
			PCB_DAD_LABEL(ctx.dlg, message);
			PCB_DAD_PROGRESS(ctx.dlg);
				wp = PCB_DAD_CURRENT(ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
		PCB_DAD_END(ctx.dlg);

		PCB_DAD_NEW(ctx.dlg, "pcb-rnd progress", &ctx, pcb_false, progress_close_cb);
		active = 1;

		timer = pcb_gui->add_timer(progress_refresh_cb, REFRESH_RATE, timer);
	}

	val.real_value = (double)so_far / (double)total;

	now = pcb_dtime();
	if (now >= (last + (REFRESH_RATE / 1000.0))) {
		last = now;
		goto refresh_now;
	}
	return 0;
}


static pcb_action_t hid_dlg_action_list[] = {
	{"PromptFor", pcb_act_PromptFor, pcb_acth_PromptFor, pcb_acts_PromptFor},
	{"MessageBox", pcb_act_MessageBox, pcb_acth_MessageBox, pcb_acts_MessageBox}
};

PCB_REGISTER_ACTIONS(hid_dlg_action_list, NULL)

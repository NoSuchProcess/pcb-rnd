/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"

#include "actions.h"
#include "conf_hid.h"
#include "hid_dad.h"
#include "event.h"
#include "error.h"

#include "dlg_log.h"

static const char *log_cookie = "dlg_log";

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	unsigned long last_added;
	int active;
	int wtxt, wscroll;
	int gui_inited;
} log_ctx_t;

static log_ctx_t log_ctx;

static void log_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	log_ctx_t *ctx = caller_data;
	ctx->active = 0;
}

static void log_append(log_ctx_t *ctx, pcb_hid_attribute_t *atxt, pcb_logline_t *line)
{
	pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;
	const char *prefix = NULL;
	int popup;

	conf_loglevel_props(line->level, &prefix, &popup);

	if (pcb_gui->supports_dad_text_markup) {
		if (prefix != NULL) {
			gds_t tmp;
			gds_init(&tmp);
			gds_enlarge(&tmp, line->len+32);
			tmp.used = 0;
			gds_append_str(&tmp, prefix);
			gds_append_len(&tmp, line->str, line->len);
			if (*prefix == '<') {
				gds_append(&tmp, '<');
				gds_append(&tmp, '/');
				gds_append_str(&tmp, prefix+1);
			}
			txt->hid_set_text(atxt, ctx->dlg_hid_ctx, PCB_HID_TEXT_APPEND | PCB_HID_TEXT_MARKUP, tmp.array);
			gds_uninit(&tmp);
		}
		else
			txt->hid_set_text(atxt, ctx->dlg_hid_ctx, PCB_HID_TEXT_APPEND, line->str);
	}
	else {
		if ((line->prev == NULL) || (line->prev->str[line->prev->len-1] == '\n')) {
			switch(line->level) {
				case PCB_MSG_DEBUG:   prefix = "D: "; break;
				case PCB_MSG_INFO:    prefix = "I: "; break;
				case PCB_MSG_WARNING: prefix = "W: "; break;
				case PCB_MSG_ERROR:   prefix = "E: "; break;
			}
			if (prefix != NULL)
				txt->hid_set_text(atxt, ctx->dlg_hid_ctx, PCB_HID_TEXT_APPEND | PCB_HID_TEXT_MARKUP, prefix);
		}
		txt->hid_set_text(atxt, ctx->dlg_hid_ctx, PCB_HID_TEXT_APPEND | PCB_HID_TEXT_MARKUP, line->str);
	}
	if (popup && (pcb_gui->attr_dlg_raise != NULL))
		pcb_gui->attr_dlg_raise(ctx->dlg_hid_ctx);
	if (line->ID > ctx->last_added)
		ctx->last_added = line->ID;
	line->seen = 1;
}

static void log_import(log_ctx_t *ctx)
{
	pcb_logline_t *n;
	pcb_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];

	for(n = pcb_log_find_min(ctx->last_added); n != NULL; n = n->next)
		log_append(ctx, atxt, n);
}

static void btn_clear_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("log", "clear", NULL);
}

static void btn_export_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("log", "export", NULL);
}

static void maybe_scroll_to_bottom()
{
	pcb_hid_attribute_t *atxt = &log_ctx.dlg[log_ctx.wtxt];
	pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;

	if ((log_ctx.dlg[log_ctx.wscroll].default_val.int_value) && (txt->hid_scroll_to_bottom != NULL))
		txt->hid_scroll_to_bottom(atxt, log_ctx.dlg_hid_ctx);
}

static void log_window_create(void)
{
	log_ctx_t *ctx = &log_ctx;
	pcb_hid_attr_val_t hv;

	if (ctx->active)
		return;

	memset(ctx, 0, sizeof(log_ctx_t));

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_TEXT(ctx->dlg, NULL);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_SCROLL | PCB_HATF_EXPFILL);
			ctx->wtxt = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "clear");
				PCB_DAD_CHANGE_CB(ctx->dlg, btn_clear_cb);
			PCB_DAD_BUTTON(ctx->dlg, "export");
				PCB_DAD_CHANGE_CB(ctx->dlg, btn_export_cb);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
				PCB_DAD_BOOL(ctx->dlg, "");
					ctx->wscroll = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "scroll");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON_CLOSE(ctx->dlg, "close", 0);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	ctx->active = 1;
	ctx->last_added = -1;
	PCB_DAD_DEFSIZE(ctx->dlg, 200, 300);
	PCB_DAD_NEW("log", ctx->dlg, "pcb-rnd message log", ctx, pcb_false, log_close_cb);

	{
		pcb_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
		pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;
		txt->hid_set_readonly(atxt, ctx->dlg_hid_ctx, 1);
	}
	hv.int_value = 1;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wscroll, &hv);
	log_import(ctx);
	maybe_scroll_to_bottom();
}


const char pcb_acts_LogDialog[] = "LogDialog()\n";
const char pcb_acth_LogDialog[] = "Open the log dialog.";
fgw_error_t pcb_act_LogDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	log_window_create();
	PCB_ACT_IRES(0);
	return 0;
}

static void log_append_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_logline_t *line = argv[1].d.p;

	if (log_ctx.active) {
		pcb_hid_attribute_t *atxt = &log_ctx.dlg[log_ctx.wtxt];
		pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;

		log_append(&log_ctx, atxt, line);
		maybe_scroll_to_bottom();
	}
	else if ((PCB_HAVE_GUI_ATTR_DLG) && (log_ctx.gui_inited)) {
		const char *prefix;
		int popup;

		conf_loglevel_props(line->level, &prefix, &popup);
		if (popup)
			log_window_create();
	}
}

static void log_clear_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (log_ctx.active) {
		pcb_hid_attribute_t *atxt = &log_ctx.dlg[log_ctx.wtxt];
		pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;

		txt->hid_set_text(atxt, log_ctx.dlg_hid_ctx, PCB_HID_TEXT_REPLACE, "");
		log_import(&log_ctx);
	}
}

static void log_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_logline_t *n;

	log_ctx.gui_inited = 1;

	/* if there's pending popup-message in the queue, pop up the dialog */
	for(n = pcb_log_first; n != NULL; n = n->next) {
		const char *prefix;
		int popup;

		conf_loglevel_props(n->level, &prefix, &popup);
		if (popup) {
			log_window_create();
			return;
		}
	}
}


void pcb_dlg_log_uninit(void)
{
	pcb_event_unbind_allcookie(log_cookie);
}

void pcb_dlg_log_init(void)
{
	pcb_event_bind(PCB_EVENT_LOG_APPEND, log_append_ev, NULL, log_cookie);
	pcb_event_bind(PCB_EVENT_LOG_CLEAR, log_clear_ev, NULL, log_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, log_gui_init_ev, NULL, log_cookie);
}

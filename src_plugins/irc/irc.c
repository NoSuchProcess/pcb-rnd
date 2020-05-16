/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
     lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include <ctype.h>
#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>

#include "board.h"

#include <libuirc/libuirc.h>

static int pcb_dlg_irc(void);

typedef enum {
	IRC_OFF,
	IRC_LOGIN,
	IRC_ONLINE,
	IRC_DISCONNECTED
} irc_state_t;

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	RND_DAD_DECL_NOINIT(dlg_login)
	int wnick, wserver, wchan,   wtxt, wscroll, winput, wraise;
	irc_state_t state;
	uirc_t irc;
	rnd_hidval_t timer;
	char *server, *chan;
	int port;
} irc_ctx_t;

irc_ctx_t irc_ctx;

static void maybe_scroll_to_bottom()
{
	rnd_hid_attribute_t *atxt = &irc_ctx.dlg[irc_ctx.wtxt];
	rnd_hid_text_t *txt = atxt->wdata;

	if ((irc_ctx.dlg[irc_ctx.wscroll].val.lng) && (txt->hid_scroll_to_bottom != NULL))
		txt->hid_scroll_to_bottom(atxt, irc_ctx.dlg_hid_ctx);
}

static void irc_poll(void)
{
	uirc_event_t ev = uirc_poll(&irc_ctx.irc);
	if (ev & UIRC_CONNECT) {
		char *tmp = rnd_concat("join :", irc_ctx.chan, NULL);
		uirc_raw(&irc_ctx.irc, tmp);
		free(tmp);
	}
}

static void timer_cb(rnd_hidval_t hv)
{
	if (irc_ctx.state != IRC_OFF) {
		irc_poll();
		rnd_gui->add_timer(rnd_gui, timer_cb, 100, hv);
	}
}

static void irc_append(irc_ctx_t *ctx, const char *str, int may_hilight)
{
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;

	txt->hid_set_text(atxt, ctx->dlg_hid_ctx, RND_HID_TEXT_APPEND, str);

	maybe_scroll_to_bottom();
	if (may_hilight && irc_ctx.dlg[irc_ctx.wraise].val.lng) {
		if ((may_hilight == 2) || (strstr(str, irc_ctx.irc.nick) != NULL))
			rnd_gui->attr_dlg_raise(irc_ctx.dlg_hid_ctx);
	}
}

#define irc_printf(may_hiliht, fmt) \
do { \
	char *__tmp__ = rnd_strdup_printf fmt; \
	irc_append(&irc_ctx, __tmp__, may_hiliht); \
	free(__tmp__); \
} while(0)

void on_me_join(uirc_t *ctx, int query, char *chan)
{
	irc_printf(0, ("*** Connected. ***\n"));
	irc_printf(0, ("*** You may ask your question now - then please be patient. ***\n"));
	rnd_gui->attr_dlg_widget_state(irc_ctx.dlg_hid_ctx, irc_ctx.winput, 1);
}


static void on_msg(uirc_t *ctx, char *from, int query, char *to, char *text)
{
	irc_printf(1, ("<%s> %s\n", from, text));
}

static void on_notice(uirc_t *ctx, char *from, int query, char *to, char *text)
{
	irc_printf(1, ("-%s- %s\n", from, text));
}


static void on_topic(uirc_t *ctx, char *from, int query, char *to, char *text)
{
	irc_printf(0, ("*** topic on %s: %s\n", to, text));
}


static void on_kick(uirc_t *ctx, char *nick, int query, char *chan, char *victim, char *reason)
{
	irc_printf(1, ("*** %s kicks %s from %s (%s)\n", nick, victim, chan, reason));
}

static void irc_disconnect(const char *reason)
{
	if (reason != NULL) {
		char *tmp = rnd_concat("quit :", reason, NULL);
		uirc_raw(&irc_ctx.irc, tmp);
		free(tmp);
		irc_poll();
	}

	irc_printf(2, ("*** Disconnected. ***\n"));
	rnd_gui->attr_dlg_widget_state(irc_ctx.dlg_hid_ctx, irc_ctx.winput, 0);
	uirc_disconnect(&irc_ctx.irc);
	irc_ctx.state = IRC_DISCONNECTED;
}

static void irc_connect(int open_dlg)
{
	if (uirc_connect(&irc_ctx.irc, irc_ctx.server, irc_ctx.port, "pcb-rnd irc action") == 0) {
		if (open_dlg)
			pcb_dlg_irc();
		irc_printf(0, ("*** Connecting %s:%d... ***\n", irc_ctx.server, irc_ctx.port));
	}
	else
		irc_printf(2, ("*** ERROR: failed to connect the server at %s:%p. ***\n", irc_ctx.server, irc_ctx.port));
}

static void on_me_part(uirc_t *ctx, int query, char *chan)
{
	irc_disconnect("quit on part");
}

static void irc_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	irc_ctx_t *ctx = caller_data;

	ctx->state = IRC_OFF;
	{
		uirc_raw(&irc_ctx.irc, "quit :closed");
		irc_poll();
	}
	uirc_disconnect(&ctx->irc);

	free(ctx->server); ctx->server = NULL;
	free(ctx->chan); ctx->chan = NULL;

	RND_DAD_FREE(ctx->dlg);
}

static void btn_reconn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	irc_disconnect("reconnect");
	irc_connect(0);
}

static void irc_msg(const char *txt)
{
	uirc_privmsg(&irc_ctx.irc, irc_ctx.chan, txt);
	irc_printf(0, ("<%s> %s\n", irc_ctx.irc.nick, txt));
}

static void enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	irc_msg(irc_ctx.dlg[irc_ctx.winput].val.str);
	RND_DAD_SET_VALUE(irc_ctx.dlg_hid_ctx, irc_ctx.winput, str, "");
}

static void btn_sendver_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	irc_msg("pcb-rnd version: " PCB_VERSION " (" PCB_REVISION ")");
}

static void btn_savelog_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	char *fn = rnd_gui->fileselect(rnd_gui, "Export IRC log", NULL, "pcb-rnd.irc-log.txt", NULL, NULL, "log", RND_HID_FSD_MAY_NOT_EXIST, NULL);
	if (fn != NULL) {
		FILE *f = rnd_fopen(&PCB->hidlib, fn, "w");
		if (f != NULL) {
			rnd_hid_attribute_t *atxt = &irc_ctx.dlg[irc_ctx.wtxt];
			rnd_hid_text_t *txt = atxt->wdata;
			const char *str = txt->hid_get_text(atxt, irc_ctx.dlg_hid_ctx);
			fputs(str, f);
			fclose(f);
		}
		else
			rnd_message(RND_MSG_ERROR, "Can not open '%s' for write\n", fn);
		free(fn);
	}
}

static int pcb_dlg_irc(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	RND_DAD_BEGIN_VBOX(irc_ctx.dlg);
		RND_DAD_COMPFLAG(irc_ctx.dlg, RND_HATF_EXPFILL);

		RND_DAD_TEXT(irc_ctx.dlg, &irc_ctx);
			RND_DAD_COMPFLAG(irc_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			irc_ctx.wtxt = RND_DAD_CURRENT(irc_ctx.dlg);

		RND_DAD_STRING(irc_ctx.dlg);
			RND_DAD_WIDTH_CHR(irc_ctx.dlg, 80);
			irc_ctx.winput = RND_DAD_CURRENT(irc_ctx.dlg);
			RND_DAD_ENTER_CB(irc_ctx.dlg, enter_cb);

		RND_DAD_BEGIN_HBOX(irc_ctx.dlg);
			RND_DAD_BUTTON(irc_ctx.dlg, "Send ver");
				RND_DAD_CHANGE_CB(irc_ctx.dlg, btn_sendver_cb);
			RND_DAD_BUTTON(irc_ctx.dlg, "Save log");
				RND_DAD_CHANGE_CB(irc_ctx.dlg, btn_savelog_cb);
			RND_DAD_BUTTON(irc_ctx.dlg, "Reconnect");
				RND_DAD_CHANGE_CB(irc_ctx.dlg, btn_reconn_cb);

			RND_DAD_BEGIN_HBOX(irc_ctx.dlg);
				RND_DAD_COMPFLAG(irc_ctx.dlg, RND_HATF_TIGHT | RND_HATF_FRAME);
				RND_DAD_BOOL(irc_ctx.dlg, "");
					irc_ctx.wscroll = RND_DAD_CURRENT(irc_ctx.dlg);
					RND_DAD_DEFAULT_NUM(irc_ctx.dlg, 1);
				RND_DAD_LABEL(irc_ctx.dlg, "scroll");
			RND_DAD_END(irc_ctx.dlg);


			RND_DAD_BEGIN_HBOX(irc_ctx.dlg);
				RND_DAD_COMPFLAG(irc_ctx.dlg, RND_HATF_TIGHT | RND_HATF_FRAME);
				RND_DAD_BOOL(irc_ctx.dlg, "");
					irc_ctx.wraise = RND_DAD_CURRENT(irc_ctx.dlg);
					RND_DAD_DEFAULT_NUM(irc_ctx.dlg, 1);
				RND_DAD_LABEL(irc_ctx.dlg, "raise");
			RND_DAD_END(irc_ctx.dlg);

			RND_DAD_BEGIN_VBOX(irc_ctx.dlg);
				RND_DAD_COMPFLAG(irc_ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_END(irc_ctx.dlg);
			RND_DAD_BUTTON_CLOSES(irc_ctx.dlg, clbtn);
		RND_DAD_END(irc_ctx.dlg);
	RND_DAD_END(irc_ctx.dlg);

	RND_DAD_DEFSIZE(irc_ctx.dlg, 400, 500);

	RND_DAD_NEW("irc", irc_ctx.dlg, "pcb-rnd online-help: IRC", &irc_ctx, rnd_false, irc_close_cb);

	{
		rnd_hid_attribute_t *atxt = &irc_ctx.dlg[irc_ctx.wtxt];
		rnd_hid_text_t *txt = atxt->wdata;
		txt->hid_set_readonly(atxt, irc_ctx.dlg_hid_ctx, 1);
	}

	{
		static rnd_hidval_t hv;
		timer_cb(hv);
	}
	rnd_gui->attr_dlg_widget_state(irc_ctx.dlg_hid_ctx, irc_ctx.winput, 0);
	return 0;
}


static void irc_login_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	irc_ctx_t *ctx = caller_data;

	if (ctx->dlg_login_ret_override->valid && (ctx->dlg_login_ret_override->value == IRC_LOGIN)) {
		/* connect */
		ctx->port = 6667;
		ctx->server = rnd_strdup(ctx->dlg_login[ctx->wserver].val.str);
		ctx->chan = rnd_strdup(ctx->dlg_login[ctx->wchan].val.str);
		ctx->state = IRC_ONLINE;
		ctx->irc.nick = rnd_strdup(ctx->dlg_login[ctx->wnick].val.str);
		ctx->irc.on_msg = on_msg;
		ctx->irc.on_notice = on_notice;
		ctx->irc.on_topic = on_topic;
		ctx->irc.on_me_part = on_me_part;
		ctx->irc.on_me_join = on_me_join;
		ctx->irc.on_kick = on_kick;
		irc_connect(1);
	}
	else {
		ctx->state = IRC_OFF;
	}
	RND_DAD_FREE(ctx->dlg_login);
}

static int pcb_dlg_login_irc_login(void)
{
	char *nick, *s;
	rnd_hid_dad_buttons_t clbtn[] = {{"Connect!", IRC_LOGIN}, {"Cancel", 0}, {NULL, 0}};
	if (irc_ctx.state != IRC_OFF)
		return -1; /* do not open another */

	nick = (char *)rnd_get_user_name();
	if (nick != NULL) {
		nick = rnd_strdup(nick);
		for(s = nick; !isalnum(*s); s++) ;
		*s = '\0';
	}
	if ((nick == NULL) || (*nick == '\0') || (strcmp(nick, "Unknown") == 0)) {
		int r = rnd_getpid();
		r ^= rand();
		r ^= time(NULL);
		free(nick);
		nick = rnd_strdup_printf("pcb-rnd-%d", r % 100000);
	}

	RND_DAD_BEGIN_VBOX(irc_ctx.dlg_login);
		RND_DAD_COMPFLAG(irc_ctx.dlg_login, RND_HATF_EXPFILL);
		RND_DAD_LABEL(irc_ctx.dlg_login, "On-line help: IRC login");
		RND_DAD_BEGIN_TABLE(irc_ctx.dlg_login, 2);
			RND_DAD_LABEL(irc_ctx.dlg_login, "nickname:");
			RND_DAD_STRING(irc_ctx.dlg_login);
				irc_ctx.wnick = RND_DAD_CURRENT(irc_ctx.dlg_login);
				RND_DAD_DEFAULT_PTR(irc_ctx.dlg_login, nick);

			RND_DAD_LABEL(irc_ctx.dlg_login, "channel:");
			RND_DAD_STRING(irc_ctx.dlg_login);
				RND_DAD_DEFAULT_PTR(irc_ctx.dlg_login, rnd_strdup("#pcb-rnd"));
				irc_ctx.wchan = RND_DAD_CURRENT(irc_ctx.dlg_login);

			RND_DAD_LABEL(irc_ctx.dlg_login, "server:");
			RND_DAD_STRING(irc_ctx.dlg_login);
				RND_DAD_DEFAULT_PTR(irc_ctx.dlg_login, rnd_strdup("irc.repo.hu"));
				irc_ctx.wserver = RND_DAD_CURRENT(irc_ctx.dlg_login);

		RND_DAD_END(irc_ctx.dlg_login);

		RND_DAD_LABEL(irc_ctx.dlg_login, "The 'Connect!' button will connect you to a public\nIRC server where pcb-rnd users and developers will\nhelp you in a chat.");

		RND_DAD_BUTTON_CLOSES(irc_ctx.dlg_login, clbtn);
	RND_DAD_END(irc_ctx.dlg_login);

	/* set up the context */
	irc_ctx.state = IRC_LOGIN;

	RND_DAD_NEW("irc_login", irc_ctx.dlg_login, "pcb-rnd online-help: IRC", &irc_ctx, rnd_false, irc_login_close_cb);
	return 0;
}


const char pcb_acts_irc[] = "irc()";
const char pcb_acth_irc[] = "non-modal, single-instance, single-server, single-channel irc window for online support";
fgw_error_t pcb_act_irc(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_ACT_IRES(pcb_dlg_login_irc_login());
	return 0;
}


rnd_action_t irc_action_list[] = {
	{"irc", pcb_act_irc, pcb_acth_irc, pcb_acts_irc}
};

static const char *irc_cookie = "irc plugin";

int pplg_check_ver_irc(int ver_needed) { return 0; }

void pplg_uninit_irc(void)
{
	rnd_remove_actions_by_cookie(irc_cookie);
}

int pplg_init_irc(void)
{
	RND_API_CHK_VER;

	RND_REGISTER_ACTIONS(irc_action_list, irc_cookie)

	return 0;
}

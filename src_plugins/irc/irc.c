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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/plugins.h>

#include <libuirc/libuirc.h>

typedef enum {
	IRC_OFF,
	IRC_LOGIN,
	IRC_ONLINE,
	IRC_DISCONNECTED
} irc_state_t;

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	RND_DAD_DECL_NOINIT(dlg_login)
	int wnick, wserver;
	irc_state_t state;
	uirc_t irc;
} irc_ctx_t;

irc_ctx_t irc_ctx;

static void irc_login_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	irc_ctx_t *ctx = caller_data;

	if (ctx->dlg_login_ret_override->valid && (ctx->dlg_login_ret_override->value == IRC_LOGIN)) {
		/* connect */
		int port = 6667;
		const char *server = ctx->dlg_login[ctx->wserver].val.str;
		ctx->state = IRC_ONLINE;
		ctx->irc.nick = rnd_strdup(ctx->dlg_login[ctx->wnick].val.str);
		if (uirc_connect(&ctx->irc, server, port, "pcb-rnd irc action") == 0) {
			printf("conn!\n");
		}
		else
			rnd_message(RND_MSG_ERROR, "IRC: on-line support: failed to connect the server at %s:%p.\n", server, port);
	}
	else {
		ctx->state = IRC_OFF;
	}
	RND_DAD_FREE(ctx->dlg_login);
}

static int pcb_dlg_login_irc_login(void)
{
	char *nick;
	rnd_hid_dad_buttons_t clbtn[] = {{"Connect!", IRC_LOGIN}, {"Cancel", 0}, {NULL, 0}};
	if (irc_ctx.state != IRC_OFF)
		return -1; /* do not open another */

	nick = rnd_get_user_name();
	if ((nick == NULL) || (*nick == '\0') || (strcmp(nick, "Unknown") == 0)) {
		int r = rnd_getpid();
		free(nick);
		r ^= rand();
		r ^= time(NULL);
		nick = rnd_strdup_printf("pcb-rnd-%d", r);
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
				RND_DAD_DEFAULT_PTR(irc_ctx.dlg_login, "#pcb-rnd");

			RND_DAD_LABEL(irc_ctx.dlg_login, "server:");
			RND_DAD_STRING(irc_ctx.dlg_login);
				RND_DAD_DEFAULT_PTR(irc_ctx.dlg_login, "irc.repo.hu");
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

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

extern const char pcb_acts_irc[] = "irc()";
extern const char pcb_acth_irc[] = "non-modal, single-instance, single-server, single-channel irc window for online support";
extern fgw_error_t pcb_act_irc(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_trace("irc: not yet\n");
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

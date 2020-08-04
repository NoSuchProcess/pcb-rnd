/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with external router process
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <gensexpr/gsxl.h>
#include <genht/htpi.h>

#include "board.h"
#include "data.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/safe_fs.h>
#include "conf_core.h"
#include "obj_pstk_inlines.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"
#include "src_plugins/lib_netmap/netmap.h"

static const char *extern_cookie = "extern autorouter plugin";


static const char pcb_acts_extroute[] = "extroute(board|selected, router, [confkey=value, ...])";
static const char pcb_acth_extroute[] = "Executed external autorouter to route the board or parts of the board";
fgw_error_t pcb_act_extroute(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *scope, *router;
	RND_ACT_CONVARG(1, FGW_STR, extroute, scope = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, extroute, scope = argv[2].val.str);

	return 0;
}

static rnd_action_t extern_action_list[] = {
	{"extroute", pcb_act_extroute, pcb_acth_extroute, pcb_acts_extroute}
};

int pplg_check_ver_ar_extern(int ver_needed) { return 0; }

void pplg_uninit_ar_extern(void)
{
	rnd_remove_actions_by_cookie(extern_cookie);
}


int pplg_init_ar_extern(void)
{
	RND_API_CHK_VER;

	RND_REGISTER_ACTIONS(extern_action_list, extern_cookie)

	return 0;
}

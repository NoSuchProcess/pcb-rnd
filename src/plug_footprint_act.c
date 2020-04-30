/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
 *
 *  This module was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
#include <librnd/core/actions.h>
#include "board.h"
#include "plug_footprint.h"

static const char pcb_acts_fp_rehash[] = "fp_rehash()" ;
static const char pcb_acth_fp_rehash[] = "Flush the library index; rescan all library search paths and rebuild the library index. Useful if there are changes in the library during a pcb-rnd session.";
static fgw_error_t pcb_act_fp_rehash(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name = NULL;
	pcb_fplibrary_t *l;

	RND_ACT_MAY_CONVARG(1, FGW_STR, fp_rehash, name = argv[1].val.str);
	RND_ACT_IRES(0);

	if (name == NULL) {
		pcb_fp_rehash(RND_ACT_HIDLIB, NULL);
		return 0;
	}

	l = pcb_fp_lib_search(&pcb_library, name);
	if (l == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't find library path %s\n", name);
		RND_ACT_IRES(1);
		return 0;
	}

	if (l->type != PCB_LIB_DIR) {
		rnd_message(RND_MSG_ERROR, "Library path %s is not a directory\n", name);
		RND_ACT_IRES(1);
		return 0;
	}

	if (l->data.dir.backend == NULL) {
		rnd_message(RND_MSG_ERROR, "Library path %s is not a top level directory of a fp_ plugin\n", name);
		RND_ACT_IRES(1);
		return 0;
	}

	if (pcb_fp_rehash(RND_ACT_HIDLIB, l) != 0) {
		rnd_message(RND_MSG_ERROR, "Failed to rehash %s\n", name);
		RND_ACT_IRES(1);
		return 0;
	}

	return 0;
}


static rnd_action_t plug_footprint_list[] = {
	{"fp_rehash", pcb_act_fp_rehash, pcb_acth_fp_rehash, pcb_acts_fp_rehash}
};

void pcb_plug_footprint_act_init2(void)
{
	RND_REGISTER_ACTIONS(plug_footprint_list, NULL);
}

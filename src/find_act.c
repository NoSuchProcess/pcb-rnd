/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "actions.h"
#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "error.h"
#include "find.h"
#include "compat_misc.h"

static const char pcb_acts_DRC[] = "DRC([list|simple])";
static const char pcb_acth_DRC[] = "Invoke the DRC check. Results are presented as the argument requests.";
/* DOC: drc.html */
static fgw_error_t pcb_act_DRC(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	fgw_arg_t args[2];

	PCB_ACT_MAY_CONVARG(1, FGW_STR, DRC, dlg_type = argv[1].val.str);
	args[1].type = FGW_STR;

	if (pcb_strcasecmp(dlg_type, "list") == 0) {
		args[1].val.str = "list";
		return pcb_actionv_bin("drcdialog", res, 2, args);
	}

	if (pcb_strcasecmp(dlg_type, "simple") == 0) {
		args[1].val.str = "simple";
		return pcb_actionv_bin("drcdialog", res, 2, args);
	}

	PCB_ACT_IRES(-1);
	return 0;
}

pcb_action_t find_action_list[] = {
	{"DRC", pcb_act_DRC, pcb_acth_DRC, pcb_acts_DRC}
};

PCB_REGISTER_ACTIONS(find_action_list, NULL)

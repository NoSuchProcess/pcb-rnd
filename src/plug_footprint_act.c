/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "actions.h"
#include "plug_footprint.h"

static const char pcb_acts_fp_rehash[] = "fp_rehash()" ;
static const char pcb_acth_fp_rehash[] = "Flush the library index; rescan all library search paths and rebuild the library index. Useful if there are changes in the library during a pcb-rnd session.";
static fgw_error_t pcb_act_fp_rehash(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_fplibrary_t *l;

	if (argc == 0) {
		pcb_fp_rehash(NULL);
		return 0;
	}

	l = pcb_fp_lib_search(&pcb_library, argv[0]);
	if (l == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't find library path %s\n", argv[0]);
		return 1;
	}

	if (l->type != LIB_DIR) {
		pcb_message(PCB_MSG_ERROR, "Library path %s is not a directory\n", argv[0]);
		return 1;
	}

	if (l->data.dir.backend == NULL) {
		pcb_message(PCB_MSG_ERROR, "Library path %s is not a top level directory of a fp_ plugin\n", argv[0]);
		return 1;
	}

	if (pcb_fp_rehash(l) != 0) {
		pcb_message(PCB_MSG_ERROR, "Failed to rehash %s\n", argv[0]);
		return 1;
	}
	return 0;
	PCB_OLD_ACT_END;
}


pcb_action_t conf_plug_footprint_list[] = {
	{"fp_rehash", pcb_act_fp_rehash, pcb_acth_fp_rehash, pcb_acts_fp_rehash}
};

PCB_REGISTER_ACTIONS(conf_plug_footprint_list, NULL)

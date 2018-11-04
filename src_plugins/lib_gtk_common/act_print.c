/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include "act_print.h"

#include "board.h"
#include "actions.h"
#include "hid.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "data.h"
#include "compat_nls.h"

#include "dlg_print.h"

const char pcb_gtk_acts_print[] = "Print()";
const char pcb_gtk_acth_print[] = "Print the layout.";
/* DOC: print.html */
fgw_error_t pcb_gtk_act_print(pcb_gtk_common_t *com, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_hid_t **hids;
	int i;
	pcb_hid_t *printer = NULL;

	hids = pcb_hid_enumerate();
	for (i = 0; hids[i]; i++) {
		if (hids[i]->printer)
			printer = hids[i];
	}

	if (printer == NULL) {
		pcb_gui->log("Can't find a suitable printer HID");
		PCB_ACT_IRES(-1);
		return 0;
	}

	/* check if layout is empty */
	if (!pcb_data_is_empty(PCB->Data)) {
		ghid_dialog_print(printer, NULL, com);
	}
	else
		pcb_gui->log("Can't print empty layout");

	PCB_ACT_IRES(0);
	return 0;
}


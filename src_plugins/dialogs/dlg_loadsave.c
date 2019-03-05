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

#include <genvector/gds_char.h>
#include "actions.h"
#include "hid_dad.h"
#include "dlg_loadsave.h"

const char pcb_acts_Load[] = "Load()\n" "Load(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert)";
const char pcb_acth_Load[] = "Load layout data from a user-selected file.";
/* DOC: load.html */
fgw_error_t pcb_act_Load(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return -1;
}

const char pcb_acts_Save[] = "Save()\n" "Save(Layout|LayoutAs)\n" "Save(AllConnections|AllUnusedPins|ElementConnections)\n" "Save(PasteBuffer)";
const char pcb_acth_Save[] = "Save layout data to a user-selected file.";
/* DOC: save.html */
fgw_error_t pcb_act_Save(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return -1;
}

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
 */

#include <librnd/core/hid_init.h>


const char pcb_acts_PrintCalibrate[] = "PrintCalibrate()";
const char pcb_acth_PrintCalibrate[] = "DEPRECATED: Calibrate the printer.";
/* DOC: printcalibrate.html */
fgw_error_t pcb_act_PrintCalibrate(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "This function has been removed. Please refer to http://www.repo.hu/projects/pcb-rnd/help/err0004.html\n");
	RND_ACT_IRES(1);
	return 0;
}

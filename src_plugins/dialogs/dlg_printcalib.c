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

static rnd_hid_attribute_t printer_calibrate_attrs[] = {
	{"Enter Values here:", "",
	 RND_HATT_LABEL, 0, 0, {0, 0, 0}, 0, 0},
	{"x-calibration", "X scale for calibrating your printer",
	 RND_HATT_REAL, 0.5, 25, {0, 0, 1.00}, 0, 0},
	{"y-calibration", "Y scale for calibrating your printer",
	 RND_HATT_REAL, 0.5, 25, {0, 0, 1.00}, 0, 0}
};

const char pcb_acts_PrintCalibrate[] = "PrintCalibrate()";
const char pcb_acth_PrintCalibrate[] = "Calibrate the printer.";
/* DOC: printcalibrate.html */
fgw_error_t pcb_act_PrintCalibrate(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_t *printer = rnd_hid_find_printer();

	if (printer == NULL) {
		rnd_message(RND_MSG_ERROR, "No printer available\n");
		RND_ACT_IRES(1);
		return 0;
	}
	printer->calibrate(printer, 0.0, 0.0);

	if (rnd_attribute_dialog("printer_calibrate", printer_calibrate_attrs, 3, "Printer Calibration Values", NULL))
		return 1;
	printer->calibrate(printer, printer_calibrate_attrs[1].val.dbl, printer_calibrate_attrs[2].val.dbl);
	RND_ACT_IRES(0);
	return 0;
}

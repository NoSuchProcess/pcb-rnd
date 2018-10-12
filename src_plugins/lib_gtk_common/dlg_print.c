/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port. */

#include "config.h"
#include "conf_core.h"
#include "dlg_print.h"
#include <stdlib.h>

#include "pcb-printf.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "dlg_attribute.h"

void ghid_dialog_print(pcb_hid_t * hid, GtkWidget *export_dialog, pcb_gtk_common_t *com)
{
	pcb_hid_attribute_t *attr;
	int n = 0;
	int i;
	pcb_hid_attr_val_t *results = NULL;

	/* signal the initial export select dialog that it should close */
	if (export_dialog)
		gtk_dialog_response(GTK_DIALOG(export_dialog), GTK_RESPONSE_CANCEL);

	pcb_exporter = hid;

	attr = pcb_exporter->get_export_options(&n);
	if (n > 0) {
		results = (pcb_hid_attr_val_t *) malloc(n * sizeof(pcb_hid_attr_val_t));
		if (results == NULL) {
			fprintf(stderr, "ghid_dialog_print() -- malloc failed\n");
			exit(1);
		}

		/* non-zero means cancel was picked */
		if (ghid_attribute_dialog(com, attr, n, results, _("PCB Print Layout"), pcb_exporter->description, NULL))
			return;

	}

	pcb_exporter->do_export(results);

	for (i = 0; i < n; i++) {
		if (results[i].str_value)
			free((void *) results[i].str_value);
	}

	if (results)
		free(results);

	pcb_exporter = NULL;
}


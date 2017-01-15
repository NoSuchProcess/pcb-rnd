/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
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


void ghid_dialog_print(pcb_hid_t * hid, GtkWidget *export_dialog)
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
		if (ghid_attribute_dialog(attr, n, results, _("PCB Print Layout"), pcb_exporter->description))
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


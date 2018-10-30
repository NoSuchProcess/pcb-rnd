/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include <gtk/gtk.h>
#include "glue.h"

/* object property edit dialog */

typedef struct {
	/* property list */
	GtkWidget *tree;							/* property list widget               */
	GtkListStore *props;					/* list model of properties           */

	/* value entry */
	GtkWidget *entry_val;					/* text entry                         */
	GtkWidget *val_box;						/* combo box                          */
	GtkListStore *vals;						/* model of the combo box             */
	int stock_val;								/* 1 if the value in the entry box is being edited from the combo             */
	GtkTreeIter last_add_iter;		/* the iterator of the last added row (sometimes it needs to be selected)     */
	int last_add_iter_valid;

	/* buttons */
	GtkWidget *apply, *remove, *addattr;

	/* glue to hid_gtk* */
	const char *(*propedit_query)(void *pe, const char *cmd, const char *key, const char *val, int idx);
	void *propedit_pe;

	pcb_gtk_common_t *com;
} pcb_gtk_dlg_propedit_t;

/* Creates and runs a dialog to edit selected object properties */
GtkWidget *pcb_gtk_dlg_propedit_create(pcb_gtk_dlg_propedit_t *dlg, pcb_gtk_common_t *com);

void pcb_gtk_dlg_propedit_prop_add(pcb_gtk_dlg_propedit_t * dlg, const char *name, const char *common,
																	 const char *min, const char *max, const char *avg);

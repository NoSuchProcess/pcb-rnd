/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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

#include "hid_dad.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int mthi;

	int wmethod, wfmt, wcmd;
} ee_t;

#define NUM_METHODS (sizeof(methods) / sizeof(methods[0]))

static void ee_data2dialog(ee_t *ee)
{
	PCB_DAD_SET_VALUE(ee->dlg_hid_ctx, ee->wmethod, lng, ee->mthi);
	PCB_DAD_SET_VALUE(ee->dlg_hid_ctx, ee->wfmt, lng, methods[ee->mthi].fmt);
	PCB_DAD_SET_VALUE(ee->dlg_hid_ctx, ee->wcmd, str_value, pcb_strdup(methods[ee->mthi].command));

	/* we have only one format, so disable the combo box for selecting it */
	pcb_gui->attr_dlg_widget_state(ee->dlg_hid_ctx, ee->wfmt, pcb_false);
}

static void ee_chg_method(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	static int lock = 0;
	ee_t *ee = caller_data;

	if (lock)
		return;

	ee->mthi = ee->dlg[ee->wmethod].val.lng;

	lock = 1;
	ee_data2dialog(ee);
	lock = 0;
}

static void ee_chg_cmd(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	static int lock = 0;
	ee_t *ee = caller_data;

	if (lock)
		return;

	methods[ee->mthi].command = pcb_strdup(ee->dlg[ee->wcmd].val.str_value);

	lock = 1;
	ee_data2dialog(ee);
	lock = 0;
}


/* DAD-based interactive method editor */
static extedit_method_t *extedit_interactive(void)
{
	ee_t ee;
	char tmp[256];
	const char *names[NUM_METHODS+1];
	int n, res;
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"Edit!", 0}, {NULL, 0}};

	for(n = 0; n < NUM_METHODS; n++)
		names[n] = methods[n].name;
	names[n] = NULL;

	memset(&ee, 0, sizeof(ee));

	PCB_DAD_BEGIN_VBOX(ee.dlg);
		sprintf(tmp, "Select external editor...");
		PCB_DAD_LABEL(ee.dlg, tmp);

		PCB_DAD_BEGIN_HBOX(ee.dlg);
			PCB_DAD_LABEL(ee.dlg, "Method name:");
			PCB_DAD_ENUM(ee.dlg, names);
			ee.wmethod = PCB_DAD_CURRENT(ee.dlg);
			PCB_DAD_CHANGE_CB(ee.dlg, ee_chg_method);
		PCB_DAD_END(ee.dlg);

		PCB_DAD_BEGIN_HBOX(ee.dlg);
			PCB_DAD_LABEL(ee.dlg, "File format:");
			PCB_DAD_ENUM(ee.dlg, extedit_fmt_names);
			ee.wfmt = PCB_DAD_CURRENT(ee.dlg);
		PCB_DAD_END(ee.dlg);

		PCB_DAD_BEGIN_HBOX(ee.dlg);
			PCB_DAD_LABEL(ee.dlg, "Command template:");
			PCB_DAD_STRING(ee.dlg);
			ee.wcmd = PCB_DAD_CURRENT(ee.dlg);
			PCB_DAD_CHANGE_CB(ee.dlg, ee_chg_cmd);
		PCB_DAD_END(ee.dlg);
		PCB_DAD_BUTTON_CLOSES(ee.dlg, clbtn);
	PCB_DAD_END(ee.dlg);

	PCB_DAD_NEW("extedit", ee.dlg, "External editor", &ee, pcb_true, NULL);

	ee_data2dialog(&ee);
	res = PCB_DAD_RUN(ee.dlg);

	PCB_DAD_FREE(ee.dlg);
	if (res != 0)
		return NULL;
	return &methods[ee.mthi];
}

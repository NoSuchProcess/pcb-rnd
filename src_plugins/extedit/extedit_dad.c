/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <librnd/core/hid_dad.h>

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int mthi;

	int wmethod, wfmt, wcmd;
} ee_t;

#define NUM_METHODS (sizeof(methods) / sizeof(methods[0]))

static void ee_data2dialog(ee_t *ee)
{
	RND_DAD_SET_VALUE(ee->dlg_hid_ctx, ee->wmethod, lng, ee->mthi);
	RND_DAD_SET_VALUE(ee->dlg_hid_ctx, ee->wfmt, lng, methods[ee->mthi].fmt);
	RND_DAD_SET_VALUE(ee->dlg_hid_ctx, ee->wcmd, str, rnd_strdup(methods[ee->mthi].command));

	/* we have only one format, so disable the combo box for selecting it */
	rnd_gui->attr_dlg_widget_state(ee->dlg_hid_ctx, ee->wfmt, rnd_false);
}

static void ee_chg_method(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
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

static void ee_chg_cmd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	static int lock = 0;
	ee_t *ee = caller_data;

	if (lock)
		return;

	methods[ee->mthi].command = rnd_strdup(ee->dlg[ee->wcmd].val.str);

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
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"Edit!", 0}, {NULL, 0}};

	for(n = 0; n < NUM_METHODS; n++)
		names[n] = methods[n].name;
	names[n] = NULL;

	memset(&ee, 0, sizeof(ee));

	RND_DAD_BEGIN_VBOX(ee.dlg);
		sprintf(tmp, "Select external editor...");
		RND_DAD_LABEL(ee.dlg, tmp);

		RND_DAD_BEGIN_HBOX(ee.dlg);
			RND_DAD_LABEL(ee.dlg, "Method name:");
			RND_DAD_ENUM(ee.dlg, names);
			ee.wmethod = RND_DAD_CURRENT(ee.dlg);
			RND_DAD_CHANGE_CB(ee.dlg, ee_chg_method);
		RND_DAD_END(ee.dlg);

		RND_DAD_BEGIN_HBOX(ee.dlg);
			RND_DAD_LABEL(ee.dlg, "File format:");
			RND_DAD_ENUM(ee.dlg, extedit_fmt_names);
			ee.wfmt = RND_DAD_CURRENT(ee.dlg);
		RND_DAD_END(ee.dlg);

		RND_DAD_BEGIN_HBOX(ee.dlg);
			RND_DAD_LABEL(ee.dlg, "Command template:");
			RND_DAD_STRING(ee.dlg);
			ee.wcmd = RND_DAD_CURRENT(ee.dlg);
			RND_DAD_CHANGE_CB(ee.dlg, ee_chg_cmd);
		RND_DAD_END(ee.dlg);
		RND_DAD_BUTTON_CLOSES(ee.dlg, clbtn);
	RND_DAD_END(ee.dlg);

	RND_DAD_NEW("extedit", ee.dlg, "External editor", &ee, rnd_true, NULL);

	ee_data2dialog(&ee);
	res = RND_DAD_RUN(ee.dlg);

	RND_DAD_FREE(ee.dlg);
	if (res != 0)
		return NULL;
	return &methods[ee.mthi];
}

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

/* widget-type-independent DAD functions */

#include <librnd/config.h>
#include <librnd/core/hid_dad.h>

int rnd_dock_is_vert[RND_HID_DOCK_max]   = {0, 0, 0, 1, 0, 1}; /* Update this if rnd_hid_dock_t changes */
int rnd_dock_has_frame[RND_HID_DOCK_max] = {0, 0, 0, 1, 0, 0}; /* Update this if rnd_hid_dock_t changes */

typedef struct {
	rnd_hatt_compflags_t flag;
	const char *name;
} comflag_name_t;

static comflag_name_t compflag_names[] = {
	{RND_HATF_FRAME,         "frame"},
	{RND_HATF_SCROLL,        "scroll"},
	{RND_HATF_HIDE_TABLAB,   "hide_tablab"},
	{RND_HATF_LEFT_TAB,      "left_tab"},
	{RND_HATF_TREE_COL,      "tree_col"},
	{RND_HATF_EXPFILL,       "expfill"},
	{RND_HATF_TIGHT,         "tight"},
	{0, NULL}
};

const char *rnd_hid_compflag_bit2name(rnd_hatt_compflags_t bit)
{
	comflag_name_t *n;
	for(n = compflag_names; n->flag != 0; n++)
		if (n->flag == bit)
			return n->name;
	return NULL;
}

rnd_hatt_compflags_t rnd_hid_compflag_name2bit(const char *name)
{
	comflag_name_t *n;
	for(n = compflag_names; n->flag != 0; n++)
		if (strcmp(n->name, name) == 0)
			return n->flag;
	return 0;
}

void rnd_hid_dad_close(void *hid_ctx, rnd_dad_retovr_t *retovr, int retval)
{
	retovr->valid = 1;
	retovr->value = retval;
	rnd_gui->attr_dlg_close(hid_ctx);
}

void rnd_hid_dad_close_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_dad_retovr_t **retovr = attr->wdata;
	rnd_hid_dad_close(hid_ctx, *retovr, attr->val.lng);
}

int rnd_hid_dad_run(void *hid_ctx, rnd_dad_retovr_t *retovr)
{
	int ret;

	retovr->valid = 0;
	retovr->dont_free++;
	ret = rnd_gui->attr_dlg_run(hid_ctx);
	if (retovr->valid)
		ret = retovr->value;
	retovr->dont_free--;
	return ret;
}

int rnd_hid_attrdlg_num_children(rnd_hid_attribute_t *attrs, int start_from, int n_attrs)
{
	int n, level = 1, cnt = 0;

	for(n = start_from; n < n_attrs; n++) {
		if ((level == 1) && (attrs[n].type != RND_HATT_END))
			cnt++;
		switch(attrs[n].type) {
			case RND_HATT_END:
				level--;
				if (level == 0)
					return cnt;
				break;
			case RND_HATT_BEGIN_TABLE:
			case RND_HATT_BEGIN_HBOX:
			case RND_HATT_BEGIN_VBOX:
			case RND_HATT_BEGIN_COMPOUND:
				level++;
				break;
			default:
				break;
		}
	}
	return cnt;
}

int rnd_attribute_dialog_(const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, void **retovr, int defx, int defy, int minx, int miny, void **hid_ctx_out)
{
	int rv;
	void *hid_ctx;

	if ((rnd_gui == NULL) || (rnd_gui->attr_dlg_new == NULL))
		return -1;

	hid_ctx = rnd_gui->attr_dlg_new(rnd_gui, id, attrs, n_attrs, title, caller_data, rnd_true, NULL, defx, defy, minx, miny);
	if (hid_ctx_out != NULL)
		*hid_ctx_out = hid_ctx;
	rv = rnd_gui->attr_dlg_run(hid_ctx);
	if ((retovr == NULL) || (*retovr != 0))
		rnd_gui->attr_dlg_close(hid_ctx);

	return rv ? 0 : 1;
}

int rnd_attribute_dialog(const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data)
{
	return rnd_attribute_dialog_(id, attrs, n_attrs, title, caller_data, NULL, 0, 0, 0, 0, NULL);
}

int rnd_hid_dock_enter(rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id)
{
	if ((rnd_gui == NULL) || (rnd_gui->dock_enter == NULL))
		return -1;
	return rnd_gui->dock_enter(rnd_gui, sub, where, id);
}

void rnd_hid_dock_leave(rnd_hid_dad_subdialog_t *sub)
{
	if ((rnd_gui == NULL) || (rnd_gui->dock_leave == NULL))
		return;
	rnd_gui->dock_leave(rnd_gui, sub);
}


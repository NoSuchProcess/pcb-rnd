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

/* Compound DAD widget for numeric value entry, creating a spinbox */

#include <librnd/config.h>

#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_unit.h>
#include <librnd/core/unit.h>

void pcb_dad_unit_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_hid_dad_unit_t *unit = (pcb_hid_dad_unit_t *)attr->user_data;
	rnd_hid_attribute_t *enu = attr;
	rnd_hid_attribute_t *end = attr - unit->wenum + unit->cmp.wend;
	const char **vals = enu->wdata;
	const rnd_unit_t *u = get_unit_by_suffix(vals[enu->val.lng]);
	int unit_id = u == NULL ? -1 : u - pcb_units;

	end->val.lng = unit_id;
	end->changed = 1;
	if (end->change_cb != NULL)
		end->change_cb(hid_ctx, caller_data, end);
}

void pcb_dad_unit_set_num(rnd_hid_attribute_t *attr, long unit_id, double unused1, rnd_coord_t unused2)
{
	int l;
	pcb_hid_dad_unit_t *unit = attr->wdata;
	rnd_hid_attribute_t *enu = attr - unit->cmp.wend + unit->wenum;
	const char *target = pcb_units[unit_id].suffix;
	const char **vals = enu->wdata;

	for(l = 0; vals[l] != NULL; l++) {
		if (strcmp(target, vals[l]) == 0) {
			enu->val.lng = l;
			attr->val.lng = l;
		}
	}
}

void pcb_dad_unit_set_val_ptr(rnd_hid_attribute_t *end, void *val_)
{
	const rnd_unit_t *val = val_;
	int __n__, __v__ = pcb_get_n_units(1);
	if (val != NULL) {
		for(__n__ = 0; __n__ < __v__; __n__++) {
			if (&pcb_units[__n__] == val) {
				pcb_dad_unit_set_num(end, __n__, 0, 0);
				return;
			}
		}
	}
}

int pcb_dad_unit_widget_state(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool enabled)
{
	pcb_hid_dad_unit_t *unit = end->wdata;
	return pcb_gui->attr_dlg_widget_state(hid_ctx, unit->wenum, enabled);
}

int pcb_dad_unit_widget_hide(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool hide)
{
	pcb_hid_dad_unit_t *unit = end->wdata;
	return pcb_gui->attr_dlg_widget_hide(hid_ctx, unit->wenum, hide);
}

int pcb_dad_unit_set_value(rnd_hid_attribute_t *end, void *hid_ctx, int idx, const rnd_hid_attr_val_t *val)
{
	pcb_hid_dad_unit_t *unit = end->wdata;
	if (pcb_gui->attr_dlg_set_value(hid_ctx, unit->wenum, val) != 0)
		return -1;
	end->val.lng = val->lng;
	return 0;
}

void pcb_dad_unit_set_help(rnd_hid_attribute_t *end, const char *help)
{
	pcb_hid_dad_unit_t *unit = end->wdata;
	rnd_hid_attribute_t *enu = end - unit->cmp.wend + unit->wenum;

	if ((unit->hid_ctx == NULL) || (*unit->hid_ctx == NULL)) /* while building */
		enu->help_text = help;
	else if (pcb_gui->attr_dlg_set_help != NULL) /* when the dialog is already running */
		pcb_gui->attr_dlg_set_help(*unit->hid_ctx, unit->wenum, help);
}

const char **pcb_dad_unit_enum = NULL;

void pcb_dad_unit_init(enum pcb_family_e family)
{
	int len, n, i;

	if (pcb_dad_unit_enum != NULL)
		return;

	len = pcb_get_n_units(0);
	pcb_dad_unit_enum = malloc(sizeof(char *) * (len+1));
	for(n = i = 0; i < len; i++) {
		if (pcb_units[i].family & family)
			pcb_dad_unit_enum[n++] = pcb_units[i].suffix;
	}
	pcb_dad_unit_enum[n] = NULL;
}

void pcb_dad_unit_uninit(void)
{
	free(pcb_dad_unit_enum);
	pcb_dad_unit_enum = NULL;
}

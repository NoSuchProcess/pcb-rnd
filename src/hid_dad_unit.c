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

#include "config.h"

#include "hid_attrib.h"
#include "hid_dad.h"
#include "hid_dad_unit.h"
#include "unit.h"

void pcb_dad_unit_change_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_unit_t *unit = (pcb_hid_dad_unit_t *)attr->user_data;
	pcb_hid_attribute_t *enu = attr;
	pcb_hid_attribute_t *end = attr - unit->wenum + unit->cmp.wend;

	end->default_val.int_value = enu->default_val.int_value;
	end->changed = 1;
	if (end->change_cb != NULL)
		end->change_cb(hid_ctx, caller_data, end);
}

void pcb_dad_unit_set_num(pcb_hid_attribute_t *attr, long l, double unused1, pcb_coord_t unused2)
{
	pcb_hid_dad_unit_t *unit = (pcb_hid_dad_unit_t *)attr->enumerations;
	pcb_hid_attribute_t *enu = attr - unit->cmp.wend + unit->wenum;

	enu->default_val.int_value = l;
}

int pcb_dad_unit_widget_state(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool enabled)
{
	pcb_hid_dad_unit_t *unit = (pcb_hid_dad_unit_t *)end->enumerations;
	return pcb_gui->attr_dlg_widget_state(hid_ctx, unit->wenum, enabled);
}

int pcb_dad_unit_widget_hide(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool hide)
{
	pcb_hid_dad_unit_t *unit = (pcb_hid_dad_unit_t *)end->enumerations;
	return pcb_gui->attr_dlg_widget_hide(hid_ctx, unit->wenum, hide);
}

int pcb_dad_unit_set_value(pcb_hid_attribute_t *end, void *hid_ctx, int idx, const pcb_hid_attr_val_t *val)
{
	pcb_hid_dad_unit_t *unit = (pcb_hid_dad_unit_t *)end->enumerations;
	if (pcb_gui->attr_dlg_set_value(hid_ctx, unit->wenum, val) != 0)
		return -1;
	end->default_val.int_value = val->int_value;
	return 0;
}

void pcb_dad_unit_set_help(pcb_hid_attribute_t *end, const char *help)
{
	pcb_hid_dad_unit_t *unit = (pcb_hid_dad_unit_t *)end->enumerations;
	pcb_hid_attribute_t *enu = end - unit->cmp.wend + unit->wenum;

	if ((unit->hid_ctx == NULL) || (*unit->hid_ctx == NULL)) /* while building */
		enu->help_text = help;
	else if (pcb_gui->attr_dlg_set_help != NULL) /* when the dialog is already running */
		pcb_gui->attr_dlg_set_help(*unit->hid_ctx, unit->wenum, help);
}

const char **pcb_dad_unit_enum = NULL;

void pcb_dad_unit_init(void)
{
	int len, n;

	if (pcb_dad_unit_enum != NULL)
		return;

	len = pcb_get_n_units();
	pcb_dad_unit_enum = malloc(sizeof(char *) * (len+1));
	for(n = 0; n < len; n++)
		pcb_dad_unit_enum[n] = pcb_units[n].suffix;
	pcb_dad_unit_enum[n] = NULL;
}

void pcb_dad_unit_uninit(void)
{
	free(pcb_dad_unit_enum);
	pcb_dad_unit_enum = NULL;
}

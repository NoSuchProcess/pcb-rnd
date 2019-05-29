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

/* Compound DAD widget for creating a unit change combo box */

#ifndef PCB_HID_DAD_UNIT_H
#define PCB_HID_DAD_UNIT_H

#include <genlist/gendlist.h>

typedef struct {
	pcb_hid_compound_t cmp;
	void **hid_ctx;
	int family; /* to be used later, when the unit system supports unit family */
	int wenum;
} pcb_hid_dad_unit_t;

/*** implementation ***/


#define PCB_DAD_UNIT_NEW(table, family) \
do { \
	pcb_hid_dad_unit_t *unit = calloc(sizeof(pcb_hid_dad_unit_t), 1); \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_COMPOUND); \
		unit->cmp.wbegin = PCB_DAD_CURRENT(table); \
		pcb_dad_unit_init(); \
		PCB_DAD_ENUM(table, pcb_dad_unit_enum); \
			PCB_DAD_CHANGE_CB(table, pcb_dad_unit_change_cb); \
			unit->wenum = PCB_DAD_CURRENT(table); \
	PCB_DAD_END(table); \
		PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)unit); \
		unit->cmp.wend = PCB_DAD_CURRENT(table); \
	\
	unit->cmp.set_val_num = pcb_dad_unit_set_num; \
	unit->cmp.widget_state = pcb_dad_unit_widget_state; \
	unit->cmp.widget_hide = pcb_dad_unit_widget_hide; \
	unit->cmp.set_value = pcb_dad_unit_set_value; \
	unit->cmp.set_help = pcb_dad_unit_set_help; \
	unit->family = family; \
	unit->attrs = &table; \
	unit->hid_ctx = &table ## _hid_ctx; \
} while(0)

extern const char **pcb_dad_unit_enum;

void pcb_dad_unit_set_num(pcb_hid_attribute_t *attr, int uidx);
int pcb_dad_unit_widget_state(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool enabled);
int pcb_dad_unit_widget_hide(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool hide);
int pcb_dad_unit_set_value(pcb_hid_attribute_t *end, void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);
void pcb_dad_unit_set_help(pcb_hid_attribute_t *end, const char *help);
void pcb_dad_unit_change_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);

void pcb_dad_unit_init(void);
void pcb_dad_unit_uninit(void);

#endif

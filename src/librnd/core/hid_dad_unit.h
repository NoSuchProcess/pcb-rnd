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

#ifndef RND_HID_DAD_UNIT_H
#define RND_HID_DAD_UNIT_H

#include <genlist/gendlist.h>

#include <librnd/core/hid_dad.h>

typedef struct {
	rnd_hid_compound_t cmp;
	void **hid_ctx;
	rnd_family_t family; /* which families of units are allowed in this spinbox */
	int wenum;
} rnd_hid_dad_unit_t;

/*** implementation ***/


#define RND_DAD_UNIT(table, family_) \
do { \
	rnd_hid_dad_unit_t *unit = calloc(sizeof(rnd_hid_dad_unit_t), 1); \
	RND_DAD_BEGIN(table, RND_HATT_BEGIN_COMPOUND); \
		unit->cmp.wbegin = RND_DAD_CURRENT(table); \
		rnd_dad_unit_init(family_); \
		RND_DAD_ENUM(table, rnd_dad_unit_enum); \
			RND_DAD_CHANGE_CB(table, rnd_dad_unit_change_cb); \
			RND_DAD_SET_ATTR_FIELD(table, user_data, (const char **)unit); \
			unit->wenum = RND_DAD_CURRENT(table); \
	RND_DAD_END(table); \
		RND_DAD_SET_ATTR_FIELD(table, wdata, unit); \
		unit->cmp.wend = RND_DAD_CURRENT(table); \
	\
	unit->cmp.set_val_num = rnd_dad_unit_set_num; \
	unit->cmp.widget_state = rnd_dad_unit_widget_state; \
	unit->cmp.widget_hide = rnd_dad_unit_widget_hide; \
	unit->cmp.set_value = rnd_dad_unit_set_value; \
	unit->cmp.set_val_ptr = rnd_dad_unit_set_val_ptr; \
	unit->cmp.set_help = rnd_dad_unit_set_help; \
	unit->family = family_; \
	unit->hid_ctx = &table ## _hid_ctx; \
} while(0)

extern const char **rnd_dad_unit_enum;

void rnd_dad_unit_set_num(rnd_hid_attribute_t *attr, long l, double unused1, rnd_coord_t unused2);
int rnd_dad_unit_widget_state(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool enabled);
int rnd_dad_unit_widget_hide(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool hide);
int rnd_dad_unit_set_value(rnd_hid_attribute_t *end, void *hid_ctx, int idx, const rnd_hid_attr_val_t *val);
void rnd_dad_unit_set_val_ptr(rnd_hid_attribute_t *end, void *val);
void rnd_dad_unit_set_help(rnd_hid_attribute_t *end, const char *help);
void rnd_dad_unit_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);

void rnd_dad_unit_init(enum rnd_family_e family);
void rnd_dad_unit_uninit(void);

#endif

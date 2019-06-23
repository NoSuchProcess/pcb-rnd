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

#ifndef PCB_HID_DAD_SPIN_H
#define PCB_HID_DAD_SPIN_H

#include <genlist/gendlist.h>

typedef struct {
	pcb_hid_compound_t cmp;
	double step; /* how much an up/down step modifies; 0 means automatic */
	double vmin, vmax;
	unsigned vmin_valid:1;
	unsigned vmax_valid:1;
	unsigned no_unit_chg:1;
	int wall, wstr, wup, wdown, wunit, wwarn;
	const pcb_unit_t *unit; /* for PCB_DAD_SPIN_COORD only: current unit */
	pcb_hid_attribute_t **attrs;
	void **hid_ctx;
	int set_writeback_lock;
	enum {
		PCB_DAD_SPIN_INT,
		PCB_DAD_SPIN_DOUBLE,
		PCB_DAD_SPIN_COORD
	} type;
	gdl_elem_t link;
} pcb_hid_dad_spin_t;

#define PCB_DAD_SPIN_INT(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_INT, 0)
#define PCB_DAD_SPIN_DOUBLE(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_DOUBLE, 0)
#define PCB_DAD_SPIN_COORD(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_COORD, 1)

/*** implementation ***/

#define PCB_DAD_SPIN_ANY(table, typ, has_unit) \
do { \
	pcb_hid_dad_spin_t *spin = calloc(sizeof(pcb_hid_dad_spin_t), 1); \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_COMPOUND); \
		spin->cmp.wbegin = PCB_DAD_CURRENT(table); \
		PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)spin); \
		PCB_DAD_BEGIN_HBOX(table); \
			spin->wall = PCB_DAD_CURRENT(table); \
			PCB_DAD_COMPFLAG(table, PCB_HATF_TIGHT); \
			PCB_DAD_STRING(table); \
				PCB_DAD_DEFAULT_PTR(table, pcb_strdup("")); \
				PCB_DAD_ENTER_CB(table, pcb_dad_spin_txt_enter_cb); \
				PCB_DAD_CHANGE_CB(table, pcb_dad_spin_txt_change_cb); \
				PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
				spin->wstr = PCB_DAD_CURRENT(table); \
			PCB_DAD_BEGIN_VBOX(table); \
				PCB_DAD_COMPFLAG(table, PCB_HATF_TIGHT); \
				PCB_DAD_PICBUTTON(table, pcb_hid_dad_spin_up); \
					PCB_DAD_CHANGE_CB(table, pcb_dad_spin_up_cb); \
					PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wup = PCB_DAD_CURRENT(table); \
				PCB_DAD_PICBUTTON(table, pcb_hid_dad_spin_down); \
					PCB_DAD_CHANGE_CB(table, pcb_dad_spin_down_cb); \
					PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wdown = PCB_DAD_CURRENT(table); \
			PCB_DAD_END(table); \
			PCB_DAD_BEGIN_VBOX(table); \
				PCB_DAD_COMPFLAG(table, PCB_HATF_TIGHT); \
				if (has_unit) { \
					PCB_DAD_PICBUTTON(table, pcb_hid_dad_spin_unit); \
						PCB_DAD_CHANGE_CB(table, pcb_dad_spin_unit_cb); \
						PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
						spin->wunit = PCB_DAD_CURRENT(table); \
				} \
				PCB_DAD_PICTURE(table, pcb_hid_dad_spin_warn); \
					PCB_DAD_COMPFLAG(table, PCB_HATF_HIDE); \
					PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wwarn = PCB_DAD_CURRENT(table); \
			PCB_DAD_END(table); \
		PCB_DAD_END(table); \
	PCB_DAD_END(table); \
		PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)spin); \
		spin->cmp.wend = PCB_DAD_CURRENT(table); \
	\
	spin->cmp.free = pcb_dad_spin_free; \
	spin->cmp.set_val_num = pcb_dad_spin_set_num; \
	spin->cmp.widget_state = pcb_dad_spin_widget_state; \
	spin->cmp.widget_hide = pcb_dad_spin_widget_hide; \
	spin->cmp.set_value = pcb_dad_spin_set_value; \
	spin->cmp.set_help = pcb_dad_spin_set_help; \
	spin->type = typ; \
	spin->attrs = &table; \
	spin->hid_ctx = &table ## _hid_ctx; \
	\
	if (typ == PCB_DAD_SPIN_COORD) \
		gdl_append(&pcb_dad_coord_spins, spin, link); \
} while(0)

extern const char *pcb_hid_dad_spin_up[];
extern const char *pcb_hid_dad_spin_down[];
extern const char *pcb_hid_dad_spin_unit[];
extern const char *pcb_hid_dad_spin_unit[];
extern const char *pcb_hid_dad_spin_warn[];

void pcb_dad_spin_up_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_down_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_txt_enter_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_txt_enter_cb_dry(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_txt_enter_call_users(void *hid_ctx, pcb_hid_attribute_t *end); /* call the user's enter_cb on end */
void pcb_dad_spin_txt_change_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_unit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);

void pcb_dad_spin_free(pcb_hid_attribute_t *attrib);
void pcb_dad_spin_set_num(pcb_hid_attribute_t *attr, long l, double d, pcb_coord_t c);
int pcb_dad_spin_widget_state(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool enabled);
int pcb_dad_spin_widget_hide(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool hide);
int pcb_dad_spin_set_value(pcb_hid_attribute_t *end, void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);
void pcb_dad_spin_set_help(pcb_hid_attribute_t *end, const char *help);

extern gdl_list_t pcb_dad_coord_spins; /* list of all active coord spinboxes */

#endif

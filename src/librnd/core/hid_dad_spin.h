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
	const rnd_unit_t *unit; /* for PCB_DAD_SPIN_COORD and PCB_DAD_SPIN_FREQ only: current unit */
	pcb_family_t unit_family;
	rnd_hid_attribute_t **attrs;
	void **hid_ctx;
	int set_writeback_lock;
	rnd_coord_t last_good_crd;
	enum {
		PCB_DAD_SPIN_INT,
		PCB_DAD_SPIN_DOUBLE,
		PCB_DAD_SPIN_COORD,
		PCB_DAD_SPIN_FREQ
	} type;
	rnd_hid_attr_type_t wtype;
	gdl_elem_t link;
} pcb_hid_dad_spin_t;

#define PCB_DAD_SPIN_INT(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_INT, RND_HATT_INTEGER, 0, 0)
#define PCB_DAD_SPIN_DOUBLE(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_DOUBLE, RND_HATT_REAL, 0, 0)
#define PCB_DAD_SPIN_COORD(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_COORD, RND_HATT_COORD, 1, PCB_UNIT_METRIC | PCB_UNIT_IMPERIAL)
#define PCB_DAD_SPIN_FREQ(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_FREQ, RND_HATT_REAL, 1, PCB_UNIT_FREQ)

/* Return the widget-type (PCB_DAD_HATT) of a spinbox at the RND_HATT_END widget;
   useful for dispatching what value set to use */
#define PCB_DAD_SPIN_GET_TYPE(attr) \
	((((attr)->type == RND_HATT_END) && (((pcb_hid_dad_spin_t *)((attr)->wdata))->cmp.free == pcb_dad_spin_free)) ? ((pcb_hid_dad_spin_t *)((attr)->wdata))->wtype : RND_HATT_END)

/*** implementation ***/

#define PCB_DAD_SPIN_ANY(table, typ, wtyp, has_unit, unit_family_) \
do { \
	pcb_hid_dad_spin_t *spin = calloc(sizeof(pcb_hid_dad_spin_t), 1); \
	PCB_DAD_BEGIN(table, RND_HATT_BEGIN_COMPOUND); \
		spin->cmp.wbegin = PCB_DAD_CURRENT(table); \
		PCB_DAD_SET_ATTR_FIELD(table, wdata, spin); \
		PCB_DAD_BEGIN_HBOX(table); \
			spin->wall = PCB_DAD_CURRENT(table); \
			PCB_DAD_COMPFLAG(table, RND_HATF_TIGHT); \
			PCB_DAD_STRING(table); \
				PCB_DAD_DEFAULT_PTR(table, rnd_strdup("")); \
				PCB_DAD_ENTER_CB(table, pcb_dad_spin_txt_enter_cb); \
				PCB_DAD_CHANGE_CB(table, pcb_dad_spin_txt_change_cb); \
				PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
				spin->wstr = PCB_DAD_CURRENT(table); \
			PCB_DAD_BEGIN_VBOX(table); \
				PCB_DAD_COMPFLAG(table, RND_HATF_TIGHT); \
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
				PCB_DAD_COMPFLAG(table, RND_HATF_TIGHT); \
				if (has_unit) { \
					PCB_DAD_PICBUTTON(table, pcb_hid_dad_spin_unit); \
						PCB_DAD_CHANGE_CB(table, pcb_dad_spin_unit_cb); \
						PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
						spin->wunit = PCB_DAD_CURRENT(table); \
				} \
				PCB_DAD_PICTURE(table, pcb_hid_dad_spin_warn); \
					PCB_DAD_COMPFLAG(table, RND_HATF_HIDE); \
					PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wwarn = PCB_DAD_CURRENT(table); \
			PCB_DAD_END(table); \
		PCB_DAD_END(table); \
	PCB_DAD_END(table); \
		PCB_DAD_SET_ATTR_FIELD(table, wdata, spin); \
		spin->cmp.wend = PCB_DAD_CURRENT(table); \
	\
	spin->cmp.free = pcb_dad_spin_free; \
	spin->cmp.set_val_num = pcb_dad_spin_set_num; \
	spin->cmp.widget_state = pcb_dad_spin_widget_state; \
	spin->cmp.widget_hide = pcb_dad_spin_widget_hide; \
	spin->cmp.set_value = pcb_dad_spin_set_value; \
	spin->cmp.set_help = pcb_dad_spin_set_help; \
	spin->cmp.set_geo = pcb_dad_spin_set_geo; \
	spin->type = typ; \
	spin->wtype = wtyp; \
	spin->attrs = &table; \
	spin->hid_ctx = &table ## _hid_ctx; \
	spin->unit_family = unit_family_; \
	\
	if (typ == PCB_DAD_SPIN_COORD) \
		gdl_append(&pcb_dad_coord_spins, spin, link); \
} while(0)

extern const char *pcb_hid_dad_spin_up[];
extern const char *pcb_hid_dad_spin_down[];
extern const char *pcb_hid_dad_spin_unit[];
extern const char *pcb_hid_dad_spin_unit[];
extern const char *pcb_hid_dad_spin_warn[];

void pcb_dad_spin_up_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void pcb_dad_spin_down_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void pcb_dad_spin_txt_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void pcb_dad_spin_txt_enter_cb_dry(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void pcb_dad_spin_txt_enter_call_users(void *hid_ctx, rnd_hid_attribute_t *end); /* call the user's enter_cb on end */
void pcb_dad_spin_txt_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void pcb_dad_spin_unit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);

void pcb_dad_spin_free(rnd_hid_attribute_t *attrib);
void pcb_dad_spin_set_num(rnd_hid_attribute_t *attr, long l, double d, rnd_coord_t c);
int pcb_dad_spin_widget_state(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool enabled);
int pcb_dad_spin_widget_hide(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool hide);
int pcb_dad_spin_set_value(rnd_hid_attribute_t *end, void *hid_ctx, int idx, const rnd_hid_attr_val_t *val);
void pcb_dad_spin_set_help(rnd_hid_attribute_t *end, const char *help);
void pcb_dad_spin_set_geo(rnd_hid_attribute_t *end, rnd_hatt_compflags_t flg, int geo);

void pcb_dad_spin_update_internal(pcb_hid_dad_spin_t *spin); /* update the widget from spin, before or after the dialog is realized */

extern gdl_list_t pcb_dad_coord_spins; /* list of all active coord spinboxes */

#endif

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

#ifndef RND_HID_DAD_SPIN_H
#define RND_HID_DAD_SPIN_H

#include <genlist/gendlist.h>

typedef struct {
	rnd_hid_compound_t cmp;
	double step; /* how much an up/down step modifies; 0 means automatic */
	double vmin, vmax;
	unsigned vmin_valid:1;
	unsigned vmax_valid:1;
	unsigned no_unit_chg:1;
	int wall, wstr, wup, wdown, wunit, wwarn;
	const rnd_unit_t *unit; /* for RND_DAD_SPIN_COORD and RND_DAD_SPIN_FREQ only: current unit */
	pcb_family_t unit_family;
	rnd_hid_attribute_t **attrs;
	void **hid_ctx;
	int set_writeback_lock;
	rnd_coord_t last_good_crd;
	enum {
		RND_DAD_SPIN_INT,
		RND_DAD_SPIN_DOUBLE,
		RND_DAD_SPIN_COORD,
		RND_DAD_SPIN_FREQ
	} type;
	rnd_hid_attr_type_t wtype;
	gdl_elem_t link;
} rnd_hid_dad_spin_t;

#define RND_DAD_SPIN_INT(table) RND_DAD_SPIN_ANY(table, RND_DAD_SPIN_INT, RND_HATT_INTEGER, 0, 0)
#define RND_DAD_SPIN_DOUBLE(table) RND_DAD_SPIN_ANY(table, RND_DAD_SPIN_DOUBLE, RND_HATT_REAL, 0, 0)
#define RND_DAD_SPIN_COORD(table) RND_DAD_SPIN_ANY(table, RND_DAD_SPIN_COORD, RND_HATT_COORD, 1, PCB_UNIT_METRIC | PCB_UNIT_IMPERIAL)
#define RND_DAD_SPIN_FREQ(table) RND_DAD_SPIN_ANY(table, RND_DAD_SPIN_FREQ, RND_HATT_REAL, 1, PCB_UNIT_FREQ)

/* Return the widget-type (PCB_DAD_HATT) of a spinbox at the RND_HATT_END widget;
   useful for dispatching what value set to use */
#define RND_DAD_SPIN_GET_TYPE(attr) \
	((((attr)->type == RND_HATT_END) && (((rnd_hid_dad_spin_t *)((attr)->wdata))->cmp.free == rnd_dad_spin_free)) ? ((rnd_hid_dad_spin_t *)((attr)->wdata))->wtype : RND_HATT_END)

/*** implementation ***/

#define RND_DAD_SPIN_ANY(table, typ, wtyp, has_unit, unit_family_) \
do { \
	rnd_hid_dad_spin_t *spin = calloc(sizeof(rnd_hid_dad_spin_t), 1); \
	RND_DAD_BEGIN(table, RND_HATT_BEGIN_COMPOUND); \
		spin->cmp.wbegin = RND_DAD_CURRENT(table); \
		RND_DAD_SET_ATTR_FIELD(table, wdata, spin); \
		RND_DAD_BEGIN_HBOX(table); \
			spin->wall = RND_DAD_CURRENT(table); \
			RND_DAD_COMPFLAG(table, RND_HATF_TIGHT); \
			RND_DAD_STRING(table); \
				RND_DAD_DEFAULT_PTR(table, rnd_strdup("")); \
				RND_DAD_ENTER_CB(table, rnd_dad_spin_txt_enter_cb); \
				RND_DAD_CHANGE_CB(table, rnd_dad_spin_txt_change_cb); \
				RND_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
				spin->wstr = RND_DAD_CURRENT(table); \
			RND_DAD_BEGIN_VBOX(table); \
				RND_DAD_COMPFLAG(table, RND_HATF_TIGHT); \
				RND_DAD_PICBUTTON(table, rnd_hid_dad_spin_up); \
					RND_DAD_CHANGE_CB(table, rnd_dad_spin_up_cb); \
					RND_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wup = RND_DAD_CURRENT(table); \
				RND_DAD_PICBUTTON(table, rnd_hid_dad_spin_down); \
					RND_DAD_CHANGE_CB(table, rnd_dad_spin_down_cb); \
					RND_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wdown = RND_DAD_CURRENT(table); \
			RND_DAD_END(table); \
			RND_DAD_BEGIN_VBOX(table); \
				RND_DAD_COMPFLAG(table, RND_HATF_TIGHT); \
				if (has_unit) { \
					RND_DAD_PICBUTTON(table, rnd_hid_dad_spin_unit); \
						RND_DAD_CHANGE_CB(table, rnd_dad_spin_unit_cb); \
						RND_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
						spin->wunit = RND_DAD_CURRENT(table); \
				} \
				RND_DAD_PICTURE(table, rnd_hid_dad_spin_warn); \
					RND_DAD_COMPFLAG(table, RND_HATF_HIDE); \
					RND_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wwarn = RND_DAD_CURRENT(table); \
			RND_DAD_END(table); \
		RND_DAD_END(table); \
	RND_DAD_END(table); \
		RND_DAD_SET_ATTR_FIELD(table, wdata, spin); \
		spin->cmp.wend = RND_DAD_CURRENT(table); \
	\
	spin->cmp.free = rnd_dad_spin_free; \
	spin->cmp.set_val_num = rnd_dad_spin_set_num; \
	spin->cmp.widget_state = rnd_dad_spin_widget_state; \
	spin->cmp.widget_hide = rnd_dad_spin_widget_hide; \
	spin->cmp.set_value = rnd_dad_spin_set_value; \
	spin->cmp.set_help = rnd_dad_spin_set_help; \
	spin->cmp.set_geo = rnd_dad_spin_set_geo; \
	spin->type = typ; \
	spin->wtype = wtyp; \
	spin->attrs = &table; \
	spin->hid_ctx = &table ## _hid_ctx; \
	spin->unit_family = unit_family_; \
	\
	if (typ == RND_DAD_SPIN_COORD) \
		gdl_append(&rnd_dad_coord_spins, spin, link); \
} while(0)

extern const char *rnd_hid_dad_spin_up[];
extern const char *rnd_hid_dad_spin_down[];
extern const char *rnd_hid_dad_spin_unit[];
extern const char *rnd_hid_dad_spin_unit[];
extern const char *rnd_hid_dad_spin_warn[];

void rnd_dad_spin_up_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void rnd_dad_spin_down_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void rnd_dad_spin_txt_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void rnd_dad_spin_txt_enter_cb_dry(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void rnd_dad_spin_txt_enter_call_users(void *hid_ctx, rnd_hid_attribute_t *end); /* call the user's enter_cb on end */
void rnd_dad_spin_txt_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
void rnd_dad_spin_unit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);

void rnd_dad_spin_free(rnd_hid_attribute_t *attrib);
void rnd_dad_spin_set_num(rnd_hid_attribute_t *attr, long l, double d, rnd_coord_t c);
int rnd_dad_spin_widget_state(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool enabled);
int rnd_dad_spin_widget_hide(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool hide);
int rnd_dad_spin_set_value(rnd_hid_attribute_t *end, void *hid_ctx, int idx, const rnd_hid_attr_val_t *val);
void rnd_dad_spin_set_help(rnd_hid_attribute_t *end, const char *help);
void rnd_dad_spin_set_geo(rnd_hid_attribute_t *end, rnd_hatt_compflags_t flg, int geo);

void rnd_dad_spin_update_internal(rnd_hid_dad_spin_t *spin); /* update the widget from spin, before or after the dialog is realized */

extern gdl_list_t rnd_dad_coord_spins; /* list of all active coord spinboxes */

#endif

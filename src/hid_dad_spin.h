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

typedef struct {
	pcb_hid_compound_t cmp;
	double step; /* how much an up/down step modifies; 0 means automatic */
	double vmin, vmax;
	unsigned vmin_valid:1;
	unsigned vmax_valid:1;
	int wstr, wup, wdown, wunit, wwarn;
	enum {
		PCB_DAD_SPIN_INT,
		PCB_DAD_SPIN_DOUBLE
	} type;
} pcb_hid_dad_spin_t;

#define PCB_DAD_SPIN_INT(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_INT, 0)
#define PCB_DAD_SPIN_DOUBLE(table) PCB_DAD_SPIN_ANY(table, PCB_DAD_SPIN_DOUBLE, 0)

/*** implementation ***/

#define PCB_DAD_SPIN_ANY(table, typ, has_unit) \
do { \
	pcb_hid_dad_spin_t *spin = calloc(sizeof(pcb_hid_dad_spin_t), 1); \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_COMPOUND); \
		spin->cmp.wbegin = PCB_DAD_CURRENT(table); \
		PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)spin); \
		PCB_DAD_BEGIN_HBOX(table); \
			PCB_DAD_COMPFLAG(table, PCB_HATF_TIGHT); \
			PCB_DAD_STRING(table); \
				PCB_DAD_CHANGE_CB(ctx.dlg, pcb_dad_spin_txt_cb); \
				PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
				spin->wstr = PCB_DAD_CURRENT(table); \
			PCB_DAD_BEGIN_VBOX(table); \
				PCB_DAD_COMPFLAG(table, PCB_HATF_TIGHT); \
				PCB_DAD_PICBUTTON(table, pcb_hid_dad_spin_up); \
					PCB_DAD_CHANGE_CB(ctx.dlg, pcb_dad_spin_up_cb); \
					PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wup = PCB_DAD_CURRENT(table); \
				PCB_DAD_PICBUTTON(table, pcb_hid_dad_spin_down); \
					PCB_DAD_CHANGE_CB(ctx.dlg, pcb_dad_spin_down_cb); \
					PCB_DAD_SET_ATTR_FIELD(table, user_data, (const char **)spin); \
					spin->wdown = PCB_DAD_CURRENT(table); \
			PCB_DAD_END(table); \
			PCB_DAD_BEGIN_VBOX(table); \
				PCB_DAD_COMPFLAG(table, PCB_HATF_TIGHT); \
				if (has_unit) { \
					PCB_DAD_PICBUTTON(table, pcb_hid_dad_spin_unit); \
						PCB_DAD_CHANGE_CB(ctx.dlg, pcb_dad_spin_unit_cb); \
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
	spin->type = typ; \
} while(0)

extern const char *pcb_hid_dad_spin_up[];
extern const char *pcb_hid_dad_spin_down[];
extern const char *pcb_hid_dad_spin_unit[];
extern const char *pcb_hid_dad_spin_unit[];
extern const char *pcb_hid_dad_spin_warn[];

void pcb_dad_spin_up_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_down_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_txt_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
void pcb_dad_spin_unit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);

void pcb_dad_spin_free(pcb_hid_attribute_t *attrib);
void pcb_dad_spin_set_num(pcb_hid_attribute_t *attr, long l, double d, pcb_coord_t c);

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
#include "hid_dad_spin.h"
#include "pcb-printf.h"
#include "compat_misc.h"

const char *pcb_hid_dad_spin_up[] = {
"5 3 2 1",
" 	c None",
"+	c #000000",
"  +  ",
" +++ ",
"+++++",
};

const char *pcb_hid_dad_spin_down[] = {
"5 3 2 1",
" 	c None",
"+	c #000000",
"+++++",
" +++ ",
"  +  ",
};

const char *pcb_hid_dad_spin_unit[] = {
"4 3 2 1",
" 	c None",
"+	c #000000",
"+  +",
"+  +",
" ++ ",
};

const char *pcb_hid_dad_spin_warn[] = {
"9 9 3 1",
" 	c None",
"+	c #000000",
"*	c #FF0000",
" ******* ",
"**     **",
"* +   + *",
"* +   + *",
"* + + + *",
"* + + + *",
"* +++++ *",
"**     **",
" ******* ",
};

static void spin_changed(void *hid_ctx, void *caller_data, pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *end)
{
	if (end->change_cb != NULL)
		end->change_cb(hid_ctx, caller_data, end);
}

static void spin_warn(void *hid_ctx, pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *end, const char *msg)
{
	pcb_gui->attr_dlg_widget_hide(hid_ctx, spin->wwarn, (msg == NULL));
}

static double get_step(pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *end)
{
	double step;

	if (spin->step > 0)
		return spin->step;

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			step = pow(10, floor(log10(fabs(end->default_val.int_value)) - 1.0));
			if (step < 1)
				step = 1;
			break;
	}
	return step;
}

static void do_step(void *hid_ctx, pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *str, pcb_hid_attribute_t *end, double step)
{
	pcb_hid_attr_val_t hv;
	char buf[128];

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			end->default_val.int_value += step;
			sprintf(buf, "%d", end->default_val.int_value);
			break;
	}

	spin_warn(hid_ctx, spin, end, NULL);
	hv.str_value = pcb_strdup(buf);
	pcb_gui->attr_dlg_set_value(hid_ctx, spin->wstr, &hv);
}

void pcb_dad_spin_up_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wup + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wup + spin->cmp.wend;

	do_step(hid_ctx, spin, str, end, get_step(spin, end));
	spin_changed(hid_ctx, caller_data, spin, end);
}

void pcb_dad_spin_down_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wdown + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wdown + spin->cmp.wend;

	do_step(hid_ctx, spin, str, end, -get_step(spin, end));
	spin_changed(hid_ctx, caller_data, spin, end);
}

void pcb_dad_spin_txt_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr;
	pcb_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;
	char *ends;
	long l;

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			l = strtol(str->default_val.str_value, &ends, 10);
TODO("handle conv error using *ends");
			end->default_val.int_value = l;
			break;
		default: pcb_trace("INTERNAL ERROR: spin_set_num\n");
	}

	spin_changed(hid_ctx, caller_data, spin, end);
}

void pcb_dad_spin_unit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wdown + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wdown + spin->cmp.wend;
}

void pcb_dad_spin_set_num(pcb_hid_attribute_t *attr, long l, double d, pcb_coord_t c)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->enumerations;
	pcb_hid_attribute_t *str = attr - spin->cmp.wend + spin->wstr;

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			attr->default_val.int_value = l;
			free((char *)str->default_val.str_value);
			str->default_val.str_value = pcb_strdup_printf("%ld", l);
			break;
		default: pcb_trace("INTERNAL ERROR: spin_set_num\n");
	}
}

void pcb_dad_spin_free(pcb_hid_attribute_t *attr)
{
	if (attr->type == PCB_HATT_END) {
		pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->enumerations;
		free(spin);
	}
}

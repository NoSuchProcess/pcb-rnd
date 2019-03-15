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
#include "hid_dad_spin.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "conf_core.h"

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
	if (pcb_gui->attr_dlg_set_help != NULL)
		pcb_gui->attr_dlg_set_help(hid_ctx, spin->wwarn, msg);
}

static char *gen_str_coord(pcb_hid_dad_spin_t *spin, pcb_coord_t c, char *buf, int buflen)
{
	const pcb_unit_t *unit;
	if (spin->unit != NULL)
		unit = spin->unit;
	else
		unit = conf_core.editor.grid_unit;
	if (buf != NULL) {
		pcb_snprintf(buf, buflen, "%$m*", unit->suffix, c);
		return buf;
	}
	return pcb_strdup_printf("%$m*", unit->suffix, c);
}

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_hid_attribute_t *end;
	int wout, wunit, wstick, wglob, valid;
	char buf[128];
} spin_unit_t;

static void spin_unit_chg_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_attr_val_t hv;
	spin_unit_t *su = (spin_unit_t *)caller_data;
	const pcb_unit_t *unit;
	int unum = su->dlg[su->wunit].default_val.int_value;

	if ((!su->dlg[su->wglob].default_val.int_value) && (unum >= 0) && (unum < pcb_get_n_units()))
		unit = &pcb_units[unum];
	else
		unit = conf_core.editor.grid_unit;

	pcb_snprintf(su->buf, sizeof(su->buf), "%$m*", unit->suffix, su->end->default_val.coord_value);
	hv.str_value = su->buf;
	pcb_gui->attr_dlg_set_value(hid_ctx, su->wout, &hv);
	su->valid = 1;
}

static void spin_unit_dialog(void *spin_hid_ctx, pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *end, pcb_hid_attribute_t *str)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	spin_unit_t ctx;
	int dlgres;

	memset(&ctx, 0, sizeof(ctx));
	ctx.end = end;

	PCB_DAD_BEGIN_VBOX(ctx.dlg);
		PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(ctx.dlg, 2);
			PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_FRAME);
			PCB_DAD_LABEL(ctx.dlg, "Original:");
			PCB_DAD_LABEL(ctx.dlg, str->default_val.str_value);
			PCB_DAD_LABEL(ctx.dlg, "With new unit:");
			PCB_DAD_LABEL(ctx.dlg, "");
				ctx.wout = PCB_DAD_CURRENT(ctx.dlg);
		PCB_DAD_END(ctx.dlg);


		PCB_DAD_BEGIN_TABLE(ctx.dlg, 2);
			PCB_DAD_LABEL(ctx.dlg, "Preferred unit");
			PCB_DAD_UNIT(ctx.dlg);
				ctx.wunit = PCB_DAD_CURRENT(ctx.dlg);
				PCB_DAD_HELP(ctx.dlg, "Convert value to this unit and rewrite\nthe text entry field with the converted value.");
				PCB_DAD_CHANGE_CB(ctx.dlg, spin_unit_chg_cb);

			PCB_DAD_LABEL(ctx.dlg, "Use the global");
			PCB_DAD_BOOL(ctx.dlg, "");
				PCB_DAD_HELP(ctx.dlg, "Ignore the above unit selection,\nuse the global unit (grid unit) in this spinbox,\nfollow changes of the global unit");
				ctx.wglob = PCB_DAD_CURRENT(ctx.dlg);
				PCB_DAD_DEFAULT_NUM(ctx.dlg, (spin->unit == NULL));
				PCB_DAD_CHANGE_CB(ctx.dlg, spin_unit_chg_cb);

			PCB_DAD_LABEL(ctx.dlg, "Stick to unit");
			PCB_DAD_BOOL(ctx.dlg, "");
				PCB_DAD_HELP(ctx.dlg, "Upon any update from software, switch back to\the selected unit even if the user specified\na different unit in the text field.");
				ctx.wstick = PCB_DAD_CURRENT(ctx.dlg);
				PCB_DAD_DEFAULT_NUM(ctx.dlg, spin->no_unit_chg);
				PCB_DAD_CHANGE_CB(ctx.dlg, spin_unit_chg_cb);
		PCB_DAD_END(ctx.dlg);

		PCB_DAD_BEGIN_HBOX(ctx.dlg);
			PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_HBOX(ctx.dlg);
				PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx.dlg);
			PCB_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
		PCB_DAD_END(ctx.dlg);
	PCB_DAD_END(ctx.dlg);

	PCB_DAD_AUTORUN("unit", ctx.dlg, "spinbox coord unit change", &ctx, dlgres);
	if ((dlgres == 0) && (ctx.valid)) {
		pcb_hid_attr_val_t hv;
		int unum = ctx.dlg[ctx.wunit].default_val.int_value;

		if ((!ctx.dlg[ctx.wglob].default_val.int_value) && (unum >= 0) && (unum < pcb_get_n_units()))
				spin->unit = &pcb_units[unum];
			else
				spin->unit = NULL;

		spin->no_unit_chg = ctx.dlg[ctx.wstick].default_val.int_value;
		hv.str_value = pcb_strdup(ctx.buf);
		pcb_gui->attr_dlg_set_value(spin_hid_ctx, spin->wstr, &hv);
	}

	PCB_DAD_FREE(ctx.dlg);
}

static double get_step(pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *end, pcb_hid_attribute_t *str)
{
	double v, step;
	const pcb_unit_t *unit;

	if (spin->step > 0)
		return spin->step;

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			step = pow(10, floor(log10(fabs(end->default_val.int_value)) - 1.0));
			if (step < 1)
				step = 1;
			break;
		case PCB_DAD_SPIN_DOUBLE:
			step = pow(10, floor(log10(fabs(end->default_val.real_value)) - 1.0));
			break;
		case PCB_DAD_SPIN_COORD:
			if (spin->unit == NULL) {
				pcb_bool succ = pcb_get_value_unit(str->default_val.str_value, NULL, 0, &v, &unit);
				if (!succ) {
					v = end->default_val.coord_value;
					unit = conf_core.editor.grid_unit;
				}
			}
			else
				unit = spin->unit;
			v = pcb_coord_to_unit(unit, end->default_val.coord_value);
			step = pow(10, floor(log10(fabs(v)) - 1.0));
			step = pcb_unit_to_coord(unit, step);
			break;
	}
	return step;
}

#define SPIN_CLAMP(dst) \
	do { \
		if ((spin->vmin_valid) && (dst < spin->vmin)) { \
			dst = spin->vmin; \
			warn = "Value already at the minimum"; \
		} \
		if ((spin->vmax_valid) && (dst > spin->vmax)) { \
			dst = spin->vmax; \
			warn = "Value already at the maximum"; \
		} \
	} while(0)

static void do_step(void *hid_ctx, pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *str, pcb_hid_attribute_t *end, double step)
{
	pcb_hid_attr_val_t hv;
	const char *warn = NULL;
	char buf[128];

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			end->default_val.int_value += step;
			SPIN_CLAMP(end->default_val.int_value);
			sprintf(buf, "%d", end->default_val.int_value);
			break;
		case PCB_DAD_SPIN_DOUBLE:
			end->default_val.real_value += step;
			SPIN_CLAMP(end->default_val.real_value);
			sprintf(buf, "%f", end->default_val.real_value);
			break;
		case PCB_DAD_SPIN_COORD:
			end->default_val.coord_value += step;
			SPIN_CLAMP(end->default_val.coord_value);
			gen_str_coord(spin, end->default_val.coord_value, buf, sizeof(buf));
			break;
	}

	spin_warn(hid_ctx, spin, end, warn);
	hv.str_value = pcb_strdup(buf);
	pcb_gui->attr_dlg_set_value(hid_ctx, spin->wstr, &hv);
}

void pcb_dad_spin_up_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wup + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wup + spin->cmp.wend;

	do_step(hid_ctx, spin, str, end, get_step(spin, end, str));
	spin_changed(hid_ctx, caller_data, spin, end);
}

void pcb_dad_spin_down_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wdown + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wdown + spin->cmp.wend;

	do_step(hid_ctx, spin, str, end, -get_step(spin, end, str));
	spin_changed(hid_ctx, caller_data, spin, end);
}

void pcb_dad_spin_txt_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr;
	pcb_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;
	char *ends, *warn = NULL;
	long l;
	double d;
	pcb_bool succ, absolute;
	const pcb_unit_t *unit;

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			l = strtol(str->default_val.str_value, &ends, 10);
			SPIN_CLAMP(l);
			if (*ends != '\0')
				warn = "Invalid integer - result is truncated";
			end->default_val.int_value = l;
			break;
		case PCB_DAD_SPIN_DOUBLE:
			d = strtod(str->default_val.str_value, &ends);
			SPIN_CLAMP(d);
			if (*ends != '\0')
				warn = "Invalid numeric - result is truncated";
			end->default_val.real_value = d;
			break;
		case PCB_DAD_SPIN_COORD:
			succ = pcb_get_value_unit(str->default_val.str_value, &absolute, 0, &d, &unit);
			if (succ)
				SPIN_CLAMP(d);
			else
				warn = "Invalid coord value or unit - result is truncated";
			if (!spin->no_unit_chg)
				spin->unit = unit;
			end->default_val.coord_value = d;
			break;
		default: pcb_trace("INTERNAL ERROR: spin_set_num\n");
	}

	spin_warn(hid_ctx, spin, end, warn);
	spin_changed(hid_ctx, caller_data, spin, end);
}

void pcb_dad_spin_unit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wunit + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wunit + spin->cmp.wend;
	spin_unit_dialog(hid_ctx, spin, end, str);
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
		case PCB_DAD_SPIN_DOUBLE:
			attr->default_val.real_value = d;
			free((char *)str->default_val.str_value);
			str->default_val.str_value = pcb_strdup_printf("%f", d);
			break;
		case PCB_DAD_SPIN_COORD:
			attr->default_val.coord_value = c;
			spin->unit = NULL;
			free((char *)str->default_val.str_value);
			str->default_val.str_value = gen_str_coord(spin, c, NULL, 0);
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

int pcb_dad_spin_widget_state(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool enabled)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)end->enumerations;
	return pcb_gui->attr_dlg_widget_state(hid_ctx, spin->wall, enabled);
}

int pcb_dad_spin_widget_hide(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool hide)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)end->enumerations;
	return pcb_gui->attr_dlg_widget_hide(hid_ctx, spin->wall, hide);
}

int pcb_dad_spin_set_value(pcb_hid_attribute_t *end, void *hid_ctx, int idx, const pcb_hid_attr_val_t *val)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)end->enumerations;
	pcb_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	/* do not modify the text field if the value is the same */
	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			if (val->int_value == end->default_val.int_value)
				return 0;
			end->default_val.int_value = val->int_value;
			break;
		case PCB_DAD_SPIN_DOUBLE:
			if (val->real_value == end->default_val.real_value)
				return 0;
			end->default_val.real_value = val->real_value;
			break;
		case PCB_DAD_SPIN_COORD:
			if (val->coord_value == end->default_val.coord_value)
				return 0;
			end->default_val.coord_value = val->coord_value;
			break;
	}
	do_step(hid_ctx, spin, str, end, 0); /* cheap conversion + error checks */
	return 0;
}

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

#include <ctype.h>

#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_spin.h>
#include <librnd/core/hid_dad_unit.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/hidlib_conf.h>

gdl_list_t pcb_dad_coord_spins;

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
"4 4 2 1",
" 	c None",
"+	c #000000",
"+  +",
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
	const char *s;
	pcb_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	end->changed = 1;

	/* determine whether textual input is empty and indicate that in the compound end widget */
	s = str->val.str;
	if (s == NULL) s = "";
	while(isspace(*s)) s++;
	end->empty = (*s == '\0');

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
		unit = pcbhl_conf.editor.grid_unit;
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
	int unum = su->dlg[su->wunit].val.lng;
	int can_glob = su->wglob > 0;
	int is_globbing = &su->dlg[su->wglob] == attr;

	if (!can_glob)
		unit = &pcb_units[unum];
	else if ((!is_globbing) && (unum >= 0) && (unum < pcb_get_n_units(0)))
		unit = &pcb_units[unum];
	else
		unit = pcbhl_conf.editor.grid_unit;

	if (is_globbing && su->dlg[su->wglob].val.lng) {
		/* global ticked in: also set the unit by force */
		unum = pcbhl_conf.editor.grid_unit - pcb_units;
		hv.lng = unum;
		pcb_gui->attr_dlg_set_value(hid_ctx, su->wunit, &hv);
	}

	pcb_snprintf(su->buf, sizeof(su->buf), "%$m*", unit->suffix, su->end->val.crd);
	hv.str = su->buf;
	pcb_gui->attr_dlg_set_value(hid_ctx, su->wout, &hv);
	if (!is_globbing && can_glob) {
		/* unit changed: disable global, accept the user wants to use this unit */
		hv.lng = 0;
		pcb_gui->attr_dlg_set_value(hid_ctx, su->wglob, &hv);
	}
	su->valid = 1;
}

static void spin_unit_dialog(void *spin_hid_ctx, pcb_hid_dad_spin_t *spin, pcb_hid_attribute_t *end, pcb_hid_attribute_t *str)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	spin_unit_t ctx;
	const pcb_unit_t *def_unit;
	int dlgfail;

	memset(&ctx, 0, sizeof(ctx));
	ctx.end = end;

	def_unit = spin->unit == NULL ? pcbhl_conf.editor.grid_unit : spin->unit;

	PCB_DAD_BEGIN_VBOX(ctx.dlg);
		PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(ctx.dlg, 2);
			PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_FRAME);
			PCB_DAD_LABEL(ctx.dlg, "Original:");
			PCB_DAD_LABEL(ctx.dlg, str->val.str);
			PCB_DAD_LABEL(ctx.dlg, "With new unit:");
			PCB_DAD_LABEL(ctx.dlg, "");
				ctx.wout = PCB_DAD_CURRENT(ctx.dlg);
		PCB_DAD_END(ctx.dlg);


		PCB_DAD_BEGIN_TABLE(ctx.dlg, 2);
			PCB_DAD_LABEL(ctx.dlg, "Preferred unit");
			PCB_DAD_UNIT(ctx.dlg, spin->unit_family);
				ctx.wunit = PCB_DAD_CURRENT(ctx.dlg);
				PCB_DAD_HELP(ctx.dlg, "Convert value to this unit and rewrite\nthe text entry field with the converted value.");
				PCB_DAD_DEFAULT_PTR(ctx.dlg, def_unit);
				PCB_DAD_CHANGE_CB(ctx.dlg, spin_unit_chg_cb);
			
			if (spin->unit_family == (PCB_UNIT_METRIC | PCB_UNIT_IMPERIAL)) {
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
			}
			else {
				ctx.wglob = -1;
				ctx.wstick = -1;
			}

		PCB_DAD_END(ctx.dlg);

		PCB_DAD_BEGIN_HBOX(ctx.dlg);
			PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_HBOX(ctx.dlg);
				PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx.dlg);
			PCB_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
		PCB_DAD_END(ctx.dlg);
	PCB_DAD_END(ctx.dlg);

	PCB_DAD_AUTORUN("unit", ctx.dlg, "spinbox coord unit change", &ctx, dlgfail);
	if ((dlgfail == 0) && (ctx.valid)) {
		pcb_hid_attr_val_t hv;
		int unum = ctx.dlg[ctx.wunit].val.lng;
		int can_glob = (spin->unit_family == (PCB_UNIT_METRIC | PCB_UNIT_IMPERIAL));

		if (!can_glob)
			spin->unit = &pcb_units[unum];
		else if ((!ctx.dlg[ctx.wglob].val.lng) && (unum >= 0) && (unum < pcb_get_n_units(0)))
			spin->unit = &pcb_units[unum];
		else
			spin->unit = NULL;

		if (can_glob)
			spin->no_unit_chg = ctx.dlg[ctx.wstick].val.lng;
		else
			spin->no_unit_chg = 1;

		hv.str = pcb_strdup(ctx.buf);
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
			step = pow(10, floor(log10(fabs((double)end->val.lng)) - 1.0));
			if (step < 1)
				step = 1;
			break;
		case PCB_DAD_SPIN_DOUBLE:
		case PCB_DAD_SPIN_FREQ:
			step = pow(10, floor(log10(fabs((double)end->val.dbl)) - 1.0));
			break;
		case PCB_DAD_SPIN_COORD:
			if (spin->unit == NULL) {
				pcb_bool succ = 0;
				if (str->val.str != NULL)
					succ = pcb_get_value_unit(str->val.str, NULL, 0, &v, &unit);
				if (!succ) {
					v = end->val.crd;
					unit = pcbhl_conf.editor.grid_unit;
				}
			}
			else
				unit = spin->unit;
			v = pcb_coord_to_unit(unit, end->val.crd);
			step = pow(10, floor(log10(fabs(v)) - 1.0));
			if (step <= 0.0)
				step = 1;
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
			end->val.lng += step;
			SPIN_CLAMP(end->val.lng);
			sprintf(buf, "%ld", end->val.lng);
			break;
		case PCB_DAD_SPIN_DOUBLE:
		case PCB_DAD_SPIN_FREQ:
			end->val.dbl += step;
			SPIN_CLAMP(end->val.dbl);
			sprintf(buf, "%f", end->val.dbl);
			break;
		case PCB_DAD_SPIN_COORD:
			end->val.crd += step;
			SPIN_CLAMP(end->val.crd);
			spin->last_good_crd = end->val.crd;
			gen_str_coord(spin, end->val.crd, buf, sizeof(buf));
			break;
	}

	spin_warn(hid_ctx, spin, end, warn);
	hv.str = pcb_strdup(buf);
	spin->set_writeback_lock++;
	pcb_gui->attr_dlg_set_value(hid_ctx, spin->wstr, &hv);
	spin->set_writeback_lock--;
}

void pcb_dad_spin_up_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wup + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wup + spin->cmp.wend;

	pcb_dad_spin_txt_enter_cb_dry(hid_ctx, caller_data, str); /* fix up missing unit */

	do_step(hid_ctx, spin, str, end, get_step(spin, end, str));
	spin_changed(hid_ctx, caller_data, spin, end);
	pcb_dad_spin_txt_enter_call_users(hid_ctx, end);
}

void pcb_dad_spin_down_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr - spin->wdown + spin->wstr;
	pcb_hid_attribute_t *end = attr - spin->wdown + spin->cmp.wend;

	pcb_dad_spin_txt_enter_cb_dry(hid_ctx, caller_data, str); /* fix up missing unit */

	do_step(hid_ctx, spin, str, end, -get_step(spin, end, str));
	spin_changed(hid_ctx, caller_data, spin, end);
	pcb_dad_spin_txt_enter_call_users(hid_ctx, end);
}

void pcb_dad_spin_txt_change_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr;
	pcb_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;
	char *ends, *warn = NULL;
	long l;
	double d;
	pcb_bool succ, absolute;
	const pcb_unit_t *unit;

	if (spin->set_writeback_lock)
		return;

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			l = strtol(str->val.str, &ends, 10);
			SPIN_CLAMP(l);
			if (*ends != '\0')
				warn = "Invalid integer - result is truncated";
			end->val.lng = l;
			break;
		case PCB_DAD_SPIN_DOUBLE:
			d = strtod(str->val.str, &ends);
			SPIN_CLAMP(d);
			if (*ends != '\0')
				warn = "Invalid numeric - result is truncated";
			end->val.dbl = d;
			break;
		case PCB_DAD_SPIN_COORD:
			succ = pcb_get_value_unit(str->val.str, &absolute, 0, &d, &unit);
			if (succ) {
				SPIN_CLAMP(d);
				end->val.crd = d;
				spin->last_good_crd = d;
			}
			else {
				warn = "Invalid coord value or unit - result is truncated";
				end->val.crd = spin->last_good_crd;
			}
			if (!spin->no_unit_chg)
				spin->unit = unit;
			break;
		default: pcb_trace("INTERNAL ERROR: spin_set_num\n");
	}

	spin_warn(hid_ctx, spin, end, warn);
	spin_changed(hid_ctx, caller_data, spin, end);
}

void pcb_dad_spin_txt_enter_cb_dry(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *str = attr;
	pcb_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;
	const char *inval;
	char *ends, *warn = NULL;
	int changed = 0;
	double d;
	pcb_bool succ, absolute;
	const pcb_unit_t *unit;

	if (spin->set_writeback_lock)
		return;

	switch(spin->type) {
		case PCB_DAD_SPIN_COORD:
			inval = str->val.str;
			while(isspace(*inval)) inval++;
			if (*inval == '\0')
				inval = "0";
			succ = pcb_get_value_unit(inval, &absolute, 0, &d, &unit);
			if (succ)
				break;
			strtod(inval, &ends);
			while(isspace(*ends)) ends++;
			if (*ends == '\0') {
				pcb_hid_attr_val_t hv;
				char *tmp = pcb_concat(inval, " ", pcbhl_conf.editor.grid_unit->suffix, NULL);

				changed = 1;
				hv.str = tmp;
				spin->set_writeback_lock++;
				pcb_gui->attr_dlg_set_value(hid_ctx, spin->wstr, &hv);
				spin->set_writeback_lock--;
				succ = pcb_get_value_unit(str->val.str, &absolute, 0, &d, &unit);
				if (succ) {
					end->val.crd = d;
					spin->last_good_crd = d;
				}
				free(tmp);
			}
			break;
		default:
			/* don't do anything extra for the rest */
			break;
	}

	if (changed) {
		spin_warn(hid_ctx, spin, end, warn);
		spin_changed(hid_ctx, caller_data, spin, end);
	}
}

void pcb_dad_spin_txt_enter_call_users(void *hid_ctx, pcb_hid_attribute_t *end)
{
	struct {
		void *caller_data;
	} *hc = hid_ctx;

	if (end->enter_cb != NULL)
		end->enter_cb(hid_ctx, hc->caller_data, end);
}

void pcb_dad_spin_txt_enter_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_spin_t *spin = (pcb_hid_dad_spin_t *)attr->user_data;
	pcb_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;

	pcb_dad_spin_txt_enter_cb_dry(hid_ctx, caller_data, attr);
	pcb_dad_spin_txt_enter_call_users(hid_ctx, end);
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
	pcb_hid_dad_spin_t *spin = attr->wdata;
	pcb_hid_attribute_t *str = attr - spin->cmp.wend + spin->wstr;

	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			attr->val.lng = l;
			free((char *)str->val.str);
			str->val.str = pcb_strdup_printf("%ld", l);
			break;
		case PCB_DAD_SPIN_DOUBLE:
			attr->val.dbl = d;
			free((char *)str->val.str);
			str->val.str = pcb_strdup_printf("%f", d);
			break;
		case PCB_DAD_SPIN_COORD:
			attr->val.crd = c;
			spin->last_good_crd = c;
			spin->unit = NULL;
			free((char *)str->val.str);
			str->val.str = gen_str_coord(spin, c, NULL, 0);
			break;
		default: pcb_trace("INTERNAL ERROR: spin_set_num\n");
	}
}

void pcb_dad_spin_free(pcb_hid_attribute_t *attr)
{
	if (attr->type == PCB_HATT_END) {
		pcb_hid_dad_spin_t *spin = attr->wdata;
		if (spin->type == PCB_DAD_SPIN_COORD)
			gdl_remove(&pcb_dad_coord_spins, spin, link);
		free(spin);
	}
}

int pcb_dad_spin_widget_state(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool enabled)
{
	pcb_hid_dad_spin_t *spin = end->wdata;
	return pcb_gui->attr_dlg_widget_state(hid_ctx, spin->wall, enabled);
}

int pcb_dad_spin_widget_hide(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool hide)
{
	pcb_hid_dad_spin_t *spin = end->wdata;
	return pcb_gui->attr_dlg_widget_hide(hid_ctx, spin->wall, hide);
}

int pcb_dad_spin_set_value(pcb_hid_attribute_t *end, void *hid_ctx, int idx, const pcb_hid_attr_val_t *val)
{
	pcb_hid_dad_spin_t *spin = end->wdata;
	pcb_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	/* do not modify the text field if the value is the same */
	switch(spin->type) {
		case PCB_DAD_SPIN_INT:
			if (val->lng == end->val.lng)
				return 0;
			end->val.lng = val->lng;
			break;
		case PCB_DAD_SPIN_DOUBLE:
		case PCB_DAD_SPIN_FREQ:
			if (val->dbl == end->val.dbl)
				return 0;
			end->val.dbl = val->dbl;
			break;
		case PCB_DAD_SPIN_COORD:
			if (val->crd == end->val.crd)
				return 0;
			end->val.crd = val->crd;
			spin->last_good_crd = val->crd;
			break;
	}
	do_step(hid_ctx, spin, str, end, 0); /* cheap conversion + error checks */
	return 0;
}

void pcb_dad_spin_set_help(pcb_hid_attribute_t *end, const char *help)
{
	pcb_hid_dad_spin_t *spin = end->wdata;
	pcb_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	if ((spin->hid_ctx == NULL) || (*spin->hid_ctx == NULL)) /* while building */
		str->help_text = help;
	else if (pcb_gui->attr_dlg_set_help != NULL) /* when the dialog is already running */
		pcb_gui->attr_dlg_set_help(*spin->hid_ctx, spin->wstr, help);
}

void pcb_dad_spin_update_global_coords(void)
{
	pcb_hid_dad_spin_t *spin;


	for(spin = gdl_first(&pcb_dad_coord_spins); spin != NULL; spin = gdl_next(&pcb_dad_coord_spins, spin)) {
		pcb_hid_attribute_t *dlg;
		void *hid_ctx;

		if ((spin->unit != NULL) || (spin->attrs == NULL) || (*spin->attrs == NULL) || (spin->hid_ctx == NULL) || (*spin->hid_ctx == NULL))
			continue;

		dlg = *spin->attrs;
		hid_ctx = *spin->hid_ctx;
		do_step(hid_ctx, spin, &dlg[spin->wstr], &dlg[spin->cmp.wend], 0); /* cheap conversion*/
	}
}

void pcb_dad_spin_update_internal(pcb_hid_dad_spin_t *spin)
{
	pcb_hid_attribute_t *dlg = *spin->attrs, *end = dlg+spin->cmp.wend;
	pcb_hid_attribute_t *str = dlg + spin->wstr;
	void *hid_ctx = *spin->hid_ctx;
	char buf[128];

	if (hid_ctx == NULL) /* before run() */
		str->val.str = pcb_strdup(gen_str_coord(spin, end->val.crd, buf, sizeof(buf)));
	else
		do_step(hid_ctx, spin, &dlg[spin->wstr], &dlg[spin->cmp.wend], 0); /* cheap conversion*/
}

void pcb_dad_spin_set_geo(pcb_hid_attribute_t *end, pcb_hatt_compflags_t flg, int geo)
{
	pcb_hid_dad_spin_t *spin = end->wdata;
	pcb_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	if (flg == PCB_HATF_HEIGHT_CHR) {
		str->hatt_flags |= PCB_HATF_HEIGHT_CHR;
		str->geo_width = geo;
	}
}

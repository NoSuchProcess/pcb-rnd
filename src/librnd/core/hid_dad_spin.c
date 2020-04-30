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

gdl_list_t rnd_dad_coord_spins;

const char *rnd_hid_dad_spin_up[] = {
"5 3 2 1",
" 	c None",
"+	c #000000",
"  +  ",
" +++ ",
"+++++",
};

const char *rnd_hid_dad_spin_down[] = {
"5 3 2 1",
" 	c None",
"+	c #000000",
"+++++",
" +++ ",
"  +  ",
};

const char *rnd_hid_dad_spin_unit[] = {
"4 4 2 1",
" 	c None",
"+	c #000000",
"+  +",
"+  +",
"+  +",
" ++ ",
};

const char *rnd_hid_dad_spin_warn[] = {
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

static void spin_changed(void *hid_ctx, void *caller_data, rnd_hid_dad_spin_t *spin, rnd_hid_attribute_t *end)
{
	const char *s;
	rnd_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	end->changed = 1;

	/* determine whether textual input is empty and indicate that in the compound end widget */
	s = str->val.str;
	if (s == NULL) s = "";
	while(isspace(*s)) s++;
	end->empty = (*s == '\0');

	if (end->change_cb != NULL)
		end->change_cb(hid_ctx, caller_data, end);
}

static void spin_warn(void *hid_ctx, rnd_hid_dad_spin_t *spin, rnd_hid_attribute_t *end, const char *msg)
{
	rnd_gui->attr_dlg_widget_hide(hid_ctx, spin->wwarn, (msg == NULL));
	if (rnd_gui->attr_dlg_set_help != NULL)
		rnd_gui->attr_dlg_set_help(hid_ctx, spin->wwarn, msg);
}

static char *gen_str_coord(rnd_hid_dad_spin_t *spin, rnd_coord_t c, char *buf, int buflen)
{
	const rnd_unit_t *unit;
	if (spin->unit != NULL)
		unit = spin->unit;
	else
		unit = rnd_conf.editor.grid_unit;
	if (buf != NULL) {
		rnd_snprintf(buf, buflen, "%$m*", unit->suffix, c);
		return buf;
	}
	return rnd_strdup_printf("%$m*", unit->suffix, c);
}

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	rnd_hid_attribute_t *end;
	int wout, wunit, wstick, wglob, valid;
	char buf[128];
} spin_unit_t;

static void spin_unit_chg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_attr_val_t hv;
	spin_unit_t *su = (spin_unit_t *)caller_data;
	const rnd_unit_t *unit;
	int unum = su->dlg[su->wunit].val.lng;
	int can_glob = su->wglob > 0;
	int is_globbing = &su->dlg[su->wglob] == attr;

	if (!can_glob)
		unit = &pcb_units[unum];
	else if ((!is_globbing) && (unum >= 0) && (unum < pcb_get_n_units(0)))
		unit = &pcb_units[unum];
	else
		unit = rnd_conf.editor.grid_unit;

	if (is_globbing && su->dlg[su->wglob].val.lng) {
		/* global ticked in: also set the unit by force */
		unum = rnd_conf.editor.grid_unit - pcb_units;
		hv.lng = unum;
		rnd_gui->attr_dlg_set_value(hid_ctx, su->wunit, &hv);
	}

	rnd_snprintf(su->buf, sizeof(su->buf), "%$m*", unit->suffix, su->end->val.crd);
	hv.str = su->buf;
	rnd_gui->attr_dlg_set_value(hid_ctx, su->wout, &hv);
	if (!is_globbing && can_glob) {
		/* unit changed: disable global, accept the user wants to use this unit */
		hv.lng = 0;
		rnd_gui->attr_dlg_set_value(hid_ctx, su->wglob, &hv);
	}
	su->valid = 1;
}

static void spin_unit_dialog(void *spin_hid_ctx, rnd_hid_dad_spin_t *spin, rnd_hid_attribute_t *end, rnd_hid_attribute_t *str)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	spin_unit_t ctx;
	const rnd_unit_t *def_unit;
	int dlgfail;

	memset(&ctx, 0, sizeof(ctx));
	ctx.end = end;

	def_unit = spin->unit == NULL ? rnd_conf.editor.grid_unit : spin->unit;

	RND_DAD_BEGIN_VBOX(ctx.dlg);
		RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABLE(ctx.dlg, 2);
			RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_FRAME);
			RND_DAD_LABEL(ctx.dlg, "Original:");
			RND_DAD_LABEL(ctx.dlg, str->val.str);
			RND_DAD_LABEL(ctx.dlg, "With new unit:");
			RND_DAD_LABEL(ctx.dlg, "");
				ctx.wout = RND_DAD_CURRENT(ctx.dlg);
		RND_DAD_END(ctx.dlg);


		RND_DAD_BEGIN_TABLE(ctx.dlg, 2);
			RND_DAD_LABEL(ctx.dlg, "Preferred unit");
			PCB_DAD_UNIT(ctx.dlg, spin->unit_family);
				ctx.wunit = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_HELP(ctx.dlg, "Convert value to this unit and rewrite\nthe text entry field with the converted value.");
				RND_DAD_DEFAULT_PTR(ctx.dlg, def_unit);
				RND_DAD_CHANGE_CB(ctx.dlg, spin_unit_chg_cb);
			
			if (spin->unit_family == (PCB_UNIT_METRIC | PCB_UNIT_IMPERIAL)) {
				RND_DAD_LABEL(ctx.dlg, "Use the global");
				RND_DAD_BOOL(ctx.dlg, "");
					RND_DAD_HELP(ctx.dlg, "Ignore the above unit selection,\nuse the global unit (grid unit) in this spinbox,\nfollow changes of the global unit");
					ctx.wglob = RND_DAD_CURRENT(ctx.dlg);
					RND_DAD_DEFAULT_NUM(ctx.dlg, (spin->unit == NULL));
					RND_DAD_CHANGE_CB(ctx.dlg, spin_unit_chg_cb);

				RND_DAD_LABEL(ctx.dlg, "Stick to unit");
				RND_DAD_BOOL(ctx.dlg, "");
					RND_DAD_HELP(ctx.dlg, "Upon any update from software, switch back to\the selected unit even if the user specified\na different unit in the text field.");
					ctx.wstick = RND_DAD_CURRENT(ctx.dlg);
					RND_DAD_DEFAULT_NUM(ctx.dlg, spin->no_unit_chg);
					RND_DAD_CHANGE_CB(ctx.dlg, spin_unit_chg_cb);
			}
			else {
				ctx.wglob = -1;
				ctx.wstick = -1;
			}

		RND_DAD_END(ctx.dlg);

		RND_DAD_BEGIN_HBOX(ctx.dlg);
			RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_BEGIN_HBOX(ctx.dlg);
				RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ctx.dlg);
			RND_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
		RND_DAD_END(ctx.dlg);
	RND_DAD_END(ctx.dlg);

	RND_DAD_AUTORUN("unit", ctx.dlg, "spinbox coord unit change", &ctx, dlgfail);
	if ((dlgfail == 0) && (ctx.valid)) {
		rnd_hid_attr_val_t hv;
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

		hv.str = rnd_strdup(ctx.buf);
		rnd_gui->attr_dlg_set_value(spin_hid_ctx, spin->wstr, &hv);
	}

	RND_DAD_FREE(ctx.dlg);
}

static double get_step(rnd_hid_dad_spin_t *spin, rnd_hid_attribute_t *end, rnd_hid_attribute_t *str)
{
	double v, step;
	const rnd_unit_t *unit;

	if (spin->step > 0)
		return spin->step;

	switch(spin->type) {
		case RND_DAD_SPIN_INT:
			step = pow(10, floor(log10(fabs((double)end->val.lng)) - 1.0));
			if (step < 1)
				step = 1;
			break;
		case RND_DAD_SPIN_DOUBLE:
		case RND_DAD_SPIN_FREQ:
			step = pow(10, floor(log10(fabs((double)end->val.dbl)) - 1.0));
			break;
		case RND_DAD_SPIN_COORD:
			if (spin->unit == NULL) {
				rnd_bool succ = 0;
				if (str->val.str != NULL)
					succ = rnd_get_value_unit(str->val.str, NULL, 0, &v, &unit);
				if (!succ) {
					v = end->val.crd;
					unit = rnd_conf.editor.grid_unit;
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

static void do_step(void *hid_ctx, rnd_hid_dad_spin_t *spin, rnd_hid_attribute_t *str, rnd_hid_attribute_t *end, double step)
{
	rnd_hid_attr_val_t hv;
	const char *warn = NULL;
	char buf[128];

	switch(spin->type) {
		case RND_DAD_SPIN_INT:
			end->val.lng += step;
			SPIN_CLAMP(end->val.lng);
			sprintf(buf, "%ld", end->val.lng);
			break;
		case RND_DAD_SPIN_DOUBLE:
		case RND_DAD_SPIN_FREQ:
			end->val.dbl += step;
			SPIN_CLAMP(end->val.dbl);
			sprintf(buf, "%f", end->val.dbl);
			break;
		case RND_DAD_SPIN_COORD:
			end->val.crd += step;
			SPIN_CLAMP(end->val.crd);
			spin->last_good_crd = end->val.crd;
			gen_str_coord(spin, end->val.crd, buf, sizeof(buf));
			break;
	}

	spin_warn(hid_ctx, spin, end, warn);
	hv.str = rnd_strdup(buf);
	spin->set_writeback_lock++;
	rnd_gui->attr_dlg_set_value(hid_ctx, spin->wstr, &hv);
	spin->set_writeback_lock--;
}

void rnd_dad_spin_up_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_dad_spin_t *spin = (rnd_hid_dad_spin_t *)attr->user_data;
	rnd_hid_attribute_t *str = attr - spin->wup + spin->wstr;
	rnd_hid_attribute_t *end = attr - spin->wup + spin->cmp.wend;

	rnd_dad_spin_txt_enter_cb_dry(hid_ctx, caller_data, str); /* fix up missing unit */

	do_step(hid_ctx, spin, str, end, get_step(spin, end, str));
	spin_changed(hid_ctx, caller_data, spin, end);
	rnd_dad_spin_txt_enter_call_users(hid_ctx, end);
}

void rnd_dad_spin_down_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_dad_spin_t *spin = (rnd_hid_dad_spin_t *)attr->user_data;
	rnd_hid_attribute_t *str = attr - spin->wdown + spin->wstr;
	rnd_hid_attribute_t *end = attr - spin->wdown + spin->cmp.wend;

	rnd_dad_spin_txt_enter_cb_dry(hid_ctx, caller_data, str); /* fix up missing unit */

	do_step(hid_ctx, spin, str, end, -get_step(spin, end, str));
	spin_changed(hid_ctx, caller_data, spin, end);
	rnd_dad_spin_txt_enter_call_users(hid_ctx, end);
}

void rnd_dad_spin_txt_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_dad_spin_t *spin = (rnd_hid_dad_spin_t *)attr->user_data;
	rnd_hid_attribute_t *str = attr;
	rnd_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;
	char *ends, *warn = NULL;
	long l;
	double d;
	rnd_bool succ, absolute;
	const rnd_unit_t *unit;

	if (spin->set_writeback_lock)
		return;

	switch(spin->type) {
		case RND_DAD_SPIN_INT:
			l = strtol(str->val.str, &ends, 10);
			SPIN_CLAMP(l);
			if (*ends != '\0')
				warn = "Invalid integer - result is truncated";
			end->val.lng = l;
			break;
		case RND_DAD_SPIN_DOUBLE:
			d = strtod(str->val.str, &ends);
			SPIN_CLAMP(d);
			if (*ends != '\0')
				warn = "Invalid numeric - result is truncated";
			end->val.dbl = d;
			break;
		case RND_DAD_SPIN_COORD:
			succ = rnd_get_value_unit(str->val.str, &absolute, 0, &d, &unit);
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
		default: rnd_trace("INTERNAL ERROR: spin_set_num\n");
	}

	spin_warn(hid_ctx, spin, end, warn);
	spin_changed(hid_ctx, caller_data, spin, end);
}

void rnd_dad_spin_txt_enter_cb_dry(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_dad_spin_t *spin = (rnd_hid_dad_spin_t *)attr->user_data;
	rnd_hid_attribute_t *str = attr;
	rnd_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;
	const char *inval;
	char *ends, *warn = NULL;
	int changed = 0;
	double d;
	rnd_bool succ, absolute;
	const rnd_unit_t *unit;

	if (spin->set_writeback_lock)
		return;

	switch(spin->type) {
		case RND_DAD_SPIN_COORD:
			inval = str->val.str;
			while(isspace(*inval)) inval++;
			if (*inval == '\0')
				inval = "0";
			succ = rnd_get_value_unit(inval, &absolute, 0, &d, &unit);
			if (succ)
				break;
			strtod(inval, &ends);
			while(isspace(*ends)) ends++;
			if (*ends == '\0') {
				rnd_hid_attr_val_t hv;
				char *tmp = rnd_concat(inval, " ", rnd_conf.editor.grid_unit->suffix, NULL);

				changed = 1;
				hv.str = tmp;
				spin->set_writeback_lock++;
				rnd_gui->attr_dlg_set_value(hid_ctx, spin->wstr, &hv);
				spin->set_writeback_lock--;
				succ = rnd_get_value_unit(str->val.str, &absolute, 0, &d, &unit);
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

void rnd_dad_spin_txt_enter_call_users(void *hid_ctx, rnd_hid_attribute_t *end)
{
	struct {
		void *caller_data;
	} *hc = hid_ctx;

	if (end->enter_cb != NULL)
		end->enter_cb(hid_ctx, hc->caller_data, end);
}

void rnd_dad_spin_txt_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_dad_spin_t *spin = (rnd_hid_dad_spin_t *)attr->user_data;
	rnd_hid_attribute_t *end = attr - spin->wstr + spin->cmp.wend;

	rnd_dad_spin_txt_enter_cb_dry(hid_ctx, caller_data, attr);
	rnd_dad_spin_txt_enter_call_users(hid_ctx, end);
}


void rnd_dad_spin_unit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_dad_spin_t *spin = (rnd_hid_dad_spin_t *)attr->user_data;
	rnd_hid_attribute_t *str = attr - spin->wunit + spin->wstr;
	rnd_hid_attribute_t *end = attr - spin->wunit + spin->cmp.wend;
	spin_unit_dialog(hid_ctx, spin, end, str);
}

void rnd_dad_spin_set_num(rnd_hid_attribute_t *attr, long l, double d, rnd_coord_t c)
{
	rnd_hid_dad_spin_t *spin = attr->wdata;
	rnd_hid_attribute_t *str = attr - spin->cmp.wend + spin->wstr;

	switch(spin->type) {
		case RND_DAD_SPIN_INT:
			attr->val.lng = l;
			free((char *)str->val.str);
			str->val.str = rnd_strdup_printf("%ld", l);
			break;
		case RND_DAD_SPIN_DOUBLE:
			attr->val.dbl = d;
			free((char *)str->val.str);
			str->val.str = rnd_strdup_printf("%f", d);
			break;
		case RND_DAD_SPIN_COORD:
			attr->val.crd = c;
			spin->last_good_crd = c;
			spin->unit = NULL;
			free((char *)str->val.str);
			str->val.str = gen_str_coord(spin, c, NULL, 0);
			break;
		default: rnd_trace("INTERNAL ERROR: spin_set_num\n");
	}
}

void rnd_dad_spin_free(rnd_hid_attribute_t *attr)
{
	if (attr->type == RND_HATT_END) {
		rnd_hid_dad_spin_t *spin = attr->wdata;
		if (spin->type == RND_DAD_SPIN_COORD)
			gdl_remove(&rnd_dad_coord_spins, spin, link);
		free(spin);
	}
}

int rnd_dad_spin_widget_state(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool enabled)
{
	rnd_hid_dad_spin_t *spin = end->wdata;
	return rnd_gui->attr_dlg_widget_state(hid_ctx, spin->wall, enabled);
}

int rnd_dad_spin_widget_hide(rnd_hid_attribute_t *end, void *hid_ctx, int idx, rnd_bool hide)
{
	rnd_hid_dad_spin_t *spin = end->wdata;
	return rnd_gui->attr_dlg_widget_hide(hid_ctx, spin->wall, hide);
}

int rnd_dad_spin_set_value(rnd_hid_attribute_t *end, void *hid_ctx, int idx, const rnd_hid_attr_val_t *val)
{
	rnd_hid_dad_spin_t *spin = end->wdata;
	rnd_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	/* do not modify the text field if the value is the same */
	switch(spin->type) {
		case RND_DAD_SPIN_INT:
			if (val->lng == end->val.lng)
				return 0;
			end->val.lng = val->lng;
			break;
		case RND_DAD_SPIN_DOUBLE:
		case RND_DAD_SPIN_FREQ:
			if (val->dbl == end->val.dbl)
				return 0;
			end->val.dbl = val->dbl;
			break;
		case RND_DAD_SPIN_COORD:
			if (val->crd == end->val.crd)
				return 0;
			end->val.crd = val->crd;
			spin->last_good_crd = val->crd;
			break;
	}
	do_step(hid_ctx, spin, str, end, 0); /* cheap conversion + error checks */
	return 0;
}

void rnd_dad_spin_set_help(rnd_hid_attribute_t *end, const char *help)
{
	rnd_hid_dad_spin_t *spin = end->wdata;
	rnd_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	if ((spin->hid_ctx == NULL) || (*spin->hid_ctx == NULL)) /* while building */
		str->help_text = help;
	else if (rnd_gui->attr_dlg_set_help != NULL) /* when the dialog is already running */
		rnd_gui->attr_dlg_set_help(*spin->hid_ctx, spin->wstr, help);
}

void pcb_dad_spin_update_global_coords(void)
{
	rnd_hid_dad_spin_t *spin;


	for(spin = gdl_first(&rnd_dad_coord_spins); spin != NULL; spin = gdl_next(&rnd_dad_coord_spins, spin)) {
		rnd_hid_attribute_t *dlg;
		void *hid_ctx;

		if ((spin->unit != NULL) || (spin->attrs == NULL) || (*spin->attrs == NULL) || (spin->hid_ctx == NULL) || (*spin->hid_ctx == NULL))
			continue;

		dlg = *spin->attrs;
		hid_ctx = *spin->hid_ctx;
		do_step(hid_ctx, spin, &dlg[spin->wstr], &dlg[spin->cmp.wend], 0); /* cheap conversion*/
	}
}

void rnd_dad_spin_update_internal(rnd_hid_dad_spin_t *spin)
{
	rnd_hid_attribute_t *dlg = *spin->attrs, *end = dlg+spin->cmp.wend;
	rnd_hid_attribute_t *str = dlg + spin->wstr;
	void *hid_ctx = *spin->hid_ctx;
	char buf[128];

	if (hid_ctx == NULL) /* before run() */
		str->val.str = rnd_strdup(gen_str_coord(spin, end->val.crd, buf, sizeof(buf)));
	else
		do_step(hid_ctx, spin, &dlg[spin->wstr], &dlg[spin->cmp.wend], 0); /* cheap conversion*/
}

void rnd_dad_spin_set_geo(rnd_hid_attribute_t *end, rnd_hatt_compflags_t flg, int geo)
{
	rnd_hid_dad_spin_t *spin = end->wdata;
	rnd_hid_attribute_t *str = end - spin->cmp.wend + spin->wstr;

	if (flg == RND_HATF_HEIGHT_CHR) {
		str->hatt_flags |= RND_HATF_HEIGHT_CHR;
		str->geo_width = geo;
	}
}

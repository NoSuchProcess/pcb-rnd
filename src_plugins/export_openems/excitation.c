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

#include "config.h"

#include <librnd/core/hid_dad.h>
#include "event.h"

#define MAX_EXC 16
#define FREQ_MAX ((double)(100.0*1000.0*1000.0*1000.0))
#define AEPREFIX "openems::excitation::"

typedef struct {
	int w[8];
} exc_data_t;

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int wselector, wtab;
	int selected;
	exc_data_t exc_data[MAX_EXC];
} exc_ctx_t;

exc_ctx_t exc_ctx;

typedef struct {
	const char *name;
	void (*dad)(int idx);
	char *(*get)(int idx, int fmt_matlab);
	void (*ser)(int idx, int save);  /* serialization: if save is 1, set attributes, else load attributes */
	int type_id;
} exc_t;
#define MAX_EXC_TYPES 5
static const exc_t excitations[MAX_EXC_TYPES];


static void ser_save(const char *data, const char *attrkey)
{
	const char *orig = pcb_attribute_get(&PCB->Attributes, attrkey);
	if ((orig == NULL) || (strcmp(orig, data) != 0)) {
		pcb_attribute_put(&PCB->Attributes, attrkey, data);
		pcb_board_set_changed_flag(PCB, rnd_true);
	}
}

static const char *ser_load(const char *attrkey)
{
	return pcb_attribute_get(&PCB->Attributes, attrkey);
}

#if 0
/* unused at the moment */
static void ser_int(int save, int widx, const char *attrkey)
{
	if (save) {
		char tmp[128];
		sprintf(tmp, "%d", exc_ctx.dlg[widx].val.lng);
		ser_save(tmp, attrkey);
	}
	else {
		rnd_hid_attr_val_t hv;
		char *end;
		const char *orig = ser_load(attrkey);

		if (orig != NULL) {
			hv.lng = strtol(orig, &end, 10);
			if (*end != '\0') {
				rnd_message(RND_MSG_ERROR, "Invalid integer value in board attribute '%s': '%s'\n", attrkey, orig);
				hv.lng = 0;
			}
		}
		else
			hv.lng = 0;

		rnd_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, widx, &hv);
	}
}
#endif

static void ser_hz(int save, int widx, const char *attrkey)
{
	if (save) {
		char tmp[128];
		sprintf(tmp, "%f Hz", exc_ctx.dlg[widx].val.dbl);
		ser_save(tmp, attrkey);
	}
	else {
		rnd_hid_attr_val_t hv;
		char *end;
		const char *orig = ser_load(attrkey);

		if (orig != NULL) {
			hv.dbl = strtod(orig, &end);
			if (*end != '\0') {
				while(isspace(*end)) end++;
				if (rnd_strcasecmp(end, "hz") != 0) {
					rnd_message(RND_MSG_ERROR, "Invalid real value (Hz) in board attribute '%s': '%s'\n", attrkey, orig);
					hv.dbl = 0;
				}
			}
		}
		else
			hv.dbl = 0;

		rnd_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, widx, &hv);
	}
}

static void ser_str(int save, int widx, const char *attrkey)
{
	if (save) {
		ser_save(exc_ctx.dlg[widx].val.str, attrkey);
	}
	else {
		rnd_hid_attr_val_t hv;
		hv.str = ser_load(attrkey);
		if (hv.str == NULL)
			hv.str = "";
		rnd_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, widx, &hv);
	}
}

static void exc_val_chg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);

static rnd_bool to_hz(const char *s, double *out)
{
	const rnd_unit_t *u;
	double d;
	if (!rnd_get_value_unit(s, NULL, 0, &d, &u))
		return rnd_false;
	if (u->family != RND_UNIT_FREQ)
		return rnd_false;
	*out = d;
	return rnd_true;
}

/*** excitation "micro-plugins" ***/

#define I_FC 0
#define I_F0 1

/** gaussian **/
static void exc_gaus_dad(int idx)
{
	RND_DAD_BEGIN_TABLE(exc_ctx.dlg, 2);
		RND_DAD_LABEL(exc_ctx.dlg, "fc");
		RND_DAD_SPIN_FREQ(exc_ctx.dlg);
			RND_DAD_MINMAX(exc_ctx.dlg, 0, FREQ_MAX);
			RND_DAD_HELP(exc_ctx.dlg, "20db Cutoff Frequency\nbandwidth is 2*fc");
			RND_DAD_CHANGE_CB(exc_ctx.dlg, exc_val_chg_cb);
			exc_ctx.exc_data[idx].w[I_FC] = RND_DAD_CURRENT(exc_ctx.dlg);

		RND_DAD_LABEL(exc_ctx.dlg, "f0");
		RND_DAD_REAL(exc_ctx.dlg);
			RND_DAD_MINMAX(exc_ctx.dlg, 0, FREQ_MAX);
			RND_DAD_HELP(exc_ctx.dlg, "Center Frequency [Hz]");
			RND_DAD_CHANGE_CB(exc_ctx.dlg, exc_val_chg_cb);
			exc_ctx.exc_data[idx].w[I_F0] = RND_DAD_CURRENT(exc_ctx.dlg);

	RND_DAD_END(exc_ctx.dlg);
}


static char *exc_gaus_get(int idx, int fmt_matlab)
{
	double f0 = 0, fc = 0;

	if (!to_hz(pcb_attribute_get(&PCB->Attributes, AEPREFIX "gaussian::f0"), &f0))
		rnd_message(RND_MSG_ERROR, "Gauss excitation: unable to parse frequency gaussian::f0\n");

	if (!to_hz(pcb_attribute_get(&PCB->Attributes, AEPREFIX "gaussian::fc"), &fc))
		rnd_message(RND_MSG_ERROR, "Gauss excitation: unable to parse frequency gaussian::fc\n");

	if (fmt_matlab)
		return rnd_strdup_printf("FDTD = SetGaussExcite(FDTD, %f, %f);", fc, f0);

	return rnd_strdup_printf("Type='%d' f0='%f' fc='%f'", excitations[idx].type_id, fc, f0);
}

static void exc_gaus_ser(int idx, int save)
{
	ser_hz(save, exc_ctx.exc_data[idx].w[I_F0], AEPREFIX "gaussian::f0");
	ser_hz(save, exc_ctx.exc_data[idx].w[I_FC], AEPREFIX "gaussian::fc");
}

#undef I_FC
#undef I_F0

/** sinusoidal **/

#define I_F0 0

static void exc_sin_dad(int idx)
{
	RND_DAD_BEGIN_TABLE(exc_ctx.dlg, 2);
		RND_DAD_LABEL(exc_ctx.dlg, "f0");
		RND_DAD_REAL(exc_ctx.dlg);
			RND_DAD_MINMAX(exc_ctx.dlg, 0, FREQ_MAX);
			RND_DAD_HELP(exc_ctx.dlg, "Center Frequency [Hz]");
			RND_DAD_CHANGE_CB(exc_ctx.dlg, exc_val_chg_cb);
			exc_ctx.exc_data[idx].w[I_F0] = RND_DAD_CURRENT(exc_ctx.dlg);
	RND_DAD_END(exc_ctx.dlg);
}

static char *exc_sin_get(int idx, int fmt_matlab)
{
	double f0;

	if (!to_hz(pcb_attribute_get(&PCB->Attributes, AEPREFIX "sinusoidal::f0"), &f0))
		rnd_message(RND_MSG_ERROR, "Sinus excitation: unable to parse frequency sinusoidal::f0\n");

	if (fmt_matlab)
		return rnd_strdup_printf("FDTD = SetSinusExcite(FDTD, %f);", f0);
	return rnd_strdup_printf("Type='%d' f0='%f'", excitations[idx].type_id, f0);
}

static void exc_sin_ser(int idx, int save)
{
	ser_hz(save, exc_ctx.exc_data[idx].w[I_F0], AEPREFIX "sinusoidal::f0");
}

#undef I_F0

/** custom **/

#define I_F0 0
#define I_FUNC 1

static void exc_cust_dad(int idx)
{
	RND_DAD_BEGIN_TABLE(exc_ctx.dlg, 2);
		RND_DAD_LABEL(exc_ctx.dlg, "f0");
		RND_DAD_REAL(exc_ctx.dlg);
			RND_DAD_MINMAX(exc_ctx.dlg, 0, FREQ_MAX);
			RND_DAD_HELP(exc_ctx.dlg, "Nyquest Rate [Hz]");
			RND_DAD_CHANGE_CB(exc_ctx.dlg, exc_val_chg_cb);
			exc_ctx.exc_data[idx].w[I_F0] = RND_DAD_CURRENT(exc_ctx.dlg);

		RND_DAD_LABEL(exc_ctx.dlg, "function");
		RND_DAD_STRING(exc_ctx.dlg);
			RND_DAD_HELP(exc_ctx.dlg, "Custom function");
			RND_DAD_CHANGE_CB(exc_ctx.dlg, exc_val_chg_cb);
			exc_ctx.exc_data[idx].w[I_FUNC] = RND_DAD_CURRENT(exc_ctx.dlg);
	RND_DAD_END(exc_ctx.dlg);
}

static char *exc_cust_get(int idx, int fmt_matlab)
{
	double f0;

	if (!to_hz(pcb_attribute_get(&PCB->Attributes, AEPREFIX "custom::f0"), &f0))
		rnd_message(RND_MSG_ERROR, "Custom excitation: unable to parse frequency custom::f0\n");

	if (fmt_matlab)
		return rnd_strdup_printf(
			"FDTD = SetCustomExcite(FDTD, %f, %s)",
			f0,
			pcb_attribute_get(&PCB->Attributes, AEPREFIX "custom::func")
		);

	return rnd_strdup_printf("Type='%d' f0='%f' Function='%s'",
		excitations[idx].type_id,
		f0,
		pcb_attribute_get(&PCB->Attributes, AEPREFIX "custom::func")
	);
}

static void exc_cust_ser(int idx, int save)
{
	ser_hz(save, exc_ctx.exc_data[idx].w[I_F0], AEPREFIX "custom::f0");
	ser_str(save, exc_ctx.exc_data[idx].w[I_FUNC], AEPREFIX "custom::func");
}

#undef I_F0
#undef I_FUNC


/** user-specified **/

#define I_SCRIPT 0

static void exc_user_dad(int idx)
{
	RND_DAD_BEGIN_VBOX(exc_ctx.dlg);
		RND_DAD_COMPFLAG(exc_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(exc_ctx.dlg, "Specify the excitation setup script:");
		RND_DAD_TEXT(exc_ctx.dlg, NULL);
			RND_DAD_COMPFLAG(exc_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			RND_DAD_CHANGE_CB(exc_ctx.dlg, exc_val_chg_cb);
			exc_ctx.exc_data[idx].w[I_SCRIPT] = RND_DAD_CURRENT(exc_ctx.dlg);
	RND_DAD_END(exc_ctx.dlg);
}

static char *exc_user_get(int idx, int fmt_matlab)
{
	if (fmt_matlab)
		return rnd_strdup(pcb_attribute_get(&PCB->Attributes, AEPREFIX "user-defined::script"));
	return NULL;
}

static void exc_user_ser(int idx, int save)
{
	int wscript;
	rnd_hid_attribute_t *attr;
	rnd_hid_text_t *txt;

	wscript = exc_ctx.exc_data[idx].w[I_SCRIPT];
	attr = &exc_ctx.dlg[wscript];
	txt = attr->wdata;

	ser_save(txt->hid_get_text(attr, exc_ctx.dlg_hid_ctx), AEPREFIX "user-defined::script");
}

#undef I_SCRIPT
/*** generic code ***/

static const exc_t excitations[MAX_EXC_TYPES] = {
	{ "gaussian",      exc_gaus_dad, exc_gaus_get, exc_gaus_ser, 0 },
	{ "sinusoidal",    exc_sin_dad,  exc_sin_get,  exc_sin_ser,  1 },
	{ "custom",        exc_cust_dad, exc_cust_get, exc_cust_ser, 10 },
	{ "user-defined",  exc_user_dad, exc_user_get, exc_user_ser, -1 },
	/* pending IDs: dirac=2, step=3 */
	{ NULL, NULL}
};

static void exc_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	exc_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(exc_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void exc_load_all(void)
{
	const exc_t *e;
	int n;
	for(n = 0, e = excitations; e->name != NULL; n++,e++)
		e->ser(n, 0);
}

static int load_selector(void)
{
	const char *type = pcb_attribute_get(&PCB->Attributes, AEPREFIX "type");
	const exc_t *e;
	int n;

	if (type == NULL) {
		exc_ctx.selected = 0;
		return 0;
	}

	for(n = 0, e = excitations; e->name != NULL; n++,e++) {
		if (strcmp(e->name, type) == 0) {
			exc_ctx.selected = n;
			return 0;
		}
	}

	return -1;
}

static void select_update(int setattr)
{
	rnd_hid_attr_val_t hv;
	hv.lng = exc_ctx.selected;

	if ((exc_ctx.selected < 0) || (exc_ctx.selected >= sizeof(excitations)/sizeof(excitations[0]))) {
		rnd_message(RND_MSG_ERROR, "Invalid excitation selected\n");
		exc_ctx.selected = 0;
	}

	rnd_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, exc_ctx.wtab, &hv);
	rnd_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, exc_ctx.wselector, &hv);
	if (setattr) {
		const char *orig = pcb_attribute_get(&PCB->Attributes, "openems::excitation::type");
		if ((orig == NULL) || (strcmp(orig, excitations[exc_ctx.selected].name) != 0)) {
			pcb_attribute_put(&PCB->Attributes, "openems::excitation::type", excitations[exc_ctx.selected].name);
			pcb_board_set_changed_flag(PCB, rnd_true);
		}
	}
}

static void select_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	exc_ctx.selected = attr->val.lng;
	select_update(1);
}

static void exc_val_chg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	excitations[exc_ctx.selected].ser(exc_ctx.selected, 1);
}

static void pcb_dlg_exc(void)
{
	static const char *excnames[MAX_EXC+1];
	const exc_t *e;
	int n;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (exc_ctx.active)
		return; /* do not open another */

	if (excnames[0] == NULL) {
		for(n = 0, e = excitations; e->name != NULL; n++,e++) {
			if (n >= MAX_EXC) {
				rnd_message(RND_MSG_ERROR, "internal error: too many excitations");
				break;
			}
			excnames[n] = e->name;
		}
		excnames[n] = NULL;
	}

	RND_DAD_BEGIN_VBOX(exc_ctx.dlg);
		RND_DAD_COMPFLAG(exc_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_HBOX(exc_ctx.dlg);
			RND_DAD_LABEL(exc_ctx.dlg, "Excitation type:");
			RND_DAD_ENUM(exc_ctx.dlg, excnames);
				exc_ctx.wselector = RND_DAD_CURRENT(exc_ctx.dlg);
				RND_DAD_CHANGE_CB(exc_ctx.dlg, select_cb);
		RND_DAD_END(exc_ctx.dlg);
		RND_DAD_BEGIN_TABBED(exc_ctx.dlg, excnames);
			RND_DAD_COMPFLAG(exc_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_HIDE_TABLAB);
			exc_ctx.wtab = RND_DAD_CURRENT(exc_ctx.dlg);
			for(n = 0, e = excitations; e->name != NULL; n++,e++) {
				if (e->dad != NULL)
					e->dad(n);
				else
					RND_DAD_LABEL(exc_ctx.dlg, "Not yet available.");
			}
		RND_DAD_END(exc_ctx.dlg);
		RND_DAD_BUTTON_CLOSES(exc_ctx.dlg, clbtn);
	RND_DAD_END(exc_ctx.dlg);

	/* set up the context */
	exc_ctx.active = 1;

	RND_DAD_NEW("openems_excitation", exc_ctx.dlg, "openems: excitation", &exc_ctx, rnd_false, exc_close_cb);

	load_selector();
	select_update(1);
	exc_load_all();
}

static const char pcb_acts_OpenemsExcitation[] = 
	"OpenemsExcitation([interactive])\n"
	"OpenemsExcitation(select, excitationname)\n"
	"OpenemsExcitation(set, [excitationnme], paramname, paramval)\n"
	"OpenemsExcitation(get, [excitationnme], paramname)\n"
	;
static const char pcb_acth_OpenemsExcitation[] = "Select which openEMS excitation method should be exported and manipulate the associated parameters. When invoked without arguments a dialog box with the same functionality is presented.";
/* DOC: openemsexcication.html */
static fgw_error_t pcb_act_OpenemsExcitation(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op = "interactive", *a1 = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, OpenemsExcitation, op = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, OpenemsExcitation, a1 = argv[2].val.str);

	RND_ACT_IRES(0);

	if (strcmp(op, "interactive") == 0)
		pcb_dlg_exc();
	else if (strcmp(op, "select") == 0) {
		if (a1 == NULL) {
			rnd_message(RND_MSG_ERROR, "OpenemsExcitation(select) needs a excitation name");
			goto error;
		}
		pcb_attribute_put(&PCB->Attributes, AEPREFIX "type", a1);
		load_selector();
		select_update(1);
	}
	else if (strcmp(op, "set") == 0) {
		int start;
		const char *key, *val;
		char *attrkey;
		switch(argc) {
			case 4: a1 = excitations[exc_ctx.selected].name; start = 2; break;
			case 5: start = 3; break;
			default:
				rnd_message(RND_MSG_ERROR, "OpenemsExcitation(set) needs exactly 2 or 3 more arguments");
				goto error;
		}

		RND_ACT_CONVARG(start+0, FGW_STR, OpenemsExcitation, key = argv[start+0].val.str);
		RND_ACT_CONVARG(start+1, FGW_STR, OpenemsExcitation, val = argv[start+1].val.str);

		attrkey = rnd_strdup_printf(AEPREFIX "%s::%s", a1, key);
		pcb_attribute_put(&PCB->Attributes, attrkey, val);
		free(attrkey);

		exc_load_all();
	}
	else if (strcmp(op, "get") == 0) {
		int start;
		const char *key;
		char *attrkey;
		switch(argc) {
			case 3: a1 = excitations[exc_ctx.selected].name; start = 2; break;
			case 4: start = 3; break;
			default:
				rnd_message(RND_MSG_ERROR, "OpenemsExcitation(get) needs exactly 1 or 2 more arguments");
				goto error;
		}

		RND_ACT_CONVARG(start+0, FGW_STR, OpenemsExcitation, key = argv[start+0].val.str);

		attrkey = rnd_strdup_printf(AEPREFIX "%s::%s", a1, key);
		res->type = FGW_STR;
		res->val.cstr = pcb_attribute_get(&PCB->Attributes, attrkey);
		free(attrkey);
	}

	return 0;

	error:;
	RND_ACT_IRES(1);
	return 0;
}

static char *pcb_openems_excitation_get(pcb_board_t *pcb, int fmt_matlab)
{
	if ((exc_ctx.selected < 0) || (exc_ctx.selected >= sizeof(excitations)/sizeof(excitations[0]))) {
		rnd_message(RND_MSG_ERROR, "No excitation selected\n");
		if (fmt_matlab)
			return rnd_strdup("%% ERROR: no excitation selected\n");
		else
			return NULL;
	}
	return excitations[exc_ctx.selected].get(exc_ctx.selected, fmt_matlab);
}

static void exc_ev_board_changed(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	load_selector();
	if (exc_ctx.active)
		exc_load_all();
}

static void pcb_openems_excitation_init(void)
{
	rnd_event_bind(RND_EVENT_BOARD_CHANGED, exc_ev_board_changed, NULL, openems_cookie);
}

static void pcb_openems_excitation_uninit(void)
{
	rnd_event_unbind_allcookie(openems_cookie);
}

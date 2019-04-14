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

#include "hid_dad.h"

#define MAX_EXC 16

#define AEPREFIX "openems::excitation::"

typedef struct {
	int w[8];
} exc_data_t;

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int wselector, wtab;
	int selected;
	exc_data_t exc_data[MAX_EXC];
} exc_ctx_t;

exc_ctx_t exc_ctx;

typedef struct {
	const char *name;
	void (*dad)(int idx);
	char *(*get)(int idx);
	void (*ser)(int idx, int save);  /* serialization: if save is 1, set attributes, else load attributes */
} exc_t;


static void ser_save(const char *data, const char *attrkey)
{
	const char *orig = pcb_attribute_get(&PCB->Attributes, attrkey);
	if ((orig == NULL) || (strcmp(orig, data) != 0)) {
		pcb_attribute_put(&PCB->Attributes, attrkey, data);
		pcb_board_set_changed_flag(pcb_true);
	}
}

static const char *ser_load(const char *attrkey)
{
	return pcb_attribute_get(&PCB->Attributes, attrkey);
}

static void ser_int(int save, int widx, const char *attrkey)
{
	if (save) {
		char tmp[128];
		sprintf(tmp, "%d", exc_ctx.dlg[widx].default_val.int_value);
		ser_save(tmp, attrkey);
	}
	else {
		pcb_hid_attr_val_t hv;
		char *end;
		const char *orig = ser_load(attrkey);

		if (orig != NULL) {
			hv.int_value = strtol(orig, &end, 10);
			if (*end != '\0') {
				pcb_message(PCB_MSG_ERROR, "Invalid integer value in board attribute '%s': '%s'\n", attrkey, orig);
				hv.int_value = 0;
			}
		}
		else
			hv.int_value = 0;

		pcb_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, widx, &hv);
	}
}

static void ser_str(int save, int widx, const char *attrkey)
{
	if (save) {
		ser_save(exc_ctx.dlg[widx].default_val.str_value, attrkey);
	}
	else {
		pcb_hid_attr_val_t hv;
		hv.str_value = ser_load(attrkey);
		if (hv.str_value == NULL)
			hv.str_value = "";
		pcb_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, widx, &hv);
	}
}

/*** excitation "micro-plugins" ***/

#define I_FC 0
#define I_F0 1

/** gaussian **/
static void exc_gaus_dad(int idx)
{
	PCB_DAD_BEGIN_TABLE(exc_ctx.dlg, 2);
		PCB_DAD_LABEL(exc_ctx.dlg, "fc");
		PCB_DAD_INTEGER(exc_ctx.dlg, "");
			PCB_DAD_HELP(exc_ctx.dlg, "20db Cutoff Frequency [Hz]\nbandwidth is 2*fc");
			exc_ctx.exc_data[idx].w[I_FC] = PCB_DAD_CURRENT(exc_ctx.dlg);

		PCB_DAD_LABEL(exc_ctx.dlg, "f0");
		PCB_DAD_INTEGER(exc_ctx.dlg, "");
			PCB_DAD_HELP(exc_ctx.dlg, "Center Frequency [Hz]");
			exc_ctx.exc_data[idx].w[I_F0] = PCB_DAD_CURRENT(exc_ctx.dlg);

	PCB_DAD_END(exc_ctx.dlg);
}

static char *exc_gaus_get(int idx)
{
	int wf0, wfc;

	wf0 = exc_ctx.exc_data[idx].w[I_F0];
	wfc = exc_ctx.exc_data[idx].w[I_FC];

	return pcb_strdup_printf(
		"FDTD = SetGaussExcite(FDTD, %d, %d);",
		exc_ctx.dlg[wf0].default_val.int_value,
		exc_ctx.dlg[wfc].default_val.int_value
	);
}

static void exc_gaus_ser(int idx, int save)
{
	ser_int(save, exc_ctx.exc_data[idx].w[I_F0], AEPREFIX "gaussian::f0");
	ser_int(save, exc_ctx.exc_data[idx].w[I_FC], AEPREFIX "gaussian::f0");
}

#undef I_FC
#undef I_F0

/** sinusoidal **/

#define I_F0 0

static void exc_sin_dad(int idx)
{
	PCB_DAD_BEGIN_TABLE(exc_ctx.dlg, 2);
		PCB_DAD_LABEL(exc_ctx.dlg, "f0");
		PCB_DAD_INTEGER(exc_ctx.dlg, "");
			PCB_DAD_HELP(exc_ctx.dlg, "Center Frequency [Hz]");
			exc_ctx.exc_data[idx].w[I_F0] = PCB_DAD_CURRENT(exc_ctx.dlg);
	PCB_DAD_END(exc_ctx.dlg);
}

static char *exc_sin_get(int idx)
{
	int wf0;

	wf0 = exc_ctx.exc_data[idx].w[I_F0];

	return pcb_strdup_printf(
		"FDTD = SetSinusExcite(FDTD, %d);",
		exc_ctx.dlg[wf0].default_val.int_value
	);
}

static void exc_sin_ser(int idx, int save)
{
	ser_int(save, exc_ctx.exc_data[idx].w[I_F0], AEPREFIX "sinusoidal:f0");
}

#undef I_F0

/** custom **/

#define I_F0 0
#define I_FUNC 1

static void exc_cust_dad(int idx)
{
	PCB_DAD_BEGIN_TABLE(exc_ctx.dlg, 2);
		PCB_DAD_LABEL(exc_ctx.dlg, "f0");
		PCB_DAD_INTEGER(exc_ctx.dlg, "");
			PCB_DAD_HELP(exc_ctx.dlg, "Nyquest Rate [Hz]");
			exc_ctx.exc_data[idx].w[I_F0] = PCB_DAD_CURRENT(exc_ctx.dlg);

		PCB_DAD_LABEL(exc_ctx.dlg, "function");
		PCB_DAD_STRING(exc_ctx.dlg);
			PCB_DAD_HELP(exc_ctx.dlg, "Custom function");
			exc_ctx.exc_data[idx].w[I_FUNC] = PCB_DAD_CURRENT(exc_ctx.dlg);
	PCB_DAD_END(exc_ctx.dlg);
}

static char *exc_cust_get(int idx)
{
	int wf0, wfunc;

	wf0 = exc_ctx.exc_data[idx].w[I_F0];
	wfunc = exc_ctx.exc_data[idx].w[I_FUNC];

	return pcb_strdup_printf(
		"FDTD = SetCustomExcite(FDTD, %d, %s)",
		exc_ctx.dlg[wf0].default_val.int_value,
		exc_ctx.dlg[wfunc].default_val.str_value
	);
}

static void exc_cust_ser(int idx, int save)
{
	ser_int(save, exc_ctx.exc_data[idx].w[I_F0], AEPREFIX "custom::f0");
	ser_str(save, exc_ctx.exc_data[idx].w[I_FUNC], AEPREFIX "custom::func");
}

#undef I_F0
#undef I_FUNC


/** user-specified **/

#define I_SCRIPT 0

static void exc_user_dad(int idx)
{
	PCB_DAD_BEGIN_VBOX(exc_ctx.dlg);
		PCB_DAD_COMPFLAG(exc_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(exc_ctx.dlg, "Specify the excitation setup script:");
		PCB_DAD_TEXT(exc_ctx.dlg, NULL);
			PCB_DAD_COMPFLAG(exc_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			exc_ctx.exc_data[idx].w[I_SCRIPT] = PCB_DAD_CURRENT(exc_ctx.dlg);
	PCB_DAD_END(exc_ctx.dlg);
}

static char *exc_user_get(int idx)
{
	int wscript;
	pcb_hid_attribute_t *attr;
	pcb_hid_text_t *txt;

	wscript = exc_ctx.exc_data[idx].w[I_SCRIPT];
	attr = &exc_ctx.dlg[wscript];
	txt = (pcb_hid_text_t *)attr->enumerations;

	return txt->hid_get_text(attr, exc_ctx.dlg_hid_ctx);
}

static void exc_user_ser(int idx, int save)
{
	ser_str(save, exc_ctx.exc_data[idx].w[I_SCRIPT], AEPREFIX "user-defined::script");
}

#undef I_SCRIPT
/*** generic code ***/

static const exc_t excitations[] = {
	{ "gaussian",      exc_gaus_dad, exc_gaus_get, exc_gaus_ser },
	{ "sinusoidal",    exc_sin_dad,  exc_sin_get,  exc_sin_ser  },
	{ "custom",        exc_cust_dad, exc_cust_get, exc_cust_ser },
	{ "user-defined",  exc_user_dad, exc_user_get, exc_user_ser },
	{ NULL, NULL}
};

static void exc_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	exc_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(exc_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
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
	pcb_hid_attr_val_t hv;
	hv.int_value = exc_ctx.selected;

	if ((exc_ctx.selected < 0) || (exc_ctx.selected >= sizeof(excitations)/sizeof(excitations[0]))) {
		pcb_message(PCB_MSG_ERROR, "Invalid excitation selected\n");
		exc_ctx.selected = 0;
	}

	pcb_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, exc_ctx.wtab, &hv);
	pcb_gui->attr_dlg_set_value(exc_ctx.dlg_hid_ctx, exc_ctx.wselector, &hv);
	if (setattr) {
		const char *orig = pcb_attribute_get(&PCB->Attributes, "openems::excitation::type");
		if ((orig == NULL) || (strcmp(orig, excitations[exc_ctx.selected].name) != 0)) {
			pcb_attribute_put(&PCB->Attributes, "openems::excitation::type", excitations[exc_ctx.selected].name);
			pcb_board_set_changed_flag(pcb_true);
		}
	}
}

static void select_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	exc_ctx.selected = attr->default_val.int_value;
	select_update(1);
}

static void pcb_dlg_exc(void)
{
	static const char *excnames[MAX_EXC+1];
	const exc_t *e;
	int n;
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (exc_ctx.active)
		return; /* do not open another */

	if (excnames[0] == NULL) {
		for(n = 0, e = excitations; e->name != NULL; n++,e++) {
			if (n >= MAX_EXC) {
				pcb_message(PCB_MSG_ERROR, "internal error: too many excitations");
				break;
			}
			excnames[n] = e->name;
		}
		excnames[n] = NULL;
	}

	PCB_DAD_BEGIN_VBOX(exc_ctx.dlg);
		PCB_DAD_COMPFLAG(exc_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HBOX(exc_ctx.dlg);
			PCB_DAD_LABEL(exc_ctx.dlg, "Excitation type:");
			PCB_DAD_ENUM(exc_ctx.dlg, excnames);
				exc_ctx.wselector = PCB_DAD_CURRENT(exc_ctx.dlg);
				PCB_DAD_CHANGE_CB(exc_ctx.dlg, select_cb);
		PCB_DAD_END(exc_ctx.dlg);
		PCB_DAD_BEGIN_TABBED(exc_ctx.dlg, excnames);
			PCB_DAD_COMPFLAG(exc_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_HIDE_TABLAB);
			exc_ctx.wtab = PCB_DAD_CURRENT(exc_ctx.dlg);
			for(n = 0, e = excitations; e->name != NULL; n++,e++) {
				if (e->dad != NULL)
					e->dad(n);
				else
					PCB_DAD_LABEL(exc_ctx.dlg, "Not yet available.");
			}
		PCB_DAD_END(exc_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(exc_ctx.dlg, clbtn);
	PCB_DAD_END(exc_ctx.dlg);

	/* set up the context */
	exc_ctx.active = 1;

	PCB_DAD_NEW("openems_excitation", exc_ctx.dlg, "openems: excitation", &exc_ctx, pcb_false, exc_close_cb);

	load_selector();
	select_update(1);
}

static const char pcb_acts_OpenemsExcitation[] = 
	"OpenemsExcitation()\n"
	"OpenemsExcitation(select, excitationname)\n"
	"OpenemsExcitation(set, paramname, paramval)\n"
	"OpenemsExcitation(get, paramname)\n"
	;

static const char pcb_acth_OpenemsExcitation[] = "Select which openEMS excitation method should be exported and manipulate the associated parameters. When invoked without arguments a dialog box with the same functionality is presented.";
static fgw_error_t pcb_act_OpenemsExcitation(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_exc();
	return 0;
}

static char *pcb_openems_excitation_get(pcb_board_t *pcb)
{
	if ((exc_ctx.selected < 0) || (exc_ctx.selected >= sizeof(excitations)/sizeof(excitations[0]))) {
		pcb_message(PCB_MSG_ERROR, "No excitation selected\n");
		return pcb_strdup("%% ERROR: no excitation selected\n");
	}
	return excitations[exc_ctx.selected].get(exc_ctx.selected);
}


/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, diag, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <genvector/gds_char.h>

#include "brave.h"
#include "conf_core.h"
#include "conf_hid.h"
#include "compat_misc.h"
#include "error.h"
#include "hid_dad.h"

pcb_brave_t pcb_brave = 0;
static const char brave_cookie[] = "brave";
static conf_hid_id_t brave_conf_id;

typedef struct {
	pcb_brave_t bit;
	const char *name, *shrt, *lng;
	int widget;
} desc_t;

static desc_t desc[] = {
	{PCB_BRAVE_PSTK_VIA, "pstk_via", "use padstack for vias",
		"placing new vias will place padstacks instsad of old via objects", 0 },
	{0, NULL, NULL, NULL}
};

static desc_t *find_by_name(const char *name)
{
	desc_t *d;
	for(d = desc; d->name != NULL; d++)
		if (pcb_strcasecmp(name, d->name) == 0)
			return d;
	return NULL;
}

/* convert the brave bits into a config string, save in CLI */
static void set_conf(pcb_brave_t br)
{
	gds_t tmp;
	desc_t *d;

	gds_init(&tmp);

	for(d = desc; d->name != NULL; d++) {
		if (br & d->bit) {
			gds_append_str(&tmp, d->name);
			gds_append(&tmp, ',');
		}
	}

	/* truncate last comma */
	gds_truncate(&tmp, gds_len(&tmp)-1);

	conf_set(CFR_CLI, "rc/brave", 0, tmp.array, POL_OVERWRITE);

	gds_uninit(&tmp);
}

/* Upon a change in the config, parse the new config string and set the brave bits */
static void brave_conf_chg(conf_native_t *cfg, int arr_idx)
{
	char *curr, *next, old;
	desc_t *d;

	pcb_brave = 0;
	if ((conf_core.rc.brave == NULL) || (*conf_core.rc.brave == '\0'))
		return;
	for(curr = (char *)conf_core.rc.brave; *curr != '\0'; curr = next) {
		next = strpbrk(curr, ", ");
		if (next == NULL)
			next = curr + strlen(curr);
		old = *next;
		*next = '\0';
		d = find_by_name(curr);

		if (d != NULL)
			pcb_brave |= d->bit;
		*next = old;
		next++;
		while((*next == ',') || (*next == ' ')) next++;
	}
}

/* Set one bit; if changed, update the config */
static void brave_set(pcb_brave_t bit, int on)
{
	int state = pcb_brave & bit;
	pcb_brave_t nb;

	if (state == on)
		return;

	if (on)
		nb = pcb_brave | bit;
	else
		nb = pcb_brave & ~bit;

	set_conf(nb);
}

/*** interactive mode (DAD dialog) ***/

/* Convert the current brave bits into dialog box widget states, update the dialog */
static void brave2dlg(void *hid_ctx)
{
	desc_t *d;
	int len;

	for(d = desc, len=0; d->name != NULL; d++,len++) {
		if (pcb_brave & d->bit)
			PCB_DAD_SET_VALUE(hid_ctx, d->widget, int_value, 1);
		else
			PCB_DAD_SET_VALUE(hid_ctx, d->widget, int_value, 0);
	}
}

/* Convert the current state of dialog box widget into brave bits, update the conf */
static void dlg2brave(pcb_hid_attribute_t *attrs)
{
	desc_t *d;
	for(d = desc; d->name != NULL; d++)
		brave_set(d->bit, attrs[d->widget].default_val.int_value);
}

/* If a checkbox changes, do the conversion forth and back to ensure sync */
static void brave_dialog_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	dlg2brave(caller_data);
	brave2dlg(hid_ctx);
}

/* Tick in all - button callback */
static void brave_dialog_allon(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	desc_t *d;
	for(d = desc; d->name != NULL; d++)
		brave_set(d->bit, 1);
	brave2dlg(hid_ctx);
}

/* Tick off all - button callback  */
static void brave_dialog_alloff(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	desc_t *d;
	for(d = desc; d->name != NULL; d++)
		brave_set(d->bit, 0);
	brave2dlg(hid_ctx);
}

/* Copy the config from CLI to USER and flush the file */
static void brave_dialog_save(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	conf_set(CFR_USER, "rc/brave", 0, conf_core.rc.brave, POL_OVERWRITE);
	if (conf_isdirty(CFR_USER))
		conf_save_file(NULL, NULL, CFR_USER, NULL);
}

static int brave_interact(void)
{
	desc_t *d;
	int len;

	PCB_DAD_DECL(dlg);

	PCB_DAD_BEGIN_VBOX(dlg);
	PCB_DAD_LABEL(dlg, "Experimental features for the brave");

	PCB_DAD_BEGIN_TABLE(dlg, 3);
		for(d = desc, len=0; d->name != NULL; d++,len++) {
			PCB_DAD_LABEL(dlg, d->name);
				PCB_DAD_HELP(dlg, d->lng);
			PCB_DAD_BOOL(dlg, "");
				d->widget = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, brave_dialog_chg);
				PCB_DAD_HELP(dlg, d->lng);
			PCB_DAD_LABEL(dlg, d->shrt);
				PCB_DAD_HELP(dlg, d->lng);
		}
		if (len != 0) {
			PCB_DAD_BUTTON(dlg, "all ON");
				PCB_DAD_HELP(dlg, "Tick in all boxes\nenabling all experimental features\n(Super Brave Mode)");
				PCB_DAD_CHANGE_CB(dlg, brave_dialog_allon);
			PCB_DAD_BUTTON(dlg, "all OFF");
				PCB_DAD_HELP(dlg, "Tick off all boxes\ndisabling all experimental features\n(Safe Mode)");
				PCB_DAD_CHANGE_CB(dlg, brave_dialog_alloff);
			PCB_DAD_BUTTON(dlg, "save in user cfg");
				PCB_DAD_HELP(dlg, "Save current brave state in the \nuser configuration file");
				PCB_DAD_CHANGE_CB(dlg, brave_dialog_save);
		}
	PCB_DAD_END(dlg);

	if (len == 0)
		PCB_DAD_LABEL(dlg, "(There are no brave features at the moment)");
	PCB_DAD_END(dlg);


	PCB_DAD_NEW(dlg, "dlg_padstack_edit", "Edit padstack", dlg);
	brave2dlg(dlg_hid_ctx);
	PCB_DAD_RUN(dlg);

	PCB_DAD_FREE(dlg);
	return 0;
}

/*** action ***/

static const char pcb_acts_Brave[] =
	"Brave()\n"
	"Brave(setting, on|off)\n";
static const char pcb_acth_Brave[] = "Changes brave settings.";
static int pcb_act_Brave(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	desc_t *d;
	if (argc == 0)
		return brave_interact();

	/* look up */
	if (argc > 0) {
		d = find_by_name(argv[0]);
		if (d == NULL) {
			pcb_message(PCB_MSG_ERROR, "Unknown brave setting: %s\n", argv[0]);
			return -1;
		}
	}

	if (argc > 1)
		brave_set(d->bit, (pcb_strcasecmp(argv[1], "on") == 0));

	pcb_message(PCB_MSG_INFO, "Brave setting: %s in %s\n", argv[0], (pcb_brave & d->bit) ? "on" : "off");

	return 0;
}

pcb_hid_action_t brave_action_list[] = {
	{"Brave", 0, pcb_act_Brave,
	 pcb_acth_Brave, pcb_acts_Brave}
};

PCB_REGISTER_ACTIONS(brave_action_list, NULL)


void pcb_brave_init(void)
{
	conf_native_t *n = conf_get_field("rc/brave");
	brave_conf_id = conf_hid_reg(brave_cookie, NULL);

	if (n != NULL) {
		static conf_hid_callbacks_t cbs;
		memset(&cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs.val_change_post = brave_conf_chg;
		conf_hid_set_cb(n, brave_conf_id, &cbs);
	}
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <genht/htsp.h>
#include <genht/hash.h>
#include "actions.h"
#include "compat_misc.h"
#include "hid_dad.h"
#include "error.h"

#include "act_dad.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	char *name;
	unsigned running:1;
} dad_t;

htsp_t dads;

static int dad_new(const char *name)
{
	dad_t *dad;

	if (htsp_get(&dads, name) != NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't create named DAD dialog %s: already exists\n", name);
		return -1;
	}

	dad = calloc(sizeof(dad_t), 1);
	dad->name = pcb_strdup(name);
	htsp_set(&dads, dad->name, dad);
	return 0;
}

static void dad_destroy(dad_t *dad)
{
	htsp_pop(&dads, dad->name);
	free(dad->name);
	free(dad);
}

static void dad_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	dad_t *dad = caller_data;
	PCB_DAD_FREE(dad->dlg);
	dad_destroy(dad);
}

const char pcb_acts_dad[] =
	"dad(new, dlgname) - create new dialog\n"
	"dad(label, dlgname, text) - append a label widget\n"
	"dad(run, dlgname, longname, shortname) - present dlgname as a non-modal dialog\n"
	"dad(run_modal, dlgname, longname, shortname) - present dlgname as a modal dialog\n"
	;
const char pcb_acth_dad[] = "Manipulate Dynamic Attribute Dialogs";
fgw_error_t pcb_act_dad(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd, *dlgname, *txt;
	dad_t *dad;
	int rv = 0;

	PCB_ACT_CONVARG(1, FGW_STR, dad, cmd = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, dad, dlgname = argv[2].val.str);

	if (pcb_strcasecmp(cmd, "new") == 0) {
		PCB_ACT_IRES(dad_new(dlgname));
		return 0;
	}

	dad = htsp_get(&dads, dlgname);
	if (dad == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
		PCB_ACT_IRES(-1);
		return 0;
	}


	if (pcb_strcasecmp(cmd, "label") == 0) {
		if (dad->running) goto cant_chg;
		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_LABEL(dad->dlg, txt);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if ((pcb_strcasecmp(cmd, "run") == 0) || (pcb_strcasecmp(cmd, "run_modal") == 0)) {
		char *sh;
		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_ACT_CONVARG(4, FGW_STR, dad, sh = argv[4].val.str);
		PCB_DAD_NEW(dad->dlg, txt, sh, dad, (cmd[3] == '_'), dad_close_cb);
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid DAD dialog command: '%s'\n", cmd);
		rv = -1;
	}

	PCB_ACT_IRES(rv);
	return 0;

	cant_chg:;
	pcb_message(PCB_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
	PCB_ACT_IRES(-1);
	return 0;
}

void pcb_act_dad_init(void)
{
	htsp_init(&dads, strhash, strkeyeq);
}

void pcb_act_dad_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&dads); e != NULL; e = htsp_next(&dads, e))
		dad_destroy(e->value);
	htsp_uninit(&dads);
}

/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"

#include <librnd/core/actions.h>
#include "board.h"
#include "drc.h"
#include <librnd/core/hidlib_conf.h>
#include "conf_core.h"
#include <librnd/core/compat_misc.h>
#include "event.h"


pcb_view_list_t pcb_drc_lst;
extern pcb_view_list_t pcb_io_incompat_lst;
gdl_list_t pcb_drc_impls;

void pcb_drc_impl_reg(pcb_drc_impl_t *impl)
{
	gdl_insert(&pcb_drc_impls, impl, link);
}

void pcb_drc_impl_unreg(pcb_drc_impl_t *impl)
{
	gdl_remove(&pcb_drc_impls, impl, link);
}



void pcb_drc_set_data(pcb_view_t *violation, const fgw_arg_t *measured_value, fgw_arg_t required_value)
{
	violation->data_type = PCB_VIEW_DRC;
	violation->data.drc.required_value = required_value;
	if (measured_value != NULL) {
		violation->data.drc.have_measured = 1;
		violation->data.drc.measured_value = *measured_value;
	}
	else
		violation->data.drc.have_measured = 0;
}


static fgw_error_t view_dlg(fgw_arg_t *res, int argc, fgw_arg_t *argv, const char *dlg_type, const char *dlgact, pcb_view_list_t *lst, void (*refresh)())
{
	fgw_arg_t args[2];

	args[1].type = FGW_STR;

	if (rnd_strcasecmp(dlg_type, "list") == 0) {
		if (RND_HAVE_GUI_ATTR_DLG) {
			args[1].val.str = "list";
			return rnd_actionv_bin(RND_ACT_HIDLIB, dlgact, res, 2, args);
		}
		dlg_type = "print";
	}

	if (rnd_strcasecmp(dlg_type, "simple") == 0) {
		if (RND_HAVE_GUI_ATTR_DLG) {
			args[1].val.str = "simple";
			return rnd_actionv_bin(RND_ACT_HIDLIB, dlgact, res, 2, args);
		}
		dlg_type = "print";
	}


	RND_ACT_IRES(-1);
	if (refresh != NULL)
		refresh();

	if (rnd_strcasecmp(dlg_type, "print") == 0) {
		pcb_view_t *v;
		for(v = pcb_view_list_first(lst); v != NULL; v = pcb_view_list_next(v)) {
			printf("%ld: %s: %s\n", v->uid, v->type, v->title);
			if (v->have_bbox)
				rnd_printf("%m+within %$m4\n", rnd_conf.editor.grid_unit->allow, v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				rnd_printf("%m+at %$m2\n", rnd_conf.editor.grid_unit->allow, v->x, v->y);
			if (v->data.drc.required_value.type != FGW_INVALID)
				rnd_printf("%m+required value %mw\n", rnd_conf.editor.grid_unit->allow, v->data.drc.required_value);
			if ((v->data.drc.have_measured) && (v->data.drc.measured_value.type != FGW_INVALID))
				rnd_printf("%m+measured value %mw\n", rnd_conf.editor.grid_unit->allow, v->data.drc.measured_value);
			printf("%s\n\n", v->description);
		}
	}
	else if (rnd_strcasecmp(dlg_type, "log") == 0) {
		pcb_view_t *v;
		for(v = pcb_view_list_first(lst); v != NULL; v = pcb_view_list_next(v)) {
			rnd_message(RND_MSG_INFO, "%ld: %s: %s\n", v->uid, v->type, v->title);
			if (v->have_bbox)
				rnd_message(RND_MSG_INFO, "%m+within %$m4\n", rnd_conf.editor.grid_unit->allow, v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				rnd_message(RND_MSG_INFO, "%m+at %$m2\n", rnd_conf.editor.grid_unit->allow, v->x, v->y);
			if (v->data.drc.required_value.type != FGW_INVALID)
				rnd_printf("%m+required value %mw\n", rnd_conf.editor.grid_unit->allow, v->data.drc.required_value);
			if ((v->data.drc.have_measured) && (v->data.drc.measured_value.type != FGW_INVALID))
				rnd_message(RND_MSG_INFO, "%m+measured value %mw\n", rnd_conf.editor.grid_unit->allow, v->data.drc.measured_value);
			rnd_message(RND_MSG_INFO, "%s\n\n", v->description);
		}
	}
	if (rnd_strcasecmp(dlg_type, "dump") == 0) {
		pcb_view_t *v;
		char *s;
		for(v = pcb_view_list_first(lst); v != NULL; v = pcb_view_list_next(v)) {
			printf("V%ld\n", v->uid);
			printf("T%s\n", v->type);
			printf("t%s\n", v->title);
			if (v->have_bbox)
				rnd_printf("B%mm %mm %mm %mm mm\n", v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				rnd_printf("A%mm %mm mm\n", v->x, v->y);
			rnd_printf("R%$mw\n", v->data.drc.required_value);
			if (v->data.drc.have_measured)
				rnd_printf("M%$mw\n", v->data.drc.measured_value);
			for(s = v->description; *s != '\0'; s++)
				if (*s == '\n')
					*s = ' ';
			printf("E%s\n\n", v->description);
		}
	}
	else if (rnd_strcasecmp(dlg_type, "count") == 0)
		RND_ACT_IRES(pcb_view_list_length(lst));


	if (refresh != NULL)
		pcb_view_list_free_fields(lst);

	return 0;
}

void pcb_drc_all(void)
{
	int ran = 0;
	pcb_view_list_free_fields(&pcb_drc_lst);
	rnd_event(&PCB->hidlib, PCB_EVENT_DRC_RUN, "p", &ran);
	if (ran == 0)
		rnd_message(RND_MSG_ERROR, "No DRC tests ran. Is any DRC plugin compiled? Are they disabled? Are all rules disabled?\n");
}

static const char pcb_acts_DRC[] = "DRC([list|simple|print|log|dump])";
static const char pcb_acth_DRC[] = "Invoke the DRC check. Results are presented as the argument requests.";
/* DOC: drc.html */
static fgw_error_t pcb_act_DRC(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	RND_ACT_MAY_CONVARG(1, FGW_STR, DRC, dlg_type = argv[1].val.str);
	return view_dlg(res, argc, argv, dlg_type, "drcdialog", &pcb_drc_lst, pcb_drc_all);
}

const char pcb_acts_IOIncompatList[] = "IOIncompatList([list|simple])\n";
const char pcb_acth_IOIncompatList[] = "Present the format incompatibilities of the last save to file operation.";
fgw_error_t pcb_act_IOIncompatList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	const char *aauto = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, IOIncompatList, dlg_type = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, IOIncompatList, aauto = argv[2].val.str);

	if ((aauto != NULL) && (strcmp(aauto, "auto") == 0)) {
		if (rnd_conf.rc.quiet && !RND_HAVE_GUI_ATTR_DLG) {
			/* if not explicitly asked for a listing style and we are on CLI and quiet is set, don't print anything */
			RND_ACT_IRES(0);
			return 0;
		}
	}

	return view_dlg(res, argc, argv, dlg_type, "ioincompatlistdialog", &pcb_io_incompat_lst, NULL);
}

static rnd_action_t drc_action_list[] = {
	{"DRC", pcb_act_DRC, pcb_acth_DRC, pcb_acts_DRC},
	{"IOIncompatList", pcb_act_IOIncompatList, pcb_acth_IOIncompatList, pcb_acts_IOIncompatList},
};

void pcb_drc_act_init2(void)
{
	RND_REGISTER_ACTIONS(drc_action_list, NULL);
}

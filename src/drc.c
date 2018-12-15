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

#include "actions.h"
#include "drc.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "event.h"


pcb_view_list_t pcb_drc_lst;

void pcb_drc_set_data(pcb_view_t *violation, const pcb_coord_t *measured_value, pcb_coord_t required_value)
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

	if (pcb_strcasecmp(dlg_type, "list") == 0) {
		if (PCB_HAVE_GUI_ATTR_DLG) {
			args[1].val.str = "list";
			return pcb_actionv_bin(dlgact, res, 2, args);
		}
		dlg_type = "print";
	}

	if (pcb_strcasecmp(dlg_type, "simple") == 0) {
		if (PCB_HAVE_GUI_ATTR_DLG) {
			args[1].val.str = "simple";
			return pcb_actionv_bin(dlgact, res, 2, args);
		}
		dlg_type = "print";
	}


	PCB_ACT_IRES(-1);
	if (refresh != NULL)
		refresh();

	if (pcb_strcasecmp(dlg_type, "print") == 0) {
		pcb_view_t *v;
		for(v = pcb_view_list_first(lst); v != NULL; v = pcb_view_list_next(v)) {
			printf("%ld: %s: %s\n", v->uid, v->type, v->title);
			if (v->have_bbox)
				pcb_printf("%m+within %$m4\n", conf_core.editor.grid_unit->allow, v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				pcb_printf("%m+at %$m2\n", conf_core.editor.grid_unit->allow, v->x, v->y);
			pcb_printf("%m+required value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.required_value);
			if (v->data.drc.have_measured)
				pcb_printf("%m+measured value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.measured_value);
			printf("%s\n\n", v->description);
		}
	}
	else if (pcb_strcasecmp(dlg_type, "log") == 0) {
		pcb_view_t *v;
		for(v = pcb_view_list_first(lst); v != NULL; v = pcb_view_list_next(v)) {
			pcb_message(PCB_MSG_INFO, "%ld: %s: %s\n", v->uid, v->type, v->title);
			if (v->have_bbox)
				pcb_message(PCB_MSG_INFO, "%m+within %$m4\n", conf_core.editor.grid_unit->allow, v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				pcb_message(PCB_MSG_INFO, "%m+at %$m2\n", conf_core.editor.grid_unit->allow, v->x, v->y);
			pcb_printf("%m+required value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.required_value);
			if (v->data.drc.have_measured)
				pcb_message(PCB_MSG_INFO, "%m+measured value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.measured_value);
			pcb_message(PCB_MSG_INFO, "%s\n\n", v->description);
		}
	}
	if (pcb_strcasecmp(dlg_type, "dump") == 0) {
		pcb_view_t *v;
		char *s;
		for(v = pcb_view_list_first(lst); v != NULL; v = pcb_view_list_next(v)) {
			printf("V%ld\n", v->uid);
			printf("T%s\n", v->type);
			printf("t%s\n", v->title);
			if (v->have_bbox)
				pcb_printf("B%mm %mm %mm %mm mm\n", v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				pcb_printf("A%mm %mm mm\n", v->x, v->y);
			pcb_printf("R%$mm\n", v->data.drc.required_value);
			if (v->data.drc.have_measured)
				pcb_printf("M%$mm\n", v->data.drc.measured_value);
			for(s = v->description; *s != '\0'; s++)
				if (*s == '\n')
					*s = ' ';
			printf("E%s\n\n", v->description);
		}
	}
	else if (pcb_strcasecmp(dlg_type, "count") == 0)
		PCB_ACT_IRES(pcb_view_list_length(lst));


	if (refresh != NULL)
		pcb_view_list_free_fields(lst);

	return 0;
}

void pcb_drc_all(void)
{
	pcb_event(PCB_EVENT_DRC_RUN, NULL);
}

static const char pcb_acts_DRC[] = "DRC([list|simple|print|log|dump])";
static const char pcb_acth_DRC[] = "Invoke the DRC check. Results are presented as the argument requests.";
/* DOC: drc.html */
static fgw_error_t pcb_act_DRC(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	PCB_ACT_MAY_CONVARG(1, FGW_STR, DRC, dlg_type = argv[1].val.str);
	return view_dlg(res, argc, argv, dlg_type, "drcdialog", &pcb_drc_lst, pcb_drc_all);
}

const char pcb_acts_IOIncompatList[] = "IOIncompatList([list|simple])\n";
const char pcb_acth_IOIncompatList[] = "Present the format incompatibilities of the last save to file operation.";
fgw_error_t pcb_act_IOIncompatList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	const char *aauto = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, IOIncompatList, dlg_type = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, IOIncompatList, aauto = argv[2].val.str);

	if ((aauto != NULL) && (strcmp(aauto, "auto") == 0)) {
		if (conf_core.rc.quiet && !PCB_HAVE_GUI_ATTR_DLG) {
			/* if not explicitly asked for a listing style and we are on CLI and quiet is set, don't print anything */
			PCB_ACT_IRES(0);
			return 0;
		}
	}

	return view_dlg(res, argc, argv, dlg_type, "ioincompatlistdialog", &pcb_drc_lst, pcb_drc_all);
}

pcb_action_t find_action_list[] = {
	{"DRC", pcb_act_DRC, pcb_acth_DRC, pcb_acts_DRC},
	{"IOIncompatList", pcb_act_IOIncompatList, pcb_acth_IOIncompatList, pcb_acts_IOIncompatList},
};

PCB_REGISTER_ACTIONS(find_action_list, NULL)

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


static const char pcb_acts_DRC[] = "DRC([list|simple|print|log|dump])";
static const char pcb_acth_DRC[] = "Invoke the DRC check. Results are presented as the argument requests.";
/* DOC: drc.html */
static fgw_error_t pcb_act_DRC(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	fgw_arg_t args[2];

	PCB_ACT_MAY_CONVARG(1, FGW_STR, DRC, dlg_type = argv[1].val.str);
	args[1].type = FGW_STR;

	if (pcb_strcasecmp(dlg_type, "list") == 0) {
		args[1].val.str = "list";
		return pcb_actionv_bin("drcdialog", res, 2, args);
	}

	if (pcb_strcasecmp(dlg_type, "simple") == 0) {
		args[1].val.str = "simple";
		return pcb_actionv_bin("drcdialog", res, 2, args);
	}


	PCB_ACT_IRES(-1);
	pcb_drc_all();

	if (pcb_strcasecmp(dlg_type, "print") == 0) {
		pcb_view_t *v;
		for(v = pcb_view_list_first(&pcb_drc_lst); v != NULL; v = pcb_view_list_next(v)) {
			printf("%ld: %s: %s\n", v->uid, v->type, v->title);
			if (v->have_bbox)
				pcb_printf("%m+within %$m4\n", conf_core.editor.grid_unit->allow, v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				pcb_printf("%m+at %$m2\n", conf_core.editor.grid_unit->allow, v->x, v->y);
			pcb_printf("%m+required value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.required_value);
			if (v->data.drc.have_measured)
				pcb_printf("%m+measured value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.measured_value);
			printf("%s\n\n", v->explanation);
		}
	}
	else if (pcb_strcasecmp(dlg_type, "log") == 0) {
		pcb_view_t *v;
		for(v = pcb_view_list_first(&pcb_drc_lst); v != NULL; v = pcb_view_list_next(v)) {
			pcb_message(PCB_MSG_INFO, "%ld: %s: %s\n", v->uid, v->type, v->title);
			if (v->have_bbox)
				pcb_message(PCB_MSG_INFO, "%m+within %$m4\n", conf_core.editor.grid_unit->allow, v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
			if (v->have_xy)
				pcb_message(PCB_MSG_INFO, "%m+at %$m2\n", conf_core.editor.grid_unit->allow, v->x, v->y);
			pcb_printf("%m+required value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.required_value);
			if (v->data.drc.have_measured)
				pcb_message(PCB_MSG_INFO, "%m+measured value %$mS\n", conf_core.editor.grid_unit->allow, v->data.drc.measured_value);
			pcb_message(PCB_MSG_INFO, "%s\n\n", v->explanation);
		}
	}
	if (pcb_strcasecmp(dlg_type, "dump") == 0) {
		pcb_view_t *v;
		char *s;
		for(v = pcb_view_list_first(&pcb_drc_lst); v != NULL; v = pcb_view_list_next(v)) {
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
			for(s = v->explanation; *s != '\0'; s++)
				if (*s == '\n')
					*s = ' ';
			printf("E%s\n\n", v->explanation);
		}
	}
	else if (pcb_strcasecmp(dlg_type, "count") == 0)
		PCB_ACT_IRES(pcb_view_list_length(&pcb_drc_lst));


	pcb_view_list_free_fields(&pcb_drc_lst);


	return 0;
}

pcb_action_t find_action_list[] = {
	{"DRC", pcb_act_DRC, pcb_acth_DRC, pcb_acts_DRC}
};

PCB_REGISTER_ACTIONS(find_action_list, NULL)

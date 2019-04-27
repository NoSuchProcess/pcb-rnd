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

#include <genvector/gds_char.h>

#include "board.h"
#include "hid.h"
#include "hid_cfg_input.h"
#include "hid_dad.h"
#include "conf_core.h"
#include "crosshair.h"

#include "status.h"

typedef struct {
	pcb_hid_dad_subdialog_t stsub, rdsub; /* st is for the bottom status line, rd is for the top readouts */
	int stsub_inited, rdsub_inited;
	int wst1, wst2, wsttxt;
	int st_has_text;
	int wrdunit, wrd1[3], wrd2[2];
	gds_t buf; /* save on allocation */
} status_ctx_t;

static status_ctx_t status;

static void build_st_line1(void)
{
	char kbd[128];
	const char *flag = conf_core.editor.all_direction_lines ? "*" : (conf_core.editor.line_refraction == 0 ? "X" : (conf_core.editor.line_refraction == 1 ? "_/" : "\\_"));
	pcb_hid_cfg_keys_t *kst = pcb_gui->key_state;

	if (kst != NULL) {
		if (kst->seq_len_action > 0) {
			int len;
			memcpy(kbd, "(last: ", 7);
			len = pcb_hid_cfg_keys_seq(kst, kbd+7, sizeof(kbd)-9);
			memcpy(kbd+len+7, ")", 2);
		}
		else
			pcb_hid_cfg_keys_seq(kst, kbd, sizeof(kbd));
	}
	else
		*kbd = '\0';


	pcb_append_printf(&status.buf,
	"%m+view=%s  "
	"grid=%$mS  "
	"line=%mS (%s%s) "
	"kbd=%s",
	conf_core.editor.grid_unit->allow, conf_core.editor.show_solder_side ? "bottom" : "top",
	PCB->hidlib.grid,
	conf_core.design.line_thickness, flag, conf_core.editor.rubber_band_mode ? ",R" : "",
	kbd);
}

static void build_st_line2(void)
{
	pcb_append_printf(&status.buf,
		"via=%mS (%mS)  "
		"clr=%mS  "
		"text=%d%% %$mS "
		"buff=#%d",
		conf_core.design.via_thickness, conf_core.design.via_drilling_hole,
		conf_core.design.clearance,
		conf_core.design.text_scale,
		conf_core.design.text_thickness,
		conf_core.editor.buffer_number + 1);
}

static void build_st_help(void)
{
	static const pcb_unit_t *unit_mm = NULL, *unit_mil;
	const pcb_unit_t *unit_inv;

	if (unit_mm == NULL) { /* cache mm and mil units to save on the lookups */
		unit_mm  = get_unit_struct("mm");
		unit_mil = get_unit_struct("mil");
	}
	if (conf_core.editor.grid_unit == unit_mm)
		unit_inv = unit_mil;
	else
		unit_inv = unit_mm;

	pcb_append_printf(&status.buf,
		"%m+"
		"grid=%$mS  "
		"line=%mS "
		"via=%mS (%mS) "
		"clearance=%mS",
		unit_inv->allow,
		PCB->hidlib.grid,
		conf_core.design.line_thickness,
		conf_core.design.via_thickness, conf_core.design.via_drilling_hole, 
		conf_core.design.clearance);
}


static void status_st_pcb2dlg(void)
{
	static pcb_hid_attr_val_t hv;

	if (status.stsub_inited) {
		status.buf.used = 0;
		build_st_line1();
		if (!conf_core.appearance.compact) {
			build_st_line2();
			pcb_gui->attr_dlg_widget_hide(status.stsub.dlg_hid_ctx, status.wst2, 1);
		}
		hv.str_value = status.buf.array;
		pcb_gui->attr_dlg_set_value(status.stsub.dlg_hid_ctx, status.wst1, &hv);

		if (conf_core.appearance.compact) {
			status.buf.used = 0;
			build_st_line2();
			hv.str_value = status.buf.array;
			pcb_gui->attr_dlg_set_value(status.stsub.dlg_hid_ctx, status.wst2, &hv);
			if (!status.st_has_text)
				pcb_gui->attr_dlg_widget_hide(status.stsub.dlg_hid_ctx, status.wst2, 0);
		}

		status.buf.used = 0;
		build_st_help();
		pcb_gui->attr_dlg_set_help(status.stsub.dlg_hid_ctx, status.wst1, status.buf.array);
		pcb_gui->attr_dlg_set_help(status.stsub.dlg_hid_ctx, status.wst2, status.buf.array);
	}
}

static void status_rd_pcb2dlg(void)
{
	static pcb_hid_attr_val_t hv;

	if (status.rdsub_inited) {
		/* coordinate readout (right side box) */
		if (conf_core.appearance.compact) {
			status.buf.used = 0;
			pcb_append_printf(&status.buf, "%m+%-mS", conf_core.editor.grid_unit->allow, pcb_crosshair.X);
			hv.str_value = status.buf.array;
			pcb_gui->attr_dlg_set_value(status.rdsub.dlg_hid_ctx, status.wrd2[0], &hv);

			status.buf.used = 0;
			pcb_append_printf(&status.buf, "%m+%-mS", conf_core.editor.grid_unit->allow, pcb_crosshair.Y);
			hv.str_value = status.buf.array;
			pcb_gui->attr_dlg_set_value(status.rdsub.dlg_hid_ctx, status.wrd2[1], &hv);
			pcb_gui->attr_dlg_widget_hide(status.rdsub.dlg_hid_ctx, status.wrd2[1], 0);
		}
		else {
			status.buf.used = 0;
			pcb_append_printf(&status.buf, "%m+%-mS %-mS", conf_core.editor.grid_unit->allow, pcb_crosshair.X, pcb_crosshair.Y);
			hv.str_value = status.buf.array;
			pcb_gui->attr_dlg_set_value(status.rdsub.dlg_hid_ctx, status.wrd2[0], &hv);
			pcb_gui->attr_dlg_widget_hide(status.rdsub.dlg_hid_ctx, status.wrd2[1], 1);
		}
	}
}

static void status_docked_create_st()
{
	PCB_DAD_BEGIN_VBOX(status.stsub.dlg);
		PCB_DAD_COMPFLAG(status.stsub.dlg, PCB_HATF_EXPFILL | PCB_HATF_TIGHT);
		PCB_DAD_LABEL(status.stsub.dlg, "");
			PCB_DAD_COMPFLAG(status.stsub.dlg, PCB_HATF_HIDE);
			status.wsttxt = PCB_DAD_CURRENT(status.stsub.dlg);
		PCB_DAD_LABEL(status.stsub.dlg, "<pending update>");
			status.wst1 = PCB_DAD_CURRENT(status.stsub.dlg);
		PCB_DAD_LABEL(status.stsub.dlg, "<pending update>");
			PCB_DAD_COMPFLAG(status.stsub.dlg, PCB_HATF_HIDE);
			status.wst2 = PCB_DAD_CURRENT(status.stsub.dlg);
	PCB_DAD_END(status.stsub.dlg);
}


static void status_docked_create_rd()
{
	int n;
	PCB_DAD_BEGIN_HBOX(status.rdsub.dlg);
		PCB_DAD_COMPFLAG(status.rdsub.dlg, PCB_HATF_TIGHT);
		PCB_DAD_BEGIN_VBOX(status.rdsub.dlg);
			PCB_DAD_COMPFLAG(status.rdsub.dlg, PCB_HATF_EXPFILL | PCB_HATF_FRAME);
			for(n = 0; n < 3; n++) {
				PCB_DAD_LABEL(status.rdsub.dlg, "<pending>");
					status.wrd1[n] = PCB_DAD_CURRENT(status.rdsub.dlg);
			}
		PCB_DAD_END(status.rdsub.dlg);
		PCB_DAD_BEGIN_VBOX(status.rdsub.dlg);
			PCB_DAD_COMPFLAG(status.rdsub.dlg, PCB_HATF_EXPFILL | PCB_HATF_FRAME);
			PCB_DAD_BEGIN_VBOX(status.rdsub.dlg);
				PCB_DAD_COMPFLAG(status.rdsub.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(status.rdsub.dlg);
			for(n = 0; n < 2; n++) {
					PCB_DAD_LABEL(status.rdsub.dlg, "<pending>");
							status.wrd2[n] = PCB_DAD_CURRENT(status.rdsub.dlg);
			}
			PCB_DAD_BEGIN_VBOX(status.rdsub.dlg);
				PCB_DAD_COMPFLAG(status.rdsub.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(status.rdsub.dlg);
		PCB_DAD_END(status.rdsub.dlg);
		PCB_DAD_BUTTON(status.rdsub.dlg, "<un>");
			status.wrdunit = PCB_DAD_CURRENT(status.rdsub.dlg);
	PCB_DAD_END(status.rdsub.dlg);
}


void pcb_status_gui_init_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL)) {
		status_docked_create_st();
		if (pcb_hid_dock_enter(&status.stsub, PCB_HID_DOCK_BOTTOM, "status") == 0) {
			status.stsub_inited = 1;
			status_st_pcb2dlg();
		}

		status_docked_create_rd();
		if (pcb_hid_dock_enter(&status.rdsub, PCB_HID_DOCK_TOP_RIGHT, "readout") == 0) {
			status.rdsub_inited = 1;
			status_rd_pcb2dlg();
		}
	}
}

void pcb_status_st_update_conf(conf_native_t *cfg, int arr_idx)
{
	status_st_pcb2dlg();
}

void pcb_status_rd_update_conf(conf_native_t *cfg, int arr_idx)
{
	status_rd_pcb2dlg();
}

void pcb_status_st_update_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	status_st_pcb2dlg();
}

void pcb_status_rd_update_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	status_rd_pcb2dlg();
}

const char pcb_acts_StatusSetText[] = "StatusSetText([text])\n";
const char pcb_acth_StatusSetText[] = "Replace status printout with text temporarily; turn status printout back on if text is not provided.";
fgw_error_t pcb_act_StatusSetText(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *text = NULL;

	if (argc > 2)
		PCB_ACT_FAIL(StatusSetText);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, StatusSetText, text = argv[1].val.str);

	if (text != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = text;
		pcb_gui->attr_dlg_set_value(status.stsub.dlg_hid_ctx, status.wsttxt, &hv);
		hv.str_value = "";
		pcb_gui->attr_dlg_set_value(status.stsub.dlg_hid_ctx, status.wst2, &hv);
		pcb_gui->attr_dlg_widget_hide(status.stsub.dlg_hid_ctx, status.wst1, 1);
		pcb_gui->attr_dlg_widget_hide(status.stsub.dlg_hid_ctx, status.wsttxt, 0);
		status.st_has_text = 1;
	}
	else {
		status.st_has_text = 0;
		pcb_gui->attr_dlg_widget_hide(status.stsub.dlg_hid_ctx, status.wst1, 0);
		pcb_gui->attr_dlg_widget_hide(status.stsub.dlg_hid_ctx, status.wsttxt, 1);
		status_st_pcb2dlg();
	}

	PCB_ACT_IRES(0);
	return 0;
}

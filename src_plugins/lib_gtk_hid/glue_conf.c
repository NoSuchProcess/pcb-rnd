/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas
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
#include "glue_conf.h"
#include "glue_event.h"
#include "conf.h"
#include "conf_core.h"
#include "gui.h"

static void ghid_confchg_rst(conf_native_t *cfg, int arr_idx)
{
	pcb_ghid_rst_chg();
}

/* Update the status line - should be bound to any config value displayed there */
static void ghid_confchg_status_line(conf_native_t *cfg, int arr_idx)
{
	/* test if PCB struct doesn't exist at startup */
	if ((PCB == NULL) || (ghidgui->common.set_status_line_label == NULL))
		return;
	ghidgui->common.set_status_line_label();
}

static void ghid_confchg_fullscreen(conf_native_t *cfg, int arr_idx)
{
	if (ghidgui->hid_active)
		ghid_fullscreen_apply(&ghidgui->topwin);
}


void ghid_confchg_checkbox(conf_native_t *cfg, int arr_idx)
{
	if (ghidgui->hid_active)
		ghid_update_toggle_flags(&ghidgui->topwin, NULL);
}

static void ghid_confchg_grid_unit(conf_native_t *cfg, int arr_idx)
{
	/* test if PCB struct doesn't exist at startup */
	if ((PCB == NULL) || !ghidgui->hid_active)
		return;

	ghid_handle_units_changed(&ghidgui->topwin);
	ghidgui->common.set_status_line_label();
}

static void ghid_confchg_compact(conf_native_t *cfg, int arr_idx)
{
	/* test if PCB struct doesn't exist at startup */
	if ((PCB == NULL) || !ghidgui->hid_active)
		return;

	ghid_command_update_prompt(&ghidgui->topwin.cmd);
	ghidgui->common.set_status_line_label();
}

static void ghid_confchg_cli(conf_native_t *cfg, int arr_idx)
{
	ghid_command_update_prompt(&ghidgui->topwin.cmd);
}

static void ghid_confchg_spec_color(conf_native_t *cfg, int arr_idx)
{
	if (!ghidgui->hid_active)
		return;

	if (ghidgui->common.set_special_colors != NULL)
		ghidgui->common.set_special_colors(cfg);
}



static void init_conf_watch(conf_hid_callbacks_t *cbs, const char *path, void (*func)(conf_native_t *, int))
{
	conf_native_t *n = conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs->val_change_post = func;
		conf_hid_set_cb(n, ghidgui->conf_id, cbs);
	}
}

void ghid_conf_regs(const char *cookie)
{
	static conf_hid_callbacks_t 
		cbs_refraction, cbs_direction, cbs_fullscreen, cbs_show_sside, cbs_grid,
		cbs_text_scale, cbs_grid_unit, cbs_rst[4], cbs_cli[2], cbs_compactv,
		cbs_compacth, cbs_color[3];

	ghidgui->conf_id = conf_hid_reg(cookie, NULL);

	init_conf_watch(&cbs_direction, "editor/all_direction_lines", ghid_confchg_status_line);
	init_conf_watch(&cbs_refraction, "editor/line_refraction", ghid_confchg_status_line);
	init_conf_watch(&cbs_show_sside, "editor/show_solder_side", ghid_confchg_status_line);
	init_conf_watch(&cbs_grid, "editor/grid", ghid_confchg_status_line);
	init_conf_watch(&cbs_text_scale, "design/text_scale", ghid_confchg_status_line);
	init_conf_watch(&cbs_text_scale, "design/text_thickness", ghid_confchg_status_line);

	init_conf_watch(&cbs_fullscreen, "editor/fullscreen", ghid_confchg_fullscreen);
	init_conf_watch(&cbs_grid_unit, "editor/grid_unit", ghid_confchg_grid_unit);

	init_conf_watch(&cbs_rst[0], "design/line_thickness", ghid_confchg_rst);
	init_conf_watch(&cbs_rst[1], "design/clearance", ghid_confchg_rst);
TODO("padstack: remove some paths when route style has proto")
	init_conf_watch(&cbs_rst[2], "design/via_thickness", ghid_confchg_rst);
	init_conf_watch(&cbs_rst[3], "design/via_drilling_hole", ghid_confchg_rst);

	init_conf_watch(&cbs_cli[0], "rc/cli_prompt", ghid_confchg_cli);
	init_conf_watch(&cbs_cli[1], "rc/cli_backend", ghid_confchg_cli);

	init_conf_watch(&cbs_compacth, "appearance/compact", ghid_confchg_compact);

	init_conf_watch(&cbs_color[0], "appearance/color/background", ghid_confchg_spec_color);
	init_conf_watch(&cbs_color[1], "appearance/color/off_limit", ghid_confchg_spec_color);
	init_conf_watch(&cbs_color[2], "appearance/color/grid", ghid_confchg_spec_color);
}

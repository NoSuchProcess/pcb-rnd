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

#ifndef PCB_HIDLIB_CONF_H
#define PCB_HIDLIB_CONF_H

#include "conf.h"

extern CFT_INTEGER *pcbhlc_rc_verbose;
extern CFT_INTEGER *pcbhlc_rc_quiet;
extern CFT_BOOLEAN *pcbhlc_rc_dup_log_to_stderr;
extern CFT_STRING *pcbhlc_rc_cli_prompt;
extern CFT_STRING *pcbhlc_rc_cli_backend;
extern CFT_BOOLEAN *pcbhlc_rc_export_basename;
extern CFT_STRING *pcbhlc_rc_path_exec_prefix;
extern CFT_STRING *pcbhlc_rc_path_home;
extern CFT_STRING *pcbhlc_rc_menu_file;

extern CFT_BOOLEAN *pcbhlc_appearance_loglevels_debug_popup;
extern CFT_BOOLEAN *pcbhlc_appearance_loglevels_info_popup;
extern CFT_BOOLEAN *pcbhlc_appearance_loglevels_warning_popup;
extern CFT_BOOLEAN *pcbhlc_appearance_loglevels_error_popup;
extern CFT_STRING *pcbhlc_appearance_loglevels_debug_tag;
extern CFT_STRING *pcbhlc_appearance_loglevels_info_tag;
extern CFT_STRING *pcbhlc_appearance_loglevels_warning_tag;
extern CFT_STRING *pcbhlc_appearance_loglevels_error_tag;
extern CFT_COLOR *pcbhlc_appearance_color_background;
extern CFT_COLOR *pcbhlc_appearance_color_grid;
extern CFT_COLOR *pcbhlc_appearance_color_off_limit;
extern CFT_COLOR *pcbhlc_appearance_color_cross;
extern CFT_REAL *pcbhlc_appearance_layer_alpha;
extern CFT_REAL *pcbhlc_appearance_drill_alpha;

extern CFT_UNIT *pcbhlc_editor_grid_unit;
extern CFT_BOOLEAN *pcbhlc_editor_view_flip_x;
extern CFT_BOOLEAN *pcbhlc_editor_view_flip_y;
extern CFT_BOOLEAN *pcbhlc_editor_fullscreen;
extern CFT_BOOLEAN *pcbhlc_editor_auto_place;
extern CFT_BOOLEAN *pcbhlc_editor_draw_grid;

int pcb_hidlib_conf_init();

#endif

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

#include "conf.h"
#include "error.h"

#include "hidlib_conf.h"

CFT_INTEGER *pcbhlc_rc_verbose;
CFT_INTEGER *pcbhlc_rc_quiet;
CFT_BOOLEAN *pcbhlc_rc_dup_log_to_stderr;
CFT_STRING *pcbhlc_rc_cli_prompt;
CFT_STRING *pcbhlc_rc_cli_backend;
CFT_BOOLEAN *pcbhlc_rc_export_basename;
CFT_STRING *pcbhlc_rc_path_exec_prefix;
CFT_STRING *pcbhlc_rc_path_home;
CFT_STRING *pcbhlc_rc_menu_file;

CFT_BOOLEAN *pcbhlc_appearance_loglevels_debug_popup;
CFT_BOOLEAN *pcbhlc_appearance_loglevels_info_popup;
CFT_BOOLEAN *pcbhlc_appearance_loglevels_warning_popup;
CFT_BOOLEAN *pcbhlc_appearance_loglevels_error_popup;
CFT_STRING *pcbhlc_appearance_loglevels_debug_tag;
CFT_STRING *pcbhlc_appearance_loglevels_info_tag;
CFT_STRING *pcbhlc_appearance_loglevels_warning_tag;
CFT_STRING *pcbhlc_appearance_loglevels_error_tag;

CFT_UNIT *pcbhlc_editor_grid_unit;

static union {
	CFT_INTEGER i;
	CFT_BOOLEAN b;
	CFT_STRING s;
	CFT_UNIT c;
} pcb_hidlib_zero; /* implicit initialized to 0 */


#define SCALAR(name, sname, type, typef) \
do { \
	int r; \
	pcb_conf_resolve_t rsv = {sname, type, 0, NULL}; \
	r = pcb_conf_resolve(&rsv); \
	cnt += r; \
	if (r <= 0) { \
		pcbhlc_ ## name = (void *)&pcb_hidlib_zero; \
		pcb_message(PCB_MSG_ERROR, "hidlib: pcb_hidlib_conf_init(): filed to resolve hidlib conf path %s\n", sname); \
	} \
	else \
		pcbhlc_ ## name = rsv.nat->val.typef; \
} while(0)

int pcb_hidlib_conf_init()
{
	int cnt = 0;

	SCALAR(rc_verbose,           "rc/verbose",           CFN_INTEGER, integer);
	SCALAR(rc_quiet,             "rc/quiet",             CFN_INTEGER, integer);
	SCALAR(rc_dup_log_to_stderr, "rc/dup_log_to_stderr", CFN_BOOLEAN, boolean);

	SCALAR(rc_cli_prompt,        "rc/cli_prompt",        CFN_STRING,  string);
	SCALAR(rc_cli_backend,       "rc/cli_backend",       CFN_STRING,  string);
	SCALAR(rc_export_basename,   "rc/export_basename",   CFN_BOOLEAN, boolean);
	SCALAR(rc_path_exec_prefix,  "rc/path/exec_prefix",  CFN_STRING,  string);
	SCALAR(rc_path_home,         "rc/path/home",         CFN_STRING,  string);
	SCALAR(rc_menu_file,         "rc/menu_file",         CFN_STRING,  string);

	SCALAR(appearance_loglevels_debug_popup,    "appearance/loglevels/debug_popup",    CFN_BOOLEAN, boolean);
	SCALAR(appearance_loglevels_info_popup,     "appearance/loglevels/info_popup",     CFN_BOOLEAN, boolean);
	SCALAR(appearance_loglevels_warning_popup,  "appearance/loglevels/warning_popup",  CFN_BOOLEAN, boolean);
	SCALAR(appearance_loglevels_error_popup,    "appearance/loglevels/error_popup",    CFN_BOOLEAN, boolean);
	SCALAR(appearance_loglevels_debug_tag,      "appearance/loglevels/debug_tag",      CFN_STRING,  string);
	SCALAR(appearance_loglevels_info_tag,       "appearance/loglevels/info_tag",       CFN_STRING,  string);
	SCALAR(appearance_loglevels_warning_tag,    "appearance/loglevels/warning_tag",    CFN_STRING,  string);
	SCALAR(appearance_loglevels_error_tag,      "appearance/loglevels/error_tag",      CFN_STRING,  string);

	SCALAR(editor_grid_unit,  "editor/grid_unit",  CFN_UNIT,  unit);

	return cnt;
}

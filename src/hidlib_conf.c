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

#include "hidlib_conf.h"

CFT_INTEGER *pcbhlc_rc_verbose;
CFT_INTEGER *pcbhlc_rc_quiet;
CFT_STRING *pcbhlc_rc_cli_prompt;
CFT_STRING *pcbhlc_rc_cli_backend;

static union {
	CFT_INTEGER i;
	CFT_STRING s;
} pcb_hidlib_zero; /* implicit initialized to 0 */


#define SCALAR(name, sname, type, typef) \
do { \
	int r; \
	pcb_conf_resolve_t rsv = {sname, type, 0, NULL}; \
	r = pcb_conf_resolve(&rsv); \
	cnt += r; \
	if (r > 0) \
		pcbhlc_ ## name = rsv.nat->val.typef; \
	else \
		pcbhlc_ ## name = (void *)&pcb_hidlib_zero; \
} while(0)

int pcb_hidlib_conf_init()
{
	int cnt = 0;
	SCALAR(rc_verbose,      "rc/verbose",      CFN_INTEGER, integer);
	SCALAR(rc_quiet,        "rc/quiet",        CFN_INTEGER, integer);
	SCALAR(rc_cli_prompt,   "rc/cli_prompt",   CFN_STRING,  string);
	SCALAR(rc_cli_backend,  "rc/cli_backend",  CFN_STRING,  string);

	return cnt;
}

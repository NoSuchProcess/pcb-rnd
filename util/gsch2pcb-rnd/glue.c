/* gsch2pcb-rnd
 *  (C) 2015..2016, Tibor 'Igor2' Palinkas
 *
 *  This program is free software which I release under the GNU General Public
 *  License. You may redistribute and/or modify this program under the terms
 *  of that license as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.  Version 2 is in the
 *  COPYRIGHT file in the top level directory of this distribution.
 *
 *  To get a copy of the GNU General Puplic License, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdio.h>
#include <stdarg.h>
#include "../src/error.h"
#include "../src/plugins.h"
#include "gsch2pcb_rnd_conf.h"

/* glue for pcb-rnd core */

void pcb_chdir_error_message(const char *DirName)
{
	pcb_message(PCB_MSG_WARNING, "warning: can't cd to %s\n", DirName);
}

void pcb_opendir_error_message(const char *DirName)
{
	pcb_message(PCB_MSG_WARNING, "warning: can't opendir %s\n", DirName);
}

void pcb_popen_error_message(const char *cmd)
{
	pcb_message(PCB_MSG_WARNING, "warning: can't popen %s\n", cmd);
}

void pcb_message(enum pcb_message_level level, const char *fmt, ...)
{
	va_list args;
	fprintf(stderr, "gsch2pcb-rnd: ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void pcb_trace(const char *Format, ...)
{
#ifndef NDEBUG
	va_list args;
	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);
#endif
}

const char *pcb_board_get_filename(void) { return NULL; }
const char *pcb_board_get_name(void) { return NULL; }

pcb_plugin_info_t *pcb_plugin_register(const char *name, const char *path, void *handle, int dynamic_loaded, void (*uninit)(void))
{
	static pcb_plugin_info_t pif;
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("Plugin loaded: %s (%s)\n", name, path);
	return &pif;
}

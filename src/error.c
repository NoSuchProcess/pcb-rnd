/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *
 */

#include "config.h"

#include <errno.h>
#include <stdarg.h>

#include "data.h"
#include "error.h"
#include "conf_core.h"

void pcb_message(enum pcb_message_level level, const char *Format, ...)
{
	va_list args;
	pcb_message_level_t min_level = PCB_MSG_INFO;

	if (pcb_gui != NULL) {
		va_start(args, Format);
		pcb_gui->logv(level, Format, args);
		va_end(args);
	}

	if (conf_core.rc.quiet)
		min_level = PCB_MSG_ERROR;

	if ((level >= min_level) || (conf_core.rc.verbose)) {
		va_start(args, Format);
		pcb_vfprintf(stderr, Format, args);
		va_end(args);
	}
}

void pcb_trace(const char *Format, ...)
{
#ifndef NDEBUG
	va_list args;
	va_start(args, Format);
	pcb_vfprintf(stderr, Format, args);
	va_end(args);
#endif
}


/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* error and debug functions */

#include "config.h"

#include <errno.h>
#include <stdarg.h>

#include "data.h"
#include "error.h"
#include "plug_io.h"
#include "compat_misc.h"
#include "compat_nls.h"
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


void pcb_open_error_message(const char *filename)
{
	pcb_message(PCB_MSG_ERROR, _("Can't open file\n" "   '%s'\nfopen() returned: '%s'\n"), filename, strerror(errno));
}

void pcb_popen_error_message(const char *filename)
{
	pcb_message(PCB_MSG_ERROR, _("Can't execute command\n" "   '%s'\npopen() returned: '%s'\n"), filename, strerror(errno));
}

void pcb_opendir_error_message(const char *dirname)
{
	pcb_message(PCB_MSG_ERROR, _("Can't scan directory\n" "   '%s'\nopendir() returned: '%s'\n"), dirname, strerror(errno));
}

void pcb_chdir_error_message(const char *dirname)
{
	pcb_message(PCB_MSG_ERROR, _("Can't change working directory to\n" "   '%s'\nchdir() returned: '%s'\n"), dirname, strerror(errno));
}

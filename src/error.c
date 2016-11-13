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
#include <signal.h>

#include "data.h"
#include "error.h"
#include "plug_io.h"
#include "compat_misc.h"
#include "compat_nls.h"

#define utf8_dup_string(a,b) *(a) = pcb_strdup(b)

/* ----------------------------------------------------------------------
 * some external identifiers
 */

/* the list is already defined for some OS */
#if !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__linux__) && !defined(__DragonFly__)
#ifdef USE_SYS_ERRLIST
extern char *sys_errlist[];			/* array of error messages */
#endif
#endif


/* ---------------------------------------------------------------------------
 * output of message in a dialog window or log window
 */
void pcb_message(enum pcb_message_level level, const char *Format, ...)
{
	va_list args;

	/* TODO(hzeller): do something useful with the level, e.g. color coding. */

	if (gui != NULL) {
		va_start(args, Format);
		gui->logv(level, Format, args);
		va_end(args);
	}

	va_start(args, Format);
	pcb_vfprintf(stderr, Format, args);
	va_end(args);
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


/* ---------------------------------------------------------------------------
 * print standard 'open error'
 */
void OpenErrorpcb_message(const char *Filename)
{
	char *utf8 = NULL;

	utf8_dup_string(&utf8, Filename);
	pcb_message(PCB_MSG_DEFAULT, _("Can't open file\n" "   '%s'\nfopen() returned: '%s'\n"), utf8, strerror(errno));
	free(utf8);
}

/* ---------------------------------------------------------------------------
 * print standard 'popen error'
 */
void pcb_popen_error_message(const char *Filename)
{
	char *utf8 = NULL;

	utf8_dup_string(&utf8, Filename);
	pcb_message(PCB_MSG_DEFAULT, _("Can't execute command\n" "   '%s'\npopen() returned: '%s'\n"), utf8, strerror(errno));
	free(utf8);
}

/* ---------------------------------------------------------------------------
 * print standard 'opendir'
 */
void pcb_opendir_error_message(const char *DirName)
{
	char *utf8 = NULL;

	utf8_dup_string(&utf8, DirName);
	pcb_message(PCB_MSG_DEFAULT, _("Can't scan directory\n" "   '%s'\nopendir() returned: '%s'\n"), utf8, strerror(errno));
	free(utf8);
}

/* ---------------------------------------------------------------------------
 * print standard 'chdir error'
 */
void pcb_chdir_error_message(const char *DirName)
{
	char *utf8 = NULL;

	utf8_dup_string(&utf8, DirName);
	pcb_message(PCB_MSG_DEFAULT, _("Can't change working directory to\n" "   '%s'\nchdir() returned: '%s'\n"), utf8, strerror(errno));
	free(utf8);
}

/* ---------------------------------------------------------------------------
 * catches signals which abort the program
 */
void CatchSignal(int Signal)
{
	const char *s;

	switch (Signal) {
#ifdef SIGHUP
	case SIGHUP:
		s = "SIGHUP";
		break;
#endif
	case SIGINT:
		s = "SIGINT";
		break;
#ifdef SIGQUIT
	case SIGQUIT:
		s = "SIGQUIT";
		break;
#endif
	case SIGABRT:
		s = "SIGABRT";
		break;
	case SIGTERM:
		s = "SIGTERM";
		break;
	case SIGSEGV:
		s = "SIGSEGV";
		break;
	default:
		s = "unknown";
		break;
	}
	pcb_message(PCB_MSG_ERROR, "aborted by %s signal\n", s);
	exit(1);
}

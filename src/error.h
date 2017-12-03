/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

/* prototypes for error and debug functions */

#ifndef	PCB_ERROR_H
#define	PCB_ERROR_H

typedef enum pcb_message_level {
	PCB_MSG_DEBUG = 0,   /* Debug message. Should probably not be shown in regular operation. */
	PCB_MSG_INFO,        /* Info message. FYI for the user, no action needed. */
	PCB_MSG_WARNING,     /* Something the user should probably take note */
	PCB_MSG_ERROR        /* Couldn't finish an action, needs user attention. */
} pcb_message_level_t;

/* printf-like logger to the log dialog and stderr */
void pcb_message(enum pcb_message_level level, const char *Format, ...);

/* shorthands for indicating common errors using pcb_message() */
void pcb_open_error_message(const char *);
void pcb_popen_error_message(const char *);
void pcb_opendir_error_message(const char *);
void pcb_chdir_error_message(const char *);

/* pcb_printf()-like call to print temporary trace messages to stderr;
   disabled in non-debug compilation */
void pcb_trace(const char *Format, ...);

#endif

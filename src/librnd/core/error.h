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

/* Messages, error reporting, debug and logging */

#ifndef RND_ERROR_H
#define RND_ERROR_H

#include <time.h>
#include <librnd/core/global_typedefs.h>

/* rnd_printf()-like call to print temporary trace messages to stderr;
   disabled in non-debug compilation */
void rnd_trace(const char *Format, ...);

typedef enum rnd_message_level_s {
	RND_MSG_DEBUG = 0,   /* Debug message. Should probably not be shown in regular operation. */
	RND_MSG_INFO,        /* Info message. FYI for the user, no action needed. */
	RND_MSG_WARNING,     /* Something the user should probably take note */
	RND_MSG_ERROR        /* Couldn't finish an action, needs user attention. */
} rnd_message_level_t;

/*** central log write API ***/

/* printf-like logger to the log dialog and stderr */
void rnd_message(rnd_message_level_t level, const char *Format, ...);

/* shorthands for indicating common errors using rnd_message() */
#define rnd_FS_error_message(filename, func) rnd_message(RND_MSG_ERROR, "Can't open file\n   '%s'\n" func "() returned: '%s'\n", filename, strerror(errno))

#define rnd_open_error_message(filename)     rnd_FS_error_message(filename, "open")
#define rnd_popen_error_message(filename)    rnd_FS_error_message(filename, "popen")
#define rnd_opendir_error_message(filename)  rnd_FS_error_message(filename, "opendir")
#define rnd_chdir_error_message(filename)    rnd_FS_error_message(filename, "chdir")

/*** central log storage and read API ***/

typedef struct rnd_logline_s rnd_logline_t;

struct rnd_logline_s {
	time_t stamp;
	unsigned long ID;
	rnd_message_level_t level;
	unsigned seen:1; /* message ever shown to the user - set by the code that presented the message */
	rnd_logline_t *prev, *next;
	size_t len;
	char str[1];
};

extern unsigned long rnd_log_next_ID;

/* Return the first log line that has at least the specified value in its ID. */
rnd_logline_t *rnd_log_find_min(unsigned long ID);
rnd_logline_t *rnd_log_find_min_(rnd_logline_t *from, unsigned long ID);

/* Search back from the bottom of the log and return the oldest unseen entry
   (or NULL if all entries have been shown) */
rnd_logline_t *rnd_log_find_first_unseen(void);

/* Remove log lines between ID from and to, inclusive; -1 in these fields
   mean begin or end of the list. */
void rnd_log_del_range(unsigned long from, unsigned long to);

/* Export the whole log list to a file, in lihata or plain text */
int rnd_log_export(rnd_hidlib_t *hidlib, const char *fn, int fmt_lihata);

/* Free all memory and reset the log system */
void rnd_log_uninit(void);

extern rnd_logline_t *rnd_log_first, *rnd_log_last;

#endif

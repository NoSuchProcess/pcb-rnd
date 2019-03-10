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

#include <errno.h>
#include <stdarg.h>

#include "data.h"
#include "error.h"
#include "conf_core.h"
#include "genvector/gds_char.h"

unsigned long pcb_log_next_ID = 0;
pcb_logline_t *pcb_log_first, *pcb_log_last;

void pcb_message(enum pcb_message_level level, const char *Format, ...)
{
	va_list args;
	pcb_message_level_t min_level = PCB_MSG_INFO;
	gds_t tmp;
	pcb_logline_t *line;

	if (pcb_gui != NULL) {
		va_start(args, Format);
		pcb_gui->logv(level, Format, args);
		va_end(args);
	}

	if (pcb_gui == NULL) {
		if (conf_core.rc.quiet)
			min_level = PCB_MSG_ERROR;

		if ((level >= min_level) || (conf_core.rc.verbose)) {
			va_start(args, Format);
			pcb_vfprintf(stderr, Format, args);
			va_end(args);
		}
	}

	/* allocate a new log line; for efficiency pretend it is a string during
	   the allocation */
	gds_init(&tmp);
	gds_enlarge(&tmp, sizeof(pcb_logline_t));
	tmp.used = offsetof(pcb_logline_t, str);
	pcb_append_vprintf(&tmp, Format, args);

	/* add the header and link in */
	line = (pcb_logline_t *)tmp.array;
	line->stamp = time(NULL);
	line->ID = pcb_log_next_ID++;
	line->level = level;
	line->prev = pcb_log_last;
	if (pcb_log_first == NULL)
		pcb_log_first = line;
	if (pcb_log_last != NULL)
		pcb_log_last->next = line;
	pcb_log_last = line;
	line->len = tmp.used - offsetof(pcb_logline_t, str);
}

pcb_logline_t *pcb_log_find_min_(pcb_logline_t *from, unsigned long ID)
{
	pcb_logline_t *n;
	if (ID == -1)
		return from;
	for(n = from; n != NULL; n = n->next)
		if (n->ID >= ID)
			return n;
	return NULL;
}

pcb_logline_t *pcb_log_find_min(unsigned long ID)
{
	return pcb_log_find_min_(pcb_log_first, ID);
}

void pcb_log_del_range(unsigned long from, unsigned long to)
{
	pcb_logline_t *start = pcb_log_find_min(from), *end = NULL;
	pcb_logline_t *next, *n, *start_prev, *end_next;

	if (start == NULL)
		return; /* start is beyond the end of the list - do not delete anything */

	if (to != -1)
		end = pcb_log_find_min_(start, to);
	if (end == NULL)
		end = pcb_log_last;

	/* remember boundary elems near the range */
	start_prev = start->prev;
	end_next = end->next;

	/* free all elems of the range */
	n = start;
	for(;;) {
		next = n->next;
		free(n);
		if (n == end)
			break;
		n = next;
	}

	/* unlink the whole range at once */
	if (start_prev != NULL)
		start_prev->next = end_next;
	else
		pcb_log_first = end_next;

	if (end_next != NULL)
		end_next->prev = start_prev;
	else
		pcb_log_last = start_prev;
}

void pcb_log_uninit(void)
{
	pcb_log_del_range(-1, -1);
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


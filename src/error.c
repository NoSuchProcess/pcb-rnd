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

#include "actions.h"
#include "data.h"
#include "error.h"
#include "event.h"
#include "hid_dad.h"
#include "safe_fs.h"
#include "funchash_core.h"
#include "hidlib_conf.h"
#include "genvector/gds_char.h"

void pcb_trace(const char *Format, ...)
{
#ifndef NDEBUG
	va_list args;
	va_start(args, Format);
	pcb_vfprintf(stderr, Format, args);
	va_end(args);
#endif
}

unsigned long pcb_log_next_ID = 0;
pcb_logline_t *pcb_log_first, *pcb_log_last;

void pcb_message(enum pcb_message_level level, const char *Format, ...)
{
	va_list args;
	pcb_message_level_t min_level = PCB_MSG_INFO;
	gds_t tmp;
	pcb_logline_t *line;

	if ((pcb_gui == NULL) || (pcbhl_conf.rc.dup_log_to_stderr)) {
		if (pcbhl_conf.rc.quiet)
			min_level = PCB_MSG_ERROR;

		if ((level >= min_level) || (pcbhl_conf.rc.verbose)) {
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
	va_start(args, Format);
	pcb_append_vprintf(&tmp, Format, args);
	va_end(args);

	/* add the header and link in */
	line = (pcb_logline_t *)tmp.array;
	line->stamp = time(NULL);
	line->ID = pcb_log_next_ID++;
	line->level = level;
	line->seen = 0;
	line->next = NULL;
	line->prev = pcb_log_last;
	if (pcb_log_first == NULL)
		pcb_log_first = line;
	if (pcb_log_last != NULL)
		pcb_log_last->next = line;
	pcb_log_last = line;
	line->len = tmp.used - offsetof(pcb_logline_t, str);

	pcb_event(NULL, PCB_EVENT_LOG_APPEND, "p", line);
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

pcb_logline_t *pcb_log_find_first_unseen(void)
{
	pcb_logline_t *n;
	for(n = pcb_log_last; n != NULL; n = n->prev)
		if (n->seen)
			return n->next;
	return pcb_log_first;
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



int pcb_log_export(pcb_hidlib_t *hidlib, const char *fn, int fmt_lihata)
{
	FILE *f;
	pcb_logline_t *n;

	f = pcb_fopen(hidlib, fn, "w");
	if (f == NULL)
		return -1;

	if (fmt_lihata) {
		fprintf(f, "ha:pcb-rnd-log-v1 {\n");
		fprintf(f, " li:entries {\n");
	}

	for(n = pcb_log_first; n != NULL; n = n->next) {
		if (fmt_lihata) {
			fprintf(f, "  ha:%lu {", n->ID);
			fprintf(f, "stamp=%ld; level=%d; seen=%d; ", (long int)n->stamp, n->level, n->seen);
			fprintf(f, "str={%s}}\n", n->str);
		}
		else
			fprintf(f, "%s", n->str);
	}

	if (fmt_lihata)
		fprintf(f, " }\n}\n");

	fclose(f);
	return 0;
}

void pcb_log_uninit(void)
{
	pcb_log_del_range(-1, -1);
}


static const char pcb_acts_Log[] =
	"Log(clear, [fromID, [toID])\n"
	"Log(export, [filename, [text|lihata])\n";
static const char pcb_acth_Log[] = "Manages the central, in-memory log.";
static fgw_error_t pcb_act_Log(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ret, op = -1;

	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, Log, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_Clear:
			{
				unsigned long from = -1, to = -1;
				PCB_ACT_MAY_CONVARG(2, FGW_ULONG, Log, from = fgw_keyword(&argv[2]));
				PCB_ACT_MAY_CONVARG(3, FGW_ULONG, Log, from = fgw_keyword(&argv[3]));
				pcb_log_del_range(from, to);
				pcb_event(NULL, PCB_EVENT_LOG_CLEAR, "pp", &from, &to);
				ret = 0;
			}
			break;
		case F_Export:
			{
				const char *fmts[] = { "text", "lihata", NULL };
				pcb_hid_dad_subdialog_t fmtsub;
				char *fn;
				int wfmt;

				memset(&fmtsub, 0, sizeof(fmtsub));
				PCB_DAD_ENUM(fmtsub.dlg, fmts);
					wfmt = PCB_DAD_CURRENT(fmtsub.dlg);
				fn = pcb_gui->fileselect(pcb_gui, "Export log", NULL, "log.txt", NULL, NULL, "log", PCB_HID_FSD_MAY_NOT_EXIST, &fmtsub);
				if (fn != NULL) {
					ret = pcb_log_export(NULL, fn, (fmtsub.dlg[wfmt].val.lng == 1));
					if (ret != 0)
						pcb_message(PCB_MSG_ERROR, "Failed to export log to '%s'\n", fn);
					free(fn);
				}
				else
					ret = 0;
			}
			break;
		default:
			PCB_ACT_FAIL(Log);
			ret = -1;
	}

	PCB_ACT_IRES(ret);
	return 0;
}

pcb_action_t log_action_list[] = {
	{"Log", pcb_act_Log, pcb_acth_Log, pcb_acts_Log}
};

PCB_REGISTER_ACTIONS(log_action_list, NULL)

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

#include <librnd/config.h>

#include <errno.h>
#include <stdarg.h>

#include <librnd/core/actions.h>
#include <librnd/core/error.h>
#include <librnd/core/event.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/hidlib_conf.h>
#include <genvector/gds_char.h>

void rnd_trace(const char *Format, ...)
{
#ifndef NDEBUG
	va_list args;
	va_start(args, Format);
	pcb_vfprintf(stderr, Format, args);
	va_end(args);
#endif
}

unsigned long rnd_log_next_ID = 0;
rnd_logline_t *rnd_log_first, *rnd_log_last;

void rnd_message(rnd_message_level_t level, const char *Format, ...)
{
	va_list args;
	rnd_message_level_t min_level = RND_MSG_INFO;
	gds_t tmp;
	rnd_logline_t *line;

	if ((pcb_gui == NULL) || (pcbhl_conf.rc.dup_log_to_stderr)) {
		if (pcbhl_conf.rc.quiet)
			min_level = RND_MSG_ERROR;

		if ((level >= min_level) || (pcbhl_conf.rc.verbose)) {
			va_start(args, Format);
			pcb_vfprintf(stderr, Format, args);
			va_end(args);
		}
	}

	/* allocate a new log line; for efficiency pretend it is a string during
	   the allocation */
	gds_init(&tmp);
	gds_enlarge(&tmp, sizeof(rnd_logline_t));
	tmp.used = offsetof(rnd_logline_t, str);
	va_start(args, Format);
	pcb_safe_append_vprintf(&tmp, 0, Format, args);
	va_end(args);

	/* add the header and link in */
	line = (rnd_logline_t *)tmp.array;
	line->stamp = time(NULL);
	line->ID = rnd_log_next_ID++;
	line->level = level;
	line->seen = 0;
	line->next = NULL;
	line->prev = rnd_log_last;
	if (rnd_log_first == NULL)
		rnd_log_first = line;
	if (rnd_log_last != NULL)
		rnd_log_last->next = line;
	rnd_log_last = line;
	line->len = tmp.used - offsetof(rnd_logline_t, str);

	rnd_event(NULL, RND_EVENT_LOG_APPEND, "p", line);
}

rnd_logline_t *rnd_log_find_min_(rnd_logline_t *from, unsigned long ID)
{
	rnd_logline_t *n;
	if (ID == -1)
		return from;
	for(n = from; n != NULL; n = n->next)
		if (n->ID >= ID)
			return n;
	return NULL;
}

rnd_logline_t *rnd_log_find_min(unsigned long ID)
{
	return rnd_log_find_min_(rnd_log_first, ID);
}

rnd_logline_t *rnd_log_find_first_unseen(void)
{
	rnd_logline_t *n;
	for(n = rnd_log_last; n != NULL; n = n->prev)
		if (n->seen)
			return n->next;
	return rnd_log_first;
}

void rnd_log_del_range(unsigned long from, unsigned long to)
{
	rnd_logline_t *start = rnd_log_find_min(from), *end = NULL;
	rnd_logline_t *next, *n, *start_prev, *end_next;

	if (start == NULL)
		return; /* start is beyond the end of the list - do not delete anything */

	if (to != -1)
		end = rnd_log_find_min_(start, to);
	if (end == NULL)
		end = rnd_log_last;

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
		rnd_log_first = end_next;

	if (end_next != NULL)
		end_next->prev = start_prev;
	else
		rnd_log_last = start_prev;
}



int rnd_log_export(rnd_hidlib_t *hidlib, const char *fn, int fmt_lihata)
{
	FILE *f;
	rnd_logline_t *n;

	f = pcb_fopen(hidlib, fn, "w");
	if (f == NULL)
		return -1;

	if (fmt_lihata) {
		fprintf(f, "ha:pcb-rnd-log-v1 {\n");
		fprintf(f, " li:entries {\n");
	}

	for(n = rnd_log_first; n != NULL; n = n->next) {
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

void rnd_log_uninit(void)
{
	rnd_log_del_range(-1, -1);
}


static const char pcb_acts_Log[] =
	"Log(clear, [fromID, [toID])\n"
	"Log(export, [filename, [text|lihata])\n";
static const char pcb_acth_Log[] = "Manages the central, in-memory log.";
static fgw_error_t pcb_act_Log(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ret;
	const char *op = "";

	rnd_PCB_ACT_MAY_CONVARG(1, FGW_STR, Log, op = argv[1].val.str);

	if (rnd_strcasecmp(op, "Clear") == 0) {
		unsigned long from = -1, to = -1;
		rnd_PCB_ACT_MAY_CONVARG(2, FGW_ULONG, Log, from = fgw_keyword(&argv[2]));
		rnd_PCB_ACT_MAY_CONVARG(3, FGW_ULONG, Log, from = fgw_keyword(&argv[3]));
		rnd_log_del_range(from, to);
		rnd_event(NULL, RND_EVENT_LOG_CLEAR, "pp", &from, &to);
		ret = 0;
	}
	else if (rnd_strcasecmp(op, "Export") == 0) {
		const char *fmts[] = { "text", "lihata", NULL };
		rnd_hid_dad_subdialog_t fmtsub;
		char *fn;
		int wfmt;

		memset(&fmtsub, 0, sizeof(fmtsub));
		PCB_DAD_ENUM(fmtsub.dlg, fmts);
			wfmt = PCB_DAD_CURRENT(fmtsub.dlg);
		fn = pcb_gui->fileselect(pcb_gui, "Export log", NULL, "log.txt", NULL, NULL, "log", RND_HID_FSD_MAY_NOT_EXIST, &fmtsub);
		if (fn != NULL) {
			ret = rnd_log_export(NULL, fn, (fmtsub.dlg[wfmt].val.lng == 1));
			if (ret != 0)
				rnd_message(RND_MSG_ERROR, "Failed to export log to '%s'\n", fn);
			free(fn);
		}
		else
			ret = 0;
	}
	else {
		RND_ACT_FAIL(Log);
		ret = -1;
	}

	RND_ACT_IRES(ret);
	return 0;
}

static const char pcb_acts_Message[] = "message([ERROR|WARNING|INFO|DEBUG,] message)";
static const char pcb_acth_Message[] = "Writes a message to the log window.";
/* DOC: message.html */
static fgw_error_t pcb_act_Message(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int i, how = RND_MSG_INFO;

	if (argc < 2)
		RND_ACT_FAIL(Message);

	i = 1;
	if (argc > 2) {
		const char *hows;
		rnd_PCB_ACT_MAY_CONVARG(i, FGW_STR, Message, hows = argv[i].val.str);
		if (strcmp(hows, "ERROR") == 0)        { i++; how = RND_MSG_ERROR; }
		else if (strcmp(hows, "WARNING") == 0) { i++; how = RND_MSG_WARNING; }
		else if (strcmp(hows, "INFO") == 0)    { i++; how = RND_MSG_INFO; }
		else if (strcmp(hows, "DEBUG") == 0)   { i++; how = RND_MSG_DEBUG; }
	}

	RND_ACT_IRES(0);
	for(; i < argc; i++) {
		char *s = NULL;
		rnd_PCB_ACT_MAY_CONVARG(i, FGW_STR, Message, s = argv[i].val.str);
		if ((s != NULL) && (*s != '\0'))
			rnd_message(how, s);
		rnd_message(how, "\n");
	}

	return 0;
}

static rnd_action_t log_action_list[] = {
	{"Log", pcb_act_Log, pcb_acth_Log, pcb_acts_Log},
	{"Message", pcb_act_Message, pcb_acth_Message, pcb_acts_Message}
};

void pcb_hidlib_error_init2(void)
{
	RND_REGISTER_ACTIONS(log_action_list, NULL);
}

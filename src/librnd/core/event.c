/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2018, 2020 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <librnd/config.h>
#include <librnd/core/event.h>
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include <librnd/core/fptr_cast.h>

static const char *rnd_evnames_lib[] = {
	"pcbev_gui_init",
	"pcbev_cli_enter",
	"pcbeb_new_tool",
	"pcbeb_tool_select_pre",
	"pcbeb_tool_release",
	"pcbeb_tool_press",
	"pcbev_busy",
	"pcbev_log_append",
	"pcbev_log_clear",
	"pcbev_gui_lead_user",
	"pcbev_gui_draw_overlay_xor",
	"pcbev_user_input_post",
	"pcbev_user_input_key",
	"pcbev_crosshair_move",
	"pcbev_dad_new_dialog",
	"pcbev_dad_new_geo",
	"pcbev_export_session_begin",
	"pcbev_export_session_end",
	"pcbev_stroke_start",
	"pcbev_stroke_record",
	"pcbev_stroke_finish",
	"pcbev_board_changed",
	"pcbev_board_meta_changed",
	"pcbev_board_fn_changed",
	"pcbev_save_pre",
	"pcbev_save_post",
	"pcbev_load_pre",
	"pcbev_load_post"
};

static const char **rnd_evnames_app = NULL;
static long rnd_event_app_last = 0;

typedef struct event_s event_t;

struct event_s {
	pcb_event_handler_t *handler;
	void *user_data;
	const char *cookie;
	event_t *next;
};

static event_t *rnd_events_lib[RND_EVENT_last];
static event_t **rnd_events_app = NULL;

#define EVENT_SETUP(ev, err) \
	pcb_event_id_t evid = ev; \
	const char **evnames = rnd_evnames_lib; \
	event_t **events = rnd_events_lib; \
	(void)evnames; \
	(void)evid; \
	(void)events; \
	if ((ev >= RND_EVENT_app) && (rnd_event_app_last > 0)) { \
		if ((ev) > rnd_event_app_last) { err; } \
		evnames = rnd_evnames_app; \
		events = rnd_events_app; \
		ev -= RND_EVENT_app; \
	} \
	else if (!(((ev) >= 0) && ((ev) < RND_EVENT_last))) { err; }

void pcb_event_app_reg(long last_event_id, const char **event_names, long sizeof_event_names)
{
	rnd_event_app_last = last_event_id;
	rnd_evnames_app = event_names;
	assert((sizeof_event_names / sizeof(char *)) == last_event_id - RND_EVENT_app);
	rnd_events_app = calloc(sizeof(event_t), last_event_id - RND_EVENT_app + 1);
}

void pcb_event_bind(pcb_event_id_t ev, pcb_event_handler_t *handler, void *user_data, const char *cookie)
{
	event_t *e;
	EVENT_SETUP(ev, return);

	e = malloc(sizeof(event_t));
	e->handler = handler;
	e->cookie = cookie;
	e->user_data = user_data;

	/* insert the new event in front of the list */
	e->next = events[ev];
	events[ev] = e;
}

static void event_destroy(event_t *ev)
{
	free(ev);
}

void pcb_event_unbind(pcb_event_id_t ev, pcb_event_handler_t *handler)
{
	event_t *prev = NULL, *e, *next;
	EVENT_SETUP(ev, return);

	for (e = events[ev]; e != NULL; e = next) {
		next = e->next;
		if (e->handler == handler) {
			event_destroy(e);
			if (prev == NULL)
				events[ev] = next;
			else
				prev->next = next;
		}
		else
			prev = e;
	}
}

void pcb_event_unbind_cookie(pcb_event_id_t ev, const char *cookie)
{
	event_t *prev = NULL, *e, *next;
	EVENT_SETUP(ev, return);

	for (e = events[ev]; e != NULL; e = next) {
		next = e->next;
		if (e->cookie == cookie) {
			event_destroy(e);
			if (prev == NULL)
				events[ev] = next;
			else
				prev->next = next;
		}
		else
		 prev = e;
	}
}


void pcb_event_unbind_allcookie(const char *cookie)
{
	pcb_event_id_t n;
	for (n = 0; n < RND_EVENT_last; n++)
		pcb_event_unbind_cookie(n, cookie);
	for (n = RND_EVENT_app; n < rnd_event_app_last; n++)
		pcb_event_unbind_cookie(n, cookie);
}

const char *pcb_event_name(pcb_event_id_t ev)
{
	EVENT_SETUP(ev, return "<invalid event>");
	return rnd_evnames_lib[ev];
}

void pcb_event(pcb_hidlib_t *hidlib, pcb_event_id_t ev, const char *fmt, ...)
{
	va_list ap;
	pcb_event_arg_t argv[EVENT_MAX_ARG], *a;
	fgw_arg_t fargv[EVENT_MAX_ARG+1], *fa;
	event_t *e;
	int argc;
	EVENT_SETUP(ev, return);

	a = argv;
	a->type = PCB_EVARG_INT;
	a->d.i = evid;

	fa = fargv;
	fa->type = FGW_INVALID; /* first argument will be the function, as filed in by fungw; we are not passing the event number as it is impossible to bind multiple events to the same function this way */

	argc = 1;

	if (fmt != NULL) {
		va_start(ap, fmt);
		for (a++, fa++; *fmt != '\0'; fmt++, a++, fa++, argc++) {
			if (argc >= EVENT_MAX_ARG) {
				pcb_message(PCB_MSG_ERROR, "pcb_event(): too many arguments\n");
				break;
			}
			switch (*fmt) {
			case 'i':
				a->type = PCB_EVARG_INT;
				fa->type = FGW_INT;
				fa->val.nat_int = a->d.i = va_arg(ap, int);
				break;
			case 'd':
				a->type = PCB_EVARG_DOUBLE;
				fa->type = FGW_DOUBLE;
				fa->val.nat_double = a->d.d = va_arg(ap, double);
				break;
			case 's':
				a->type = PCB_EVARG_STR;
				fa->type = FGW_STR;
				a->d.s = va_arg(ap, const char *);
				fa->val.str = (char *)a->d.s;
				break;
			case 'p':
				a->type = PCB_EVARG_PTR;
				fa->type = FGW_PTR;
				fa->val.ptr_void = a->d.p = va_arg(ap, void *);
				break;
			case 'c':
				a->type = PCB_EVARG_COORD;
				a->d.c = va_arg(ap, pcb_coord_t);
				fa->type = FGW_LONG;
				fa->val.nat_long = a->d.c;
				fgw_arg_conv(&pcb_fgw, fa, FGW_COORD_);
				break;
			case 'a':
				a->type = PCB_EVARG_ANGLE;
				fa->type = FGW_DOUBLE;
				fa->val.nat_double = a->d.a = va_arg(ap, pcb_angle_t);
				break;
			default:
				a->type = PCB_EVARG_INT;
				a->d.i = 0;
				fa->type = FGW_INVALID;
				pcb_message(PCB_MSG_ERROR, "pcb_event(): invalid argument type '%c'\n", *fmt);
				break;
			}
		}
		va_end(ap);
	}

	for (e = events[ev]; e != NULL; e = e->next)
		e->handler(hidlib, e->user_data, argc, argv);

	fgw_ucall_all(&pcb_fgw, hidlib, pcb_event_name(ev), argc, fargv);
}

void pcb_events_init(void)
{
	if ((sizeof(rnd_evnames_lib) / sizeof(rnd_evnames_lib[0])) != RND_EVENT_last) {
		fprintf(stderr, "INTERNAL ERROR: event.c: rnd_evnames_lib and pcb_event_id_t are out of sync\n");
		exit(1);
	}
}

void pcb_events_uninit_(event_t **events, long last)
{
	int ev;
	for(ev = 0; ev < last; ev++) {
		event_t *e, *next;
		for(e = events[ev]; e != NULL; e = next) {
			next = e->next;
			fprintf(stderr, "WARNING: events_uninit: event %d still has %p registered for cookie %p (%s)\n", ev, pcb_cast_f2d(e->handler), (void *)e->cookie, e->cookie);
			free(e);
		}
	}
}


void pcb_events_uninit(void)
{
	pcb_events_uninit_(rnd_events_lib, RND_EVENT_last);
	if (rnd_event_app_last > 0)
		pcb_events_uninit_(rnd_events_app, rnd_event_app_last - RND_EVENT_app);
	free(rnd_events_app);
}


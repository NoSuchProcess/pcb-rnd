/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "config.h"
#include "event.h"
#include "error.h"
#include "fptr_cast.h"

typedef struct event_s event_t;

struct event_s {
	pcb_event_handler_t *handler;
	void *user_data;
	const char *cookie;
	event_t *next;
};

event_t *events[PCB_EVENT_last];

#define event_valid(ev) (((ev) >= 0) && ((ev) < PCB_EVENT_last))

void pcb_event_bind(pcb_event_id_t ev, pcb_event_handler_t * handler, void *user_data, const char *cookie)
{
	event_t *e;

	if (!(event_valid(ev)))
		return;

	e = malloc(sizeof(event_t));
	e->handler = handler;
	e->cookie = cookie;
	e->user_data = user_data;

	/* insert the new event in front of the list */
	e->next = events[ev];
	events[ev] = e;
}

static void event_destroy(event_t * ev)
{
	free(ev);
}

void pcb_event_unbind(pcb_event_id_t ev, pcb_event_handler_t * handler)
{
	event_t *prev = NULL, *e, *next;
	if (!(event_valid(ev)))
		return;
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
	if (!(event_valid(ev)))
		return;
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
	for (n = 0; n < PCB_EVENT_last; n++)
		pcb_event_unbind_cookie(n, cookie);
}

void pcb_event(pcb_event_id_t ev, const char *fmt, ...)
{
	va_list ap;
	pcb_event_arg_t argv[EVENT_MAX_ARG], *a;
	event_t *e;
	int argc;

	a = argv;
	a->type = PCB_EVARG_INT;
	a->d.i = ev;
	argc = 1;

	if (fmt != NULL) {
		va_start(ap, fmt);
		for (a++; *fmt != '\0'; fmt++, a++, argc++) {
			if (argc >= EVENT_MAX_ARG) {
				pcb_message(PCB_MSG_DEFAULT, "pcb_event(): too many arguments\n");
				break;
			}
			switch (*fmt) {
			case 'i':
				a->type = PCB_EVARG_INT;
				a->d.i = va_arg(ap, int);
				break;
			case 'd':
				a->type = PCB_EVARG_DOUBLE;
				a->d.d = va_arg(ap, double);
				break;
			case 's':
				a->type = PCB_EVARG_STR;
				a->d.s = va_arg(ap, const char *);
				break;
			case 'p':
				a->type = PCB_EVARG_PTR;
				a->d.p = va_arg(ap, void *);
				break;
			case 'c':
				a->type = PCB_EVARG_COORD;
				a->d.c = va_arg(ap, pcb_coord_t);
				break;
			case 'a':
				a->type = PCB_EVARG_ANGLE;
				a->d.a = va_arg(ap, pcb_angle_t);
				break;
			default:
				a->type = PCB_EVARG_INT;
				a->d.i = 0;
				pcb_message(PCB_MSG_DEFAULT, "pcb_event(): invalid argument type '%c'\n", *fmt);
				break;
			}
		}
		va_end(ap);
	}

	for (e = events[ev]; e != NULL; e = e->next)
		e->handler(e->user_data, argc, argv);
}

void pcb_events_init(void)
{

}

void pcb_events_uninit(void)
{
	int ev;
	for(ev = 0; ev < PCB_EVENT_last; ev++) {
		event_t *e, *next;
		for(e = events[ev]; e != NULL; e = next) {
			next = e->next;
			fprintf(stderr, "WARNING: events_uninit: event %d still has %p registered for cookie %p\n", ev, pcb_cast_f2d(e->handler), (void *)e->cookie);
			free(e);
		}
	}
}


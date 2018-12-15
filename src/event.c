/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2018 Tibor 'Igor2' Palinkas
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
#include "config.h"
#include "event.h"
#include "error.h"
#include "actions.h"
#include "fptr_cast.h"

static const char *pcb_fgw_evnames[] = {
	"pcbev_gui_init",
	"pcbev_cli_enter",
	"pcbev_save_pre",
	"pcbev_save_post",
	"pcbev_load_pre",
	"pcbev_load_post",
	"pcbev_board_changed",
	"pcbev_board_meta_changed",
	"pcbev_route_styles_changed",
	"pcbev_netlist_changed",
	"pcbev_layers_changed",
	"pcbev_layer_changed_grp",
	"pcbev_layervis_changed",
	"pcbev_library_changed",
	"pcbev_font_changed",
	"pcbev_undo_post",
	"pcbev_new_pstk",
	"pcbev_busy",
	"pcbev_rubber_reset",
	"pcbev_rubber_remove_subc",
	"pcbev_rubber_move",
	"pcbev_rubber_move_draw",
	"pcbev_rubber_rotate90",
	"pcbev_rubber_rotate",
	"pcbev_rubber_rename",
	"pcbev_rubber_lookup_lines",
	"pcbev_rubber_lookup_rats",
	"pcbev_rubber_constrain_main_line",
	"pcbev_gui_sync",
	"pcbev_gui_sync_status",
	"pcbev_user_input_post",
	"pcbev_draw_crosshair_chatt",
	"pcbev_drc_run",
	"pcbev_dad_new_dialog"
};

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
	fgw_arg_t fargv[EVENT_MAX_ARG+1], *fa;
	event_t *e;
	int argc;

	if (ev >= PCB_EVENT_last)
		return;

	a = argv;
	a->type = PCB_EVARG_INT;
	a->d.i = ev;

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
		e->handler(e->user_data, argc, argv);

	fgw_call_all(&pcb_fgw, pcb_fgw_evnames[ev], argc, fargv);
}

void pcb_events_init(void)
{
	if ((sizeof(pcb_fgw_evnames) / sizeof(pcb_fgw_evnames[0])) != PCB_EVENT_last) {
		fprintf(stderr, "INTERNAL ERROR: event.c: pcb_fgw_evnames and pcb_event_id_t are out of sync\n");
		exit(1);
	}
}

void pcb_events_uninit(void)
{
	int ev;
	for(ev = 0; ev < PCB_EVENT_last; ev++) {
		event_t *e, *next;
		for(e = events[ev]; e != NULL; e = next) {
			next = e->next;
			fprintf(stderr, "WARNING: events_uninit: event %d still has %p registered for cookie %p (%s)\n", ev, pcb_cast_f2d(e->handler), (void *)e->cookie, e->cookie);
			free(e);
		}
	}
}


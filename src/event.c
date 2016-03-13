#include <stdlib.h>
#include <stdarg.h>
#include "event.h"
#include "error.h"

typedef struct event_s event_t;

struct event_s {
	event_handler_t *handler;
	void *user_data;
	void *cookie;
	event_t *next;
};

event_t *events[EVENT_last];

#define event_valid(ev) (((ev) >= 0) && ((ev) < EVENT_last))

void event_bind(event_id_t ev, event_handler_t * handler, void *user_data, void *cookie)
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

void event_unbind(event_id_t ev, event_handler_t * handler)
{
	event_t *prev = NULL, *e, *next;
	if (!(event_valid(ev)))
		return;
	for (e = events[ev]; e != NULL; prev = e, e = next) {
		next = e->next;
		if (e->handler == handler) {
			event_destroy(e);
			if (prev == NULL)
				events[ev] = next;
			else
				prev->next = next;
		}
	}
}

void event_unbind_cookie(event_id_t ev, void *cookie)
{
	event_t *prev = NULL, *e, *next;
	if (!(event_valid(ev)))
		return;
	for (e = events[ev]; e != NULL; prev = e, e = next) {
		next = e->next;
		if (e->cookie == cookie) {
			event_destroy(e);
			if (prev == NULL)
				events[ev] = next;
			else
				prev->next = next;
		}
	}
}


void event_unbind_allcookie(void *cookie)
{
	event_id_t n;
	for (n = 0; n < EVENT_last; n++)
		event_unbind_cookie(n, cookie);
}

void event(event_id_t ev, const char *fmt, ...)
{
	va_list ap;
	const char *f;
	event_arg_t argv[EVENT_MAX_ARG], *a;
	event_t *e;
	int argc;

	a = argv;
	a->type = ARG_INT;
	a->d.i = ev;
	argc = 1;

	if (fmt != NULL) {
		va_start(ap, fmt);
		for (a++; *fmt != '\0'; fmt++, a++, argc++) {
			if (argc >= EVENT_MAX_ARG) {
				Message("event(): too many arguments\n");
				break;
			}
			switch (*fmt) {
			case 'i':
				a->type = ARG_INT;
				a->d.i = va_arg(ap, int);
				break;
			case 'd':
				a->type = ARG_DOUBLE;
				a->d.d = va_arg(ap, double);
				break;
			case 's':
				a->type = ARG_STR;
				a->d.s = va_arg(ap, const char *);
				break;
			default:
				a->type = ARG_INT;
				a->d.i = 0;
				Message("event(): invalid argument type '%c'\n", *fmt);
				break;
			}
		}
		va_end(ap);
	}

	for (e = events[ev]; e != NULL; e = e->next)
		e->handler(e->user_data, argc, (event_arg_t **) & argv);
}

void events_init(void)
{

}

void events_uninit(void)
{
	int ev;
	for(ev = 0; ev < EVENT_last; ev++) {
		event_t *e, *next;
		for(e = events[ev]; e != NULL; e = next) {
			next = e->next;
			free(e);
		}
	}
}


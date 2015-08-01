#include <stdlib.h>
#include "event.h"

typedef struct event_s event_t;

struct event_s {
	event_handler_t *handler;
	void *cookie;
	event_t *next;
};

event_t *events[EVENT_last];

#define event_valid(ev) (((ev) >= 0) && ((ev) < EVENT_last))

void event_bind(event_id_t ev, event_handler_t *handler, void *cookie)
{
	event_t *e;

	if (!(event_valid(ev)))
		return;

	e = malloc(sizeof(event_t));
	e->handler = handler;
	e->cookie = cookie;

	/* insert the new event in front of the list */
	e->next = events[ev];
	events[ev] = e;
}

static void event_destroy(event_t *ev)
{
	free(ev);
}

void event_unbind(event_id_t ev, event_handler_t *handler)
{
	event_t *prev = NULL, *e, *next;
	if (!(event_valid(ev)))
		return;
	for(e = events[ev]; e != NULL; prev=e, e = next) {
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
	for(e = events[ev]; e != NULL; prev=e, e = next) {
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
	for(n = 0; n < EVENT_last; n++)
		event_unbind_cookie(n, cookie);
}

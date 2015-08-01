typedef enum {
	EVENT_GUI_INIT,  /* finished initializing the GUI; args: (void) */
	EVENT_CLI_ENTER, /* the user pressed enter on a CLI command; args: (str commandline) */

	EVENT_last       /* not a real event */
} event_id_t;

/* Argument types in event's argv[] */
typedef enum {
	ARG_INT,
	ARG_DOUBLE,
	ARG_STR
} event_argtype_t;


/* An argument is its type and value */
typedef struct {
	event_argtype_t type;
	union {
		int i;
		double d;
		const char *str;
	} d;
} event_arg_t;

/* Event callback prototype */
typedef void (event_handler_t)(int argc, event_arg_t *argv[]);

/* Bind: add a handler to the call-list of an event; the cookie is also remembered
   so that mass-unbind is easier later. */
void event_bind(event_id_t ev, event_handler_t *handler, void *cookie);

/* Unbind: remove a handler from an event */
void event_unbind(event_id_t ev, event_handler_t *handler);

/* Unbind by cookie: remove all handlers from an event matching the cookie */
void event_unbind_cookie(event_id_t ev, void *cookie);

/* Unbind all by cookie: remove all handlers from all events matching the cookie */
void event_unbind_allcookie(void *cookie);


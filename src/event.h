typedef enum {
	EVENT_GUI_INIT,  /* finished initializing the GUI called right before the main loop of the GUI; args: (void) */
	EVENT_CLI_ENTER, /* the user pressed enter on a CLI command; args: (str commandline) */

	EVENT_last       /* not a real event */
} event_id_t;

/* Maximum number of arguments for an event handler, auto-set argv[0] included */
#define EVENT_MAX_ARG 16

/* Argument types in event's argv[] */
typedef enum {
	ARG_INT,          /* format char: i */
	ARG_DOUBLE,       /* format char: d */
	ARG_STR           /* format char: s */
} event_argtype_t;


/* An argument is its type and value */
typedef struct {
	event_argtype_t type;
	union {
		int i;
		double d;
		const char *s;
	} d;
} event_arg_t;

/* Event callback prototype; user_data is the same as in event_bind().
   argv[0] is always an ARG_INT with the event id that triggered the event. */
typedef void (event_handler_t)(void *user_data, int argc, event_arg_t *argv[]);

/* Bind: add a handler to the call-list of an event; the cookie is also remembered
   so that mass-unbind is easier later. user_data is passed to the handler. */
void event_bind(event_id_t ev, event_handler_t *handler, void *user_data, void *cookie);

/* Unbind: remove a handler from an event */
void event_unbind(event_id_t ev, event_handler_t *handler);

/* Unbind by cookie: remove all handlers from an event matching the cookie */
void event_unbind_cookie(event_id_t ev, void *cookie);

/* Unbind all by cookie: remove all handlers from all events matching the cookie */
void event_unbind_allcookie(void *cookie);

/* Event trigger: call all handlers for an event. Fmt is a list of
   format characters (e.g. i for ARG_INT). */
void event(event_id_t ev, const char *fmt, ...);

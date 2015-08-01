typedef enum {
	EVENT_GUI_INIT,  /* finished initializing the GUI; args: (void) */
	EVENT_CLI_ENTER, /* the user pressed enter on a CLI command; args: (str commandline) */

	EVENT_last       /* not a real event */
} event_id_t;

typedef enum {
	ARG_INT,
	ARG_DOUBLE,
	ARG_STR
} event_argtype_t;


typedef struct {
	event_argtype_t type;
	union {
		int i;
		double d;
		const char *str;
	} d;
} event_arg_t;

typedef void (event_handler_t)(int argc, event_arg_t *argv[]);

void event_bind(event_id_t ev, event_handler_t *handler, void *cookie);
void event_unbind(event_id_t ev, event_handler_t *handler);
void event_unbind_cookie(event_id_t ev, void *cookie);
void event_unbind_allcookie(void *cookie);


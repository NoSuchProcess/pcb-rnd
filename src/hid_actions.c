#include "config.h"
#include "conf_core.h"

#include <ctype.h>

#include <genht/hash.h>
#include <genht/htsp.h>

#include "error.h"
#include "event.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "compat_nls.h"

/* do not throw "unknown action" warning for these: they are known
   actions, the GUI HID may register them, but nothing bad happens if
   they are not registered or not handled by the GUI. */
static const char *action_no_warn[] = {
	"LayersChanged", "PointCursor", "LibraryChanged", "RouteStylesChanged",
	NULL
};
static int action_legal_unknown(const char *name)
{
	const char **s;
	for(s = action_no_warn; *s != NULL; s++)
		if (strcmp(*s, name) == 0)
			return 1;
	return 0;
}

static htsp_t *all_actions = NULL;
const pcb_hid_action_t *current_action = NULL;

static const char *check_action_name(const char *s)
{
	while (*s)
		if (isspace((int) *s++) || *s == '(')
			return (s - 1);
	return NULL;
}

typedef struct {
	const char *cookie;
	const pcb_hid_action_t *action;
} hid_cookie_action_t;

void pcb_hid_register_actions(const pcb_hid_action_t * a, int n, const char *cookie, int copy)
{
	int i;
	hid_cookie_action_t *ca;

	if (all_actions == NULL)
		all_actions = htsp_alloc(strhash_case, strkeyeq_case);

	for (i = 0; i < n; i++) {
		if (check_action_name(a[i].name)) {
			pcb_message(PCB_MSG_DEFAULT, _("ERROR! Invalid action name, " "action \"%s\" not registered.\n"), a[i].name);
			continue;
		}
		if (htsp_get(all_actions, a[i].name) != NULL) {
			pcb_message(PCB_MSG_DEFAULT, _("ERROR! Invalid action name, " "action \"%s\" is already registered.\n"), a[i].name);
			continue;
		}
		ca = malloc(sizeof(hid_cookie_action_t));
		ca->cookie = cookie;
		ca->action = a+i;
		htsp_set(all_actions, pcb_strdup(a[i].name), ca);
	}
}

void pcb_hid_register_action(const pcb_hid_action_t * a, const char *cookie, int copy)
{
	pcb_hid_register_actions(a, 1, cookie, copy);
}

void pcb_hid_remove_actions(const pcb_hid_action_t * a, int n)
{
	int i;

	if (all_actions == NULL)
		return;

	for (i = 0; i < n; i++) {
		htsp_entry_t *e;
		e = htsp_popentry(all_actions, a[i].name);
		free(e->key);
		free(e->value);
	}
}

void pcb_hid_remove_actions_by_cookie(const char *cookie)
{
	htsp_entry_t *e;

	if (all_actions == NULL)
		return;

	/* Slow linear search - probably OK, this will run only on uninit */
	for (e = htsp_first(all_actions); e; e = htsp_next(all_actions, e)) {
		hid_cookie_action_t *ca = e->value;
		if (ca->cookie == cookie) {
			htsp_pop(all_actions, e->key);
			free(e->key);
			free(e->value);
		}
	}
}

void pcb_hid_remove_action(const pcb_hid_action_t * a)
{
	htsp_entry_t *e;

	if (all_actions == NULL)
		return;

	e = htsp_popentry(all_actions, a->name);
	if (e != NULL) {
		free(e->key);
		free(e->value);
	}
}

const pcb_hid_action_t *pcb_hid_find_action(const char *name)
{
	hid_cookie_action_t *ca;

	if ((name == NULL) && (all_actions == NULL))
		return 0;

	ca = htsp_get(all_actions, (char *) name);

	if (ca)
		return ca->action;

	if (!action_legal_unknown(name))
		pcb_message(PCB_MSG_DEFAULT, "unknown action `%s'\n", name);
	return 0;
}

void pcb_print_actions()
{
	htsp_entry_t *e;

	fprintf(stderr, "Registered Actions:\n");
	for (e = htsp_first(all_actions); e; e = htsp_next(all_actions, e)) {
		hid_cookie_action_t *ca = e->value;
		if (ca->action->description)
			fprintf(stderr, "  %s - %s\n", ca->action->name, ca->action->description);
		else
			fprintf(stderr, "  %s\n", ca->action->name);
		if (ca->action->syntax) {
			const char *bb, *eb;
			bb = eb = ca->action->syntax;
			while (1) {
				for (eb = bb; *eb && *eb != '\n'; eb++);
				fwrite("    ", 4, 1, stderr);
				fwrite(bb, eb - bb, 1, stderr);
				fputc('\n', stderr);
				if (*eb == 0)
					break;
				bb = eb + 1;
			}
		}
	}
}

static void dump_string(char prefix, const char *str)
{
	int eol = 1;
	while (*str) {
		if (eol) {
			putchar(prefix);
			eol = 0;
		}
		putchar(*str);
		if (*str == '\n')
			eol = 1;
		str++;
	}
	if (!eol)
		putchar('\n');
}

void pcb_dump_actions(void)
{
	htsp_entry_t *e;

	fprintf(stderr, "Registered Actions:\n");
	for (e = htsp_first(all_actions); e; e = htsp_next(all_actions, e)) {
		hid_cookie_action_t *ca = e->value;
		const char *desc = ca->action->description;
		const char *synt = ca->action->syntax;

		desc = desc ? desc : "";
		synt = synt ? synt : "";

		printf("A%s\n", ca->action->name);
		dump_string('D', desc);
		dump_string('S', synt);
	}
}

int pcb_hid_action(const char *name)
{
	return pcb_hid_actionv(name, 0, 0);
}

int pcb_hid_actionl(const char *name, ...)
{
	const char *argv[20];
	int argc = 0;
	va_list ap;
	char *arg;

	va_start(ap, name);
	while ((arg = va_arg(ap, char *)) != 0)
		  argv[argc++] = arg;
	va_end(ap);
	return pcb_hid_actionv(name, argc, argv);
}

int pcb_hid_actionv_(const pcb_hid_action_t *a, int argc, const char **argv)
{
	pcb_coord_t x = 0, y = 0;
	int i, ret;
	const pcb_hid_action_t *old_action;

	if (a->need_coord_msg)
		gui->get_coords(_(a->need_coord_msg), &x, &y);

	if (conf_core.rc.verbose) {
		printf("Action: \033[34m%s(", a->name);
		for (i = 0; i < argc; i++)
			printf("%s%s", i ? "," : "", argv[i]);
		printf(")\033[0m\n");
	}

	old_action = current_action;
	current_action = a;
	ret = current_action->trigger_cb(argc, argv, x, y);
	current_action = old_action;

	return ret;
}

int pcb_hid_actionv(const char *name, int argc, const char **argv)
{
	const pcb_hid_action_t *a;

	if (!name)
		return 1;

	a = pcb_hid_find_action(name);
	if (!a) {
		int i;
		if (action_legal_unknown(name))
			return 1;
		pcb_message(PCB_MSG_DEFAULT, "no action %s(", name);
		for (i = 0; i < argc; i++)
			pcb_message(PCB_MSG_DEFAULT, "%s%s", i ? ", " : "", argv[i]);
		pcb_message(PCB_MSG_DEFAULT, ")\n");
		return 1;
	}
	return pcb_hid_actionv_(a, argc, argv);
}

static int hid_parse_actionstring(const char *rstr, char require_parens)
{
	const char **list = NULL;
	int max = 0;
	int num;
	char *str = NULL;
	const char *sp;
	char *cp, *aname, *cp2;
	int maybe_empty = 0;
	char in_quotes = 0;
	char parens = 0;
	int retcode = 0;

	/*fprintf(stderr, "invoke: `%s'\n", rstr); */

	sp = rstr;
	str = (char *) malloc(strlen(rstr) + 1);

another:
	num = 0;
	cp = str;

	/* eat leading spaces and tabs */
	while (*sp && isspace((int) *sp))
		sp++;

	if (!*sp) {
		retcode = 0;
		goto cleanup;
	}

	aname = cp;

	/* copy the action name, assumes name does not have a space or '('
	 * in its name */
	while (*sp && !isspace((int) *sp) && *sp != '(')
		*cp++ = *sp++;
	*cp++ = 0;

	/* skip whitespace */
	while (*sp && isspace((int) *sp))
		sp++;

	/*
	 * we only have an action name, so invoke the action
	 * with no parameters or event.
	 */
	if (!*sp) {
		retcode = pcb_hid_actionv(aname, 0, 0);
		goto cleanup;
	}

	/* are we using parenthesis? */
	if (*sp == '(') {
		parens = 1;
		sp++;
	}
	else if (require_parens) {
		pcb_message(PCB_MSG_DEFAULT, _("Syntax error: %s\n"), rstr);
		pcb_message(PCB_MSG_DEFAULT, _("    expected: Action(arg1, arg2)"));
		retcode = 1;
		goto cleanup;
	}

	/* get the parameters to pass to the action */
	while (1) {
		/*
		 * maybe_empty == 0 means that the last char examined was not a
		 * ","
		 */
		if (!maybe_empty && ((parens && *sp == ')') || (!parens && !*sp))) {
			retcode = pcb_hid_actionv(aname, num, list);
			if (retcode)
				goto cleanup;

			/* strip any white space or ';' following the action */
			if (parens)
				sp++;
			while (*sp && (isspace((int) *sp) || *sp == ';'))
				sp++;
			goto another;
		}
		else if (*sp == 0 && !maybe_empty)
			break;
		else {
			maybe_empty = 0;
			in_quotes = 0;
			/*
			 * if we have more parameters than memory in our array of
			 * pointers, then either allocate some or grow the array
			 */
			if (num >= max) {
				max += 10;
				if (list)
					list = (const char **) realloc(list, max * sizeof(char *));
				else
					list = (const char **) malloc(max * sizeof(char *));
			}
			/* Strip leading whitespace.  */
			while (*sp && isspace((int) *sp))
				sp++;
			list[num++] = cp;

			/* search for the end of the argument, we want to keep going
			 * if we are in quotes or the char is not a delimiter
			 */
			while (*sp && (in_quotes || ((*sp != ',')
																	 && (!parens || *sp != ')')
																	 && (parens || !isspace((int) *sp))))) {
				/*
				 * single quotes give literal value inside, including '\'.
				 * you can't have a single inside single quotes.
				 * doubles quotes gives literal value inside, but allows escape.
				 */
				if ((*sp == '"' || *sp == '\'') && (!in_quotes || *sp == in_quotes)) {
					in_quotes = in_quotes ? 0 : *sp;
					sp++;
					continue;
				}
				/* unless within single quotes, \<char> will just be <char> */
				else if (*sp == '\\' && in_quotes != '\'')
					sp++;
				*cp++ = *sp++;
			}
			cp2 = cp - 1;
			*cp++ = 0;
			if (*sp == ',' || (!parens && isspace((int) *sp))) {
				maybe_empty = 1;
				sp++;
			}
			/* Strip trailing whitespace.  */
			for (; isspace((int) *cp2) && cp2 >= list[num - 1]; cp2--)
				*cp2 = 0;
		}
	}

cleanup:

	if (list != NULL)
		free(list);

	if (str != NULL)
		free(str);

	return retcode;
}

int pcb_hid_parse_command(const char *str_)
{
	pcb_event(PCB_EVENT_CLI_ENTER, "s", str_);
	return hid_parse_actionstring(str_, pcb_false);
}

int pcb_hid_parse_actions(const char *str_)
{
	return hid_parse_actionstring(str_, pcb_true);
}

void pcb_hid_actions_init(void)
{

}

void pcb_hid_actions_uninit(void)
{
	htsp_entry_t *e;

	if (all_actions == NULL)
		return;

	for (e = htsp_first(all_actions); e; e = htsp_next(all_actions, e)) {
		hid_cookie_action_t *ca = e->value;
		if (ca->cookie != NULL)
			fprintf(stderr, "WARNING: hid_actions_uninit: action '%s' with cookie '%s' left registered, check your plugins!\n", e->key, ca->cookie);
		free(e->key);
		free(e->value);
	}

	htsp_free(all_actions);
	all_actions = NULL;
}

/* trick for the doc extractor */
#define static

/* %start-doc actions 00macros

@macro hidaction

This is one of a number of actions which are part of the HID
interface.  The core functions use these actions to tell the current
GUI when to change the presented information in response to changes
that the GUI may not know about.  The user normally does not invoke
these directly.

@end macro

%end-doc */


static const char pcbchanged_syntax[] = "PCBChanged([revert])";
static const char pcbchanged_help[] =
	"Tells the GUI that the whole PCB has changed. The optional \"revert\""
	"parameter can be used as a hint to the GUI that the same design is being"
	"reloaded, and that it might keep some viewport settings";

/* %start-doc actions PCBChanged

@hidaction

%end-doc */

static const char routestyleschanged_syntax[] = "RouteStylesChanged()";
static const char routestyleschanged_help[] = "Tells the GUI that the routing styles have changed.";

/* %start-doc actions RouteStylesChanged

@hidaction

%end-doc */

static const char netlistchanged_syntax[] = "NetlistChanged()";
static const char netlistchanged_help[] = "Tells the GUI that the netlist has changed.";

/* %start-doc actions NetlistChanged

@hidaction

%end-doc */

static const char layerschanged_syntax[] = "LayersChanged()";
static const char layerschanged_help[] = "Tells the GUI that the layers have changed.";

/* %start-doc actions LayersChanged

This includes layer names, colors, stacking order, visibility, etc.

@hidaction

%end-doc */

static const char librarychanged_syntax[] = "LibraryChanged()";
static const char librarychanged_help[] = "Tells the GUI that the libraries have changed.";

/* %start-doc actions LibraryChanged

@hidaction

%end-doc */

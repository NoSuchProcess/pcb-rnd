/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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
 *
 */

#include "config.h"
#include "conf_core.h"

#include <ctype.h>

#include <genht/htsp.h>
#include <libfungw/fungw_conv.h>

#include "error.h"
#include "event.h"
#include "actions.h"
#include "compat_misc.h"
#include "funchash.h"
#include "hidlib_conf.h"

const pcb_action_t *pcb_current_action = NULL;

fgw_ctx_t pcb_fgw;
fgw_obj_t *pcb_fgw_obj;

typedef struct {
	const char *cookie;
	const pcb_action_t *action;
} hid_cookie_action_t;

static const char *check_action_name(const char *s)
{
	while (*s)
		if (isspace((int) *s++) || *s == '(')
			return (s - 1);
	return NULL;
}

char *pcb_make_action_name(char *out, const char *inp, int inp_len)
{
	char *s;

	if (inp_len >= PCB_ACTION_NAME_MAX) {
		*out = '\0';
		return out;
	}

	memcpy(out, inp, inp_len+1);
	for(s = out; *s != '\0'; s++)
		*s = tolower(*s);
	return out;
}

void pcb_register_actions(const pcb_action_t *a, int n, const char *cookie)
{
	int i;
	hid_cookie_action_t *ca;
	fgw_func_t *f;

	for (i = 0; i < n; i++) {
		char fn[PCB_ACTION_NAME_MAX];
		int len;

		if (check_action_name(a[i].name)) {
			pcb_message(PCB_MSG_ERROR, "ERROR! Invalid action name, " "action \"%s\" not registered.\n", a[i].name);
			continue;
		}
		len = strlen(a[i].name);
		if (len >= sizeof(fn)) {
			pcb_message(PCB_MSG_ERROR, "Invalid action name: \"%s\" (too long).\n", a[i].name);
			continue;
		}

		ca = malloc(sizeof(hid_cookie_action_t));
		ca->cookie = cookie;
		ca->action = a+i;

		pcb_make_action_name(fn, a[i].name, len);
		f = fgw_func_reg(pcb_fgw_obj, fn, a[i].trigger_cb);
		if (f == NULL) {
			pcb_message(PCB_MSG_ERROR, "Failed to register action \"%s\" (already registered?)\n", a[i].name);
			free(ca);
			continue;
		}
		f->reg_data = ca;
	}
}

void pcb_register_action(const pcb_action_t *a, const char *cookie)
{
	pcb_register_actions(a, 1, cookie);
}

static void pcb_remove_action(fgw_func_t *f)
{
	hid_cookie_action_t *ca = f->reg_data;
	fgw_func_unreg(pcb_fgw_obj, f->name);
	free(ca);
}

fgw_func_t *pcb_act_lookup(const char *aname)
{
	char fn[PCB_ACTION_NAME_MAX];
	fgw_func_t *f = fgw_func_lookup(&pcb_fgw, pcb_aname(fn, aname));
	return f;
}

void pcb_remove_actions(const pcb_action_t *a, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		fgw_func_t *f = pcb_act_lookup(a[i].name);
		if (f == NULL) {
			pcb_message(PCB_MSG_WARNING, "Failed to remove action \"%s\" (is it registered?)\n", a[i].name);
			continue;
		}
		pcb_remove_action(f);
	}
}

void pcb_remove_actions_by_cookie(const char *cookie)
{
	htsp_entry_t *e;

	/* Slow linear search - probably OK, this will run only on uninit */
	for (e = htsp_first(&pcb_fgw.func_tbl); e; e = htsp_next(&pcb_fgw.func_tbl, e)) {
		fgw_func_t *f = e->value;
		hid_cookie_action_t *ca = f->reg_data;
		if ((ca != NULL) && (ca->cookie == cookie))
			pcb_remove_action(f);
	}
}

const pcb_action_t *pcb_find_action(const char *name, fgw_func_t **f_out)
{
	fgw_func_t *f;
	hid_cookie_action_t *ca;

	if (name == NULL)
		return NULL;

	f = pcb_act_lookup(name);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "unknown action `%s'\n", name);
		return NULL;
	}
	ca = f->reg_data;
	if (f_out != NULL)
		*f_out = f;
	return ca->action;
}

void pcb_print_actions()
{
	htsp_entry_t *e;

	fprintf(stderr, "Registered Actions:\n");
	for (e = htsp_first(&pcb_fgw.func_tbl); e; e = htsp_next(&pcb_fgw.func_tbl, e)) {
		fgw_func_t *f = e->value;
		hid_cookie_action_t *ca = f->reg_data;
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
	for (e = htsp_first(&pcb_fgw.func_tbl); e; e = htsp_next(&pcb_fgw.func_tbl, e)) {
		fgw_func_t *f = e->value;
		hid_cookie_action_t *ca = f->reg_data;
		const char *desc = ca->action->description;
		const char *synt = ca->action->syntax;
		const char *ck = ca->cookie;

		desc = desc ? desc : "";
		synt = synt ? synt : "";
		ck = ck ? ck : "";

		printf("A%s\n", ca->action->name);
		dump_string('D', desc);
		dump_string('S', synt);
		dump_string('C', ck);
	}
}

int pcb_action(const char *name)
{
	return pcb_actionv(name, 0, 0);
}

int pcb_actionl(const char *name, ...)
{
	const char *argv[20];
	int argc = 0;
	va_list ap;
	char *arg;

	va_start(ap, name);
	while ((arg = va_arg(ap, char *)) != 0)
		argv[argc++] = arg;
	va_end(ap);
	return pcb_actionv(name, argc, argv);
}

fgw_error_t pcb_actionv_(const fgw_func_t *f, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_error_t ret;
	int i;
	const pcb_action_t *old_action;
	hid_cookie_action_t *ca = f->reg_data;

	if (*pcbhlc_rc_verbose) {
		fprintf(stderr, "Action: \033[34m%s(", f->name);
		for (i = 0; i < argc; i++)
			fprintf(stderr, "%s%s", i ? "," : "", (argv[i].type & FGW_STR) == FGW_STR ? argv[i].val.str : "<non-str>");
		fprintf(stderr, ")\033[0m\n");
	}

	if (ca != NULL) {
		/* pcb-rnd action with a lot of metadata */
		old_action = pcb_current_action;
		pcb_current_action = ca->action;
		ret = pcb_current_action->trigger_cb(res, argc, argv);
		pcb_current_action = old_action;
	}
	else {
		/* direct call, no metadata */
		ret = f->func(res, argc, argv);
	}

	fgw_argv_free(&pcb_fgw, argc, argv);

	return ret;
}

fgw_error_t pcb_actionv_bin(const char *name, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_func_t *f = pcb_act_lookup(name);

	if (f == NULL)
		return FGW_ERR_NOT_FOUND;

	argv[0].type = FGW_FUNC;
	argv[0].val.func = f;

	res->type = FGW_INVALID;
	return pcb_actionv_(f, res, argc, argv);
}


int pcb_actionv(const char *name, int argc, const char **argsv)
{
	fgw_func_t *f;
	fgw_arg_t res, argv[PCB_ACTION_MAX_ARGS+1];
	int n;

	if (name == NULL)
		return 1;

	if (argc >= PCB_ACTION_MAX_ARGS) {
		pcb_message(PCB_MSG_ERROR, "can not call action %s with this many arguments (%d >= %d)\n", name, argc, PCB_ACTION_MAX_ARGS);
		return 1;
	}

	f = pcb_act_lookup(name);
	if (f == NULL) {
		int i;
		pcb_message(PCB_MSG_ERROR, "no action %s(", name);
		for (i = 0; i < argc; i++)
			pcb_message(PCB_MSG_ERROR, "%s%s", i ? ", " : "", argsv[i]);
		pcb_message(PCB_MSG_ERROR, ")\n");
		return 1;
	}
	argv[0].type = FGW_FUNC;
	argv[0].val.func = f;
	for(n = 0; n < argc; n++) {
		argv[n+1].type = FGW_STR;
		argv[n+1].val.str = (char *)argsv[n];
	}
	res.type = FGW_INVALID;
	if (pcb_actionv_(f, &res, argc+1, argv) != 0)
		return -1;
	if (fgw_arg_conv(&pcb_fgw, &res, FGW_INT) != 0)
		return -1;
	return res.val.nat_int;
}

void pcb_hid_get_coords(const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force)
{
	if (pcb_gui == NULL) {
		fprintf(stderr, "pcb_hid_get_coords: can not get coordinates (no gui) for '%s'\n", msg);
		*x = 0;
		*y = 0;
	}
	else
		pcb_gui->get_coords(msg, x, y, force);
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

	if ((*sp == '\0') || (*sp == '#')) {
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
	if (*sp == '\0') {
		retcode = pcb_actionv(aname, 0, 0);
		goto cleanup;
	}

	/* are we using parenthesis? */
	if (*sp == '(') {
		parens = 1;
		sp++;
	}
	else if (require_parens) {
		pcb_message(PCB_MSG_ERROR, "Syntax error: %s\n", rstr);
		pcb_message(PCB_MSG_ERROR, "    expected: Action(arg1, arg2)");
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
			retcode = pcb_actionv(aname, num, list);
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

const char *pcb_cli_prompt(const char *suffix)
{
	const char *base;
	static char prompt[128];
	int blen, slen, len;

	if ((*pcbhlc_rc_cli_prompt != NULL) && (**pcbhlc_rc_cli_prompt != '\0'))
		base = *pcbhlc_rc_cli_prompt;
	else if ((*pcbhlc_rc_cli_backend == NULL) || (**pcbhlc_rc_cli_backend == '\0'))
		base = "action";
	else
		base = *pcbhlc_rc_cli_backend;

	if ((suffix == NULL) || (*suffix == '\0'))
		return base;

	blen = strlen(base);
	slen = strlen(suffix);

	len = blen;
	if (len >= sizeof(prompt)-1-slen)
		len = sizeof(prompt)-1-slen;

	memcpy(prompt, base, len);
	memcpy(prompt+len, suffix, slen);
	prompt[len+slen] = '\0';
	return prompt;
}

static vtp0_t cli_stack;

static void cli_push(const char *val)
{
	if (val == NULL)
		val = "";
	vtp0_append(&cli_stack, pcb_strdup(val));
}

static char *cli_pop(void)
{
	if (cli_stack.used == 0)
		return NULL;
	return cli_stack.array[--cli_stack.used];
}

int pcb_cli_enter(const char *backend, const char *prompt)
{
	cli_push(*pcbhlc_rc_cli_backend);
	cli_push(*pcbhlc_rc_cli_prompt);

	if (conf_set(CFR_CLI, "rc/cli_backend", 0, backend, POL_OVERWRITE) != 0)
		return -1;
	return conf_set(CFR_CLI, "rc/cli_prompt", 0, prompt, POL_OVERWRITE);
}

int pcb_cli_leave(void)
{
	if (vtp0_len(&cli_stack) >= 2) {
		char *prompt = NULL, *backend = NULL;
		prompt = cli_pop();
		backend = cli_pop();
		conf_set(CFR_CLI, "rc/cli_backend", 0, backend, POL_OVERWRITE);
		conf_set(CFR_CLI, "rc/cli_prompt", 0, prompt, POL_OVERWRITE);
		free(prompt);
		free(backend);
		return 0;
	}

	conf_set(CFR_CLI, "rc/cli_backend", 0, "", POL_OVERWRITE);
	conf_set(CFR_CLI, "rc/cli_prompt", 0, "", POL_OVERWRITE);
	return -1;
}

static int pcb_cli_common(fgw_arg_t *args)
{
	const pcb_action_t *a;
	fgw_func_t *f;

	/* no backend: let the original action work */
	if ((*pcbhlc_rc_cli_backend == NULL) || (**pcbhlc_rc_cli_backend == '\0'))
		return -1;

	/* backend: let the backend action handle it */
	a = pcb_find_action(*pcbhlc_rc_cli_backend, &f);
	if (!a)
		return -1;

	args[0].type = FGW_FUNC;
	args[0].val.func = f;
	return 0;
}

int pcb_cli_tab(void)
{
	fgw_arg_t res, args[2];

	if (pcb_cli_common(args) != 0)
		return -1;

	args[1].type = FGW_STR;
	args[1].val.str = "/tab";

	if (pcb_actionv_(args[0].val.func, &res, 2, args) != 0)
			return -1;
	fgw_arg_conv(&pcb_fgw, &res, FGW_INT);
	return res.val.nat_int;
}

int pcb_cli_edit(void)
{
	fgw_arg_t res, args[2];

	if (pcb_cli_common(args) != 0)
		return -1;

	args[1].type = FGW_STR;
	args[1].val.str = "/edit";

	if (pcb_actionv_(args[0].val.func, &res, 2, args) != 0)
			return -1;
	fgw_arg_conv(&pcb_fgw, &res, FGW_INT);
	return res.val.nat_int;
}

int pcb_cli_mouse(pcb_bool notify)
{
	fgw_arg_t res, args[3];

	if (pcb_cli_common(args) != 0)
		return -1;

	args[1].type = FGW_STR;
	args[1].val.str = "/click";
	args[2].type = FGW_INT;
	args[2].val.nat_int = notify;

	if (pcb_actionv_(args[0].val.func, &res, 3, args) != 0)
			return -1;
	fgw_arg_conv(&pcb_fgw, &res, FGW_INT);
	return res.val.nat_int;
}


void pcb_cli_uninit(void)
{
	while(vtp0_len(&cli_stack) > 0)
		free(cli_pop());
}

int pcb_parse_command(const char *str_, pcb_bool force_action_mode)
{
	fgw_arg_t res, args[2];
	fgw_func_t *f;
	const pcb_action_t *a;
	const char *end;

	/* no backend or forced action mode: classic pcb-rnd action parse */
	if (force_action_mode || (*pcbhlc_rc_cli_backend == NULL) || (**pcbhlc_rc_cli_backend == '\0')) {
		pcb_event(PCB_EVENT_CLI_ENTER, "s", str_);
		return hid_parse_actionstring(str_, pcb_false);
	}

	/* backend: let the backend action handle it */
	a = pcb_find_action(*pcbhlc_rc_cli_backend, &f);
	if (!a) {
		pcb_message(PCB_MSG_ERROR, "cli: no action %s; leaving mode\n", *pcbhlc_rc_cli_backend);
		pcb_cli_leave();
		return -1;
	}

	end = strpbrk(str_, "\n\r");

	args[0].type = FGW_FUNC;
	args[0].val.func = f;
	if (end == NULL) {
		/* optimization: string doesn't contain newline - pass it as is to save an strdup */
		args[1].type = FGW_STR;
		args[1].val.str = pcb_strdup(str_);
	}
	else {
		/* string contains a newline; need to cut there, which needs a dup; let fungw free it as dynamic string, cleaning up args after the fungw call */
		args[1].type = FGW_STR | FGW_DYN;
		args[1].val.str = pcb_strdup(str_);
		args[1].val.str[end - str_] = '\0';
	}

	if (pcb_actionv_(f, &res, 2, args) != 0)
			return -1;
	fgw_arg_conv(&pcb_fgw, &res, FGW_INT);
	return res.val.nat_int;
}

int pcb_parse_actions(const char *str_)
{
	return hid_parse_actionstring(str_, pcb_true);
}

/*** custom fungw types ***/
#define conv_str2kw(dst, src) dst = pcb_funchash_get(src, NULL)

static int keyword_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_KEYWORD) { /* convert to keyword */
		long tmp;
		switch(FGW_BASE_TYPE(arg->type)) {
			ARG_CONV_CASE_LONG(tmp, conv_err)
			ARG_CONV_CASE_LLONG(tmp, conv_err)
			ARG_CONV_CASE_DOUBLE(tmp, conv_err)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_err)
			ARG_CONV_CASE_STR(tmp, conv_str2kw)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
		}
		arg->type = FGW_KEYWORD;
		fgw_keyword(arg) = tmp;
		return 0;
	}
	if (arg->type == FGW_KEYWORD) { /* convert from keyword */
		long tmp = fgw_keyword(arg);
		switch(target) {
			ARG_CONV_CASE_LONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_LLONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_DOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
			case FGW_STR:
				arg->val.str = (char *)pcb_funchash_reverse(tmp);
				arg->type = FGW_STR;
				return 0;
		}
		arg->type = target;
		return 0;
	}
	fprintf(stderr, "Neither side of the conversion is keyword\n");
	abort();
}

char *fgw_str2coord_unit = NULL;
#define conv_str2coord(dst, src) \
do { \
	pcb_bool succ; \
	dst = pcb_get_value_ex(src, NULL, NULL, NULL, fgw_str2coord_unit, &succ); \
	if (!succ) \
		return -1; \
} while(0)

static int coord_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_COORD) { /* convert to coord */
		pcb_coord_t tmp;
		switch(FGW_BASE_TYPE(arg->type)) {
			ARG_CONV_CASE_LONG(tmp, conv_assign)
			ARG_CONV_CASE_LLONG(tmp, conv_assign)
			ARG_CONV_CASE_DOUBLE(tmp, conv_assign)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_assign)
			ARG_CONV_CASE_STR(tmp, conv_str2coord)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
		}
		arg->type = FGW_COORD;
		fgw_coord(arg) = tmp;
		return 0;
	}
	if (arg->type == FGW_COORD) { /* convert from coord */
		pcb_coord_t tmp = fgw_coord(arg);
		switch(target) {
			ARG_CONV_CASE_LONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_LLONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_DOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
			case FGW_STR:
				arg->val.str = (char *)pcb_strdup_printf("%.08$mH", tmp);
				arg->type = FGW_STR | FGW_DYN;
				return 0;
		}
		arg->type = target;
		return 0;
	}
	fprintf(stderr, "Neither side of the conversion is coord\n");
	abort();
}

#define conv_str2coords(dst, src) \
do { \
	pcb_bool succ, abso; \
	dst.c[0] = pcb_get_value_ex(src, NULL, &abso, NULL, fgw_str2coord_unit, &succ); \
	if (!succ) \
		return -1; \
	dst.len = 1; \
	dst.absolute[0] = abso; \
} while(0)



#define conv_loadcoords(dst, src) \
do { \
	dst.len = 1; \
	dst.absolute[0] = 1; \
	dst.c[0] = src; \
} while(0)

static int coords_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_COORDS) { /* convert to coord */
		fgw_coords_t tmp;
		switch(FGW_BASE_TYPE(arg->type)) {
			ARG_CONV_CASE_LONG(tmp, conv_loadcoords)
			ARG_CONV_CASE_LLONG(tmp, conv_loadcoords)
			ARG_CONV_CASE_DOUBLE(tmp, conv_loadcoords)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_loadcoords)
			ARG_CONV_CASE_STR(tmp, conv_str2coords)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
		}
		arg->type = FGW_COORDS | FGW_DYN;
		fgw_coords(arg) = malloc(sizeof(tmp));
		memcpy(fgw_coords(arg), &tmp, sizeof(tmp));
		return 0;
	}
	if (arg->type == FGW_COORDS) { /* convert from coord */
		fgw_coords_t *tmp = fgw_coords(arg);
		switch(target) {
			ARG_CONV_CASE_LONG(tmp, conv_err)
			ARG_CONV_CASE_LLONG(tmp, conv_err)
			ARG_CONV_CASE_DOUBLE(tmp, conv_err)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_err)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
			case FGW_STR:
				arg->val.str = (char *)pcb_strdup_printf("%.08$mH", tmp);
				arg->type = FGW_STR | FGW_DYN;
				return 0;
		}
		arg->type = target;
		return 0;
	}
	fprintf(stderr, "Neither side of the conversion is coords\n");
	abort();
}

static int coords_arg_free(fgw_ctx_t *ctx, fgw_arg_t *arg)
{
	assert(arg->type == (FGW_COORDS | FGW_DYN));
	free(arg->val.ptr_void);
	return 0;
}

static void pcb_action_err(fgw_obj_t *obj, const char *msg)
{
	pcb_message(PCB_MSG_ERROR, "fungw(%s): %s", obj->name, msg);
}

void pcb_actions_init(void)
{
	fgw_init(&pcb_fgw, "pcb-rnd");
	pcb_fgw.async_error = pcb_action_err;
	pcb_fgw_obj = fgw_obj_reg(&pcb_fgw, "core");
	if (fgw_reg_custom_type(&pcb_fgw, FGW_KEYWORD, "keyword", keyword_arg_conv, NULL) != FGW_KEYWORD) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_KEYWORD\n");
		abort();
	}
	if (fgw_reg_custom_type(&pcb_fgw, FGW_COORD, "coord", coord_arg_conv, NULL) != FGW_COORD) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_COORD\n");
		abort();
	}
	if (fgw_reg_custom_type(&pcb_fgw, FGW_COORDS, "coords", coords_arg_conv, coords_arg_free) != FGW_COORDS) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_COORDS\n");
		abort();
	}
}

void pcb_actions_uninit(void)
{
	htsp_entry_t *e;

	for (e = htsp_first(&pcb_fgw.func_tbl); e; e = htsp_next(&pcb_fgw.func_tbl, e)) {
		fgw_func_t *f = e->value;
		hid_cookie_action_t *ca = f->reg_data;
		if (ca->cookie != NULL)
			fprintf(stderr, "ERROR: hid_actions_uninit: action '%s' with cookie '%s' left registered, check your plugins!\n", e->key, ca->cookie);
		pcb_remove_action(f);
	}

	fgw_obj_unreg(&pcb_fgw, pcb_fgw_obj);
	fgw_uninit(&pcb_fgw);
	fgw_atexit();
}


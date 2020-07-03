/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019,2020 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <librnd/core/safe_fs.h>


static void library_param_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	library_ctx_t *ctx = caller_data;
	htsi_entry_t *e;

	gds_uninit(&ctx->descr);
	free(ctx->help_params);
	ctx->help_params = NULL;

	for(e = htsi_first(&ctx->param_names); e != NULL; e = htsi_next(&ctx->param_names, e))
		free(e->key);
	htsi_uninit(&ctx->param_names);
	if (ctx->pactive) {
		ctx->pactive = 0;
		RND_DAD_FREE(ctx->pdlg);
	}
	update_edit_button(ctx);
}

#define colsplit() \
do { \
	col = strchr(col, ':'); \
	if (col != NULL) { \
		*col = '\0'; \
		col++; \
	} \
} while(0)

static void set_attr(library_ctx_t *ctx, int pidx, char *val)
{
	rnd_hid_attr_val_t hv;
	rnd_hid_attribute_t *a = &ctx->pdlg[ctx->pwid[pidx]];
	const char **s;
	char *desc;
	int vlen, len, n;

	switch(a->type) {
		case RND_HATT_ENUM:
			hv.lng = 0; /* fallback in case the loop doesn't find any match */
			vlen = strlen(val);
			for(n = 0, s = a->wdata; *s != NULL; s++,n++) {
				desc = strstr(*s, " (");
				if (desc != NULL)
					len = desc - *s;
				else
					len = strlen(*s);
				if ((len == vlen) && (strncmp(*s, val, len) == 0)) {
					hv.lng = n;
					break;
				}
			}
			break;
		case RND_HATT_STRING:
			hv.str = val;
			break;
		case RND_HATT_BOOL:
			if ((val == NULL) || (*val == '\0'))
				return;
			hv.lng = \
				(*val == '1') || (*val == 't') || (*val == 'T') || (*val == 'y') || (*val == 'Y') || \
				(((val[0] == 'o') || (val[0] == 'O')) && ((val[1] == 'n') || (val[1] == 'N')));
			break;
		case RND_HATT_COORD:
		case RND_HATT_END: /* compound widget for the spinbox! */
			hv.crd = rnd_get_value_ex(val, NULL, NULL, NULL, "mil", NULL);
			break;
		default:
			assert(!"set_attr() can't set non-data field!\n");
			return;
	}
	rnd_gui->attr_dlg_set_value(ctx->pdlg_hid_ctx, ctx->pwid[pidx], &hv);
}

static void library_param_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp);

#define pre_append() \
do { \
	if (help_def != NULL) { \
		if (help != NULL) { \
			char *tmp = rnd_concat(help, "\nDefault: ", help_def, NULL); \
			free(help); \
			free(help_def); \
			help = tmp; \
		} \
		else \
			help = help_def; \
		help_def = NULL; \
	} \
} while(0)

#define post_append() \
do { \
	vtp0_uninit(&curr_enum); \
	free(name); name = NULL; \
	free(help); help = NULL; \
	curr_type = RND_HATT_END; \
	curr = -1; \
	vtp0_init(&curr_enum); \
	vtp0_append(&curr_enum, rnd_strdup("")); \
	numrows++; \
} while(0)

#define append() \
do { \
	if (curr >= MAX_PARAMS) { \
		if (curr == MAX_PARAMS) \
			rnd_message(RND_MSG_ERROR, "too many parameters, displaying only the first %d\n", MAX_PARAMS); \
		break; \
	} \
	if (curr_type == RND_HATT_END) \
		break; \
	pre_append(); \
	RND_DAD_LABEL(library_ctx.pdlg, name); \
		RND_DAD_HELP(library_ctx.pdlg, rnd_strdup(help)); \
	switch(curr_type) { \
		case RND_HATT_COORD: \
		case RND_HATT_END: \
			RND_DAD_COORD(library_ctx.pdlg); \
				ctx->pwid[curr] = RND_DAD_CURRENT(library_ctx.pdlg); \
				RND_DAD_MINMAX(library_ctx.pdlg, 0, RND_MM_TO_COORD(512)); \
				RND_DAD_CHANGE_CB(library_ctx.pdlg, library_param_cb); \
			break; \
		case RND_HATT_STRING: \
			RND_DAD_STRING(library_ctx.pdlg); \
				ctx->pwid[curr] = RND_DAD_CURRENT(library_ctx.pdlg); \
				RND_DAD_CHANGE_CB(library_ctx.pdlg, library_param_cb); \
			break; \
		case RND_HATT_BOOL: \
			RND_DAD_BOOL(library_ctx.pdlg); \
				ctx->pwid[curr] = RND_DAD_CURRENT(library_ctx.pdlg); \
				RND_DAD_CHANGE_CB(library_ctx.pdlg, library_param_cb); \
			break; \
		case RND_HATT_ENUM: \
			RND_DAD_ENUM(library_ctx.pdlg, (const char **)curr_enum.array); \
				ctx->pwid[curr] = RND_DAD_CURRENT(library_ctx.pdlg); \
				RND_DAD_CHANGE_CB(library_ctx.pdlg, library_param_cb); \
				vtp0_init(&curr_enum); \
				vtp0_append(&curr_enum, rnd_strdup("")); \
			break; \
		default: \
			RND_DAD_LABEL(library_ctx.pdlg, "internal error: invalid type"); \
	} \
	RND_DAD_HELP(library_ctx.pdlg, rnd_strdup(help)); \
	ctx->pnames[curr] = rnd_strdup(name); \
	htsi_set(&ctx->param_names, ctx->pnames[curr], curr); \
	post_append(); \
} while(0)

static int library_param_build(library_ctx_t *ctx, pcb_fplibrary_t *l, FILE *f)
{
	char line[1024];
	char *name = NULL, *help = NULL, *help_def = NULL;
	vtp0_t curr_enum;
	int curr, examples = 0, numrows = 0;
	rnd_hid_attr_type_t curr_type = RND_HATT_END;

	curr = -1;

	free(ctx->example);
	ctx->example = NULL;
	ctx->num_params = 0;

	vtp0_init(&curr_enum);
	vtp0_append(&curr_enum, rnd_strdup(""));

	while(fgets(line, sizeof(line), f) != NULL) {
		char *end, *col, *arg, *cmd = line;

		/* rtrim */
		end = line + strlen(line) - 1;
		while((end >= line) && ((*end == '\r') || (*end == '\n'))) {
			*end = '\0';
			end--;
		}

		/* ltrim */
		cmd = strchr(cmd, '@');
		if ((cmd == NULL) || (cmd[1] != '@'))
			continue;
		cmd+=2;
		arg = strpbrk(cmd, " \t\r\n");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
			while(isspace(*arg)) arg++;
		}
		col = cmd;

		/* parse */
		if (strcmp(cmd, "desc") == 0) {
			if (examples < 2) {
				gds_append_str(&ctx->descr, arg);
				gds_append(&ctx->descr, '\n');
			}
		}
		else if (strcmp(cmd, "params") == 0) {
			free(ctx->help_params);
			ctx->help_params = rnd_strdup(arg);
		}
		else if (strcmp(cmd, "example") == 0) {
			if (examples == 0) {
				free(ctx->example);
				ctx->example = rnd_strdup(arg);
			}
			examples++;
		}
		else if (strncmp(cmd, "optional:", 9) == 0) {
			if (ctx->first_optional < 0)
				ctx->first_optional = ctx->num_params-1;
		}
		else if (strncmp(cmd, "param:", 6) == 0) {
			append();
			colsplit();
			curr = ctx->num_params;
			ctx->num_params++;
			free(name);
			free(help);
			name = rnd_strdup(col);
			help = rnd_strdup(arg);
			curr_type = RND_HATT_STRING; /* assume string until a dim or enum overrides that */
		}
		else if (strncmp(cmd, "default:", 6) == 0) {
			free(help_def);
			if (arg == NULL)
				arg = "";
			help_def = rnd_strdup(arg);
		}
		else if (strncmp(cmd, "dim:", 4) == 0) {
			curr_type = RND_HATT_COORD;
		}
		else if (strncmp(cmd, "bool:", 5) == 0) {
			curr_type = RND_HATT_BOOL;
		}
		else if (strncmp(cmd, "enum:", 5) == 0) {
			char *evl;

			curr_type = RND_HATT_ENUM;
			colsplit(); colsplit();
			if (arg != NULL) {
				if (strlen(arg) > 32) {
					arg[32] = '\0';
					evl = rnd_strdup_printf("%s (%s...)", col, arg);
				}
				else
					evl = rnd_strdup_printf("%s (%s)", col, arg);
			}
			else
				evl = rnd_strdup(col);
			vtp0_append(&curr_enum, evl);
		}
	}
	append();
	return numrows;
}

static char *gen_cmd(library_ctx_t *ctx)
{
	int n, pushed = 0;
	gds_t sres;
	char *tmp;

	memset(&sres, 0, sizeof(sres));

	gds_append_str(&sres, ctx->last_l->name);

	/* cut original name at "(" */
	tmp = strchr(sres.array, '(');
	if (tmp != NULL)
		gds_truncate(&sres, tmp - sres.array);

	gds_append_str(&sres, "(");

	for(n = 0; n < ctx->num_params; n++) {
		char *desc, buff[128];
		const char *val;
		rnd_hid_attribute_t *a = &ctx->pdlg[ctx->pwid[n]];

		if (((n >= ctx->first_optional) && (!a->changed)) || (a->empty))
			continue;

		switch(a->type) {
			case RND_HATT_LABEL:
			case RND_HATT_BEGIN_TABLE:
				continue;
			case RND_HATT_ENUM:
				val = ((const char **)(a->wdata))[a->val.lng];
				if (val != NULL) {
					if (*val != '\0') {
						desc = strstr((char *)val, " (");
						if (desc != NULL)
							*desc = '\0';
					}
					else
						val = NULL;
				}
				break;
			case RND_HATT_STRING:
				val = a->val.str;
				if ((val != NULL) && (*val == '\0'))
					continue;
				break;
			case RND_HATT_BOOL:
				val = a->val.lng ? "yes" : "no";
				break;
			case RND_HATT_COORD:
			case RND_HATT_END:
				val = buff;
				rnd_snprintf(buff, sizeof(buff), "%.09$$mH", a->val.crd);
				break;
			default:;
		}

		if (val == NULL)
			continue;

		if (pushed)
			gds_append_str(&sres, ", ");

		if ((n == pushed) && (n < ctx->first_optional))
			gds_append_str(&sres, val); /* positional */
		else
			rnd_append_printf(&sres, "%s=%s", ctx->pnames[n], val);
		pushed++;
	}

	gds_append_str(&sres, ")");
	return sres.array;
}

static void library_param_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	char *cmd = gen_cmd(ctx);
	rnd_hid_attr_val_t hv;

	hv.str = cmd;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilt, &hv);
	free(cmd);
	timed_update_preview(ctx, 1);
}

static int param_split(char *buf, char *argv[], int amax)
{
	int n;
	char *next, *bump;

	for(n=0; ;n++) {
		if (n >= amax)
			return n+1;

		/* ltrim */
		while(isspace(*buf)) buf++;
		argv[n] = buf;

		/* find next param */
		bump = buf;
		next = strchr(buf, ',');
		if (next == NULL)
			return n+1;
		buf = next+1;

		/* rtrim */
		*next = '\0';
		next--;
		while((next >= bump) && (isspace(*next))) {
			*next = '\0';
			next--;
		}
	}
	return -1;
}

static void load_params(library_ctx_t *ctx, char *user_params)
{
	char *parain;
	char *parahlp;
	int argc_in, argc_help, posi, n;
	char *end, *argv_in[MAX_PARAMS], *argv_help[MAX_PARAMS];
	char *help_params = ctx->help_params;

	if (user_params == NULL)
		user_params = "";
	if (help_params == NULL)
		help_params = "";


	parain = rnd_strdup(user_params);
	parahlp = rnd_strdup(help_params);

	/* truncate trailing ")" */
	if (*parain != '\0') {
		end = parain + strlen(parain) - 1;
		if (*end == ')')
			*end = '\0';
	}

	argc_in = param_split(parain, argv_in, MAX_PARAMS);
	argc_help = param_split(parahlp, argv_help, MAX_PARAMS);

	/* iterate and assign default values and mark them changed to get them back */
	for(posi = n = 0; n < argc_in; n++) {
		char *key, *val, *sep;
		htsi_entry_t *e;
		int pidx;

		sep = strchr(argv_in[n], '=');
		if (sep != NULL) {
			key = argv_in[n];
			*sep = '\0';
			val = sep+1;

			/* rtrim key */
			sep--;
			while((sep >= key) && (isspace(*sep))) {
				*sep = '\0';
				sep--;
			}

			/* ltrim val */
			while(isspace(*val)) val++;
		}
		else {
			if (posi >= argc_help) {
				rnd_message(RND_MSG_ERROR, "More positional parameters than expected - ignoring %s", argv_in[n]);
				continue;
			}
			key = argv_help[posi];
			val = argv_in[n];
			posi++;
		}

		e = htsi_getentry(&ctx->param_names, key);
		if (e == NULL) {
			rnd_message(RND_MSG_ERROR, "Unknown parameter %s - ignoring value %s", key, val);
			continue;
		}
		pidx = e->value;
		set_attr(ctx, pidx, val);
	}

	/* clean up */
	free(parain);
	free(parahlp);
}

int pcb_library_param_fillin(library_ctx_t *ctx, pcb_fplibrary_t *l)
{
	rnd_hid_attr_val_t hv;
	const char *filter_txt = ctx->dlg[ctx->wfilt].val.str;

	if (filter_txt != NULL) {
		char *sep;
		int len;
		sep = strchr(l->name, '(');
		if (sep != NULL)
			len = sep - l->name;
		else
			len = strlen(l->name);
		if (strncmp(filter_txt, l->name, len) != 0) {
			/* clicked away from the previous parametric, but the filter text is still for that one; replace it */
			filter_txt = NULL;
		}
	}


	if (filter_txt == NULL) {

		filter_txt = ctx->example;

		hv.str = filter_txt;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilt, &hv);
	}

	if (filter_txt != NULL) {
		const char *n1, *n2;
		char *prm = strchr(filter_txt, '(');

		/* if filter text doesn't have parameters, try the example */
		if ((prm == NULL) || (prm[1] == ')')) {
			if (ctx->example != NULL) {
				filter_txt = ctx->example;
				hv.str = filter_txt;
				rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilt, &hv);
				prm = strchr(filter_txt, '(');
			}
		}

		/* do not load parameters from the comamnd of a differently named
		   footprint to avoid invalid mixing of similar named parameters; in
		   that case rather fill in the dialog with the example */
		for(n1 = filter_txt, n2 = l->name;; n1++, n2++) {
			if (*n1 != *n2) {
				prm = ctx->example;
				if (prm == NULL)
					return -1;
				while((*prm != '(') && (*prm != '\0')) prm++;
				break;
			}
			else if ((*n1 == '(') || (*n1 == '\0'))
				break;
		}

		if (prm != NULL)
			load_params(ctx, prm+1);
	}

	hv.str = ctx->descr.array;
	if (hv.str == NULL)
		hv.str = "";
	rnd_gui->attr_dlg_set_value(ctx->pdlg_hid_ctx, ctx->pwdesc, &hv);
	timed_update_preview(ctx, 1);
	return 0;
}

static int library_param_open(library_ctx_t *ctx, pcb_fplibrary_t *l, FILE *f)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	int w, oversized = 0;

	RND_DAD_BEGIN_VBOX(library_ctx.pdlg);
		RND_DAD_COMPFLAG(library_ctx.pdlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(library_ctx.pdlg, "n/a");
			ctx->pwdesc = RND_DAD_CURRENT(library_ctx.pdlg);
		RND_DAD_BEGIN_TABLE(library_ctx.pdlg, 2);
			RND_DAD_COMPFLAG(library_ctx.pdlg, RND_HATF_EXPFILL);
			w = RND_DAD_CURRENT(library_ctx.pdlg);
			if (library_param_build(ctx, l, f) > 16) {
				library_ctx.pdlg[w].rnd_hatt_flags |= RND_HATF_SCROLL;
				oversized = 1;
			}
		RND_DAD_END(library_ctx.pdlg);
		RND_DAD_BUTTON_CLOSES(library_ctx.pdlg, clbtn);
	RND_DAD_END(library_ctx.pdlg);
	return oversized;
}

static FILE *library_param_get_help(library_ctx_t *ctx, pcb_fplibrary_t *l)
{
	FILE *f;
	char *cmd;

#ifdef __WIN32__
	{
		char *s;
		cmd = rnd_strdup_printf("%s/sh -c \"%s --help\"", rnd_w32_bindir, l->data.fp.loc_info);
		for(s = cmd; *s != '\0'; s++)
			if (*s == '\\')
				*s = '/';
	}
#else
	cmd = rnd_strdup_printf("%s --help", l->data.fp.loc_info);
#endif
	f = rnd_popen(NULL, cmd, "r");
	free(cmd);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can not execute parametric footprint %s\n", l->data.fp.loc_info);
		return NULL;
	}

	return f;
}

static void library_param_dialog(library_ctx_t *ctx, pcb_fplibrary_t *l)
{
	FILE *f;

	if (ctx->last_l != l) {
		if (ctx->pactive) {
			ctx->pactive = 0;
			RND_DAD_FREE(ctx->pdlg);
		}
		update_edit_button(ctx);
	}
	else if (ctx->pactive) /* reopening the same -> nop */
		return;

	ctx->last_l = l;

	if (l == NULL)
		return;

	f = library_param_get_help(ctx, l);
	if (f == NULL)
		return;

	htsi_init(&ctx->param_names, strhash, strkeyeq);
	gds_init(&ctx->descr);
	ctx->first_optional = -1;


	ctx->pactive = 1;
	if (library_param_open(ctx, l, f)) {
		/* oversized dialog got the scroll bar, which would make it small;
		   set preferred size so it opens in reasonable area even when win size
		   not persistent (window palcement code) */
		RND_DAD_DEFSIZE(library_ctx.pdlg, 700, 500);
	}
	rnd_pclose(f);

	RND_DAD_NEW("lib_param", library_ctx.pdlg, "pcb-rnd parametric footprint", ctx, rnd_false, library_param_close_cb);

	update_edit_button(ctx);
	pcb_library_param_fillin(ctx, l);
}


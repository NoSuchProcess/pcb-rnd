/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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

#include "safe_fs.h"


static void library_param_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	library_ctx_t *ctx = caller_data;
	htsi_entry_t *e;

	gds_uninit(&ctx->descr);
	free(ctx->help_params);

	for(e = htsi_first(&ctx->param_names); e != NULL; e = htsi_next(&ctx->param_names, e))
		free(e->key);
	htsi_uninit(&ctx->param_names);
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
	pcb_hid_attr_val_t hv;
	pcb_hid_attribute_t *a = &ctx->pdlg[ctx->pwid[pidx]];
	const char **s;
	char *desc;
	int vlen, len, n;

	switch(a->type) {
		case PCB_HATT_ENUM:
			vlen = strlen(val);
			for(n = 0, s = a->enumerations; *s != NULL; s++,n++) {
				desc = strstr(*s, " (");
				if (desc != NULL)
					len = desc - *s;
				else
					len = strlen(*s);
				if ((len == vlen) && (strncmp(*s, val, len) == 0)) {
					hv.int_value = n;
					break;
				}
			}
			break;
		case PCB_HATT_STRING:
			hv.str_value = val;
			break;
		case PCB_HATT_COORD:
			hv.coord_value = pcb_get_value_ex(val, NULL, NULL, NULL, "mil", NULL);
			break;
		default:
			assert(!"set_attr() can't set non-data field!\n");
			return;
	}
	pcb_gui->attr_dlg_set_value(ctx->pdlg_hid_ctx, ctx->pwid[pidx], &hv);
}

#define pre_append() \
do { \
	if (help_def != NULL) { \
		if (help != NULL) { \
			char *tmp = pcb_concat(help, "\nDefault: ", help_def, NULL); \
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
	curr_type = PCB_HATT_END; \
	curr = -1; \
	vtp0_init(&curr_enum); \
} while(0)

#define append() \
do { \
	if (curr >= MAX_PARAMS) { \
		if (curr == MAX_PARAMS) \
			pcb_message(PCB_MSG_ERROR, "too many parameters, displaying only the first %d\n", MAX_PARAMS); \
		break; \
	} \
	if (curr_type == PCB_HATT_END) \
		break; \
	pre_append(); \
	PCB_DAD_LABEL(library_ctx.pdlg, name); \
		PCB_DAD_HELP(library_ctx.pdlg, pcb_strdup(help)); \
	switch(curr_type) { \
		case PCB_HATT_COORD: \
			PCB_DAD_COORD(library_ctx.pdlg, ""); \
				ctx->pwid[curr] = PCB_DAD_CURRENT(library_ctx.pdlg); \
				PCB_DAD_MINMAX(library_ctx.pdlg, 0, PCB_MM_TO_COORD(512)); \
			break; \
		case PCB_HATT_STRING: \
			PCB_DAD_STRING(library_ctx.pdlg); \
				ctx->pwid[curr] = PCB_DAD_CURRENT(library_ctx.pdlg); \
			break; \
		case PCB_HATT_ENUM: \
			PCB_DAD_ENUM(library_ctx.pdlg, (const char **)curr_enum.array); \
				ctx->pwid[curr] = PCB_DAD_CURRENT(library_ctx.pdlg); \
			vtp0_init(&curr_enum); \
			break; \
		default: \
			PCB_DAD_LABEL(library_ctx.pdlg, "internal error: invalid type"); \
	} \
	PCB_DAD_HELP(library_ctx.pdlg, pcb_strdup(help)); \
	htsi_set(&ctx->param_names, pcb_strdup(name), curr); \
	post_append(); \
} while(0)

static void library_param_build(library_ctx_t *ctx, pcb_fplibrary_t *l, FILE *f)
{
	char *sres, line[1024];
	char *name = NULL, *help = NULL, *help_def = NULL;
	vtp0_t curr_enum;
	int curr;
	pcb_hids_t curr_type = PCB_HATT_END;

	curr = -1;

	free(ctx->example);
	ctx->example = NULL;

	vtp0_init(&curr_enum);

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
			gds_append_str(&ctx->descr, arg);
			gds_append(&ctx->descr, '\n');
		}
		else if (strcmp(cmd, "params") == 0) {
			free(ctx->help_params);
			ctx->help_params = pcb_strdup(arg);
		}
		else if (strcmp(cmd, "example") == 0) {
			free(ctx->example);
			ctx->example = pcb_strdup(arg);
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
			name = pcb_strdup(col);
			help = pcb_strdup(arg);
			curr_type = PCB_HATT_STRING; /* assume string until a dim or enum overrides that */
		}
		else if (strncmp(cmd, "default:", 6) == 0) {
			free(help_def);
			help_def = pcb_strdup(arg);
		}
		else if (strncmp(cmd, "dim:", 4) == 0) {
			curr_type = PCB_HATT_COORD;
		}
		else if (strncmp(cmd, "enum:", 5) == 0) {
			char *evl;

			curr_type = PCB_HATT_ENUM;
			colsplit(); colsplit();
			if (arg != NULL)
				evl = pcb_strdup_printf("%s (%s)", col, arg);
			else
				evl = pcb_strdup(col);
			vtp0_append(&curr_enum, evl);
		}
	}
	append();
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


	parain = pcb_strdup(user_params);
	parahlp = pcb_strdup(help_params);

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
		pcb_hid_attribute_t *a;
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
				pcb_message(PCB_MSG_ERROR, "More positional parameters than expected - ignoring %s", argv_in[n]);
				continue;
			}
			key = argv_help[posi];
			val = argv_in[n];
			posi++;
		}

		e = htsi_getentry(&ctx->param_names, key);
		if (e == NULL) {
			pcb_message(PCB_MSG_ERROR, "Unknown parameter %s - ignoring value %s", key, val);
			continue;
		}
		pidx = e->value;
		set_attr(ctx, pidx, val);
	}

	/* clean up */
	free(parain);
	free(parahlp);
}

static int library_param_fillin(library_ctx_t *ctx, pcb_fplibrary_t *l)
{
	pcb_hid_attr_val_t hv;
	int dirty = 0;
	const char *filter_txt = ctx->dlg[ctx->wfilt].default_val.str_value;

	if (filter_txt == NULL) {

		filter_txt = ctx->example;
		dirty = 1;

		hv.str_value = filter_txt;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilt, &hv);
	}

	if (filter_txt != NULL) {
		char *prm = strchr(filter_txt, '(');

		/* if filter text doesn't have parameters, try the example */
		if ((prm == NULL) || (prm[1] == ')')) {
			if (ctx->example != NULL) {
				filter_txt = ctx->example;
				hv.str_value = filter_txt;
				pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilt, &hv);
				prm = strchr(filter_txt, '(');
				dirty = 1;
			}
		}

		if (prm != NULL)
			load_params(ctx, prm+1);
	}

	hv.str_value = ctx->descr.array;
	if (hv.str_value == NULL)
		hv.str_value = "";
	pcb_gui->attr_dlg_set_value(ctx->pdlg_hid_ctx, ctx->pwdesc, &hv);
	return dirty;
}

static void library_param_open(library_ctx_t *ctx, pcb_fplibrary_t *l, FILE *f)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	PCB_DAD_BEGIN_VBOX(library_ctx.pdlg);
		PCB_DAD_COMPFLAG(library_ctx.pdlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(library_ctx.pdlg, "n/a");
			ctx->pwdesc = PCB_DAD_CURRENT(library_ctx.pdlg);
		PCB_DAD_BEGIN_TABLE(library_ctx.pdlg, 2);
			PCB_DAD_COMPFLAG(library_ctx.pdlg, PCB_HATF_EXPFILL);
			library_param_build(ctx, l, f);
		PCB_DAD_END(library_ctx.pdlg);
		PCB_DAD_BUTTON_CLOSES(library_ctx.pdlg, clbtn);
	PCB_DAD_END(library_ctx.pdlg);
}

static void library_param_dialog(library_ctx_t *ctx, pcb_fplibrary_t *l)
{
	FILE *f;
	char *cmd;
	int n, dirty = 0;


	if (ctx->last_l != l) {
TODO("if active: close if new l differs\n");
		ctx->last_l = l;
	}

	if (l == NULL)
		return;

	cmd = pcb_strdup_printf("%s --help", l->data.fp.loc_info);
	f = pcb_popen(NULL, cmd, "r");
	free(cmd);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not execute parametric footprint %s\n", l->data.fp.loc_info);
		return;
	}

	htsi_init(&ctx->param_names, strhash, strkeyeq);
	gds_init(&ctx->descr);
	ctx->first_optional = -1;


	library_param_open(ctx, l, f);
	pcb_pclose(f);

	PCB_DAD_NEW("lib_param", library_ctx.pdlg, "pcb-rnd parametric footprint", ctx, pcb_false, library_param_close_cb);

	library_param_fillin(ctx, l);
}


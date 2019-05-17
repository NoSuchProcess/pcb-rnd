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

static void close()
{
	library_ctx_t *ctx;
	free(ctx->help_params);
	htsi_uninit(&ctx->param_names);
}

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


static void library_param_build(library_ctx_t *ctx, pcb_fplibrary_t *l, FILE *f)
{
	char *sres, line[1024];
	char *descr = NULL;

	free(ctx->example);
	ctx->example = NULL;

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

#if 0
		/* parse */
		if (strcmp(cmd, "desc") == 0) {
			append_strdup(descr, arg, "\n");
		}
		else if (strcmp(cmd, "params") == 0) {
			free(ctx->help_params);
			ctx->help_params = pcb_strdup(arg);
		}
		else if (strcmp(cmd, "example") == 0) {
			ctx->example = pcb_strdup(arg);
		}
		else if (strncmp(cmd, "optional:", 9) == 0) {
			if (ctx.first_optional < 0)
				ctx.first_optional = ctx->num_params-1;
		}
		else if (strncmp(cmd, "param:", 6) == 0) {
			colsplit();
			curr = append(col, &ctx);
			curr->help_text = pcb_strdup(arg);
			if (cb != NULL)
				curr->change_cb = attr_change_cb;
		}
		else if (strncmp(cmd, "default:", 6) == 0) {
			char *nstr;
			nstr = pcb_strdup_printf("%s\n(default: %s)", curr->help_text, arg);
			free((char *)curr->help_text);
			curr->help_text = nstr;
		}
		else if (strncmp(cmd, "dim:", 4) == 0) {
			curr->type = PCB_HATT_COORD;
			curr->min_val = 0;
			curr->max_val = PCB_MM_TO_COORD(512);
		}
		else if (strncmp(cmd, "enum:", 5) == 0) {
			char *evl;
			curr->type = PCB_HATT_ENUM;
			colsplit(); colsplit();
			if (arg != NULL)
				evl = pcb_strdup_printf("%s (%s)", col, arg);
			else
				evl = pcb_strdup(col);
			append_enum(curr, evl);
		}
#endif
	}
	pcb_pclose(f);
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

	return dirty;
}

static void library_param_open(library_ctx_t *ctx, pcb_fplibrary_t *l, FILE *f)
{
	PCB_DAD_BEGIN_VBOX(library_ctx.pdlg);
		PCB_DAD_COMPFLAG(library_ctx.pdlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(library_ctx.pdlg, 2);
			PCB_DAD_COMPFLAG(library_ctx.pdlg, PCB_HATF_EXPFILL);
			library_param_build(ctx, l, f);
		PCB_DAD_END(library_ctx.pdlg);
	PCB_DAD_END(library_ctx.pdlg);
}

static void library_param_dialog(library_ctx_t *ctx, pcb_fplibrary_t *l)
{
	FILE *f;
	char *cmd;
	int n, dirty = 0;


	if (ctx->last_l != l) {
TODO("if active: close if new l differs\n");
	}

	cmd = pcb_strdup_printf("%s --help", l->data.fp.loc_info);
	f = pcb_popen(NULL, cmd, "r");
	free(cmd);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not execute parametric footprint %s\n", l->data.fp.loc_info);
		return;
	}

	ctx->last_l = l;

	htsi_init(&ctx->param_names, strhash, strkeyeq);
}


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

static void library_param_build(library_ctx_t *ctx, pcb_fplibrary_t *l, FILE *f)
{
	char *sres, line[1024];
	char *params = NULL, *descr = NULL, *example = NULL;

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
			params = pcb_strdup(arg);
		}
		else if (strcmp(cmd, "example") == 0) {
			example = pcb_strdup(arg);
		}
		else if (strncmp(cmd, "optional:", 9) == 0) {
			if (ctx.first_optional < 0)
				ctx.first_optional = numattr-1;
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

static void library_param_dialog(library_ctx_t *ctx, pcb_fplibrary_t *l, const char *filter_txt)
{
	FILE *f;
	char *cmd;
	int n, numattr = 0, dirty = 0;


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

#if 0
	if (filter_txt == NULL) {
		filter_txt = example;
		dirty = 1;
	}

	if (filter_txt != NULL) {
		char *prm = strchr(filter_txt, '(');

		/* if filter text doesn't have parameters, try the example */
		if ((prm == NULL) || (prm[1] == ')')) {
			if (example != NULL) {
				filter_txt = example;
				prm = strchr(filter_txt, '(');
				dirty = 1;
			}
		}

		if (prm != NULL)
			load_params(prm+1, params, attrs, numattr);
	}

	/* make a snapshot to res so that callback gen_cmd() works from live data */
	for(n = 0; n < numattr; n++)
		res[n] = attrs[n].default_val;

	if (dirty) /* had to replace the filter text, make it effective */
		attr_change_cb(NULL, NULL, &attrs[0]);

	if (pcb_attribute_dialog("lib_param", attrs, numattr, res, descr, NULL) == 0)
		sres = gen_cmd(entry->name, attrs, res, numattr, ctx.first_optional);
	else
		sres = NULL; /* cancel */

	/* clean up */
	for(n = 0; n < numattr; n++)
		free_attr(&attrs[n]);
	free(descr);
	free(params);
	free(example);

	return sres;
#endif
}


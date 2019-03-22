/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  cam export jobs plugin - compile/decompile jobs
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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


#define GVT_DONT_UNDEF
#include "cam_compile.h"
#include <genvector/genvector_impl.c>

/* mkdir -p on arg - changes the string in arg */
static int prefix_mkdir(char *arg, char **filename)
{
	char *curr, *next, *end;
	int res;

		/* mkdir -p if there's a path sep in the prefix */
		end = strrchr(arg, PCB_DIR_SEPARATOR_C);
		if (end == NULL) {
			if (filename != NULL)
				*filename = arg;
			return 0;
		}

		*end = '\0';
		res = end - arg;
		if (filename != NULL)
			*filename = end+1;

		for(curr = arg; curr != NULL; curr = next) {
			next = strrchr(curr, PCB_DIR_SEPARATOR_C);
			if (next != NULL)
				*next = '\0';
			pcb_mkdir(arg, 0755);
			if (next != NULL) {
				*next = PCB_DIR_SEPARATOR_C;
				next++;
			}
		}
	return res;
}

static int cam_exec_inst(cam_ctx_t *ctx, char *cmd, char *arg)
{
	char *curr, *next;
	char **argv = ctx->argv;

	if (*cmd == '#') {
		/* ignore comments */
	}
	else if (strcmp(cmd, "prefix") == 0) {
		free(ctx->prefix);
		ctx->prefix = pcb_strdup(arg);
		prefix_mkdir(arg, NULL);
	}
	else if (strcmp(cmd, "desc") == 0) {
		/* ignore */
	}
	else if (strcmp(cmd, "write") == 0) {
		int argc = ctx->argc;
		if (ctx->exporter == NULL) {
			pcb_message(PCB_MSG_ERROR, "cam: no exporter selected before write\n");
			return -1;
		}

		/* build the --cam args using the prefix */
		ctx->argv[0] = "--cam";
		gds_truncate(&ctx->tmp, 0);
		if (ctx->prefix != NULL)
			gds_append_str(&ctx->tmp, ctx->prefix);
		gds_append_str(&ctx->tmp, arg);
		ctx->argv[1] = ctx->tmp.array;

		if (ctx->exporter->parse_arguments(&argc, &argv) != 0) {
			pcb_message(PCB_MSG_ERROR, "cam: exporter '%s' refused the arguments\n", arg);
			return -1;
		}
		ctx->exporter->do_export(0);
		return 0;
	}
	else if (strcmp(cmd, "plugin") == 0) {
		curr = strpbrk(arg, " \t");
		if (curr != NULL) {
			*curr = '\0';
			curr++;
		}
		ctx->exporter = pcb_hid_find_exporter(arg);
		if (ctx->exporter == NULL) {
			pcb_message(PCB_MSG_ERROR, "cam: can not find export plugin: '%s'\n", arg);
			return -1;
		}
		free(ctx->args);
		curr = ctx->args = pcb_strdup(curr == NULL ? "" : curr);
		ctx->argc = 2; /* [0] and [1] are reserved for the --cam argument */
		for(; curr != NULL; curr = next) {
			if (ctx->argc >= (sizeof(ctx->argv) / sizeof(ctx->argv[0]))) {
				pcb_message(PCB_MSG_ERROR, "cam: too many arguments for plugin '%s'\n", arg);
				return -1;
			}
			while(isspace(*curr)) curr++;
			next = strpbrk(curr, " \t");
			if (next != NULL) {
				*next = '\0';
				next++;
			}
			if (*curr == '\0')
				continue;
			argv[ctx->argc] = curr;
			ctx->argc++;
			
		}
		argv[ctx->argc] = NULL;
	}
	else {
		pcb_message(PCB_MSG_ERROR, "cam: syntax error (unknown instruction): '%s'\n", cmd);
		return -1;
	}
	return 0;
}

/* Parse and execute a script */
static int cam_exec(cam_ctx_t *ctx, const char *script_in, int (*exec)(cam_ctx_t *ctxctx, char *cmd, char *arg))
{
	char *arg, *curr, *next, *script = pcb_strdup(script_in);
	int res = 0;
	void *old_vars, *tmp;

	old_vars = pcb_cam_vars_use(ctx->vars);
	for(curr = script; curr != NULL; curr = next) {
		while(isspace(*curr)) curr++;
		next = strpbrk(curr, ";\r\n");
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		if (*curr == '\0')
			continue;
		arg = strpbrk(curr, " \t");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
		}
		res |= exec(ctx, curr, arg);
	}

	tmp = pcb_cam_vars_use(old_vars);
	assert(tmp == ctx->vars); /* we must be restoring from our own context else the recursion is broken */

	free(script);
	return res;
}


/*** new ***/


static int cam_compile_line(cam_ctx_t *ctx, char *cmd, char *arg, pcb_cam_code_t *code)
{

	if (strcmp(cmd, "prefix") == 0) {
		code->inst = PCB_CAM_PREFIX;
		code->op.prefix.arg = pcb_strdup(arg);
	}
	else if (strcmp(cmd, "desc") == 0) {
		code->inst = PCB_CAM_DESC;
		code->op.desc.arg = pcb_strdup(arg);
	}
	else if (strcmp(cmd, "write") == 0) {
		code->inst = PCB_CAM_WRITE;
		code->op.write.arg = pcb_strdup(arg);
	}
	else if (strcmp(cmd, "plugin") == 0) {
		char *curr, *next;
		int maxa;
		char *s;
	
		code->inst = PCB_CAM_PLUGIN;
		curr = strpbrk(arg, " \t");
		if (curr != NULL) {
			*curr = '\0';
			curr++;
		}
		code->op.plugin.exporter = pcb_hid_find_exporter(arg);
		if (code->op.plugin.exporter == NULL) {
			pcb_message(PCB_MSG_ERROR, "cam: can not find export plugin: '%s'\n", arg);
			return -1;
		}
		free(ctx->args);

		curr = pcb_strdup(curr == NULL ? "" : curr);
		for(maxa = 0, s = curr; *s != '\0'; s++)
			if (isspace(*s))
				maxa++;

		code->op.plugin.argc = 2; /* [0] and [1] are reserved for the --cam argument */
		code->op.plugin.argv = malloc(sizeof(char *) * (maxa+1));
		if (code->op.plugin.argv == NULL)
			return 1;
		for(; curr != NULL; curr = next) {
			while(isspace(*curr)) curr++;
			next = strpbrk(curr, " \t");
			if (next != NULL) {
				*next = '\0';
				next++;
			}
			if (*curr == '\0')
				continue;
			code->op.plugin.argv[code->op.plugin.argc] = curr;
			code->op.plugin.argc++;
			
		}
		code->op.plugin.argv[ctx->argc] = NULL;
	}
	else {
		pcb_message(PCB_MSG_ERROR, "cam: syntax error (unknown instruction): '%s'\n", cmd);
		return -1;
	}
	return 0;
}

static int pcb_cam_compile(cam_ctx_t *ctx, const char *script_in)
{
	char *arg, *curr, *next, *script = pcb_strdup(script_in);
	int res = 0, r;

	for(curr = script; curr != NULL; curr = next) {
		pcb_cam_code_t code;

		while(isspace(*curr)) curr++;
		next = strpbrk(curr, ";\r\n");
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		if (*curr == '\0')
			continue;
		arg = strpbrk(curr, " \t");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
		}
		if ((*arg == '#') || (*arg == '\0'))
			continue;
		r = cam_compile_line(ctx, curr, arg, &code);
		if (r == 0) {
			TODO("append");
		}
		else
			res = 1;
	}

	free(script);
	return res;
}



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

#include "data.h"

#define GVT_DONT_UNDEF
#include "cam_compile.h"
#include "layer_vis.h"
#include "event.h"
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
			pcb_mkdir(&PCB->hidlib, arg, 0755);
			if (next != NULL) {
				*next = PCB_DIR_SEPARATOR_C;
				next++;
			}
		}
	return res;
}

static int cam_exec_inst(cam_ctx_t *ctx, pcb_cam_code_t *code)
{
	int argc;
	char **argv;

	switch(code->inst) {
		case PCB_CAM_DESC:
			/* ignore */
			break;

		case PCB_CAM_WRITE:
			if (ctx->exporter == NULL) {
				pcb_message(PCB_MSG_ERROR, "cam: no exporter selected before write\n");
				return -1;
			}

			/* build the --cam args using the prefix */
			ctx->argv[0] = "--cam";
			gds_truncate(&ctx->tmp, 0);
			if (ctx->prefix != NULL)
				gds_append_str(&ctx->tmp, ctx->prefix);
			gds_append_str(&ctx->tmp, code->op.write.arg);
			ctx->argv[1] = ctx->tmp.array;

			argc = ctx->argc;
			argv = ctx->argv;
			if (ctx->exporter->parse_arguments(ctx->exporter, &argc, &argv) != 0) {
				pcb_message(PCB_MSG_ERROR, "cam: exporter '%s' refused the arguments\n", code->op.write.arg);
				ctx->argv[0] = NULL;
				ctx->argv[1] = NULL;
				return -1;
			}

			{ /* call the exporter */
				void *old_vars, *tmp;
				old_vars = pcb_cam_vars_use(ctx->vars);
				ctx->exporter->do_export(ctx->exporter, 0);
				tmp = pcb_cam_vars_use(old_vars);
				assert(tmp == ctx->vars); /* we must be restoring from our own context else the recursion is broken */
			}

			ctx->argv[0] = NULL;
			ctx->argv[1] = NULL;
			break;

		case PCB_CAM_PLUGIN:
			ctx->exporter = code->op.plugin.exporter;
			ctx->argc = code->op.plugin.argc;
			ctx->argv = code->op.plugin.argv;
			break;

		default:
			assert(!"invalid cam code");
	}
	return 0;
}


static int cam_exec(cam_ctx_t *ctx)
{
	int res = 0, n, have_gui, currly = INDEXOFCURRENT;
	int save_l_ons[PCB_MAX_LAYER], save_g_ons[PCB_MAX_LAYERGRP];
	
	have_gui = (pcb_gui != NULL) && pcb_gui->gui;
	if (have_gui) {
		pcb_hid_save_and_show_layer_ons(save_l_ons);
		pcb_hid_save_and_show_layergrp_ons(save_g_ons);
	}

	for(n = 0; n < ctx->code.used; n++) {
		if (cam_exec_inst(ctx, &ctx->code.array[n]) != 0) {
			res = 1;
			break;
		}
	}

	if (have_gui) {
		pcb_hid_restore_layer_ons(save_l_ons);
		pcb_hid_restore_layergrp_ons(save_g_ons);
		pcb_layervis_change_group_vis(currly, 1, 1);
		pcb_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	return res;
}

static int cam_compile_line(cam_ctx_t *ctx, char *cmd, char *arg, pcb_cam_code_t *code)
{

	if (strcmp(cmd, "desc") == 0) {
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
		code->op.plugin.argv = malloc(sizeof(char *) * (maxa+3));
		code->op.plugin.argv[0] = NULL;
		code->op.plugin.argv[1] = NULL;
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
			code->op.plugin.argv[code->op.plugin.argc] = pcb_strdup(curr);
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

static int cam_compile(cam_ctx_t *ctx, const char *script_in)
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
		if ((*curr == '#') || (*curr == '\0'))
			continue;
		r = cam_compile_line(ctx, curr, arg, &code);
		if (r == 0)
			vtcc_append(&ctx->code, code);
		else
			res = 1;
	}

	free(script);
	return res;
}

static void cam_free_inst(cam_ctx_t *ctx, pcb_cam_code_t *code)
{
	int n;

	switch(code->inst) {
		case PCB_CAM_DESC:
			break;

		case PCB_CAM_WRITE:
			free(code->op.write.arg);
			break;

		case PCB_CAM_PLUGIN:
			for(n = 0; n < code->op.plugin.argc; n++)
				free(code->op.plugin.argv[n]);
			free(code->op.plugin.argv);
			break;
	}
}

static void cam_free_code(cam_ctx_t *ctx)
{
	int n;
	for(n = 0; n < ctx->code.used; n++)
		cam_free_inst(ctx, &ctx->code.array[n]);
	vtcc_uninit(&ctx->code);
}

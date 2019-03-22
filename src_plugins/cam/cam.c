/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  cam export jobs plugin
 *  pcb-rnd Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <ctype.h>
#include <genvector/gds_char.h>

#include "board.h"
#include "hid_cam.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "plugins.h"
#include "actions.h"
#include "cam_conf.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "../src_plugins/cam/conf_internal.c"

static const char *cam_cookie = "cam exporter";

const conf_cam_t conf_cam;
#define CAM_CONF_FN "cam.conf"

#include "cam_compile.c"

static void cam_init_inst(cam_ctx_t *ctx)
{
	memset(ctx, 0, sizeof(cam_ctx_t));

	ctx->vars = pcb_cam_vars_alloc();

	if (PCB->Filename != NULL) {
		char *val, *end = strrchr(PCB->Filename, PCB_DIR_SEPARATOR_C);
		if (end != NULL)
			val = pcb_strdup(end+1);
		else
			val = pcb_strdup(PCB->Filename);
		pcb_cam_set_var(ctx->vars, pcb_strdup("base"), val);
	}
}

static void cam_uninit_inst(cam_ctx_t *ctx)
{
	pcb_cam_vars_free(ctx->vars);
	cam_free_code(ctx);
	free(ctx->prefix);
	free(ctx->args);
	gds_uninit(&ctx->tmp);
}

/* look up a job by name in the config */
static const char *cam_find_job(const char *job)
{
	conf_listitem_t *j;
	int idx;

	conf_loop_list(&conf_cam.plugins.cam.jobs, j, idx)
		if (strcmp(j->name, job) == 0)
			return j->payload;

	return NULL;
}

/* execute the instructions of a job specified by name */
static int cam_call(const char *job, cam_ctx_t *ctx)
{
	const char *script = cam_find_job(job);

	if (script != NULL) {
		if (cam_compile(ctx, script) != 0)
			return -1;
		return cam_exec(ctx);
	}

	pcb_message(PCB_MSG_ERROR, "cam: can not find job configuration '%s'\n", job);
	return -1;
}

static int cam_parse_opt_outfile(cam_ctx_t *ctx, const char *optval)
{
	char *fn, *tmp = pcb_strdup(optval);
	int dirlen = prefix_mkdir(tmp, &fn);

	free(ctx->prefix);
	if (dirlen > 0) {
		ctx->prefix = malloc(dirlen+2);
		memcpy(ctx->prefix, optval, dirlen);
		ctx->prefix[dirlen] = PCB_DIR_SEPARATOR_C;
		ctx->prefix[dirlen+1] = '\0';
	}
	else
		ctx->prefix = NULL;
	pcb_cam_set_var(ctx->vars, pcb_strdup("base"), pcb_strdup(fn));
	free(tmp);
	return 0;
}

static int cam_parse_opt(cam_ctx_t *ctx, const char *opt)
{
	if (strncmp(opt, "outfile=", 8) == 0) {
		return cam_parse_opt_outfile(ctx, opt+8);
	}
	else {
		char *sep = strchr(opt, '=');
		if (sep != NULL) {
			char *key = pcb_strndup(opt, sep-opt);
			char *val = pcb_strdup(sep+1);
			pcb_cam_set_var(ctx->vars, key, val);
			return 0;
		}
	}

	return 1;
}

#include "cam_gui.c"

static const char pcb_acts_cam[] = "cam(exec, script, [options])\ncam(call, jobname, [options])\ncam([gui])";
static const char pcb_acth_cam[] = "Export jobs for feeding cam processes";
static fgw_error_t pcb_act_cam(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	cam_ctx_t ctx;
	const char *cmd = "gui", *arg = NULL, *opt;
	int n, rs = -1;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, cam, cmd = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, cam, arg = argv[2].val.str);

	cam_init_inst(&ctx);
	for(n = 3; n < argc; n++) {
		PCB_ACT_CONVARG(n, FGW_STR, cam, opt = argv[n].val.str);
		if (cam_parse_opt(&ctx, opt) != 0) {
			pcb_message(PCB_MSG_ERROR, "cam: invalid action option '%s'\n", opt);
			cam_uninit_inst(&ctx);
			PCB_ACT_IRES(1);
			return 0;
		}
	}

	if (pcb_strcasecmp(cmd, "gui") == 0) {
		rs = cam_gui(arg);
	}
	else {
		if (arg == NULL) {
			pcb_message(PCB_MSG_ERROR, "cam: need one more argument for '%s'\n", cmd);
			cam_uninit_inst(&ctx);
			PCB_ACT_IRES(1);
			return 0;
		}
		if (pcb_strcasecmp(cmd, "exec") == 0) {
			rs = cam_compile(&ctx, arg);
			if (rs == 0)
				rs = cam_exec(&ctx);
		}
		else if (pcb_strcasecmp(cmd, "call") == 0)
			rs = cam_call(arg, &ctx);
	}

	cam_uninit_inst(&ctx);
	PCB_ACT_IRES(rs);
	return 0;
}


static pcb_action_t cam_action_list[] = {
	{"cam", pcb_act_cam, pcb_acth_cam, pcb_acts_cam}
};

PCB_REGISTER_ACTIONS(cam_action_list, cam_cookie)

int pplg_check_ver_cam(int ver_needed) { return 0; }

void pplg_uninit_cam(void)
{
	conf_unreg_file(CAM_CONF_FN, cam_conf_internal);
	conf_unreg_fields("plugins/cam/");
	pcb_remove_actions_by_cookie(cam_cookie);
}

#include "dolists.h"
int pplg_init_cam(void)
{
	PCB_API_CHK_VER;
	conf_reg_file(CAM_CONF_FN, cam_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_cam, field,isarray,type_name,cpath,cname,desc,flags);
#include "cam_conf_fields.h"

	PCB_REGISTER_ACTIONS(cam_action_list, cam_cookie)

	return 0;
}

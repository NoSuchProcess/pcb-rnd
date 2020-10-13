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
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include "cam_conf.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include "../src_plugins/cam/conf_internal.c"

static const char *cam_cookie = "cam exporter";

conf_cam_t conf_cam;
#define CAM_CONF_FN "cam.conf"

#include "cam_compile.c"

static void cam_init_inst_fn(cam_ctx_t *ctx)
{
	if ((PCB != NULL) && (PCB->hidlib.filename != NULL)) {
		char *fn = pcb_derive_default_filename_(PCB->hidlib.filename, "");
		char *val, *end = strrchr(fn, RND_DIR_SEPARATOR_C);
		if (end != NULL)
			val = rnd_strdup(end+1);
		else
			val = rnd_strdup(fn);
		pcb_cam_set_var(ctx->vars, rnd_strdup("base"), val);
		free(fn);
	}
}

static void cam_init_inst(cam_ctx_t *ctx)
{
	memset(ctx, 0, sizeof(cam_ctx_t));

	ctx->vars = pcb_cam_vars_alloc();
	cam_init_inst_fn(ctx);
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
	rnd_conf_listitem_t *j;
	int idx;

	rnd_conf_loop_list(&conf_cam.plugins.cam.jobs, j, idx)
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

	rnd_message(RND_MSG_ERROR, "cam: can not find job configuration '%s'\n", job);
	return -1;
}

static int cam_parse_opt_outfile(cam_ctx_t *ctx, const char *optval, rnd_bool nomkdir)
{
	char *fn, *tmp = rnd_strdup(optval);
	int dirlen = prefix_mkdir(tmp, &fn, nomkdir);

	free(ctx->prefix);
	if (dirlen > 0) {
		ctx->prefix = malloc(dirlen+2);
		memcpy(ctx->prefix, optval, dirlen);
		ctx->prefix[dirlen] = RND_DIR_SEPARATOR_C;
		ctx->prefix[dirlen+1] = '\0';
	}
	else
		ctx->prefix = NULL;
	pcb_cam_set_var(ctx->vars, rnd_strdup("base"), rnd_strdup(fn));
	free(tmp);
	return 0;
}

static int cam_parse_set_var(cam_ctx_t *ctx, const char *opt)
{
	char *key, *val, *sep = strchr(opt, '=');

	if (sep == NULL)
		return 1;

	key = rnd_strndup(opt, sep-opt);
	val = rnd_strdup(sep+1);
	pcb_cam_set_var(ctx->vars, key, val);
	return 0;
}

static int cam_parse_opt(cam_ctx_t *ctx, const char *opt)
{
	if (strncmp(opt, "outfile=", 8) == 0)
		return cam_parse_opt_outfile(ctx, opt+8, 0);
	else
		return cam_parse_set_var(ctx, opt);

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

	RND_ACT_MAY_CONVARG(1, FGW_STR, cam, cmd = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, cam, arg = argv[2].val.str);

	cam_init_inst(&ctx);
	for(n = 3; n < argc; n++) {
		RND_ACT_CONVARG(n, FGW_STR, cam, opt = argv[n].val.str);
		if (cam_parse_opt(&ctx, opt) != 0) {
			rnd_message(RND_MSG_ERROR, "cam: invalid action option '%s'\n", opt);
			cam_uninit_inst(&ctx);
			RND_ACT_IRES(1);
			return 0;
		}
	}

	if (rnd_strcasecmp(cmd, "gui") == 0) {
		rs = cam_gui(RND_ACT_HIDLIB, arg);
	}
	else {
		if (arg == NULL) {
			rnd_message(RND_MSG_ERROR, "cam: need one more argument for '%s'\n", cmd);
			cam_uninit_inst(&ctx);
			RND_ACT_IRES(1);
			return 0;
		}
		if (rnd_strcasecmp(cmd, "exec") == 0) {
			rs = cam_compile(&ctx, arg);
			if (rs == 0)
				rs = cam_exec(&ctx);
		}
		else if (rnd_strcasecmp(cmd, "call") == 0)
			rs = cam_call(arg, &ctx);
	}

	cam_uninit_inst(&ctx);
	RND_ACT_IRES(rs);
	return 0;
}


static rnd_action_t cam_action_list[] = {
	{"cam", pcb_act_cam, pcb_acth_cam, pcb_acts_cam}
};

static rnd_export_opt_t *export_cam_get_export_options(rnd_hid_t *hid, int *n)
{
	return 0;
}

static int export_cam_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nThe cam exporter shorthand:\n\n");
	fprintf(stderr, "\nUsage: pcb-rnd -x cam jobname [cam-opts] [pcb-rnd-options] filename");
	fprintf(stderr, "\n\ncam-opts:");
	fprintf(stderr, "\n --outfile path      set the output file path (affets prefix and %%base%%)");
	fprintf(stderr, "\n -o key=value        set %%key%% to value");
	fprintf(stderr, "\n\n");
	return 0;
}

static char *cam_export_job;
static cam_ctx_t cam_export_ctx;
static int cam_export_has_outfile;
static int export_cam_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	int d, s, oargc;
	if (*argc < 1) {
		rnd_message(RND_MSG_ERROR, "-x cam needs a job name\n");
		return -1;
	}

	cam_export_has_outfile = 0;
	cam_init_inst(&cam_export_ctx);
	cam_export_job = rnd_strdup((*argv)[0]);
	oargc = (*argc);
	(*argc)--;
	for(d = 0, s = 1; s < oargc; s++) {
		char *opt = (*argv)[s];
		if (strcmp(opt, "--outfile") == 0) {
			s++; (*argc)--;
			cam_parse_opt_outfile(&cam_export_ctx, (*argv)[s], 0);
			cam_export_has_outfile = 1;
		}
		else if (strcmp(opt, "-o") == 0) {
			s++; (*argc)--;
			if (cam_parse_set_var(&cam_export_ctx, (*argv)[s]) != 0) {
				rnd_message(RND_MSG_ERROR, "cam -o requires a key=value argument\n");
				goto err;
			}
		}
		else { /* copy over for pcb */
			(*argv)[d] = opt;
			d++;
		}
	}
	return 0;

	err:;
	cam_uninit_inst(&cam_export_ctx);
	free(cam_export_job);
	cam_export_job = NULL;
	return 1;
}

static void export_cam_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	if (!cam_export_has_outfile)
		cam_init_inst_fn(&cam_export_ctx);

	if (cam_call(cam_export_job, &cam_export_ctx) != 0)
		rnd_message(RND_MSG_ERROR, "CAM job %s failed\n", cam_export_job);

	cam_uninit_inst(&cam_export_ctx);
	free(cam_export_job);
	cam_export_job = NULL;
}

rnd_hid_t export_cam_hid;

int pplg_check_ver_cam(int ver_needed) { return 0; }

void pplg_uninit_cam(void)
{
	rnd_conf_unreg_file(CAM_CONF_FN, cam_conf_internal);
	rnd_conf_unreg_fields("plugins/cam/");
	rnd_remove_actions_by_cookie(cam_cookie);
	rnd_export_remove_opts_by_cookie(cam_cookie);
	rnd_hid_remove_hid(&export_cam_hid);
}

int pplg_init_cam(void)
{
	RND_API_CHK_VER;
	rnd_conf_reg_file(CAM_CONF_FN, cam_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_cam, field,isarray,type_name,cpath,cname,desc,flags);
#include "cam_conf_fields.h"

	RND_REGISTER_ACTIONS(cam_action_list, cam_cookie)

	memset(&export_cam_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&export_cam_hid);

	export_cam_hid.struct_size = sizeof(rnd_hid_t);
	export_cam_hid.name = "cam";
	export_cam_hid.description = "Shorthand for exporting by can job name";
	export_cam_hid.exporter = 1;
	export_cam_hid.hide_from_gui = 1;

	export_cam_hid.get_export_options = export_cam_get_export_options;
	export_cam_hid.do_export = export_cam_do_export;
	export_cam_hid.parse_arguments = export_cam_parse_arguments;

	export_cam_hid.usage = export_cam_usage;

	rnd_hid_register_hid(&export_cam_hid);
	return 0;
}

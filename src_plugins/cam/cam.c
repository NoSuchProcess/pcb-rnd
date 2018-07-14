/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  cam export jobs plugin
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>

#include "hid_cam.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "plugins.h"
#include "actions.h"
#include "cam_conf.h"
#include "compat_misc.h"
#include "../src_plugins/cam/conf_internal.c"


static const char *cam_cookie = "cam exporter";

conf_cam_t conf_cam;
#define CAM_CONF_FN "cam.conf"

static int cam_exec_inst(char *cmd, char *arg)
{
	printf("inst: '%s' '%s'\n", cmd, arg);
	return 0;
}

/* Parse and execute a script */
static int cam_exec(const char *script_in, int (*exec)(char *cmd, char *arg))
{
	char *arg, *curr, *next, *script = pcb_strdup(script_in);
	int res = 0;

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
		res |= exec(curr, arg);
	}

	free(script);
	return res;
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
static int cam_call(const char *job)
{
	const char *script = cam_find_job(job);
	
	if (script != NULL)
		return cam_exec(script, cam_exec_inst);

	pcb_message(PCB_MSG_ERROR, "cam: can not find job configuration '%s'\n", job);
	return -1;
}


static const char pcb_acts_cam[] = "cam(exec, script)\ncam(call, jobname)";
static const char pcb_acth_cam[] = "Export jobs for feeding cam processes";
static fgw_error_t pcb_act_cam(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *arg;
	int rs = -1;

	PCB_ACT_CONVARG(1, FGW_STR, cam, cmd = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, cam, arg = argv[2].val.str);

	if (pcb_strcasecmp(cmd, "exec") == 0)
		rs = cam_exec(arg, cam_exec_inst);
	else if (pcb_strcasecmp(cmd, "call") == 0)
		rs = cam_call(arg);

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

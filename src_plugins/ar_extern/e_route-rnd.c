/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with external router process
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

TODO("this should be in config")
static const char *exe = "route-rnd";

static int rtrnd_route(pcb_board_t *pcb, ext_route_scope_t scope, const char *method, int argc, fgw_arg_t *argv)
{
	const char *route_req = "rtrnd.1.tdx", *route_res = "rtrnd.2.tdx";
	rnd_hidlib_t *hl = &pcb->hidlib;
	char *cmd;
	int r;

	r = rnd_actionva(hl, "SaveTedax", "route_req", route_req, NULL);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to export route request in tEDAx\n");
		return 1;
	}

	if (method != NULL)
		cmd = rnd_strdup_printf("%s '%s' -m '%s' -o '%s'", exe, route_req, method, route_res);
	else
		cmd = rnd_strdup_printf("%s '%s' -o '%s'", exe, route_req, route_res);
	r = rnd_system(hl, cmd);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to execute the router: '%s'\n", cmd);
		free(cmd);
		return 1;
	}
	free(cmd);

	r = rnd_actionva(hl, "LoadTedaxFrom", "route_res", route_res, NULL);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to import the route result from tEDAx\n");
		return 1;
	}

	return 0;
}

static int rtrnd_list_methods(rnd_hidlib_t *hl, vts0_t *dst)
{
	FILE *f;
	char *cmd, line[1024];

	cmd = rnd_strdup_printf("%s -M", exe);
	f = rnd_popen(hl, cmd, "r");
	free(cmd);
	if (f == NULL)
		return -1;

	while(fgets(line, sizeof(line), f) != NULL) {
		char *name, *desc;
		name = line;
		while(isspace(*name)) name++;
		if (*name == '\0') continue;
		desc = strchr(name, '\t');
		if (desc != NULL) {
			*desc = '\0';
			desc++;
		}
		else
			desc = "n/a";
		vts0_append(dst, rnd_strdup(name));
		vts0_append(dst, rnd_strdup(desc));
	}

	fclose(f);
	return 0;
}


static rnd_hid_attribute_t *rtrnd_list_conf(rnd_hidlib_t *hl, const char *method)
{
	char *cmd;
	const char *route_lst = "rtrnd.l.tdx";
	int r;
	fgw_error_t err;
	fgw_arg_t res;
	fgw_arg_t argv[3];

	cmd = rnd_strdup_printf("%s -l -m '%s' > '%s'", exe, method, route_lst);
	r = rnd_system(hl, cmd);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to execute the router: '%s'\n", cmd);
		free(cmd);
		return NULL;
	}
	free(cmd);

	argv[1].type = FGW_STR; argv[1].val.cstr = "route_conf_keys";
	argv[2].type = FGW_STR; argv[2].val.cstr = route_lst;
	err = rnd_actionv_bin(hl, "LoadTedaxFrom", &res, 3, argv);
	if ((err != 0) || !(res.type & FGW_PTR)) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to import the conf key list from tEDAx\n");
		return NULL;
	}

	return res.val.ptr_void;
}

static const ext_router_t route_rnd = {
	"route-rnd",
	rtrnd_route,
	rtrnd_list_methods,
	rtrnd_list_conf
};

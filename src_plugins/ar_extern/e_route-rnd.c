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

static int rtrnd_route(pcb_board_t *pcb, ext_route_scope_t scope, const char *method, int argc, fgw_arg_t *argv)
{
	const char *exe = "route-rnd", *route_req = "rtrnd.1.tdx", *route_res = "rtrnd.2.tdx";
	rnd_hidlib_t *hl = &pcb->hidlib;
	char *cmd;
	int r;

	r = rnd_actionva(hl, "SaveTedax", "route_req", route_req);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to export route request in tEDAx\n");
		return 1;
	}

	cmd = rnd_strdup_printf("%s '%s' -o '%s'", exe, route_req, route_res);
	r = rnd_system(hl, cmd);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to execute the router: '%s'\n", cmd);
		free(cmd);
		return 1;
	}
	free(cmd);

	r = rnd_actionva(hl, "LoadTedaxFrom", "route_res", route_res);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "route-rnd: failed to import the route result from tEDAx\n");
		return 1;
	}

	return 0;
}

static const ext_router_t route_rnd = {
	"route-rnd",
	rtrnd_route
};

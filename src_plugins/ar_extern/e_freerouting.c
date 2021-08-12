/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with external router process
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

#include <librnd/core/compat_fs.h>

static int freert_route(pcb_board_t *pcb, ext_route_scope_t scope, const char *method, int argc, fgw_arg_t *argv)
{
	char *route_req, *route_res, *end, line[1024], *s;
	rnd_hidlib_t *hl = &pcb->hidlib;
	char *cmd;
	int n, r, sargc, rv = 1, mp = 12, debug;
	fgw_arg_t sres = {0}, *sargv;
	const char *exe, *installation, *opts;
	FILE *f;

	sargc = argc + 3;
	sargv = calloc(sizeof(fgw_arg_t), sargc);
	sargv[1].type = FGW_STR; sargv[1].val.cstr = "route_req";
	sargv[2].type = FGW_STR; sargv[2].val.cstr = route_req;

	/* copy the rest of the conf arguments */
	for(n = 0; n < argc; n++) {
		sargv[n+3] = argv[n];
		sargv[n+3].type &= ~FGW_DYN;
	}

	if (strcmp(method, "freerouting.cli") == 0) {
		exe = conf_ar_extern.plugins.ar_extern.freerouting_cli.exe;
		installation = conf_ar_extern.plugins.ar_extern.freerouting_cli.installation;
		debug = conf_ar_extern.plugins.ar_extern.freerouting_cli.debug;
		opts = "-cli";
	}
	else if (strcmp(method, "freerouting.net") == 0) {
		exe = conf_ar_extern.plugins.ar_extern.freerouting_net.exe;
		installation = conf_ar_extern.plugins.ar_extern.freerouting_net.installation;
		debug = conf_ar_extern.plugins.ar_extern.freerouting_net.debug;
		opts = "";
	}

	/* generate temp file names */
	route_req = rnd_tempfile_name_new("freert.dsn");
	if (route_req == NULL) {
		rnd_message(RND_MSG_ERROR, "freerouting: can't create temporary file name\n");
		return -1;
	}
	route_res = rnd_strdup(route_req);
	if (route_res == NULL) {
		rnd_message(RND_MSG_ERROR, "freerouting: can't create temporary file name (out of memory)\n");
		return -1;
	}
	end = route_res + strlen(route_res);
	end -= 3;
	strcpy(end, "ses");

	/* export */
	r = rnd_actionva(hl, "export", "dsn", "--dsnfile", route_req, NULL);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "freerouting: failed to export the routing request in DSN;\nis the io_dsn plugin available?\n");
		goto exit;
	}


	/* run the router */
	if ((installation != NULL) && (*installation != '\0'))
		cmd = rnd_strdup_printf("cd \"%s\"; %s %s -de '%s' -do '%s' -mp %d", installation, exe, opts, route_req, route_res, mp);
	else
		cmd = rnd_strdup_printf("%s %s -de '%s' -do '%s' -mp %d", exe, opts, route_req, route_res, mp);

	f = rnd_popen(hl, cmd, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "freerouting: failed to execute the router: '%s'\n", cmd);
		free(cmd);
		goto exit;
	}
	free(cmd);

	while((s = fgets(line, sizeof(line), f)) != NULL) {
		printf("|Fr| %s\n", s);
	}

	/* read and apply the result of the routing */
	r = rnd_actionva(hl, "ImportSes", route_res, NULL);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "freerouting: failed to import the route result from tEDAx\n");
		goto exit;
	}

	rv = 0; /* success! */

	exit:;
	if (!debug) {
		rnd_unlink(hl, route_res);
		rnd_tempfile_unlink(route_req);
	}
	else {
		rnd_message(RND_MSG_INFO, "freerouting: debug: session files are left behind as %s and %s\n", route_req, route_res);
		free(route_req);
	}

	free(route_res);
	return rv;
}

static int freert_list_methods(rnd_hidlib_t *hl, vts0_t *dst)
{
	vts0_append(dst, rnd_strdup("freerouting.cli"));
	vts0_append(dst, rnd_strdup("Erich's minimzed CLI-only fork"));
	vts0_append(dst, rnd_strdup("freerouting.net"));
	vts0_append(dst, rnd_strdup("The original variant with GUI support"));

	return 0;
}


static rnd_export_opt_t *freert_list_conf(rnd_hidlib_t *hl, const char *method)
{
	rnd_export_opt_t *rv = calloc(sizeof(rnd_export_opt_t), 1+1);

	rv[0].name = rnd_strdup("postroute optimization");
	rv[0].help_text = rnd_strdup("Maximum number of postroute optimization steps");
	rv[0].type = RND_HATT_INTEGER;
	rv[0].default_val.lng = 12;

	return rv;
}

static const ext_router_t freerouting = {
	"freerouting",
	freert_route,
	freert_list_methods,
	freert_list_conf
};

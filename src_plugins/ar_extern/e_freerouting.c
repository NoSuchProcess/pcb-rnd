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

#define PROG_STAGES 5.0

static void freert_track_progress(pcb_board_t *pcb, FILE *f, int debug)
{
	char line[1024], *s, *end;
	double stage = 0, prg, p1 = 0, p2 = 0;
	long curr, max;
	int type;

	while((s = fgets(line, sizeof(line), f)) != NULL) {
		if (debug)
			rnd_message(RND_MSG_DEBUG, "freerouting: %s", s);
		if (strncmp(s, "--FRCLI--", 9) != 0)
			continue;
		s += 9;

		/* we could process non-progress reports here */

		if (strncmp(s, "PROGRESS--", 10) != 0)
			continue;
		s += 10;

		type = 0;
		if (strncmp(s, "fanout_board", 12) == 0) {
			stage = 1;
			s += 12;
			type = 1;
		}
		else if (strncmp(s, "autoroute", 9) == 0) {
			if (stage == 1)
				stage = 2;
			type = 2;
			s += 9;
		}
		else if (strncmp(s, "post_route", 10) == 0) {
			if (stage < 3)
				p2 = 0;
			stage = 3;
			s += 10;
			type = 3;
		}
		else
			continue;

		while((*s == ':') || isspace(*s)) s++;
		if (!isdigit(*s))
			continue;

		curr = strtol(s, &end, 10);
		if (*end != '/')
			continue;
		s = end+1;
		max = strtol(s, &end, 10);
		if ((max > 0) && (curr > 0) && (curr <= max))
			prg = (double)curr/(double)max;
		else
			prg = 0;

		switch(type) {
			case 1: /* fanout */
				p1 = prg;
				p2 = 0;
				break;
			case 2: /* autoroute */
				if (stage == 2) { /* main autorouting */
					p1 = prg;
					p2 = 0;
				}
				else /* postprocess routing */
					p2 = prg;
				break;
			case 3: /* postproc */
				p1 = prg;
				break;
		}

		if (pcb_ar_extern_progress(stage/PROG_STAGES, p1, p2) != 0)
			break; /* cancel */
	}

}

static int freert_route(pcb_board_t *pcb, ext_route_scope_t scope, const char *method, int argc, fgw_arg_t *argv)
{
	char *route_req, *route_res, *end;
	rnd_hidlib_t *hl = &pcb->hidlib;
	char *cmd;
	int n, r, rv = 1, mp = 12, debug;
	const char *exe, *installation, *opts;
	FILE *f;

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

	freert_track_progress(pcb, f, debug);

	/* read and apply the result of the routing */
	r = rnd_actionva(hl, "ImportSes", route_res, NULL);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "freerouting: failed to import the route result from tEDAx\n");
		goto exit;
	}

	rv = 0; /* success! */

	exit:;
	pcb_ar_extern_progress(1, 1, 1);
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

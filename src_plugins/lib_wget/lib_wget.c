/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "pcb-printf.h"
#include "plugins.h"
#include "safe_fs.h"
#include "lib_wget.h"

const char *wget_cmd = "wget -U 'pcb-rnd-fp_wget'";

static char *pcb_wget_command(const char *url, const char *ofn, int update, const pcb_wget_opts_t *opts)
{
	gds_t tmp;

	gds_init(&tmp);
	pcb_append_printf(&tmp, "%s -O '%s'", wget_cmd, ofn);
	if (update)
		gds_append_str(&tmp, " -c");

	if (opts != NULL) {
		const char **h;
		for(h = opts->header; *h != NULL; h++) {
			gds_append_str(&tmp, " --header '");
			gds_append_str(&tmp, *h);
			gds_append(&tmp, '\'');
		}
		if (opts->post_file != NULL) {
			gds_append_str(&tmp, " '--post-file=");
			gds_append_str(&tmp, opts->post_file);
			gds_append(&tmp, '\'');
		}

	}

	gds_append(&tmp, ' ');
	gds_append(&tmp, '\'');
	gds_append_str(&tmp, url);
	gds_append(&tmp, '\'');
	return tmp.array;
}

int pcb_wget_disk(const char *url, const char *ofn, int update, const pcb_wget_opts_t *opts)
{
	int res;
	char *cmd = pcb_wget_command(url, ofn, update, opts);

	res = pcb_system(NULL, cmd);
	free(cmd);
	return res;
}

FILE *pcb_wget_popen(const char *url, int update, const pcb_wget_opts_t *opts)
{
	FILE *f;
	char *cmd = pcb_wget_command(url, "-", update, opts);

	f = pcb_popen(NULL, cmd, "rb");
	free(cmd);
	return f;
}



int pplg_check_ver_lib_wget(int ver_needed) { return 0; }

void pplg_uninit_lib_wget(void)
{
}

int pplg_init_lib_wget(void)
{
	PCB_API_CHK_VER;
	return 0;
}

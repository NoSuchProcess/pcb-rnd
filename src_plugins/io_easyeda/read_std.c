/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - dispatch std format read
 *  pcb-rnd Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust Fund in 2024)
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
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rotate.h>
#include "../lib_compat_help/pstk_compat.h"
#include "../lib_compat_help/pstk_help.h"
#include "rnd_inclib/lib_svgpath/svgpath.h"

#include "board.h"
#include "data.h"
#include "netlist.h"
#include "obj_subc_parent.h"
#include "plug_io.h"

TODO("pro: move this to somewhere more central")
#include "rnd_inclib/lib_svgpath/svgpath.c"

#include "io_easyeda_conf.h"
#include "read_std_low.c"
#include "read_std_hi.c"


/* doctype is an integer that identifies what the json is about */
static int easystd_get_doctype(char *stray, FILE *f, char *buf, long buf_len)
{
	char *line = stray, *end;
	int n, res;

	/* don't re-read this line */
	*buf = '\0';

	for(n = 0; n < 32; n++) {
		while(isspace(*line)) line++;
		if (*line == '\0') goto next;
		if (*line == ':') line++;
		while(isspace(*line)) line++;
		if (*line == '\0') goto next;
		if (*line == '"') line++;
		res = strtol(line, &end, 10);
		while(isspace(*end)) end++;
		if (*end != '"')
			return -1;
		return res;

		next:;
		line = fgets(buf, sizeof(buf), f);
		if (line == NULL)
			return -1;
	}

	return -1;
}

int io_easyeda_std_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t type, const char *Filename, FILE *f)
{
	char buf[1024], *line;
	int n, doctype;
	unsigned found = 0;

	/* ensure all vital header fields are present in the first 32 lines */
	for(n = 0; n < 32; n++) {
		line = fgets(buf, sizeof(buf), f);
		if (line == NULL)
			return 0;

		got_line:;
		while(isspace(*line)) line++;
		if (*line != '"') continue;
		line++;

		if (strncmp(line, "editorVersion\"", 14) == 0)
			found |= 1; /* generic easyeda */

		if (strncmp(line, "docType\"", 8) == 0) {
			found |= 2;        /* generic easyeda */
			doctype = easystd_get_doctype(line + 8, f, line, sizeof(line));
			if ((doctype == 3) && (type == PCB_IOT_PCB))
				found |= 4; /* load board as board */
			if ((doctype == 4) && ((type == PCB_IOT_PCB) || (type == PCB_IOT_FOOTPRINT)))
				found |= 4; /* load footprint as board or footprint */

			if (*line != '\0')
				goto got_line;
		}

		if (found == (1|2|4))
			return 1; /* accept */
	}

	return 0;
}

int io_easyeda_std_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest)
{
	return easyeda_std_parse_board(Ptr, Filename, settings_dest);
}

int io_easyeda_std_parse_footprint(pcb_plug_io_t *ctx, pcb_data_t *data, const char *filename, const char *subfpname)
{
	return easyeda_std_parse_fp(data, filename);
}

pcb_plug_fp_map_t *io_easyeda_std_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags)
{
	if (io_easyeda_std_test_parse(ctx, PCB_IOT_FOOTPRINT, fn, f) != 1)
		return NULL;

	head->type = PCB_FP_FILE;
	head->name = rnd_strdup("first");
	return head;
}

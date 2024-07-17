/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - dispatch pro format read
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

#include "io_easyeda_conf.h"
#include "easyeda_sphash.h"
#include "read_common.h"

#include <rnd_inclib/lib_easyeda/easyeda_low.c>
#include "read_pro_low.c"
#include "read_pro_hi.c"


/* assume plain text, search for ["DOCTYPE","FOOTPRINT" in the first few lines */
int io_easyeda_pro_test_parse_efoo(pcb_plug_io_t *ctx, pcb_plug_iot_t type, const char *Filename, FILE *f)
{
	int lineno;

	for(lineno = 0; lineno < 8; lineno++) {
		char buff[1024], *line;
		unsigned char *ul;

		if ((line = fgets(buff, sizeof(buff), f)) == NULL)
			return 0; /* refuse */

		/* skip utf-8 bom */
		if (lineno == 0) {
			ul = (unsigned char *)line;
			if ((ul[0] == 0xef) && (ul[1] == 0xbb) && (ul[2] == 0xbf))
				line += 3;
		}

		while(isspace(*line)) line++;
		if (strncmp(line, "[\"DOCTYPE\",\"FOOTPRINT\"", 22) == 0)
			return 1; /* accept */
	}

	return 0;
}


int io_easyeda_pro_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t type, const char *Filename, FILE *f)
{
	if (((type == PCB_IOT_FOOTPRINT) || (type == PCB_IOT_PCB)) && (io_easyeda_pro_test_parse_efoo(ctx, type, Filename, f) == 1))
		return 1;
	else
		rewind(f);

	/* return 1 for accept */
	return 0;
}

int io_easyeda_pro_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, rnd_conf_role_t settings_dest)
{
	FILE *f;
	int is_fp;

	f = rnd_fopen(&pcb->hidlib, Filename, "r");
	if (f == NULL)
		return -1;

	is_fp = (io_easyeda_pro_test_parse_efoo(ctx, PCB_IOT_PCB, Filename, f) == 1);
	if (is_fp) {
		int res;
		rewind(f);
		res = easyeda_pro_parse_fp_as_board(pcb, Filename, f, settings_dest);
		fclose(f);
		return res;
	}
	else
		fclose(f);


	return /*easyeda_pro_parse_board(pcb, Filename, settings_dest);*/ -1;
}

int io_easyeda_pro_parse_footprint(pcb_plug_io_t *ctx, pcb_data_t *data, const char *filename, const char *subfpname)
{
	return /*easyeda_pro_parse_fp(data, filename);*/ -1;
}

pcb_plug_fp_map_t *io_easyeda_pro_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags)
{
/*
	if (io_easyeda_pro_test_parse(ctx, PCB_IOT_FOOTPRINT, fn, f) != 1)
		return NULL;

	head->type = PCB_FP_FILE;
	head->name = rnd_strdup("first");
	return head;*/
	return NULL;
}

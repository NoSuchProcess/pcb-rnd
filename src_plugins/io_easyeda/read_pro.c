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

/*#include "read_pro_low.c"*/
/*#include "read_pro_hi.c"*/



int io_easyeda_pro_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t type, const char *Filename, FILE *f)
{

	/* return 1 for accept */
	return 0;
}

int io_easyeda_pro_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest)
{
	return /*easyeda_pro_parse_board(Ptr, Filename, settings_dest);*/ -1;
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

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  PADS IO plugin - read: decode, parse, interpret
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
#include <assert.h>

#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include "board.h"

#include "read.h"

typedef struct pads_read_ctx_s {
	pcb_board_t *pcb;
	FILE *f;
	double coord_scale; /* multiply input integer coord values to get pcb-rnd nanometer */
	unsigned in_error:1;
} pads_read_ctx_t;


int io_pads_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char tmp[256];
	if (fgets(tmp, sizeof(tmp), f) == NULL)
		return 0;
	return (strncmp(tmp, "!PADS-POWERPCB", 14) == 0);
}

int io_pads_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	char *s, tmp[256];
	rnd_hidlib_t *hl = &PCB->hidlib;
	FILE *f;
	int ret = 0;
	pads_read_ctx_t rctx;

	f = rnd_fopen(hl, filename, "r");
	if (f == NULL)
		return -1;

	rctx.pcb = pcb;
	rctx.f = f;

	/* read the header */
	if (fgets(tmp, sizeof(tmp), f) == NULL)
		return 0;
	s = tmp+15;
	s = strchr(s, '-');
	if (s == NULL) {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (dash)\n");
		goto error1;
	}
	s++;
	if (strncmp(s, "BASIC", 5) == 0)
		rctx.coord_scale = 2.0/3.0;
	else if (strncmp(s, "MILS", 4) == 0)
		rctx.coord_scale = RND_MIL_TO_COORD(1);
	else if (strncmp(s, "METRIC", 6) == 0)
		rctx.coord_scale = RND_MM_TO_COORD(1.0/10000.0);
	else if (strncmp(s, "INCHES", 6) == 0)
		rctx.coord_scale = RND_INCH_TO_COORD(1.0/100000.0);
	else {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (unknown unit '%s')\n", s);
		goto error1;
	}


	return ret;

	error1:;
	fclose(f);
	return -1;
}



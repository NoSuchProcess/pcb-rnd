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
#include "pads_lex.h"
#include "pads_gram.h"


int io_pads_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char tmp[256];
	if (fgets(tmp, sizeof(tmp), f) == NULL)
		return 0;
	return (strncmp(tmp, "!PADS-POWERPCB", 14) == 0);
}

void pcb_pads_error(pcb_pads_ctx_t *ctx, pcb_pads_STYPE tok, const char *s) { }

int io_pads_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	char *s, tmp[256];
	rnd_hidlib_t *hl = &PCB->hidlib;
	FILE *f;
	int chr, ret = 0;
	pcb_pads_ureglex_t lctx;
	pcb_pads_yyctx_t yyctx;
	pcb_pads_ctx_t pctx = {0};
	pcb_pads_STYPE lval;


	f = rnd_fopen(hl, filename, "r");
	if (f == NULL)
		return -1;

	pctx.pcb = pcb;

	/* read the header */
	if (fgets(tmp, sizeof(tmp), f) == NULL)
		return 0;
	s = tmp+15;
	s = strchr(s, '-');
	if (s == NULL) {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (dash)\n");
		fclose(f);
		return -1;
	}
	s++;
	if (strncmp(s, "BASIC", 5) == 0)
		pctx.coord_scale = 2.0/3.0;
	else if (strncmp(s, "MILS", 4) == 0)
		pctx.coord_scale = RND_MIL_TO_COORD(1);
	else if (strncmp(s, "METRIC", 6) == 0)
		pctx.coord_scale = RND_MM_TO_COORD(1.0/10000.0);
	else if (strncmp(s, "INCHES", 6) == 0)
		pctx.coord_scale = RND_INCH_TO_COORD(1.0/100000.0);
	else {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (unknown unit '%s')\n", s);
		fclose(f);
		return -1;
	}

	pcb_pads_lex_init(&lctx, pcb_pads_rules);
	pcb_pads_parse_init(&yyctx);
	lctx.loc_line[0] = lctx.loc_line[1] = 2; /* compensate for the manually read header */

	/* read all bytes of the binary file */
	while((ret == 0) && ((chr = fgetc(f)) != EOF)) {
		int yres, tok = pcb_pads_lex_char(&lctx, &lval, chr);
		if (tok == UREGLEX_MORE)
			continue;

		/* feed the grammar */
		lval.line = lctx.loc_line[0];
		lval.first_col = lctx.loc_col[0];
		yres = pcb_pads_parse(&yyctx, &pctx, tok, &lval);

		if ((pctx.in_error) && ((tok == T_ID) || (tok == T_QSTR)))
			free(lval.un.s);

		if (yres != 0) {
			fprintf(stderr, "PADS syntax error at %ld:%ld\n", lval.line, lval.first_col);
			ret = -1;
		}
		pcb_pads_lex_reset(&lctx); /* prepare for the next token */
	}

	if (ret != 0) {
TODO("clean up");
	}

	return ret;
}



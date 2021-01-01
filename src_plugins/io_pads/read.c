/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  PADS IO plugin - read: decode, parse, interpret
 *  pcb-rnd Copyright (C) 2020,2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020 and 2021)
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
#include <math.h>

#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include "board.h"

#include "delay_create.h"
#include "read.h"

/* Parser return value convention:
	1      = succesfully read a block or *section* or line
	0      = eof
	-1..-3 = error
	-4     = bumped into the next *section*
*/

typedef struct pads_read_ctx_s {
	pcb_board_t *pcb;
	FILE *f;
	double coord_scale; /* multiply input integer coord values to get pcb-rnd nanometer */
	double ver;
	pcb_dlcr_t dlcr;
	pcb_dlcr_layer_t *layer;

	/* location */
	const char *fn;
	long line, col, start_line, start_col;

} pads_read_ctx_t;

#define PADS_ERROR(args) \
do { \
	rnd_message(RND_MSG_ERROR, "io_pads read: syntax error at %s:%ld.%ld: ", rctx->fn, rctx->line, rctx->col); \
	rnd_message args; \
} while(0)

static void pads_update_loc(pads_read_ctx_t *rctx, int c)
{
	if (c == '\n') {
		rctx->line++;
		rctx->col = 1;
	}
	else
		rctx->col++;
}

static void pads_start_loc(pads_read_ctx_t *rctx)
{
	rctx->start_line = rctx->line;
	rctx->start_col = rctx->col;
}


#include "read_low.c"
#include "read_high.c"
#include "read_high_misc.c"

static int pads_parse_header(pads_read_ctx_t *rctx)
{
	char *s, *end, tmp[256];

	if (fgets(tmp, sizeof(tmp), rctx->f) == NULL) {
		rnd_message(RND_MSG_ERROR, "io_pads: missing header\n");
		return -1;
	}
	s = tmp+15;
	if ((*s == 'V') || (*s == 'v'))
		s++;
	rctx->ver = strtod(s, &end);
	if (*end != '-') {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (version: invalid numeric)\n");
		return -1;
	}
	s = end+1;
	if (strncmp(s, "BASIC", 5) == 0)
		rctx->coord_scale = 2.0/3.0;
	else if (strncmp(s, "MILS", 4) == 0)
		rctx->coord_scale = RND_MIL_TO_COORD(1);
	else if (strncmp(s, "METRIC", 6) == 0)
		rctx->coord_scale = RND_MM_TO_COORD(1.0/10000.0);
	else if (strncmp(s, "INCHES", 6) == 0)
		rctx->coord_scale = RND_INCH_TO_COORD(1.0/100000.0);
	else {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (unknown unit '%s')\n", s);
		return -1;
	}
	return 0;
}

static int pads_parse_block(pads_read_ctx_t *rctx)
{
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 1);
/*rnd_trace("WORD='%s'/%d\n", word, res);*/
		if (res <= 0)
			return res;

		if (*word == '\0') res = 1; /* ignore empty lines between blocks */
		else if (strcmp(word, "*PCB*") == 0) res = pads_parse_pcb(rctx);
		else if (strcmp(word, "*REUSE*") == 0) { TODO("What to do with this?"); res = pads_parse_ignore_sect(rctx); }
		else if (strcmp(word, "*TEXT*") == 0) res = pads_parse_texts(rctx);
		else if (strcmp(word, "*LINES*") == 0) res = pads_parse_lines(rctx);
		else if (strcmp(word, "*VIA*") == 0) res = pads_parse_vias(rctx);
		else if (strcmp(word, "*PARTDECAL*") == 0) res = pads_parse_partdecals(rctx);
		else if (strcmp(word, "*PARTTYPE*") == 0) res = pads_parse_parttypes(rctx);
		else if (strcmp(word, "*PARTT") == 0) res = pads_parse_parttypes(rctx);
		else if (strcmp(word, "*PART*") == 0) res = pads_parse_parts(rctx);
		else if (strcmp(word, "*SIGNAL*") == 0) res = pads_parse_signal(rctx);
		else if (strcmp(word, "*POUR*") == 0) res = pads_parse_pours(rctx);
		else if (strcmp(word, "*MISC*") == 0) res = pads_parse_misc(rctx);
		else if (strcmp(word, "*CLUSTER*") == 0) res = pads_parse_ignore_sect(rctx);
		else if (strcmp(word, "*ROUTE*") == 0) res = pads_parse_ignore_sect(rctx);
		else if (strcmp(word, "*TESTPOINT*") == 0) res = pads_parse_ignore_sect(rctx);
		else if (strcmp(word, "*END*") == 0) {
			rnd_trace("*END* - legit end of file\n");
			return 1;
		}
		else {
			PADS_ERROR((RND_MSG_ERROR, "unknown block: '%s'\n", word));
			return -1;
		}

		if (res == -4) continue; /* bumped into the next section, just go on reading */

		/* exit the loop on error */
		if (res <= 0)
			return res;
	}
	return -1;
}

int io_pads_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	rnd_hidlib_t *hl = &PCB->hidlib;
	FILE *f;
	int ret = 0;
	pads_read_ctx_t rctx;

	f = rnd_fopen(hl, filename, "r");
	if (f == NULL)
		return -1;

	rctx.col = rctx.start_col = 1;
	rctx.line = rctx.start_line = 2; /* compensate for the header we read with fgets() */
	rctx.pcb = pcb;
	rctx.fn = filename;
	rctx.f = f;
	pcb_dlcr_init(&rctx.dlcr);

	/* read the header */
	if (pads_parse_header(&rctx) != 0) {
		fclose(f);
		pcb_dlcr_uninit(&rctx.dlcr);
		return -1;
	}

	
	ret = (pads_parse_block(&rctx) == 1) ? 0 : -1;
	pcb_dlcr_create(pcb, &rctx.dlcr);
	pcb_dlcr_uninit(&rctx.dlcr);
	fclose(f);
	return ret;
}

int io_pads_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char tmp[256];
	if (fgets(tmp, sizeof(tmp), f) == NULL)
		return 0;
	return (strncmp(tmp, "!PADS-POWERPCB", 14) == 0);
}

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

	/* location */
	const char *fn;
	long line, col, start_line, start_col;

} pads_read_ctx_t;


static int pads_parse_header(pads_read_ctx_t *rctx)
{
	char *s, tmp[256];

	if (fgets(tmp, sizeof(tmp), rctx->f) == NULL)
		return 0;
	s = tmp+15;
	s = strchr(s, '-');
	if (s == NULL) {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (dash)\n");
		return -1;
	}
	s++;
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

/* whether c is horizontal space (of \r, because that should be just ignored) */
static int ishspace(int c) { return ((c == ' ') || (c == '\t') || (c == '\r')); }

/* eat up everything until the newline (including the newline) */
static void pads_eatup_till_nl(pads_read_ctx_t *rctx)
{
	int c;
	while((c = fgetc(rctx->f)) != '\n') pads_update_loc(rctx, c);
	pads_update_loc(rctx, c);
}

static int pads_read_word(pads_read_ctx_t *rctx, char *word, int maxlen, int allow_asterisk)
{
	char *s;
	int c, res;
	static char asterisk_saved[512];
	static int asterisk_saved_len = 0;

	if (asterisk_saved_len > 0) {
		if (!allow_asterisk)
			return 0;
		if (asterisk_saved_len > maxlen) {
			PADS_ERROR((RND_MSG_ERROR, "saved asterisk word too long\n"));
			return -3;
		}
		memcpy(word, asterisk_saved, asterisk_saved_len);
		asterisk_saved_len = 0;
		*asterisk_saved = '\0';
		return 1;
	}

	retry:;
	s = word;
	res = 1;
	pads_start_loc(rctx);

	/* strip leading space */
	while(ishspace(c = fgetc(rctx->f))) pads_update_loc(rctx, c);

	pads_update_loc(rctx, c);
	if (c == EOF)
		return 0;

	while((c != EOF) && !isspace(c)) {
		*s = c;
		s++;
		maxlen--;
		if (maxlen == 1) {
			PADS_ERROR((RND_MSG_ERROR, "word too long\n"));
			*s = '\0';
			res = -3;
			break;
		}
		c = fgetc(rctx->f);
		if (isspace(c)) {
			if (c == '\n')
				ungetc(c, rctx->f); /* need explicit \n */
		}
		else
			pads_update_loc(rctx, c);
	}
	*s = '\0';
	s++;

	if ((rctx->start_col == 1) && (strcmp(word, "*REMARK*") == 0)) {
		pads_eatup_till_nl(rctx);
		goto retry;
	}
	if (!allow_asterisk && (*word == '*')) {
		int len = s - word;
		if (len > sizeof(asterisk_saved)) {
			PADS_ERROR((RND_MSG_ERROR, "asterisk word too long\n"));
			return -3;
		}
		memcpy(asterisk_saved, word, len);
		asterisk_saved_len = len;
		return 0;
	}

	return res;
}


static int pads_parse_ignore_sect(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 0);
		if (res <= 0)
			return res;
		pads_eatup_till_nl(rctx);
	}
	return 0;
}

static int pads_parse_pcb(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 0);
		if (res <= 0)
			return res;
		if (*word == '\0') { /* ignore empty lines between statements */ }
		else if (strcmp(word, "UNITS") == 0) {
			printf("pcb units!\n");
		}
		else if (strcmp(word, "USERGRID") == 0) {
			printf("pcb user grid!\n");
		}
		else {
/*			printf("ignore: '%s'!\n", word);*/
		}
		pads_eatup_till_nl(rctx);
	}
	return 0;
}

static int pads_parse_text(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 0);
		if (res <= 0)
			return res;
		if (*word == '\0') { /* ignore empty lines between statements */ }
		else {
			PADS_ERROR((RND_MSG_ERROR, "unknown text statement: '%s'\n", word));
		}
		pads_eatup_till_nl(rctx);
	}
	return 0;
}

static int pads_parse_lines(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 0);
		if (res <= 0)
			return res;
		if (*word == '\0') { /* ignore empty lines between statements */ }
		else {
			PADS_ERROR((RND_MSG_ERROR, "unknown lines statement: '%s'\n", word));
		}
		pads_eatup_till_nl(rctx);
	}
	return 0;
}

static int pads_parse_block(pads_read_ctx_t *rctx)
{
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 1);
/*printf("WORD='%s'/%d\n", word, res);*/
		if (res <= 0)
			return res;

		res = 0;
		if (*word == '\0') { /* ignore empty lines between blocks */ }
		else if (strcmp(word, "*PCB*") == 0) res |= pads_parse_pcb(rctx);
		else if (strcmp(word, "*REUSE*") == 0) { TODO("What to do with this?"); pads_parse_ignore_sect(rctx); }
		else if (strcmp(word, "*TEXT*") == 0) res |= pads_parse_text(rctx);
		else if (strcmp(word, "*LINES*") == 0) res |= pads_parse_lines(rctx);
		else {
			PADS_ERROR((RND_MSG_ERROR, "unknown block: '%s'\n", word));
			return -1;
		}
		/* exit the loop on error */
		if (res != 0)
			return res;
	}
	return 0;
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

	/* read the header */
	if (pads_parse_header(&rctx) != 0) {
		fclose(f);
		return -1;
	}

	ret = pads_parse_block(&rctx);
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

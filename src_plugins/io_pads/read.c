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

	/* location */
	const char *fn;
	long line, col, start_line, start_col;

} pads_read_ctx_t;


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

static int pads_eatup_ws(pads_read_ctx_t *rctx)
{
	int c;

	while(ishspace(c = fgetc(rctx->f))) pads_update_loc(rctx, c);
	ungetc(c, rctx->f);
	if (c == EOF)
		return 0;
	return 1;
}

static int pads_has_field(pads_read_ctx_t *rctx)
{
	int c;
	if (pads_eatup_ws(rctx) == 0)
		return 0;
	c = fgetc(rctx->f);
	ungetc(c, rctx->f);
	return (c != '\n');
}

static char pads_saved_word[512];
static int pads_saved_word_len;

static int pads_read_word(pads_read_ctx_t *rctx, char *word, int maxlen, int allow_asterisk)
{
	char *s;
	int c, res;
	static char asterisk_saved[512];
	static int asterisk_saved_len = 0;

	if (pads_saved_word_len > 0) {
		if (pads_saved_word_len > maxlen) {
			PADS_ERROR((RND_MSG_ERROR, "saved word too long\n"));
			return -3;
		}
		memcpy(word, pads_saved_word, pads_saved_word_len);
		pads_saved_word_len = 0;
		*pads_saved_word = '\0';
		return 1;
	}

	if (asterisk_saved_len > 0) {
		if (!allow_asterisk)
			return -4;
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
	if (pads_eatup_ws(rctx) == 0)
		return 0;

	c = fgetc(rctx->f);
	pads_update_loc(rctx, c);
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
		return -4;
	}

	if ((res == 1) && (word == pads_saved_word)) {
		if (pads_saved_word_len > 0) {
			PADS_ERROR((RND_MSG_ERROR, "can not save multiple words\n"));
			return -3;
		}
		pads_saved_word_len = s - word;
	}

	return res;
}

static int pads_read_double(pads_read_ctx_t *rctx, double *dst)
{
	char tmp[64], *end;
	int res = pads_read_word(rctx, tmp, sizeof(tmp), 0);
	if (res <= 0)
		return res;
	*dst = strtod(tmp, &end);
	if (*end != '\0') {
		PADS_ERROR((RND_MSG_ERROR, "invalid numeric: '%s'\n", tmp));
		return -1;
	}
	return 1;
}

static int pads_read_long(pads_read_ctx_t *rctx, long *dst)
{
	char tmp[64], *end;
	int res = pads_read_word(rctx, tmp, sizeof(tmp), 0);
	if (res <= 0)
		return res;
	*dst = strtol(tmp, &end, 10);
	if (*end != '\0') {
		PADS_ERROR((RND_MSG_ERROR, "invalid integer: '%s'\n", tmp));
		return -1;
	}
	return 1;
}

static int pads_read_coord(pads_read_ctx_t *rctx, rnd_coord_t *dst)
{
	long l;
	int res = pads_read_long(rctx, &l);
	if (res <= 0)
		return res;
	*dst = rnd_round((double)l * rctx->coord_scale);
	return 1;
}

static int pads_read_degp10(pads_read_ctx_t *rctx, double *dst)
{
	long l;
	int res = pads_read_long(rctx, &l);
	if (res <= 0)
		return res;
	*dst = (double)l / 10.0;
	return 1;
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
	return 1;
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
			pads_eatup_till_nl(rctx);
		}
		else if (strcmp(word, "USERGRID") == 0) {
			printf("pcb user grid!\n");
			pads_eatup_till_nl(rctx);
		}
		else {
/*			printf("ignore: '%s'!\n", word);*/
			pads_eatup_till_nl(rctx);
		}
	}
	return 1;
}


static int pads_parse_piece_crd(pads_read_ctx_t *rctx)
{
	int res;
	rnd_coord_t x, y;

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;

	if (pads_has_field(rctx)) {
		double starta, enda, r;
		rnd_coord_t bbx1, bby1, bbx2, bby2, cx, cy;

		if ((res = pads_read_degp10(rctx, &starta)) <= 0) return res;
		if ((res = pads_read_degp10(rctx, &enda)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bbx1)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bby1)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bbx2)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bby2)) <= 0) return res;

		r = (double)(bbx2 - bbx1) / 2.0;
		cx = rnd_round((double)(bbx1 + bbx2) / 2.0);
		cy = rnd_round((double)(bby1 + bby2) / 2.0);
		rnd_trace("  crd arc %mm;%mm %f..%f r=%mm center=%mm;%mm\n", x, y, starta, enda, (rnd_coord_t)r, cx, cy);
	}
	else {
		rnd_trace("  crd line %mm;%mm\n", x, y);
	}

	pads_eatup_till_nl(rctx);


	return 1;
}


static int pads_parse_piece(pads_read_ctx_t *rctx)
{
	char ptype[32], rest[32];
	long n, num_crds, layer, lstyle;
	rnd_coord_t width;
	int res;

	if ((res = pads_read_word(rctx, ptype, sizeof(ptype), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_crds)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &width)) <= 0) return res;
	if (rctx->ver > 9.4) {
		if ((res = pads_read_long(rctx, &lstyle)) <= 0) return res;
		if ((res = pads_read_long(rctx, &layer)) <= 0) return res;
		if (pads_has_field(rctx))
			if ((res = pads_read_word(rctx, rest, sizeof(rest), 0)) <= 0) return res;
	}
	else {
		if ((res = pads_read_long(rctx, &layer)) <= 0) return res;
		if (pads_has_field(rctx))
			if ((res = pads_read_word(rctx, rest, sizeof(rest), 0)) <= 0) return res;
	}
	pads_eatup_till_nl(rctx);

	rnd_trace(" piece %s\n", ptype);
	for(n = 0; n < num_crds; n++)
		if ((res = pads_parse_piece_crd(rctx)) <= 0) return res;

	return 1;
}

static int pads_parse_text_(pads_read_ctx_t *rctx, int is_label)
{
	char name[16], hjust[16], vjust[16], font[128], str[1024];
	rnd_coord_t x, y, w, h;
	double rot;
	long level;
	int res, mirr = 0;

	if (is_label)
		if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;
	if ((res = pads_read_double(rctx, &rot)) <= 0) return res;
	if ((res = pads_read_long(rctx, &level)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &h)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &w)) <= 0) return res;

	/* next field is either mirror (M or N) or hjust */
	if ((res = pads_read_word(rctx, hjust, sizeof(hjust), 0)) <= 0) return res;
	if (*hjust == 'N') {
		mirr = 0;
		if ((res = pads_read_word(rctx, hjust, sizeof(hjust), 0)) <= 0) return res;
	}
	else if (*hjust == 'M') {
		mirr = 1;
		if ((res = pads_read_word(rctx, hjust, sizeof(hjust), 0)) <= 0) return res;
	}

	if ((res = pads_read_word(rctx, vjust, sizeof(vjust), 0)) <= 0) return res;

	/* fields ignored: ndim and .REUSE.*/
	pads_eatup_till_nl(rctx);

	if ((res = pads_read_word(rctx, font, sizeof(font), 0)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	if ((res = pads_read_word(rctx, str, sizeof(str), 0)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	rnd_trace(" text: [%s] at %mm;%mm rot=%f size=%mm;%mm: '%s'",
		font, x, y, rot, w, h, str);
	if (is_label)
		rnd_trace(" (%s)\n", name);
	else
		rnd_trace("\n");
	return 1;
}

static int pads_parse_text(pads_read_ctx_t *rctx)
{
	return pads_parse_text_(rctx, 0);
}

static int pads_parse_label(pads_read_ctx_t *rctx)
{
	return pads_parse_text_(rctx, 1);
}

static int pads_parse_lines(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);
	while(!feof(rctx->f)) {
		char word[256], type[32];
		int c, res = pads_read_word(rctx, word, sizeof(word), 0);
		if (res <= 0)
			return res;
		if (*word == '\0') { /* ignore empty lines between statements */ }
		else { /* name type xo yo numpieces [numtext] [sigstr] */
			rnd_coord_t xo, yo;
			long n, num_pcs, flags, num_texts;

			if ((res = pads_read_word(rctx, type, sizeof(type), 0)) <= 0) return res;
			if ((res = pads_read_coord(rctx, &xo)) <= 0) return res;
			if ((res = pads_read_coord(rctx, &yo)) <= 0) return res;
			if ((res = pads_read_long(rctx, &num_pcs)) <= 0) return res;
			if ((res = pads_read_long(rctx, &flags)) <= 0) return res;

			if (pads_has_field(rctx))
				if ((res = pads_read_long(rctx, &num_texts)) <= 0) return res;
			/* last optional field is netname - ignore that */

			rnd_trace("line name=%s ty=%s %mm;%mm pcs=%d texts=%d\n", word, type, xo, yo, num_pcs, num_texts);
			pads_eatup_till_nl(rctx);
			c = fgetc(rctx->f);
			ungetc(c, rctx->f);
			if (c == '.') { /* .reuse. */
				if ((res = pads_read_word(rctx, word, sizeof(word), 0)) <= 0) return res;
				rnd_trace("line reuse: '%s'\n", word);
				pads_eatup_till_nl(rctx);
			}
			for(n = 0; n < num_pcs; n++)
				if ((res = pads_parse_piece(rctx)) <= 0) return res;
			for(n = 0; n < num_texts; n++)
				if ((res = pads_parse_text(rctx)) <= 0) return res;
		}
	}
	return 1;
}

static int pads_parse_list_sect(pads_read_ctx_t *rctx, int (*parse_item)(pads_read_ctx_t *))
{
	pads_eatup_till_nl(rctx);
	while(!feof(rctx->f)) {
		char c;
		int res;

		pads_eatup_ws(rctx);

		c = fgetc(rctx->f);
		if (c == '\n') {
			pads_update_loc(rctx, c);
			continue;
		}

		ungetc(c, rctx->f);
		if (c == '*') { /* may be the next section, or just a remark */
			char tmp[256];
			if (pads_read_word(rctx, tmp, sizeof(tmp), 0) == 0) /* next section */
				break;
			if (*pads_saved_word == '\0')  /* remark */
				continue;
			/* no 3rd possibility */
			PADS_ERROR((RND_MSG_ERROR, "Can't get here in pads_parse_list_sect\n"));
		}

		res = parse_item(rctx);
		if (res <= 0)
			return res;
	}
	return 1;
}

static int pads_parse_texts(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_text);
}

static int pads_parse_pstk_proto(pads_read_ctx_t *rctx)
{
	char name[256], shape[8];
	rnd_coord_t drill, size, inner;
	long n, num_lines, level, start = 0, end = 0;
	int res;

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &drill)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_lines)) <= 0) return res;
	if (pads_has_field(rctx))
		if ((res = pads_read_long(rctx, &start)) <= 0) return res;
	if (pads_has_field(rctx))
		if ((res = pads_read_long(rctx, &end)) <= 0) return res;

	pads_eatup_till_nl(rctx);

	rnd_trace(" via: %s drill=%mm [%ld..%ld]\n", name, drill, start, end);
	for(n = 0; n < num_lines; n++) {
		if ((res = pads_read_long(rctx, &level)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &size)) <= 0) return res;
		if ((res = pads_read_word(rctx, shape, sizeof(shape), 0)) <= 0) return res;
		if (pads_has_field(rctx))
			if ((res = pads_read_coord(rctx, &inner)) <= 0) return res;
		else
			inner = 0;
		pads_eatup_till_nl(rctx);
		rnd_trace("  lev=%ld size=%mm/%mm shape=%s\n", level, size, inner, shape);
	}
}

static int pads_parse_vias(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_pstk_proto);
}

static int pads_parse_term(pads_read_ctx_t *rctx)
{
	char name[10];
	int c, res;
	rnd_coord_t x, y, nmx, nmy;

	c = fgetc(rctx->f);
	if (c != 'T') {
		PADS_ERROR((RND_MSG_ERROR, "Terminal definition line doesn't start with T\n"));
		return -1;
	}
	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &nmx)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &nmy)) <= 0) return res;
	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;

	rnd_trace("  terminal: %ld at %mm;%mm\n", name, x, y);
	return 1;
}

static int pads_parse_partdecal(pads_read_ctx_t *rctx)
{
	char name[256], unit[8];
	rnd_coord_t xo, yo;
	long n, num_pieces, num_terms, num_stacks, num_texts = 0, num_labels = 0;
	int res;

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, unit, sizeof(unit), 0)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &xo)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &yo)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_pieces)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_terms)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_stacks)) <= 0) return res;

	if (pads_has_field(rctx))
		if ((res = pads_read_long(rctx, &num_texts)) <= 0) return res;
	if (pads_has_field(rctx))
		if ((res = pads_read_long(rctx, &num_labels)) <= 0) return res;

	rnd_trace("part '%s', original %mm;%mm\n", name, xo, yo);
TODO("set unit and origin");
	for(n = 0; n < num_pieces; n++)
		if ((res = pads_parse_piece(rctx)) <= 0) return res;
	for(n = 0; n < num_texts; n++)
		if ((res = pads_parse_text(rctx)) <= 0) return res;
	for(n = 0; n < num_labels; n++)
		if ((res = pads_parse_label(rctx)) <= 0) return res;
	for(n = 0; n < num_terms; n++)
		if ((res = pads_parse_term(rctx)) <= 0) return res;
	for(n = 0; n < num_stacks; n++)
		if ((res = pads_parse_pstk_proto(rctx)) <= 0) return res;

	return 1;
}

static int pads_parse_partdecals(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_pstk_proto);
}

static int pads_parse_block(pads_read_ctx_t *rctx)
{
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 1);
/*printf("WORD='%s'/%d\n", word, res);*/
		if (res <= 0)
			return res;

		if (*word == '\0') res = 1; /* ignore empty lines between blocks */
		else if (strcmp(word, "*PCB*") == 0) res = pads_parse_pcb(rctx);
		else if (strcmp(word, "*REUSE*") == 0) { TODO("What to do with this?"); res = pads_parse_ignore_sect(rctx); }
		else if (strcmp(word, "*TEXT*") == 0) res = pads_parse_texts(rctx);
		else if (strcmp(word, "*LINES*") == 0) res = pads_parse_lines(rctx);
		else if (strcmp(word, "*VIA*") == 0) res = pads_parse_vias(rctx);
		else if (strcmp(word, "*PARTDECAL*") == 0) res = pads_parse_partdecals(rctx);
		else {
			PADS_ERROR((RND_MSG_ERROR, "unknown block: '%s'\n", word));
			return -1;
		}

		if (res == -4) continue; /* bumped into the next section, just go on reading */

		/* exit the loop on error */
		if (res <= 0)
			return res;
	}
	return 1;
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

	ret = !pads_parse_block(&rctx);
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

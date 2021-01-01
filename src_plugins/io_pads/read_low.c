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

/*** low level: words and types ***/

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

static int pads_read_word_(pads_read_ctx_t *rctx, char *word, int maxlen, int allow_asterisk, int stop_at_space)
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
	while((c != EOF)) {
		if (isspace(c)) {
			if ((stop_at_space) || (c == '\n'))
				break;
		}
		if (c != '\r') {
			*s = c;
			s++;
		}
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

static int pads_read_word(pads_read_ctx_t *rctx, char *word, int maxlen, int allow_asterisk)
{
	return pads_read_word_(rctx, word, maxlen, allow_asterisk, 1);
}

static int pads_read_word_all(pads_read_ctx_t *rctx, char *word, int maxlen, int allow_asterisk)
{
	return pads_read_word_(rctx, word, maxlen, allow_asterisk, 0);
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

/* Read next word, convert to long only if it's a valid integer, else ignore;
   returns 1 on string, 2 on long */
static int pads_maybe_read_long(pads_read_ctx_t *rctx, char *word, int wordlen, long *dst)
{
	long l;
	char *end;
	int res = pads_read_word(rctx, word, wordlen, 0);
	if (res <= 0)
		return res;
	l = strtol(word, &end, 10);
	if (*end == '\0') {
		*dst = l;
		return 2;
	}
	return 1;
}

/* Read next word, convert to double only if it's a valid numeric, else ignore;
   returns 1 on string, 2 on long */
static int pads_maybe_read_double(pads_read_ctx_t *rctx, char *word, int wordlen, double *dst)
{
	double d;
	char *end;
	int res = pads_read_word(rctx, word, wordlen, 0);
	if (res <= 0)
		return res;
	d = strtod(word, &end);
	if (*end == '\0') {
		*dst = d;
		return 2;
	}
	return 1;
}

static int pads_read_coord(pads_read_ctx_t *rctx, rnd_coord_t *dst)
{
	double d;
	int res = pads_read_double(rctx, &d);
	if (res <= 0)
		return res;
	*dst = rnd_round(d * rctx->coord_scale);
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

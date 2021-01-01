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

/*** high level: *MISC* has a different format than other sections ***/

/* read everything till the closing of this block; recurse if more blocks are
   open */
static int pads_parse_misc_ignore_block(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);

	for(;;) {
		int c, res;

		pads_eatup_ws(rctx);
		c = fgetc(rctx->f);
		if (c == EOF)
			return 0;
		ungetc(c, rctx->f);
		pads_eatup_till_nl(rctx);
		if (c == '{') {
			if ((res = pads_parse_misc_ignore_block(rctx)) <= 0) return res;
			continue;
		}
		if (c == '}')
			return 1;
	}
	return 1;
}

static int pads_parse_misc(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);

	rnd_trace("*MISC*\n");
	for(;;) {
		char key[256];
		int c, res;

		retry:;
		pads_eatup_ws(rctx);
		c = fgetc(rctx->f);
		if (c == EOF) {
			rnd_trace("unexpected eof in *MISC*\n");
			return 0;
		}
		if (c == '\n') {
			pads_update_loc(rctx, c);
			goto retry;
		}

		ungetc(c, rctx->f);
		if (c == '{') {
			rnd_trace("misc ignore block!\n");
			pads_eatup_till_nl(rctx);
			if ((res = pads_parse_misc_ignore_block(rctx)) <= 0) return res;
			continue;
		}

		/*read key; if it is a new *SECTION* then res will be -4 */
		if ((res = pads_read_word(rctx, key, sizeof(key), 0)) <= 0)
			return res;
		if (*key != '\0')
			pads_eatup_till_nl(rctx);
		rnd_trace("# MISC: '%s'\n", key);
	}
	return 1;
}

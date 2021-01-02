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

/*** high level: generic section ***/

static int pads_parse_ignore_sect(pads_read_ctx_t *rctx)
{
	pads_eatup_till_nl(rctx);
	while(!feof(rctx->f)) {
		char word[256];
		int c, res;

		/* ignore empty lines */
		pads_eatup_ws(rctx);
		c = fgetc(rctx->f);
		if (c == '\n') {
			pads_update_loc(rctx, c);
			continue;
		}
		ungetc(c, rctx->f);

		res = pads_read_word(rctx, word, sizeof(word), 0);
		if (res <= 0)
			return res;
		pads_eatup_till_nl(rctx);
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
			tmp[0] = '\0';
			if (pads_read_word(rctx, tmp, sizeof(tmp), 0) == 0) /* next section */
				break;
			if (tmp[0] == '*')
				return -4; /* next section */
			if (*pads_saved_word == '\0')  /* remark */
				continue;
			/* no other possibility */
			PADS_ERROR((RND_MSG_ERROR, "Can't get here in pads_parse_list_sect\n"));
		}

		res = parse_item(rctx);
		if (res <= 0)
			return res;
	}
	return 1;
}

/*** high level: specific section ***/

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

/* virtual layer ID for the non-existent board outline layer */
#define PADS_LID_BOARD 257

typedef enum { PLTY_LINES, PLTY_BOARD, PLTY_COPPER, PLTY_COPCUT, PLTY_KEEPOUT } pads_line_type_t;

typedef struct {
	rnd_coord_t x, y;
	rnd_coord_t width;
	pads_line_type_t ltype;
	long level;
	rnd_coord_t xo, yo;
} pads_line_piece_t;

static int pads_parse_piece_crd(pads_read_ctx_t *rctx, pads_line_piece_t *lpc, long idx)
{
	int res;
	rnd_coord_t x, y;

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;

	x += lpc->xo;
	y += lpc->yo;

	if ((idx != 0) && ((lpc->x != x) || (lpc->y != y)))  {
		pcb_dlcr_draw_t *line = pcb_dlcr_line_new(&rctx->dlcr, lpc->x, lpc->y, x, y, lpc->width, 0);
		line->val.obj.layer_id = lpc->level;
		line->loc_line = rctx->line;
	}

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
		rnd_trace("  crd move %mm;%mm\n", x, y);
	}

	lpc->x = x;
	lpc->y = y;

	pads_eatup_till_nl(rctx);

	return 1;
}


static int pads_parse_piece(pads_read_ctx_t *rctx, pads_line_type_t ltype, rnd_coord_t xo, rnd_coord_t yo)
{
	char ptype[32], rest[32];
	long n, num_crds, layer = -100, lstyle;
	rnd_coord_t width;
	int res;
	pads_line_piece_t lpc = {0};

	if ((res = pads_read_word(rctx, ptype, sizeof(ptype), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_crds)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &width)) <= 0) return res;
	if ((rctx->ver > 9.4) && (floor(rctx->ver) > 2007)) {
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

	rnd_trace(" piece %s num_crds=%ld level=%ld\n", ptype, num_crds, layer);
	lpc.width = width;
	lpc.ltype = ltype;
	lpc.level = ltype == PLTY_BOARD ? PADS_LID_BOARD : layer;
	lpc.xo = xo;
	lpc.yo = yo;
	for(n = 0; n < num_crds; n++)
		if ((res = pads_parse_piece_crd(rctx, &lpc, n)) <= 0) return res;

	return 1;
}

static int pads_parse_text_(pads_read_ctx_t *rctx, int is_label)
{
	char name[16], hjust[16], vjust[16], font[128], str[1024];
	rnd_coord_t x, y, w, h;
	double rot;
	long level;
	int res, mirr = 0;

	*font = *str = '\0';

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

	if (rctx->ver >= 6.0) { /* 4.0 and 5.0 don't have font */
		if ((res = pads_read_word(rctx, font, sizeof(font), 0)) <= 0) return res;
		pads_eatup_till_nl(rctx);
	}

	if ((res = pads_read_word(rctx, str, sizeof(str), 1)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	rnd_trace(" text: [%s] at %mm;%mm rot=%f size=%mm;%mm mirr=%d: '%s'",
		font, x, y, rot, w, h, mirr, str);
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

static int pads_parse_texts(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_text);
}

static int pads_parse_line(pads_read_ctx_t *rctx)
{
	char name[256], type[32], wtf[64];
	pads_line_type_t ltype;
	rnd_coord_t xo, yo;
	long n, c, num_pcs, flags = 0, num_texts = 0;
	int res;

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, type, sizeof(type), 0)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &xo)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &yo)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_pcs)) <= 0) return res;
	if (pads_has_field(rctx)) {
		if (floor(rctx->ver) == 2005) {
			/* _maybe_ num texts, but don't bet on it, could be netname as well */
			if ((res = pads_maybe_read_long(rctx, wtf, sizeof(wtf), &num_texts)) <= 0) return res;
		}
		else if (rctx->ver >= 6.0) /* 4.0 and 5.0 don't have flags */
			if ((res = pads_read_long(rctx, &flags)) <= 0) return res;
	}

	if (pads_has_field(rctx)) {
		/* _maybe_ num texts, but don't bet on it, could be netname as well */
		if ((res = pads_maybe_read_long(rctx, wtf, sizeof(wtf), &num_texts)) <= 0) return res;
	}

	/* last optional field is netname - ignore that */
	pads_eatup_till_nl(rctx);

	rnd_trace("line name=%s ty=%s %mm;%mm pcs=%d texts=%d\n", name, type, xo, yo, num_pcs, num_texts);

	c = fgetc(rctx->f);
	ungetc(c, rctx->f);
	if (c == '.') { /* .reuse. */
		char word[256];
		if ((res = pads_read_word(rctx, word, sizeof(word), 0)) <= 0) return res;
		rnd_trace("line reuse: '%s'\n", word);
		pads_eatup_till_nl(rctx);
	}

	if (strcmp(type, "LINES") == 0)        ltype = PLTY_LINES;
	else if (strcmp(type, "BOARD") == 0)   ltype = PLTY_BOARD;
	else if (strcmp(type, "COPPER") == 0)  ltype = PLTY_COPPER;
	else if (strcmp(type, "COPCUT") == 0)  ltype = PLTY_COPCUT;
	else if (strcmp(type, "KEEPOUT") == 0) ltype = PLTY_KEEPOUT;
	else {
		PADS_ERROR((RND_MSG_ERROR, "Unknown *LINE* type: '%s'\n", type));
		return -1;
	}

	for(n = 0; n < num_pcs; n++)
		if ((res = pads_parse_piece(rctx, ltype, xo, yo)) <= 0) return res;
	for(n = 0; n < num_texts; n++)
		if ((res = pads_parse_text(rctx)) <= 0) return res;

	return 1;
}

static int pads_parse_lines(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_line);
}

static int pads_parse_pstk_proto(pads_read_ctx_t *rctx)
{
	char name[256], shape[8];
	rnd_coord_t drill, size;
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

	rnd_trace(" pstk_proto: %s drill=%mm [%ld..%ld]\n", name, drill, start, end);
	for(n = 0; n < num_lines; n++) {
		double rot = 0, slotrot = 0, spokerot = 0;
		char plated[8];
		int c, is_thermal;
		long spoke_num = 0;
		rnd_coord_t finlen = 0, finoffs = 0, inner = 0, corner = 0, drill = 0, slotlen = 0, slotoffs = 0, spoke_outsize = 0, spoke_width = 0;

		if ((res = pads_read_long(rctx, &level)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &size)) <= 0) return res;
		if ((res = pads_read_word(rctx, shape, sizeof(shape), 0)) <= 0) return res;

		if (shape[0] == 'A') /* only annular pad has inner dia */
			if ((res = pads_read_coord(rctx, &inner)) <= 0) return res;

		if (shape[1] != 'T') { /* normal pad (T is for thermal) */
			is_thermal = 0;
			if (shape[1] == 'F') { /* finger-type pads have rotation */
				if ((res = pads_read_double(rctx, &rot)) <= 0) return res;
				if ((res = pads_read_coord(rctx, &finlen)) <= 0) return res;
				if ((res = pads_read_coord(rctx, &finoffs)) <= 0) return res;
				if (pads_has_field(rctx) && (shape[0] == 'R')) /* RF=rectangular finger */
					if ((res = pads_read_coord(rctx, &corner)) <= 0) return res;
			}
			else if (pads_has_field(rctx) && shape[0] == 'S') { /* S=square */
				if ((res = pads_read_coord(rctx, &corner)) <= 0) return res;
			}

			/* next word is either drill diameter or P/N for plated-or-not */
			pads_eatup_ws(rctx);
			c = fgetc(rctx->f);
			ungetc(c, rctx->f);

			/* optional drill */
			if (pads_has_field(rctx) && (!isalpha(c)))
				if ((res = pads_read_coord(rctx, &drill)) <= 0) return res;
			plated[0] = '\0';
			if (pads_has_field(rctx))
				if ((res = pads_read_word(rctx, plated, sizeof(plated), 0)) <= 0) return res;

			/* optional slot */
			if (pads_has_field(rctx)) {
				if ((res = pads_read_double(rctx, &slotrot)) <= 0) return res;
				if ((res = pads_read_coord(rctx, &slotlen)) <= 0) return res;
				if ((res = pads_read_coord(rctx, &slotoffs)) <= 0) return res;
			}
		}
		else { /* thermal */
			is_thermal = 1;
			if ((res = pads_read_double(rctx, &spokerot)) <= 0) return res;
			if ((res = pads_read_coord(rctx, &spoke_outsize)) <= 0) return res;
			if ((res = pads_read_coord(rctx, &spoke_width)) <= 0) return res;
			if ((res = pads_read_long(rctx, &spoke_num)) <= 0) return res;
		}

		pads_eatup_till_nl(rctx);
		rnd_trace("  lev=%ld size=%mm/%mm shape=%s is_thermal=%d\n", level, size, inner, shape, is_thermal);
	}
	return 1;
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
	if (pads_has_field(rctx)) {
		if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	}
	else
		*name = '\0';

	pads_eatup_till_nl(rctx);

	rnd_trace("  terminal: %s at %mm;%mm\n", name, x, y);
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

	pads_eatup_till_nl(rctx);

	rnd_trace("part '%s', original %mm;%mm pcs=%ld texts=%ld labels=%ld terms=%ld stacks=%ld\n",
		name, xo, yo, num_pieces, num_texts, num_labels, num_terms, num_stacks);
TODO("set unit and origin");

	dlcr_new(&rctx->dlcr, DLCR_SUBC_BEGIN);
	for(n = 0; n < num_pieces; n++)
		if ((res = pads_parse_piece(rctx, PLTY_LINES, xo, yo)) <= 0) return res;
	for(n = 0; n < num_texts; n++)
		if ((res = pads_parse_text(rctx)) <= 0) return res;
	for(n = 0; n < num_labels; n++)
		if ((res = pads_parse_label(rctx)) <= 0) return res;
	for(n = 0; n < num_terms; n++)
		if ((res = pads_parse_term(rctx)) <= 0) return res;
	for(n = 0; n < num_stacks; n++)
		if ((res = pads_parse_pstk_proto(rctx)) <= 0) return res;
	dlcr_new(&rctx->dlcr, DLCR_SUBC_END);

	return 1;
}

static int pads_parse_partdecals(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_partdecal);
}


static int pads_parse_gate(pads_read_ctx_t *rctx)
{
	char type[3], pin[256];
	long n, swaptype, num_pins;
	int res;

	if ((res = pads_read_word(rctx, type, sizeof(type), 0)) <= 0) return res;
	if (strcmp(type, "G") != 0) {
		PADS_ERROR((RND_MSG_ERROR, "Gate needs to start with a G\n"));
		return -1;
	}
	if ((res = pads_read_long(rctx, &swaptype)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_pins)) <= 0) return res;

	for(n = 0; n < num_pins; n++) {
		*pin = '\0';
		while(*pin == '\0') { /* this will ignore newlines */
			res = pads_read_word(rctx, pin, sizeof(pin), 0);
			rnd_trace(" gate '%s' %d\n", pin, res);
		}
	}

	pads_eatup_till_nl(rctx); /* ignore rest of the fields */
	return 1;
}

static int pads_parse_sigpin(pads_read_ctx_t *rctx)
{
	char sigpin[8], pin[256];
	int res;

	if ((res = pads_read_word(rctx, sigpin, sizeof(sigpin), 0)) <= 0) return res;
	if (strcmp(sigpin, "SIGPIN") != 0) {
		PADS_ERROR((RND_MSG_ERROR, "sigpin needs to start with a SIGPIN\n"));
		return -1;
	}

	rnd_trace(" sigpin\n", pin, res);

	pads_eatup_till_nl(rctx); /* ignore rest of the fields */

	return 1;
}

static int pads_parse_pinnames(pads_read_ctx_t *rctx, long num_pins)
{
	long n;
	char pin[64];
	int res;

	for(n = 0; n < num_pins; n++) {
		*pin = '\0';
		while(*pin == '\0') { /* this will ignore newlines */
			res = pads_read_word(rctx, pin, sizeof(pin), 0);
			rnd_trace(" pinname '%s' %d\n", pin, res);
		}
	}
	pads_eatup_till_nl(rctx); /* ignore rest of the fields */
	return 1;
}

static int pads_parse_parttype(pads_read_ctx_t *rctx)
{
	char name[64], decals[16*64], unit[4], ptype[8];
	long n, num_gates, num_signals, num_alpins, flags;
	int res;

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, decals, sizeof(decals), 0)) <= 0) return res;
	if ((floor(rctx->ver) == 2005) || (rctx->ver < 6.0)) { /* 4.0 and 5.0 both have unit */
		if ((res = pads_read_word(rctx, unit, sizeof(unit), 0)) <= 0) return res;
	}
	else
		*unit = '\0';

	if ((res = pads_read_word(rctx, ptype, sizeof(ptype), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_gates)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_signals)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_alpins)) <= 0) return res;
	if ((res = pads_read_long(rctx, &flags)) <= 0) return res;
	/* ECO is ignored */
	pads_eatup_till_nl(rctx);

	rnd_trace("parttype: '%s' gates=%ld signals=%ld alpins=%ld\n", name, num_gates, num_signals, num_alpins);
	for(n = 0; n < num_gates; n++)
		if ((res = pads_parse_gate(rctx)) <= 0) return res;
	for(n = 0; n < num_signals; n++)
		if ((res = pads_parse_sigpin(rctx)) <= 0) return res;
	if (num_alpins > 0)
		pads_parse_pinnames(rctx, num_alpins);

	return 1;
}

static int pads_parse_parttypes(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_parttype);
}

static int pads_parse_part(pads_read_ctx_t *rctx)
{
	char refdes[64], ptype[64], glue[4], mirr[4];
	long n, altdeclnum, clustid = -1, clsattr = 0, brotherid = -1, num_labels;
	rnd_coord_t xo, yo;
	double rot;
	int res;

	if ((res = pads_read_word(rctx, refdes, sizeof(refdes), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, ptype, sizeof(ptype), 0)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &xo)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &yo)) <= 0) return res;
	if ((res = pads_read_double(rctx, &rot)) <= 0) return res;
	if ((res = pads_read_word(rctx, glue, sizeof(glue), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, mirr, sizeof(mirr), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &altdeclnum)) <= 0) return res;

	if (pads_has_field(rctx)) {
		if ((res = pads_read_long(rctx, &clustid)) <= 0) return res;
		if ((res = pads_read_long(rctx, &clsattr)) <= 0) return res;
		if ((res = pads_read_long(rctx, &brotherid)) <= 0) return res;
		if ((res = pads_read_long(rctx, &num_labels)) <= 0) return res;
	}

	pads_eatup_till_nl(rctx);

	rnd_trace("part: '%s' of '%s' num_labels=%ld\n", refdes, ptype, num_labels);
	for(n = 0; n < num_labels; n++)
		if ((res = pads_parse_label(rctx)) <= 0) return res;
	return 1;
}

static int pads_parse_parts(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_part);
}

static int pads_parse_signal_crd(pads_read_ctx_t *rctx)
{
	char vianame[64];
	long level, flags;
	rnd_coord_t x, y, width;
	int res, arcdir = 0, thermal = 0;

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;
	if ((res = pads_read_long(rctx, &level)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &width)) <= 0) return res;
	if ((res = pads_read_long(rctx, &flags)) <= 0) return res;

	*vianame = '\0';
	if (pads_has_field(rctx)) {
		if ((res = pads_read_word(rctx, vianame, sizeof(vianame), 0)) <= 0) return res;

		if (strcmp(vianame, "CW") == 0)       { arcdir = -1; *vianame = '\0'; }
		else if (strcmp(vianame, "CCW") == 0) { arcdir = +1; *vianame = '\0'; }
		else arcdir = 0;

		if (strcmp(vianame, "THERMAL") == 0) { thermal = 1; *vianame = '\0'; }

		/* the rest of the line is jumper wire and teardrop - ignore those */
	}
	pads_eatup_till_nl(rctx);

	rnd_trace("  %mm;%mm level=%ld w=%mm flags=%ld vianame=%s arcdir=%d thermal=%d\n",
		x, y, level, width, flags, vianame, arcdir, thermal);
	return 1;
}


static int pads_parse_net(pads_read_ctx_t *rctx)
{
	char term1[128], term2[128];
	int c, res;

	if ((res = pads_read_word(rctx, term1, sizeof(term1), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, term2, sizeof(term2), 0)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	rnd_trace(" '%s' -> '%s'\n", term1, term2);

	for(;;) {
		double tmp;

		pads_eatup_ws(rctx);
		c = fgetc(rctx->f);
		ungetc(c, rctx->f);

		if (pads_maybe_read_double(rctx, pads_saved_word, sizeof(pads_saved_word), &tmp) == 1)
			break; /* when next word is a string; also save the next word so it is returned in the next read */

		if ((res = pads_parse_signal_crd(rctx)) <= 0) return res;
	}

	return 1;
}

static int pads_parse_signal(pads_read_ctx_t *rctx)
{
	char netname[128];
	int res;

	if ((res = pads_read_word(rctx, netname, sizeof(netname), 0)) <= 0) return res;

	rnd_trace("signal: netname=%s\n", netname);
	return pads_parse_list_sect(rctx, pads_parse_net);
}

static int pads_parse_pour_piece_crd(pads_read_ctx_t *rctx)
{
	rnd_coord_t x, y;
	double start, end;
	int isarc = 0, res;

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;

	if (pads_has_field(rctx)) {
		isarc = 1;
		if ((res = pads_read_double(rctx, &start)) <= 0) return res;
		if ((res = pads_read_double(rctx, &end)) <= 0) return res;
	}
	pads_eatup_till_nl(rctx);

	if (isarc)
		rnd_trace("  arc %mm;%mm %f..%f\n", x, y, start, end);
	else
		rnd_trace("  line %mm;%mm\n", x, y);

	return 1;
}

static int pads_parse_pour_piece(pads_read_ctx_t *rctx)
{
	char type[64];
	long n, num_corners, num_arcs;
	int res;

	if ((res = pads_read_word(rctx, type, sizeof(type), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_corners)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_arcs)) <= 0) return res;
	pads_eatup_till_nl(rctx); /* ignore rest of the arguments */

	rnd_trace(" %s:\n", type);
	for(n = 0; n < num_corners + num_arcs; n++)
		if ((res = pads_parse_pour_piece_crd(rctx)) <= 0) return res;

	return 1;
}


static int pads_parse_pour(pads_read_ctx_t *rctx)
{
	char name[64], type[64];
	long n, num_pieces;
	rnd_coord_t xo, yo;
	double rot;
	int res;

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, type, sizeof(type), 0)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &xo)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &yo)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_pieces)) <= 0) return res;
	pads_eatup_till_nl(rctx); /* ignore rest of the arguments */

	rnd_trace("pour '%s' type='%s' at %mm;%mm\n", name, type, xo, yo);
	for(n = 0; n < num_pieces; n++)
		if ((res = pads_parse_pour_piece(rctx)) <= 0) return res;

	return 1;
}

static int pads_parse_pours(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_pour);
}

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

		/* special case: we ignored an empty section, the first thing we see
		   is a *SECT* that is not *REMARK*. Return success before this next
		   *SECT* is read */
		c = fgetc(rctx->f);
		ungetc(c, rctx->f);
		if (c == '*')
			return 1;

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

typedef enum { PLTY_LINES, PLTY_BOARD, PLTY_COPPER, PLTY_COPCUT, PLTY_KEEPOUT } pads_line_type_t;

typedef struct {
	rnd_coord_t x, y;
	rnd_coord_t width;
	pads_line_type_t ltype;
	pcb_layer_type_t lyt;
	long level;
	rnd_coord_t xo, yo;
} pads_line_piece_t;

static int pads_parse_piece_crd(pads_read_ctx_t *rctx, pads_line_piece_t *lpc, long idx)
{
	int res;
	rnd_coord_t x, y, xe, ye;

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;

	x += lpc->xo;
	y += lpc->yo;

	if ((idx != 0) && ((lpc->x != x) || (lpc->y != y)))  {
		pcb_dlcr_draw_t *line = pcb_dlcr_line_new(&rctx->dlcr, lpc->x, lpc->y, x, y, lpc->width, 0);
		line->val.obj.layer_id = lpc->level;
		line->val.obj.lyt = lpc->lyt;
		line->loc_line = rctx->line;
	}

	if (pads_has_field(rctx)) {
		double starta, deltaa, r;
		rnd_coord_t bbx1, bby1, bbx2, bby2, cx, cy;

		if ((res = pads_read_degp10(rctx, &starta)) <= 0) return res;
		if ((res = pads_read_degp10(rctx, &deltaa)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bbx1)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bby1)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bbx2)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &bby2)) <= 0) return res;

		bbx1 += lpc->xo; bby1 += lpc->yo;
		bbx2 += lpc->xo; bby2 += lpc->yo;
		starta -= 180.0;

		r = (double)(bbx2 - bbx1) / 2.0;
		cx = rnd_round((double)(bbx1 + bbx2) / 2.0);
		cy = rnd_round((double)(bby1 + bby2) / 2.0);
		rnd_trace("  crd arc %mm;%mm %f..%f r=%mm center=%mm;%mm\n", x, y, starta, deltaa, (rnd_coord_t)r, cx, cy);
		{
			pcb_dlcr_draw_t *arc = pcb_dlcr_arc_new(&rctx->dlcr, cx, cy, r, starta, deltaa, lpc->width, 0);
			arc->val.obj.layer_id = lpc->level;
			arc->val.obj.lyt = lpc->lyt;
			arc->loc_line = rctx->line;

			/* need to determine the endpoint because of the incremental drawing;
			   for this, temporarily swap the angles (because of the y flip), check
			   which endpoint differs from the one we started from and use that, 
			   then restore the original angles */
			arc->val.obj.obj.arc.StartAngle = RND_SWAP_ANGLE(arc->val.obj.obj.arc.StartAngle);
			arc->val.obj.obj.arc.Delta = RND_SWAP_ANGLE(arc->val.obj.obj.arc.Delta);
			pcb_arc_get_end(&arc->val.obj.obj.arc, 0, &xe, &ye);
			if ((xe == x) && (ye == y))
				pcb_arc_get_end(&arc->val.obj.obj.arc, 1, &xe, &ye);
			x = xe;
			y = ye;
			arc->val.obj.obj.arc.StartAngle = starta;
			arc->val.obj.obj.arc.Delta = deltaa;
			rnd_trace("      end! %mm;%mm\n", x, y);
		}
	}
	else {
		rnd_trace("  crd move %mm;%mm\n", x, y);
	}

	lpc->x = x;
	lpc->y = y;

	pads_eatup_till_nl(rctx);

	return 1;
}

static int pads_parse_piece_circle(pads_read_ctx_t *rctx, pads_line_piece_t *lpc, long num_crds, int filled)
{
	rnd_coord_t x1, y1, x2, y2, cx, cy, dummy;
	pcb_dlcr_draw_t *arc;
	int n, res;

	if (num_crds < 2) {
		PADS_ERROR((RND_MSG_ERROR, "A circle needs to have at least two coords\n"));
		return -1;
	}


	if ((res = pads_read_coord(rctx, &x1)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y1)) <= 0) return res;
	pads_eatup_till_nl(rctx);
	if ((res = pads_read_coord(rctx, &x2)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y2)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	num_crds -= 2;
	if (num_crds > 0) {
		PADS_ERROR((RND_MSG_ERROR, "Circle with more than two coords - ignored\n(please report this bug with the file!)\n"));
		for(n = 0; n < num_crds; n++) {
			if ((res = pads_read_coord(rctx, &dummy)) <= 0) return res;
			if ((res = pads_read_coord(rctx, &dummy)) <= 0) return res;
			pads_eatup_till_nl(rctx);
		}
		exit(1);
	}


	cx = rnd_round((x1+x2)/2.0 + lpc->xo);
	cy = rnd_round((y1+y2)/2.0 + lpc->yo);

	if (filled) {
		double d = rnd_round(rnd_distance(x1, y1, x2, y2));
		pcb_dlcr_draw_t *line = pcb_dlcr_line_new(&rctx->dlcr, cx, cy, cx, cy, d, 0);
		line->val.obj.layer_id = lpc->level;
		line->val.obj.lyt = lpc->lyt;
		line->loc_line = rctx->line;
	}
	else {
		double r = rnd_round(rnd_distance(x1, y1, x2, y2)/2.0);
		arc = pcb_dlcr_arc_new(&rctx->dlcr, cx, cy, r, 0, 360, lpc->width, 0);
		arc->val.obj.layer_id = lpc->level;
		arc->val.obj.lyt = lpc->lyt;
		arc->loc_line = rctx->line;
	}
	return 1;
}


static int pads_parse_piece(pads_read_ctx_t *rctx, pads_line_type_t ltype, rnd_coord_t xo, rnd_coord_t yo, int in_partdecal)
{
	char ptype[32], rest[32];
	long n, num_crds, layer = -100, lstyle;
	rnd_coord_t width;
	int res;
	pads_line_piece_t lpc = {0};

	if ((res = pads_read_word(rctx, ptype, sizeof(ptype), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_crds)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &width)) <= 0) return res;
	if (((rctx->ver >= 9.4) && (rctx->ver < 1000)) || (floor(rctx->ver) > 2007)) {
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
	lpc.lyt = 0;
	lpc.xo = xo;
	lpc.yo = yo;

	if (in_partdecal) {
		/* in partdecals layer 1 and 2 are special: top and bottom silk */
		switch(lpc.level) {
			case 1: lpc.lyt = PCB_LYT_TOP | PCB_LYT_SILK; lpc.level = PCB_DLCR_INVALID_LAYER_ID; break;
			case 2: lpc.lyt = PCB_LYT_BOTTOM | PCB_LYT_SILK; lpc.level = PCB_DLCR_INVALID_LAYER_ID; break;
		}
	}

TODO("''KEEPOUT'' objects are not handled: CIRCUT and COPCUT and COPCIR are polygon holes; requires powerpcb to check");
	if ((strcmp(ptype, "CIRCLE") == 0) || (strcmp(ptype, "BRDCIR") == 0)) {
		if ((res = pads_parse_piece_circle(rctx, &lpc, num_crds, 0)) <= 0) return res;
	}
	else if ((strcmp(ptype, "COPCCO") == 0) || (strcmp(ptype, "COPCIR") == 0) || (strcmp(ptype, "CIRCUR") == 0)) {
		if ((res = pads_parse_piece_circle(rctx, &lpc, num_crds, 1)) <= 0) return res;
	}
	else if (strcmp(ptype, "COPCLS") == 0) {
TODO("read COPCLS here in another branch of the else if - should be poly");
		goto todo1;
	}
	else {
		/* "CLOSED" comes here: it is reallt unfilled polygon */
		todo1:;
		for(n = 0; n < num_crds; n++)
			if ((res = pads_parse_piece_crd(rctx, &lpc, n)) <= 0) return res;
	}

	return 1;
}

typedef enum { /* bitfield */
	TEXT_IS_LABEL = 1,     /* subcircuit label which may be a dyntext */
	TEXT_IN_LAST_SUBC = 2, /* in_subc means create text in the last subc placed */
	TEXT_SKIP_REFDES = 4,
	TEXT_SUBC_RELATIVE = 8
} pads_text_flags_t;

static int pads_parse_text(pads_read_ctx_t *rctx, rnd_coord_t xo, rnd_coord_t yo, pads_text_flags_t how)
{
	pcb_dlcr_draw_t *text;
	char name[16], hjust[16], vjust[16], font[128], str[1024];
	rnd_coord_t x, y, thick, h, str_width;
	double rot, anx = 0, any = 0;
	long level, flg = 0;
	int res, mirr = 0;
	rnd_bool is_label = !!(how & TEXT_IS_LABEL);
	rnd_bool in_subc  = !!(how & TEXT_IN_LAST_SUBC);
	rnd_coord_t anchx, anchy;
	pcb_text_mirror_t tmir = 0;

	*font = *str = *hjust = *vjust = '\0';

	if (is_label)
		if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;
	if ((res = pads_read_double(rctx, &rot)) <= 0) return res;
	if ((res = pads_read_long(rctx, &level)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &h)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &thick)) <= 0) return res;

	TODO("make this configurable:");
	if ((thick <= 0) || (thick >= h/4)) {
		PADS_ERROR((RND_MSG_ERROR, "invalid text thickness: %mm (for height %mm) - adjusted\n", thick, h));
		thick = h/16;
	}

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

	if (strcmp(hjust, "CENTER") == 0)     anx = 0.5;
	else if (strcmp(hjust, "LEFT") == 0)  anx = 0.0;
	else if (strcmp(hjust, "RIGHT") == 0) anx = 1.0;
	else PADS_ERROR((RND_MSG_ERROR, "invalid text horizontal adjustment: %s\n", hjust));

	if ((res = pads_read_word(rctx, vjust, sizeof(vjust), 0)) <= 0) return res;
	if (strcmp(vjust, "CENTER") == 0)     any = 0.5;
	else if (strcmp(vjust, "UP") == 0)    any = 0.0;
	else if (strcmp(vjust, "DOWN") == 0)  any = 1.0;
	else PADS_ERROR((RND_MSG_ERROR, "invalid text vertical adjustment: %s\n", vjust));

	/* fields ignored: ndim and .REUSE.*/
	pads_eatup_till_nl(rctx);

	if (rctx->ver >= 6.0) { /* 4.0 and 5.0 don't have font */
		if ((res = pads_read_word(rctx, font, sizeof(font), 0)) <= 0) return res;
		pads_eatup_till_nl(rctx);
	}

	if ((res = pads_read_word_all(rctx, str, sizeof(str), 1)) <= 0) return res;

	rnd_trace(" text: [%s] at %mm;%mm rot=%f height=%mm thick=%mm mirr=%d: '%s'",
		font, x, y, rot, h, thick, mirr, str);
	if (is_label)
		rnd_trace(" (%s)\n", name);
	else
		rnd_trace("\n");

	if (is_label) {
		if (strcmp(str, "Part Type") == 0)
			goto skip;
		if (strcmp(str, "Ref.Des.") == 0) {
			if (how & TEXT_SKIP_REFDES)
				goto skip;
			strcpy(str, "%a.parent.refdes%");
			flg |= PCB_FLAG_DYNTEXT;
		}
	}

	str_width = rnd_round(h / 1.9 * strlen(str));
	anchx = rnd_round(str_width * anx);
	anchy = rnd_round(h * any);

	text = pcb_dlcr_text_by_bbox_new(&rctx->dlcr, pcb_font(rctx->pcb, 0, 1),
		x+xo, y+yo+h, str_width, h, anchx, anchy, 1, tmir, rot, thick, str,
		(is_label ? PCB_FLAG_FLOATER : 0) | flg);

	text->loc_line = rctx->line;
	if (in_subc) {
		text->in_last_subc = 1;
		if (how & TEXT_SUBC_RELATIVE)
			text->subc_relative = 1;
	}

	if (is_label) {
		text->val.obj.layer_id = PCB_DLCR_INVALID_LAYER_ID;
#if 0
		switch(level) {
			case 1: text->val.obj.lyt = PCB_LYT_TOP | PCB_LYT_SILK; break;
			case 2: text->val.obj.lyt = PCB_LYT_BOTTOM | PCB_LYT_SILK; break;
			default:
				PADS_ERROR((RND_MSG_ERROR, "invalid label level %ld\n", level));
				text->val.obj.layer_id = level;
				break;
		}
#endif
		/* batch3/llevel2 and batch3/llevel3 shows that a label on any level
		   will end up on primary side silk. */
		text->val.obj.lyt = PCB_LYT_TOP | PCB_LYT_SILK;
	}
	else
		text->val.obj.layer_id = level;

	skip:;
	pads_eatup_till_nl(rctx);

	return 1;
}

static int pads_parse_text0(pads_read_ctx_t *rctx)
{
	return pads_parse_text(rctx, 0, 0, 0);
}

static int pads_parse_texts(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_text0);
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
		if ((res = pads_parse_piece(rctx, ltype, xo, yo, 0)) <= 0) return res;
	for(n = 0; n < num_texts; n++)
		if ((res = pads_parse_text(rctx, xo, yo, 0)) <= 0) return res;

	return 1;
}

static int pads_parse_lines(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_line);
}

#define PADS_TERM_NAME_LEN 32
typedef struct pads_term_s pads_term_t;

struct pads_term_s {
	rnd_coord_t x, y;
	long loc_line;
	pads_term_t *next;
	char name[PADS_TERM_NAME_LEN];
};

static void pads_create_pins(pads_read_ctx_t *rctx, pads_term_t *first, long pinidx, long pid)
{
	pads_term_t *t, *tnext;
rnd_trace("  pin create: %ld pid=%ld first=%p\n", pinidx, pid, first);
	for(t = first; t != NULL; t = tnext) {
		pcb_dlcr_draw_t *pin = pcb_dlcr_via_new(&rctx->dlcr, t->x, t->y, 0, pid, NULL, t->name);
		if (pin != NULL)
			pin->loc_line = t->loc_line;
rnd_trace("    %mm;%mm (%p) %p pid=%ld\n", t->x, t->y, t, pin, pid);
		tnext = t->next;
		free(t);
	}

}

static void free_terms(pads_read_ctx_t *rctx, vtp0_t *terms, long defpid)
{
	long n;
	for(n = 0; n < terms->used; n++)
		pads_create_pins(rctx, terms->array[n], -1, defpid); /* create pins using proto 0 */
	vtp0_uninit(terms);
}

static int pads_parse_pstk_optf(pads_read_ctx_t *rctx, int line, int has_arg)
{
	if (line == 0) return 1; /* first line must have all optional args */
	return has_arg;
}

#define OPTF \
	pads_parse_pstk_optf(rctx, n, pads_has_field(rctx))

static int pads_parse_pstk_proto(pads_read_ctx_t *rctx, vtp0_t *terms, long *defpid)
{
	pcb_pstk_proto_t *proto;
	pcb_pstk_tshape_t *ts;
	char name[256], shape[8], pinno[10];
	rnd_coord_t drill = 0, size;
	long sn, n, num_lines, level, start = 0, end = 0, pid;
	int res, has_slot = 0;

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if (terms != NULL) {
		if (strcmp(name, "PAD") != 0) {
			PADS_ERROR((RND_MSG_ERROR, "In *PARTDECAL* context a padstack definition needs to start with PAD, not '%s'\n", name));
			return -1;
		}
		if ((res = pads_read_word(rctx, pinno, sizeof(pinno), 0)) <= 0) return res;
	}
	else {
		if ((res = pads_read_coord(rctx, &drill)) <= 0) return res;
	}
	if ((res = pads_read_long(rctx, &num_lines)) <= 0) return res;
	if (pads_has_field(rctx))
		if ((res = pads_read_long(rctx, &start)) <= 0) return res;
	if (pads_has_field(rctx))
		if ((res = pads_read_long(rctx, &end)) <= 0) return res;

	pads_eatup_till_nl(rctx);

	proto = pcb_dlcr_pstk_proto_new(&rctx->dlcr, &pid);
	pcb_pstk_proto_change_name(proto, name, 0);
	proto->hdia = drill;

	ts = pcb_vtpadstack_tshape_get(&proto->tr, 0, 1);
	ts->shape = calloc(sizeof(pcb_pstk_shape_t), num_lines);
	ts->len = num_lines;

	rnd_trace(" pstk_proto: %s drill=%mm [%ld..%ld] pr=%p ts=%p\n", name, drill, start, end, proto, ts);

	for(n = sn = 0; n < num_lines; n++) {
		double rot = 0, slotrot = 0, spokerot = 0;
		char plated[8];
		int c, is_thermal, has_corner = 0;
		long spoke_num = 0;
		rnd_coord_t finlen = 0, finoffs = 0, inner = 0, corner = 0, drill = 0, slotlen = 0, slotoffs = 0, spoke_outsize = 0, spoke_width = 0;
		pcb_pstk_shape_t *shp = ts->shape + sn;

		if ((res = pads_read_long(rctx, &level)) <= 0) return res;
		if ((res = pads_read_coord(rctx, &size)) <= 0) return res;
		if ((res = pads_read_word(rctx, shape, sizeof(shape), 0)) <= 0) return res;

		if (shape[0] == 'A') /* only annular pad has inner dia */
			if (OPTF) if ((res = (pads_read_coord(rctx, &inner))) <= 0) return res;

		if (shape[1] != 'T') { /* normal pad (T is for thermal) */
			is_thermal = 0;
			if (shape[1] == 'F') { /* finger-type pads have rotation */
				if (OPTF) if ((res = pads_read_double(rctx, &rot)) <= 0) return res;
				if (OPTF) if ((res = pads_read_coord(rctx, &finlen)) <= 0) return res;
				if (OPTF) if ((res = pads_read_coord(rctx, &finoffs)) <= 0) return res;
				if (pads_has_field(rctx) && (shape[0] == 'R')) { /* RF=rectangular finger */
					if ((res = pads_read_coord(rctx, &corner)) <= 0) return res;
					has_corner = 1;
				}
			}
			else if (pads_has_field(rctx) && shape[0] == 'S') { /* S=square */
				if ((res = pads_read_coord(rctx, &corner)) <= 0) return res;
				has_corner = 1;
			}

			/* next word is either drill diameter or P/N for plated-or-not */
			pads_eatup_ws(rctx);
			c = fgetc(rctx->f);
			ungetc(c, rctx->f);

			/* optional drill */
			if (pads_has_field(rctx) && (!isalpha(c))) {
				if ((res = pads_read_coord(rctx, &drill)) <= 0) return res;
				proto->hdia = drill;
			}
			else if (has_corner) {
				/* special case: for Square and RF: if corner is not followed by drill, corner must be the drill instead (corner is optional) */
				proto->hdia = corner;
				corner = 0;
			}

			plated[0] = '\0';
			if (pads_has_field(rctx))
				if ((res = pads_read_word(rctx, plated, sizeof(plated), 0)) <= 0) return res;

			/* optional slot */
			if (pads_has_field(rctx)) {
				if (OPTF) if ((res = pads_read_double(rctx, &slotrot)) <= 0) return res;
				if (OPTF) if ((res = pads_read_coord(rctx, &slotlen)) <= 0) return res;
				if (OPTF) if ((res = pads_read_coord(rctx, &slotoffs)) <= 0) return res;

				if (slotlen > 0) {
					if (!has_slot) {
						TODO("Create slot; requires powerpcb for testing; pstk_slot.asc; need to redo that in batch4 with warnings fixed");
					}
					has_slot = 1;
				}
			}
		}
		else { /* thermal */
			is_thermal = 1;
			if (OPTF) if ((res = pads_read_double(rctx, &spokerot)) <= 0) return res;
			if (OPTF) if ((res = pads_read_coord(rctx, &spoke_outsize)) <= 0) return res;
			if (OPTF) if ((res = pads_read_coord(rctx, &spoke_width)) <= 0) return res;
			if (OPTF) if ((res = pads_read_long(rctx, &spoke_num)) <= 0) return res;
		}

		if ((plated[0] == 'P') || (plated[0] == '\0'))
			proto->hplated = 1;

		pads_eatup_till_nl(rctx);
		rnd_trace("  lev=%ld size=%mm/%mm shape=%s is_thermal=%d\n", level, size, inner, shape, is_thermal);

		shp->shape = -1;

		/* create the shape in shp */
		if (((shape[0] == 'R') || (shape[0] == 'A')) && (shape[1] == '\0')) {
			if (size != 0) {
				shp->shape = PCB_PSSH_CIRC;
				shp->data.circ.x = shp->data.circ.y = 0;
				shp->data.circ.dia = size;
			}
		}
		else if ((shape[0] == 'S') && (shape[1] == '\0')) {
			rnd_coord_t r2 = rnd_round(size / 2.0);
			if (r2 != 0) {
				shp->shape = PCB_PSSH_POLY;
				pcb_pstk_shape_alloc_poly(&shp->data.poly, 4);
				shp->data.poly.x[0] = -r2; shp->data.poly.y[0] = -r2;
				shp->data.poly.x[1] = +r2; shp->data.poly.y[1] = -r2;
				shp->data.poly.x[2] = +r2; shp->data.poly.y[2] = +r2;
				shp->data.poly.x[3] = -r2; shp->data.poly.y[3] = +r2;
			}
		}
		else if ((shape[0] == 'R') && (shape[1] == 'F') && (shape[2] == '\0')) {
			rnd_coord_t r1 = rnd_round(finlen / 2.0), r2 = rnd_round(size / 2.0);
			if (r2 != 0) {

				shp->shape = PCB_PSSH_POLY;
				pcb_pstk_shape_alloc_poly(&shp->data.poly, 4);
				shp->data.poly.x[0] = -r1 + finoffs; shp->data.poly.y[0] = -r2;
				shp->data.poly.x[1] = +r1 + finoffs; shp->data.poly.y[1] = -r2;
				shp->data.poly.x[2] = +r1 + finoffs; shp->data.poly.y[2] = +r2;
				shp->data.poly.x[3] = -r1 + finoffs; shp->data.poly.y[3] = +r2;

				if (rot != 0) {
					double cs = cos(rot / RND_RAD_TO_DEG), sn = sin(rot / RND_RAD_TO_DEG);
					int n;
					for(n = 0; n < shp->data.poly.len; n++)
						rnd_rotate(&shp->data.poly.x[n], &shp->data.poly.y[n], 0, 0, cs, sn);
				}
			}
		}
		else if ((shape[0] == 'O') && (shape[1] == 'F') && (shape[2] == '\0')) {
			rnd_coord_t r1 = rnd_round(finlen / 2.0), r2 = rnd_round(size / 2.0);
			if ((r1 != 0) && (r2 != 0)) {
				shp->shape = PCB_PSSH_LINE;
				shp->data.line.x1 = -r1 + r2 + finoffs; shp->data.line.y1 = 0;
				shp->data.line.x2 = +r1 - r2 + finoffs; shp->data.line.y1 = 0;
				shp->data.line.thickness = size;
				if (rot != 0) {
					double cs = cos(rot / RND_RAD_TO_DEG), sn = sin(rot / RND_RAD_TO_DEG);
					rnd_rotate(&shp->data.line.x1, &shp->data.line.y1, 0, 0, cs, sn);
					rnd_rotate(&shp->data.line.x2, &shp->data.line.y2, 0, 0, cs, sn);
				}
			}
		}
		else { /* final fallback so that we have a prototype to draw */
			shp->shape = PCB_PSSH_CIRC;
			shp->data.circ.x = shp->data.circ.y = 0;
			shp->data.circ.dia = RND_MM_TO_COORD(0.5);
			if ((shape[0] == 'O') && (shape[1] == '\0')) {
				/* O: ring shape; see batch3/pstk_o.asc; our padstack model could capture
				   this as an almost closed U shaped poly? But it's most probably never
				   used outside of normal holed pin/via */
				PADS_ERROR((RND_MSG_WARNING, "Padstack shape 'O' is not fully supported '%s'; used a circle assuming the hole will make it a ring\n", shape));
			}
			else
				PADS_ERROR((RND_MSG_ERROR, "failed to understand padstack shape '%s'; created dummy small circle\n", shape));
		}

		if (shp->shape != -1) {
			shp->dlcr_psh_layer_id = level; /* ugly hack: save the layer id for now */
			sn++;
		}
	}
	ts->len = sn;

	if ((proto->hdia <= 0) && !has_slot)
		proto->hplated = 0;
	pcb_pstk_proto_update(proto);

	if (terms != NULL) {
		pads_term_t *t = NULL, **tp;
		char *end;
		int pinidx = strtol(pinno, &end, 10);

		if (*end != '\0') {
			PADS_ERROR((RND_MSG_ERROR, "*PARTDECAL* invalid pin number '%s' (needs to be an integer)\n", pinno));
			return -1;
		}

		if (pinidx == 0)
			*defpid = pid;

		tp = (pads_term_t **)vtp0_get(terms, pinidx, 0);
		if (tp != NULL) {
			t = *tp;
			*tp = NULL;
		}
		if ((t == NULL) && (pinno[0] != '0') && (pinno[1] != '\0'))
			PADS_ERROR((RND_MSG_ERROR, "*PARTDECAL* padstack proto referencing non-existing pin number or duplicate pin number reference: '%s'\n", pinno));
		pads_create_pins(rctx, t, pinidx, pid);
	}
	return 1;
}
#undef OPTF

static int pads_parse_via_pstk_proto(pads_read_ctx_t *rctx)
{
	return pads_parse_pstk_proto(rctx, NULL, NULL);
}

static int pads_parse_vias(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_via_pstk_proto);
}

static int pads_parse_term(pads_read_ctx_t *rctx, long idx, vtp0_t *terms)
{
	pads_term_t *t, **tp;
	char name[PADS_TERM_NAME_LEN];
	int c, res;
	long loc_line = rctx->line;
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
		sprintf(name, "%ld", idx+1);

	pads_eatup_till_nl(rctx);

	rnd_trace("  terminal: '%s' at %mm;%mm IDX=%ld\n", name, x, y, idx+1);
	tp = (pads_term_t **)vtp0_get(terms, idx+1, 0);
	t = calloc(sizeof(pads_term_t), 1);
	t->x = x;
	t->y = y;
	strcpy(t->name, name);
	t->loc_line = loc_line;
	if (tp != NULL)
		t->next = *tp;
	vtp0_set(terms, idx+1, t);
	
	return 1;
}

static int pads_parse_partdecal(pads_read_ctx_t *rctx)
{
	char name[256], unit[8];
	rnd_coord_t xo = 0, yo = 0, dummy;
	long n, num_pieces, num_terms, num_stacks, num_texts = 0, num_labels = 0, defpid = -1;
	int res;
	pcb_subc_t *subc;
	vtp0_t terms = {0};
	double old_coord_scale;

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, unit, sizeof(unit), 0)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &dummy)) <= 0) return res; /* these are for the footprint editor, ignore them for the board */
	if ((res = pads_read_coord(rctx, &dummy)) <= 0) return res;
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

	old_coord_scale = rctx->coord_scale;
	if (rctx->coord_scale > 1) { /* it seems if the file is in BASIC mode, this field should be ignored because partdecals are in BASIC too */
		if ((*unit == 'I') || (*unit == 'i'))
			rctx->coord_scale = RND_MIL_TO_COORD(1);
		else if ((*unit == 'M') || (*unit == 'm'))
			rctx->coord_scale = RND_MM_TO_COORD(1.0);
	}


	subc = pcb_dlcr_subc_new_in_lib(&rctx->dlcr, name);
	pcb_dlcr_subc_begin(&rctx->dlcr, subc);

	for(n = 0; n < num_pieces; n++)
		if ((res = pads_parse_piece(rctx, PLTY_LINES, xo, yo, 1)) <= 0) return res;
	for(n = 0; n < num_texts; n++)
		if ((res = pads_parse_text(rctx, xo, yo, 0)) <= 0) return res;
	for(n = 0; n < num_labels; n++)
		if ((res = pads_parse_text(rctx, xo, yo, TEXT_IS_LABEL | TEXT_SKIP_REFDES)) <= 0) return res;
	for(n = 0; n < num_terms; n++)
		if ((res = pads_parse_term(rctx, n, &terms)) <= 0) { free_terms(rctx, &terms, defpid); return res; }
	for(n = 0; n < num_stacks; n++)
		if ((res = pads_parse_pstk_proto(rctx, &terms, &defpid)) <= 0) { free_terms(rctx, &terms, defpid); return res; }

	free_terms(rctx, &terms, defpid);

	pcb_dlcr_subc_end(&rctx->dlcr);

	rctx->coord_scale = old_coord_scale;

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
	pads_read_part_t *part;
	char partname[64], decals[16*64], unit[4], ptype[8], *s;
	long n, num_gates, num_signals, num_alpins, flags;
	int res, dnl;

	if ((res = pads_read_word(rctx, partname, sizeof(partname), 0)) <= 0) return res;
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

	rnd_trace("parttype: '%s' -> '%s' gates=%ld signals=%ld alpins=%ld\n", partname, decals, num_gates, num_signals, num_alpins);

	part = htsp_get(&rctx->parts, partname);
	if (part != NULL) {
		PADS_ERROR((RND_MSG_ERROR, "*PARTTYPE* called '%s' is defined multiple times\n", partname));
		return -1;
	}

	dnl = strlen(decals)+1;
	part = calloc(sizeof(pads_read_part_t) + dnl + 2, 1);
	memcpy(part->decal_names, decals, dnl);
	part->decal_names_len = dnl+1;
	htsp_set(&rctx->parts, rnd_strdup(partname), part);

	/* the decal name list should consist of null-terminated string with an extra
	   empty string at the end (implied by calloc() size extra above) */
	for(s = part->decal_names; *s != '\0'; s++)
		if (*s == ':')
			*s = '\0';

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
	pads_read_part_t *part;
	char refdes[64], partname[64], glue[4], mirr[4], *decalname;
	long n, altdeclnum, clustid = -1, clsattr = 0, brotherid = -1, num_labels = 0;
	rnd_coord_t xo, yo;
	double rot;
	int res, dln;

	if ((res = pads_read_word(rctx, refdes, sizeof(refdes), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, partname, sizeof(partname), 0)) <= 0) return res;
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

	rnd_trace("part: '%s' of '%s' num_labels=%ld\n", refdes, partname, num_labels);

	/* if there's a @ in the partname, it's really a partname@decalname where
	   decalname is a locally modified version of the original decal; we need
	   to use the decalname directly - but probably still use the part for pin
	   swaps? */
	decalname = strchr(partname, '@');
	if (decalname == NULL) {
		part = htsp_get(&rctx->parts, partname);
		decalname = part->decal_names;
		dln = part->decal_names_len;
	}
	else {
		*decalname = '\0';
		decalname++;
		part = htsp_get(&rctx->parts, partname); /* do the lookup so that part is valid, for a later pin swap implementation */
		dln = strlen(decalname);
	}

	if (part != NULL) {
		pcb_dlcr_draw_t *po = pcb_dlcr_subc_new_from_lib(&rctx->dlcr, xo, yo, rot, (*mirr == 'M'), decalname, dln);
		po->loc_line = rctx->line;
		pcb_dlcr_attrib_set_prev(&rctx->dlcr, "refdes", refdes);
	}
	else {
		PADS_ERROR((RND_MSG_ERROR, "*PART* on undefined parttype '%s'\n", partname));
		return -1;
	}


	for(n = 0; n < num_labels; n++)
		if ((res = pads_parse_text(rctx, 0, 0, TEXT_IS_LABEL | TEXT_IN_LAST_SUBC | TEXT_SUBC_RELATIVE)) <= 0) return res;
	return 1;
}

static int pads_parse_parts(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_part);
}

static void get_arc_angles(rnd_coord_t r, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t x0, rnd_coord_t y0, rnd_coord_t x1, rnd_coord_t y1, int arcdir, double *starta, double *deltaa)
{
	double s, e, d;

	if (((x0 == cx) && (y0 == cy)) || ((x1 == cx) && (y1 == cy))) {
		*starta = *deltaa = 0;
		return;
	}

	/* pcb-rnd's own (unusual) arc representation */
	s = atan2(y0 - cy, -(x0 - cx));
	e = atan2(y1 - cy, -(x1 - cx));
	d = e - s;

	/* flip arc dir upon mismatching CW or CCW */
	if ((d < 0) && (arcdir < 0))
		d = 2*M_PI + d;
	else if ((d > 0) && (arcdir > 0))
		d = d - 2*M_PI;

	*starta = s;
	*deltaa = d;
}

static void add_teardrop(void *rctx_, pcb_any_obj_t *obj, void *call_ctx)
{
	pads_read_ctx_t *rctx = rctx_;
	pcb_layer_t *ly;
	pcb_data_t *dt;

	if (!conf_io_pads.plugins.io_pads.load_teardrops)
		return;

	if (obj->parent_type != PCB_PARENT_LAYER)
		return;

	ly = obj->parent.layer;
	assert(ly->parent_type == PCB_PARENT_DATA);
	dt = ly->parent.data;
	if (dt->parent_type != PCB_PARENT_BOARD)
		return;

	if (!(pcb_layer_flags_(ly) & PCB_LYT_COPPER)) /* teardrop makes sense on copper only */
		return;

	if ((rctx->teardrop_eo == NULL) && !rctx->teardrop_warned) {
		pcb_extobj_t **eo = NULL;
		int idx = pcb_extobj_lookup_idx("teardrop");

		if (idx != 0)
			eo = (pcb_extobj_t **)vtp0_get(&pcb_extobj_i2o, idx, 0);

		if ((eo == NULL) || (*eo == NULL)) {
			/* extobj backend got deregistered meanwhile */
			PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* line teardrop: teardrop extended object not found\nSkipping teardrops, please enable that plugin if you need them.\n"));
			rctx->teardrop_warned = 1;
		}
		rctx->teardrop_eo = *eo;
	}
	if (rctx->teardrop_eo != NULL) {
		pcb_subc_t *eo = pcb_extobj_conv_obj(rctx->pcb, rctx->teardrop_eo, rctx->pcb->Data, obj, rnd_true);
		if (eo == NULL)
			PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* line teardrop: failed to create teardrop\n"));
	}
}

typedef struct {
	rnd_coord_t x, y; /* last x;y */
	rnd_coord_t x0, y0, cx, cy; /* last arc start coords and center, when arcdir != 0 */
	int arcdir;
	int omit; /* omit next line segment because the current segment is virtual */
	int lastlev;
} pads_sig_piece_t;

static int pads_parse_signal_crd(pads_read_ctx_t *rctx, pads_sig_piece_t *spc, long idx)
{
	char vianame[64], teardrop[16], tdir[4];
	long real_level, level, flags, loc_line;
	rnd_coord_t x, y, width;
	int res, arcdir = 0, thermal = 0;
	int teardrop_dir = 0; /* -1 is previous, +1 is next */

	if ((res = pads_read_coord(rctx, &x)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &y)) <= 0) return res;
	if ((res = pads_read_long(rctx, &level)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &width)) <= 0) return res;
	if ((res = pads_read_long(rctx, &flags)) <= 0) return res;

	loc_line = rctx->line;

	*vianame = '\0';
	if (pads_has_field(rctx)) {
		if ((res = pads_read_word(rctx, vianame, sizeof(vianame), 0)) <= 0) return res;
		if (strcmp(vianame, "TEARDROP") == 0) {
			*vianame = '\0';
			goto teardrop;
		}

		if (strcmp(vianame, "CW") == 0)       { arcdir = -1; *vianame = '\0'; }
		else if (strcmp(vianame, "CCW") == 0) { arcdir = +1; *vianame = '\0'; }
		else arcdir = 0;

		if (strcmp(vianame, "THERMAL") == 0) { thermal = 1; *vianame = '\0'; }
		/* the rest of the line is jumper wire and teardrop - ignore those */
	}

	if (pads_has_field(rctx)) {
		if ((res = pads_read_word(rctx, teardrop, sizeof(teardrop), 0)) <= 0) return res;
		if (strcmp(teardrop, "TEARDROP") == 0) {
			teardrop:;
			if ((res = pads_read_word(rctx, tdir, sizeof(tdir), 0)) <= 0) return res;
			if (tdir[1] != '\0') {
				PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* line teardrop: empty P/N field\n"));
				return -1;
			}
			if (tdir[0] == 'P') teardrop_dir = -1;
			else if (tdir[0] == 'N') teardrop_dir = +1;
			else {
				PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* line teardrop: need a P or N after the teardrop keyword\n"));
				return -1;
			}
			/* for now, ignore teardrop fields */
		}
	}


	pads_eatup_till_nl(rctx);

	rnd_trace("  %mm;%mm level=%ld w=%mm flags=%ld vianame=%s arcdir=%d thermal=%d\n",
		x, y, level, width, flags, vianame, arcdir, thermal);

	if ((idx == 0) && (arcdir != 0)) {
		PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* can not start with an arc\n"));
		return -1;
	}

	if (flags & 0x01000) {
		if (arcdir != 0) {
			spc->arcdir = arcdir;
			spc->x0 = spc->x; spc->y0 = spc->y;
			spc->cx = x; spc->cy = y;
			spc->omit = 0;
			return 1;
		}
		else {
			PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* arc arguments without the arc flag\n"));
			return -1;
		}
	}

	real_level = level;
	if (*vianame != '\0') {
		pcb_dlcr_draw_t *via;
		/* in this case level is the via's target level and our trace segment is
		   really on the same level as the previous */
		level = spc->lastlev;
		via = pcb_dlcr_via_new(&rctx->dlcr, x, y, 0, -1, vianame, rctx->signal_netname);
		if (via != NULL)
			via->loc_line = loc_line;
	}

	TODO("do not ignore the miter flag: 0x0e000: requires original powerpcb to check\n");

	if (level == 65)
		level = spc->lastlev;

	if (spc->arcdir != 0) {
		if (level != 0) {
			if (!spc->omit) {
				pcb_dlcr_draw_t *arc;
				rnd_coord_t r = rnd_distance(spc->cx, spc->cy, spc->x0, spc->y0);
				double starta, deltaa;
				get_arc_angles(r, spc->cx, spc->cy, spc->x0, spc->y0, x, y, spc->arcdir, &starta, &deltaa);
				arc = pcb_dlcr_arc_new(&rctx->dlcr, spc->cx, spc->cy, r, starta * RND_RAD_TO_DEG, deltaa * RND_RAD_TO_DEG, width, 0);
				if (arc != NULL) {
					arc->val.obj.layer_id = level;
					arc->loc_line = loc_line;
				}
				spc->arcdir = 0;
			}
			spc->omit = 0;
		}
		else
			spc->omit = 1;
	}
	else {
		if (level != 0) {
/*rnd_printf("*** line: %ld idx=%d omit=%d level=%d %mm %mm to %mm %mm\n", loc_line, idx, spc->omit, level, spc->x, spc->y, x, y);*/
			if ((idx > 0) && (!spc->omit)) {
				pcb_dlcr_draw_t *line = pcb_dlcr_line_new(&rctx->dlcr, spc->x, spc->y, x, y, width, 0);
				if (line != NULL) {
					line->val.obj.layer_id = level;
					line->loc_line = loc_line;
				}
				if (teardrop_dir != 0)
					pcb_dlcr_call_on(&rctx->dlcr, add_teardrop, rctx, NULL, (teardrop_dir > 0));
			}
			spc->omit = 0;
		}
		else
			spc->omit = 1;
	}

	spc->x = x; spc->y = y;
	spc->lastlev = real_level;
	return 1;
}

static void net_add_term(pads_read_ctx_t *rctx, pcb_net_t *net, char *pinname)
{
	char *refdes = pinname, *term = strchr(pinname, '.');

	if (term == NULL) {
		PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* terminal name error: '%s' is not refdes.pin\nPlease report this bug!\n"));
		return;
	}

	*term = '\0';
	term++;

	if (pcb_net_term_get(net, refdes, term, 1) == NULL)
		PADS_ERROR((RND_MSG_ERROR, "*SIGNAL* failed to create terminal : '%s:%s'\nPlease report this bug!\n", refdes, term));
}

static int pads_parse_net(pads_read_ctx_t *rctx)
{
	char term1[128], term2[128];
	long idx;
	int c, res;
	pads_sig_piece_t spc = {0};
	pcb_net_t *net;

	next_trace:;

	if ((res = pads_read_word(rctx, term1, sizeof(term1), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, term2, sizeof(term2), 0)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	rnd_trace(" '%s' -> '%s'\n", term1, term2);

	net = pcb_net_get(rctx->pcb, &rctx->pcb->netlist[PCB_NETLIST_INPUT], rctx->signal_netname, 1);
	net_add_term(rctx, net, term1);
	net_add_term(rctx, net, term2);

	for(idx = 0; ;idx++) {
		double tmp;

		/* look ahead for the first non-whitespace, non-newline character; it's
		   either the next coord, or the next trace in the same net (refdes),
		   or the next section ('*') */
		while(!(feof(rctx->f))) {
			pads_eatup_ws(rctx);
			c = fgetc(rctx->f);
			if ((c == '\r') || (c == '\n'))
				pads_update_loc(rctx, c);
			else
				break;
		}
		ungetc(c, rctx->f);
		if (c == '*')
			return -4;
		if (!isspace(c) && !isdigit(c)) /* if the line starts with a refdes, it's the next trace for the same net */
			goto next_trace;

		if (pads_maybe_read_double(rctx, pads_saved_word, sizeof(pads_saved_word), &tmp) == 1)
			break; /* when next word is a string; also save the next word so it is returned in the next read */

		if ((res = pads_parse_signal_crd(rctx, &spc, idx)) <= 0) return res;
	}

	return 1;
}

static int pads_parse_signal(pads_read_ctx_t *rctx)
{
	char netname[128];
	int res;

	if ((res = pads_read_word(rctx, netname, sizeof(netname), 0)) <= 0) return res;

	rnd_trace("signal: netname=%s\n", netname);
	rctx->signal_netname = netname;
	res = pads_parse_list_sect(rctx, pads_parse_net);
	rctx->signal_netname = NULL;

	return res;
}

typedef enum {
	PADS_POTY_CONTOUR,
	PADS_POTY_NOOP
} pads_poly_type_t;

static int pads_parse_pour_piece_crd(pads_read_ctx_t *rctx, pcb_dlcr_draw_t *poly, pads_poly_type_t poty, rnd_coord_t xo, rnd_coord_t yo)
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

	if (poty == PADS_POTY_NOOP)
		return 1;

	if (isarc) {
		TODO("poly arc: requires powerpcb to check; batch3/polyarc* ; resulted in errors");
		rnd_trace("  arc %mm;%mm %f..%f\n", x, y, start, end);
	}
	else {
		rnd_trace("  line %mm;%mm\n", x, y);
		pcb_dlcr_poly_lineto(&rctx->dlcr, poly, x+xo, y+yo);
	}

	return 1;
}

/* draw polygon contour from pieces */
static int pads_parse_pour_piece_polycnt(pads_read_ctx_t *rctx, pads_poly_type_t poty, rnd_coord_t xo, rnd_coord_t yo, const char *signame)
{
	char type[64];
	long n, num_corners, num_arcs, level, loc = rctx->line;
	int res;
	rnd_coord_t width;
	pcb_dlcr_draw_t *poly = NULL;

	if ((res = pads_read_word(rctx, type, sizeof(type), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_corners)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_arcs)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &width)) <= 0) return res;
	if ((res = pads_read_long(rctx, &level)) <= 0) return res;
	pads_eatup_till_nl(rctx); /* ignore rest of the arguments */

	if (!conf_io_pads.plugins.io_pads.load_polygons) {
		for(n = 0; n < num_corners + num_arcs; n++)
			if ((res = pads_parse_pour_piece_crd(rctx, NULL, PADS_POTY_NOOP, xo, yo)) <= 0) return res;
		return 1;
	}

	rnd_trace(" %s:\n", type);
	if (poty != PADS_POTY_NOOP)
		poly = pcb_dlcr_poly_new(&rctx->dlcr, 0, num_corners + num_arcs*8);
	if ((poly != NULL) || (poty == PADS_POTY_NOOP)) {
		for(n = 0; n < num_corners + num_arcs; n++)
			if ((res = pads_parse_pour_piece_crd(rctx, poly, poty, xo, yo)) <= 0) return res;
		if (poly != NULL) {
			poly->val.obj.layer_id = level;
			poly->loc_line = loc;
		}
	}

TODO("if width != 0: round corners as if poly was drawn with the line as thick as specified, with the line's _outer edge_ being the poly contour; because it probably affects not only as-drawn but clipped version too, this needs arc-in-poly support? batch3/poly_bloat*");
	if ((*signame != '\0') && (poly != NULL))
		pcb_dlcr_set_net(poly, signame);

	return 1;
}


static int pads_parse_pour(pads_read_ctx_t *rctx)
{
	char name[64], type[64], owner[64], signame[64];
	long n, num_pieces, flags = 0;
	rnd_coord_t xo, yo;
	int res;
	pads_poly_type_t poty = PADS_POTY_NOOP;

	owner[0] = signame[0] = '\0';

	if ((res = pads_read_word(rctx, name, sizeof(name), 0)) <= 0) return res;
	if ((res = pads_read_word(rctx, type, sizeof(type), 0)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &xo)) <= 0) return res;
	if ((res = pads_read_coord(rctx, &yo)) <= 0) return res;
	if ((res = pads_read_long(rctx, &num_pieces)) <= 0) return res;
	if ((pads_has_field(rctx)) && ((res = pads_read_long(rctx, &flags)) <= 0)) return res;
	if ((pads_has_field(rctx)) && ((res = pads_read_word(rctx, owner, sizeof(owner), 0)) <= 0)) return res;
	if ((pads_has_field(rctx)) && ((res = pads_read_word(rctx, signame, sizeof(signame), 0)) <= 0)) return res;
	pads_eatup_till_nl(rctx); /* ignore rest of the arguments */

	rnd_trace("pour '%s' type='%s' at %mm;%mm signame='%s'\n", name, type, xo, yo, signame);
	if ((strcmp(type, "POUROUT") == 0) || (strcmp(type, "HATOUT") == 0)) {
		poty = PADS_POTY_CONTOUR;
	}
	else if (strcmp(type, "VOIDOUT") == 0) {
		TODO("poly cutout: needs powerpcb to test");
	}
	else if (strcmp(type, "VIATHERM") == 0) {
		TODO("poly via therm: needs powerpcb to test");
	}
	else
		PADS_ERROR((RND_MSG_ERROR, "Unhandled pour type: '%s' - please report this bug\n", type));

	for(n = 0; n < num_pieces; n++)
		if ((res = pads_parse_pour_piece_polycnt(rctx, poty, xo, yo, signame)) <= 0) return res;

	return 1;
}

static int pads_parse_pours(pads_read_ctx_t *rctx)
{
	return pads_parse_list_sect(rctx, pads_parse_pour);
}

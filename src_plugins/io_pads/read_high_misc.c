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
		if (c == '{') {
			if ((res = pads_parse_misc_ignore_block(rctx)) <= 0) return res;
			continue;
		}
		pads_eatup_till_nl(rctx);
		if (c == '}')
			return 1;
	}
	return 1;
}

/* read everything till the closing of this block; recurse if more blocks are
   open */
static int pads_parse_misc_generic(pads_read_ctx_t *rctx, int (*parse_child)(pads_read_ctx_t *rctx))
{
	char open[64];

	fgets(open, sizeof(open), rctx->f);
	if (*open != '{') {
		PADS_ERROR((RND_MSG_ERROR, "Expected block open brace\n"));
		return -1;
	}

	for(;;) {
		int c, res;

		pads_eatup_ws(rctx);
		c = fgetc(rctx->f);
		if (c == EOF)
			return 0;
		ungetc(c, rctx->f);
		if (c == '{') {
			PADS_ERROR((RND_MSG_ERROR, "Unexpected block open brace\n"));
			return -1;
		}
		if (c == '}') {
			pads_eatup_till_nl(rctx);
			return 1;
		}
		res = parse_child(rctx);
		if (res <= 0)
			return res;
	}
	return 1;
}

static int pads_parse_misc_open(pads_read_ctx_t *rctx)
{
	char open[4];
	int res;

	if ((res = pads_read_word(rctx, open, sizeof(open), 0)) <= 0) return res;
	rnd_trace("open='%s'\n", open);
	if (strcmp(open, "{") != 0) {
		PADS_ERROR((RND_MSG_ERROR, "Expected block open brace\n"));
		return -1;
	}
	return 1;
}

static int pads_parse_misc_layer(pads_read_ctx_t *rctx)
{
	char key[32], val[1024];
	int res;
	pads_layer_t *pl = rctx->layer->user_data;

	if ((res = pads_read_word(rctx, key, sizeof(key), 0)) <= 0) return res;
	if ((res = pads_read_word_all(rctx, val, sizeof(val), 0)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	if (strcmp(key, "LAYER_NAME") == 0) {
		rnd_trace("  name='%s'\n", val);
		rctx->layer->name = rnd_strdup(val);
	}
	else if (strcmp(key, "LAYER_TYPE") == 0) {
		rnd_trace("  type='%s'\n", val);
		if (strcmp(val, "UNASSIGNED") == 0)         { rctx->layer->lyt = 0; }
		else if (strcmp(val, "ROUTING") == 0)       { rctx->layer->lyt |= PCB_LYT_COPPER; }
		else if (strcmp(val, "COMPONENT") == 0)     { rctx->layer->lyt |= PCB_LYT_COPPER; }
		else if (strcmp(val, "SOLDER_MASK") == 0)   { rctx->layer->lyt |= PCB_LYT_MASK; }
		else if (strcmp(val, "PASTE_MASK") == 0)    { rctx->layer->lyt |= PCB_LYT_PASTE; }
		else if (strcmp(val, "SILK_SCREEN") == 0)   { rctx->layer->lyt |= PCB_LYT_SILK; }
		else if (strcmp(val, "DRILL") == 0)         { rctx->layer->lyt |= PCB_LYT_MECH; rctx->layer->purpose = "proute"; }
		else if (strcmp(val, "ASSEMBLY") == 0)      { rctx->layer->lyt |= PCB_LYT_DOC; rctx->layer->purpose = "assy"; }
		else {
			PADS_ERROR((RND_MSG_ERROR, "Ignoring unknown layer type: %s\n", val));
		}
	}
	else if (strcmp(key, "COMPONENT") == 0) {
		rnd_trace("  component='%s'\n", val);
		if (*val == 'Y')
			rctx->layer->can_have_components = 1;
	}
	else if (strcmp(key, "ASSOCIATED_SILK_SCREEN") == 0) {
		rnd_trace("  ASSOCIATED_SILK_SCREEN='%s'\n", key, val);
		pl->assoc_silk = rnd_strdup(val);
	}
	else if (strcmp(key, "ASSOCIATED_PASTE_MASK") == 0) {
		rnd_trace("  ASSOCIATED_PASTE_MASK='%s'\n", key, val);
		pl->assoc_paste = rnd_strdup(val);
	}
	else if (strcmp(key, "ASSOCIATED_SOLDER_MASK") == 0) {
		rnd_trace("  ASSOCIATED_SOLDER_MASK='%s'\n", key, val);
		pl->assoc_mask = rnd_strdup(val);
	}
	else if (strcmp(key, "ASSOCIATED_ASSEMBLY") == 0) {
		rnd_trace("  ASSOCIATED_ASSEMBLY='%s'\n", key, val);
		pl->assoc_assy = rnd_strdup(val);
	}
	else if (strcmp(key, "COLORS") == 0) {
		if ((res = pads_parse_misc_open(rctx)) <= 0) return res;
		return pads_parse_misc_ignore_block(rctx);
	}

	return 1;
}

static int pads_parse_misc_layer_hdr(pads_read_ctx_t *rctx)
{
	char key[32];
	long id;
	int res;

	if ((res = pads_read_word(rctx, key, sizeof(key), 0)) <= 0) return res;
	if ((res = pads_read_long(rctx, &id)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	if (strcmp(key, "LAYER") != 0) {
		PADS_ERROR((RND_MSG_ERROR, "Expected LAYER block, got '%s' instead\n", key));
		return -1;
	}

	rnd_trace(" layer %ld key='%s'\n", id, key);
	rctx->layer = pads_layer_alloc(id);

	res = pads_parse_misc_generic(rctx, pads_parse_misc_layer);
	if (rctx->layer->lyt != 0)
		pcb_dlcr_layer_reg(&rctx->dlcr, rctx->layer);
	else
		pcb_dlcr_layer_free(rctx->layer);
	rctx->layer = NULL;
	return res;
}


static int pads_parse_misc_layers(pads_read_ctx_t *rctx)
{
	char unit[64];
	int res;

	if ((res = pads_read_word(rctx, unit, sizeof(unit), 0)) <= 0) return res;
	pads_eatup_till_nl(rctx);

	rnd_trace("layers: '%s'\n", unit);
	return pads_parse_misc_generic(rctx, pads_parse_misc_layer_hdr);
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
			rnd_trace("misc ignore block from %ld (%ld)!\n", rctx->line, ftell(rctx->f));
			if ((res = pads_parse_misc_ignore_block(rctx)) <= 0) return res;
			continue;
		}

		/*read key; if it is a new *SECTION* then res will be -4 */
		if ((res = pads_read_word(rctx, key, sizeof(key), 0)) <= 0)
			return res;
		if (strcmp(key, "LAYER") == 0) res = pads_parse_misc_layers(rctx);
		else if (*key != '\0') {
			pads_eatup_till_nl(rctx);
			res = 1;
		}
		if (res <= 0)
			return res;
	}
	return 1;
}

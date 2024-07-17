/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - dispatch pro format read
 *  pcb-rnd Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust Fund in 2024)
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

/*** parse objects ***/

static int easyeda_pro_parse_canvas(easy_read_ctx_t *ctx)
{
	long lineno;
	int found = 0;

	for(lineno = 0; lineno < ctx->root->value.array.used; lineno++) {
		gdom_node_t *line = ctx->root->value.array.child[lineno];

		if ((line->type != GDOM_ARRAY) || (line->name != easy_CANVAS)) continue;

		if (found) {
			error_at(ctx, line, ("multiple CANVAS nodes\n"));
			return -1;
		}
		ctx->ox = easyeda_get_double(ctx, line->value.array.child[1]);
		ctx->oy = easyeda_get_double(ctx, line->value.array.child[2]);
		found = 1;
	}

	if (!found)
		error_at(ctx, ctx->root, ("no CANVAS node; origin unknown (assuming 0;0)\n"));

	return 0;
}

static int pro_parse_layer(easy_read_ctx_t *ctx, gdom_node_t *nd, pcb_layer_type_t lyt, int easyeda_id)
{
	unsigned flags;
	int locked, visible, confd;

	if (nd == NULL)
		return 0;   /* not in the input */
	if (lyt == 0)
		return 0; /* not a valid entry in our layertab */

	flags = easyeda_get_double(ctx, nd->value.array.child[4]);
	confd   = flags & 1;
	locked  = flags & 2;
	visible = flags & 4;

	if (!confd)
		return 0; /* input did not want this layer */

	if (nd->value.array.used < 6) {
		error_at(ctx, nd, ("not enough LAYER arguments\n"));
		return -1;
	}

	if (nd->value.array.child[3]->type != GDOM_STRING) {
		error_at(ctx, nd->value.array.child[3], ("LAYER name must be a string\n"));
		return -1;
	}
	if (nd->value.array.child[5]->type != GDOM_STRING) {
		error_at(ctx, nd->value.array.child[5], ("LAYER color must be a string\n"));
		return -1;
	}

	return easyeda_layer_create(ctx, lyt, nd->value.array.child[3]->value.str, easyeda_id, nd->value.array.child[5]->value.str);

	(void)visible; (void)locked; /* supress compiler warnings on currently unused vars */
}

static int easyeda_pro_parse_layers(easy_read_ctx_t *ctx)
{
	long lineno, lid;
	gdom_node_t *lyline[EASY_MAX_LAYERS] = {0};
	int res = 0;
	const int *lt;

	/* cache each layer line per ID from input */
	for(lineno = 0; lineno < ctx->root->value.array.used; lineno++) {
		gdom_node_t *line = ctx->root->value.array.child[lineno];
		long lid;

		if ((line->type != GDOM_ARRAY) || (line->name != easy_LAYER)) continue;

		lid = easyeda_get_double(ctx, line->value.array.child[1]);
		if ((lid >= 0) && (lid < EASY_MAX_LAYERS))
			lyline[lid] = line;
	}


	/* create layers in ctx->data */
	for(lt = easyeda_layertab, lid = 1; *lt != 0; lt++,lid++) {
		if (*lt == LAYERTAB_INNER) {
			long n;
			for(n = easyeda_layertab_in_first; n <= easyeda_layertab_in_last; n++)
				res |= pro_parse_layer(ctx, lyline[n], easyeda_layertab[n-1], n);
		}
		else
			res |= pro_parse_layer(ctx, lyline[lid], *lt, lid);
	}

	res |= easyeda_create_misc_layers(ctx);

	return 0;
}



/*** glue ***/
static int easyeda_pro_parse_fp_as_board(pcb_board_t *pcb, const char *fn, FILE *f, rnd_conf_role_t settings_dest)
{
	easy_read_ctx_t ctx;
	unsigned char ul[3];
	int res = 0;
	pcb_subc_t *subc_as_board;

	ctx.pcb = pcb;
	ctx.fn = fn;
	ctx.data = pcb->Data;

	ctx.settings_dest = settings_dest;


	/* eat up the bom */
	if (fread(ul, 1, 3, f) != 3) {
		rnd_message(RND_MSG_ERROR, "easyeda pro: premature EOF on %s (bom)\n", fn);
		return -1;
	}
	if ((ul[0] != 0xef) || (ul[1] != 0xbb) || (ul[2] != 0xbf))
		rewind(f);

	/* run low level */
	ctx.root = easypro_low_parse(f);
	if (ctx.root == NULL) {
		rnd_message(RND_MSG_ERROR, "easyeda pro: failed to run the low level parser on %s\n", fn);
		return -1;
	}

	rnd_trace("load efoo as board\n");

	assert(ctx.root->type == GDOM_ARRAY);
	if (res == 0) res = easyeda_pro_parse_canvas(&ctx);
	if (res == 0) res = easyeda_pro_parse_layers(&ctx);

	subc_as_board = easyeda_subc_create(&ctx);
	ctx.data = subc_as_board->data;

	TODO("load shapes");

	easyeda_subc_finalize(&ctx, subc_as_board, 0, 0, 0);

	return res;
}

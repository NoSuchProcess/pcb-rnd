/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - std format read: high level tree parsing
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

typedef struct std_read_ctx_s {
	FILE *f;
	gdom_node_t *root;
	pcb_board_t *pcb;
	pcb_data_t *data;
	const char *fn;
	rnd_conf_role_t settings_dest;
} std_read_ctx_t;

static int std_parse_layer(std_read_ctx_t *ctx, pcb_layer_t *dst, gdom_node_t *src)
{
	return 0;
}

static int std_parse_layers(std_read_ctx_t *ctx)
{
	gdom_node_t *layers;
	long n;
	int res = 0;

	layers = gdom_hash_get(ctx->root, easy_layers);
	if ((layers == NULL) || (layers->type != GDOM_ARRAY)) {
		rnd_message(RND_MSG_ERROR, "EasyEDA std: missing or wrong typed layers tree\n");
		return -1;
	}

	for(n = 0; n < layers->value.array.used; n++) {
		if (n >= PCB_MAX_LAYER) {
			rnd_message(RND_MSG_ERROR, "Board has more layers than supported by this compilation of pcb-rnd (%d)\nIf this is a valid board, please increase PCB_MAX_LAYER and recompile.\n", PCB_MAX_LAYER);
			res = -1;
			break;
		}

		res |= std_parse_layer(ctx, &ctx->data->Layer[n], layers->value.array.child[n]);
	}



	return res;
}

static int std_parse_canvas(std_read_ctx_t *ctx)
{
	gdom_node_t *canvas;

	canvas = gdom_hash_get(ctx->root, easy_canvas);
	if ((canvas == NULL) || (canvas->type != GDOM_HASH)) {
		rnd_message(RND_MSG_ERROR, "EasyEDA std: missing or wrong typed canvas tree\n");
		return -1;
	}

	return 0;
}

static int easyeda_std_parse_board(pcb_board_t *dst, const char *fn, rnd_conf_role_t settings_dest)
{
	std_read_ctx_t ctx;
	int res = 0;

	ctx.pcb = dst;
	ctx.data = dst->Data;
	ctx.fn = fn;
	ctx.f = rnd_fopen(&dst->hidlib, fn, "r");
	ctx.settings_dest = settings_dest;
	if (ctx.f == NULL) {
		rnd_message(RND_MSG_ERROR, "filed to open %s for read\n", fn);
		return -1;
	}

	ctx.root = easystd_low_parse(ctx.f, 0);
	fclose(ctx.f);

	if (res == 0) res |= std_parse_layers(&ctx);
	if (res == 0) res |= std_parse_canvas(&ctx);

	return res;
}



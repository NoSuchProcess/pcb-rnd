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

#include <librnd/core/conf.h>
#include "global_typedefs.h"

#include <libnanojson/nanojson.h>
#include <libnanojson/semantic.h>


#include <rnd_inclib/lib_easyeda/gendom.h>
#include <rnd_inclib/lib_easyeda/gendom_json.h>


/* Return the first node that has valid location info (or fall back to root),
   starting from node, traversing up in the tree */
RND_INLINE gdom_node_t *node_parent_with_loc(gdom_node_t *node)
{
	for(;node->parent != NULL; node = node->parent)
		if (node->lineno > 0)
			return node;

	return node;
}

#define EASY_MAX_LAYERS 128
#define EASY_MULTI_LAYER 11

#define TRR_STD(c)   RND_MIL_TO_COORD((c) * 10.0)
#define TRR_PRO(c)   RND_MIL_TO_COORD((c))
#define TRY_STD(c)   TRR_STD((c) - ctx->oy)
#define TRY_PRO(c)   TRR_PRO(-(c) - ctx->oy)


/* raw coord transform (e.g. for radius, diameter, width) */
#define TRR(c)   (ctx->is_pro ? TRR_PRO(c) : TRR_STD(c))
#define TRX(c)   TRR((c) - ctx->ox)
#define TRY(c)   (ctx->is_pro ? TRY_PRO(c) : TRY_STD(c))

typedef struct easy_read_ctx_s {
	FILE *f;
	gdom_node_t *root;
	pcb_board_t *pcb;
	pcb_data_t *data;
	const char *fn;
	rnd_conf_role_t settings_dest;
	pcb_layer_t *layers[EASY_MAX_LAYERS];
	double ox, oy;
	unsigned is_footprint:1;
	unsigned is_pro:1;

	pcb_text_t *last_refdes; /* last text object created as a refdes dyntext+floater */
} easy_read_ctx_t;

#define error_at(ctx, node, args) \
	do { \
		gdom_node_t *__loc__ = node_parent_with_loc(node); \
		rnd_message(RND_MSG_ERROR, "easyeda parse error at %s:%ld.%ld\n", ctx->fn, __loc__->lineno, __loc__->col); \
		rnd_msg_error args; \
	} while(0)

#define warn_at(ctx, node, args) \
	do { \
		gdom_node_t *__loc__ = node_parent_with_loc(node); \
		rnd_message(RND_MSG_WARNING, "easyeda parse warning at %s:%ld.%ld\n", ctx->fn, __loc__->lineno, __loc__->col); \
		rnd_msg_error args; \
	} while(0)

/* Look up (long) lname within (gdom_nod_t *)nd and load the result in dst.
   Require dst->type to be typ. Invoke err_stmt when anything fails and
   print an error message */
#define HASH_GET_SUBTREE(dst, nd, lname, typ, err_stmt) \
do { \
	dst = gdom_hash_get(nd, lname); \
	if (dst == NULL) { \
		error_at(ctx, nd, ("internal: fieled to find " #lname " within %s\n", easy_keyname(nd->name))); \
		err_stmt; \
	} \
	if (dst->type != typ) { \
		error_at(ctx, dst, ("internal: " #lname " in %s must be of type " #typ "\n", easy_keyname(nd->name))); \
		err_stmt; \
	} \
} while(0)

/* Look up (long) lname within (gdom_nod_t *)nd, expect a double or long there
   and load its value into dst.
   Invoke err_stmt when anything fails and print an error message */
#define HASH_GET_DOUBLE(dst, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_DOUBLE, err_stmt); \
	dst = tmp->value.dbl; \
} while(0)

#define HASH_GET_LONG(dst, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_LONG, err_stmt); \
	dst = tmp->value.lng; \
} while(0)

#define HASH_GET_STRING(dst, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_STRING, err_stmt); \
	dst = tmp->value.str; \
} while(0)

#define HASH_GET_LAYER_GLOBAL(dst, is_any, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_LONG, err_stmt); \
	if ((tmp->value.lng < 0) || (tmp->value.lng >= EASY_MAX_LAYERS)) { \
		error_at(ctx, nd, ("layer ID %ld is out of range [0..%d]\n", tmp->value.lng, EASY_MAX_LAYERS-1)); \
		err_stmt; \
	} \
	if (tmp->value.lng != EASY_MULTI_LAYER) { \
		is_any = 0; \
		dst = ctx->layers[tmp->value.lng]; \
		if (dst == NULL) { \
			error_at(ctx, nd, ("layer ID %ld does not exist\n", tmp->value.lng)); \
			err_stmt; \
		} \
		if ((ctx->pcb != NULL) && (ctx->data != ctx->pcb->Data)) { \
			long lid = dst - ctx->pcb->Data->Layer; \
			dst = ctx->data->Layer + lid; \
		} \
	} \
	else { \
		is_any = 1; \
		dst = NULL; \
	} \
} while(0)

#define HASH_GET_LAYER(dst, nd, lname, err_stmt) \
do { \
	int tmp_is_any; \
	HASH_GET_LAYER_GLOBAL(dst, tmp_is_any, nd, lname, err_stmt); \
	(void)tmp_is_any; \
} while(0)

double easyeda_get_double(easy_read_ctx_t *ctx, gdom_node_t *nd);

extern pcb_layer_type_t easyeda_layer_id2type[];
extern int easyeda_layer_id2type_size;

#define LAYERTAB_INNER -1
extern const int easyeda_layertab[];
extern const int easyeda_layertab_in_first;
extern const int easyeda_layertab_in_last;

int easyeda_create_misc_layers(easy_read_ctx_t *ctx);
int std_parse_path(easy_read_ctx_t *ctx, const char *pathstr, gdom_node_t *nd, pcb_layer_t *layer, rnd_coord_t thickness, pcb_poly_t *in_poly);

int easyeda_layer_create(easy_read_ctx_t *ctx, unsigned ltype, const char *name, int easyeda_id, const char *clr);

pcb_subc_t *easyeda_subc_create(easy_read_ctx_t *ctx);
void easyeda_subc_finalize(easy_read_ctx_t *ctx, pcb_subc_t *subc, rnd_coord_t x, rnd_coord_t y, double rot);


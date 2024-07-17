/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - low level almost-json parser wrapper
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

/* These functions parse "EasyEDA pro" file formats; these semi-json files
   differ from the "EasyEDA std" variant. Subtrees don't need to be
   expanded/replaced but the while file needs to be reshaped a bit to
   become a valid json array */

#include <assert.h>
#include <librnd/core/compat_misc.h>

#include "io_easyeda_conf.h"
extern conf_io_easyeda_t io_easyeda_conf;
extern void easyeda_dump_tree(FILE *f, gdom_node_t *tree); /* in the std */
extern long easyeda_str2name(const char *str); /* in the std */


static void parse_pro_obj(gdom_node_t *obj)
{
	if ((obj->type == GDOM_ARRAY) && (obj->value.array.used > 0)) { /* resolve first argument of top level arrays */
		gdom_node_t *cmd = obj->value.array.child[0];
		obj->name = easyeda_str2name(cmd->value.str);
	}
}

typedef struct {
	FILE *f;
	enum { PRS_BEGIN, PRS_NORMAL, PRS_NL, PRS_END } state;
} pro_read_ctx_t;

/* Emulate a valid json by wrapping the file in {} and inserting commas before
   each newline */
static int pro_getchr(void *uctx)
{
	pro_read_ctx_t *prc = uctx;
	int c;

	switch(prc->state) {
		case PRS_BEGIN:
			prc->state = PRS_NORMAL;
			return '[';
		case PRS_NORMAL:
			c = fgetc(prc->f);
			if (c == '\n') {
				prc->state = PRS_NL;
				return ',';
			}
			else if (c == EOF) {
				prc->state = PRS_END;
				return ']';
			}
			return c;
		case PRS_NL:
			prc->state = PRS_NORMAL;
			return '\n';
		case PRS_END:
			return EOF;
	}

	abort();
}

static gdom_node_t *easyeda_pro_parse_(FILE *f)
{
	gdom_node_t *root;
	pro_read_ctx_t prc;

	prc.state = PRS_BEGIN;
	prc.f = f;

	/* low level json parse -> initial dom */
	root = gdom_json_parse_any(&prc, pro_getchr, easyeda_str2name);
	if (root == NULL)
		return NULL;

	if (root->type == GDOM_ARRAY) {
		long n;
		for(n = 0; n < root->value.array.used; n++)
			parse_pro_obj(root->value.array.child[n]);
	}

	return root;
}

gdom_node_t *easypro_low_parse(FILE *f)
{
	gdom_node_t *tree = easyeda_pro_parse_(f);

	if (conf_io_easyeda.plugins.io_easyeda.debug.dump_dom)
		easyeda_dump_tree(stdout, tree);

	return tree;

	/* suppress compiler warning on unused static functions */
	(void)replace_node;
	(void)parse_str_by_tab;
}

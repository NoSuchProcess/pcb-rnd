/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - std format read: low level string unpack
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


#include <libnanojson/nanojson.c>
#include <libnanojson/semantic.c>

#include <rnd_inclib/lib_easyeda/gendom.c>
#include <rnd_inclib/lib_easyeda/gendom_json.c>
#include <rnd_inclib/lib_easyeda/easyeda_low.c>

#include "easyeda_sphash.h"

#define SEP3 "#@$"

long easyeda_str2name(const char *str)
{
	long res = easy_sphash(str);
	if (res < 0)
		rnd_message(RND_MSG_ERROR, "Internal error: missing easyeda keyword '%s'\n", str);
	return res;
}


static void parse_pcb_any_obj(gdom_node_t *obj)
{

}

gdom_node_t *easystd_low_pcb_parse(FILE *f, int is_fp)
{
	gdom_node_t *root, *objs;

	/* low level json parse -> initial dom */
	root = gdom_json_parse(f, easyeda_str2name);
	if (root == NULL)
		return NULL;

	/* parse strings into dom subtrees */
	objs = gdom_hash_get(root, easy_objects);
	if ((objs != NULL) && (objs->type == GDOM_ARRAY)) {
		long n;
		for(n = 0; n < objs->value.array.used; n++)
			parse_pcb_any_obj(objs->value.array.child[n]);
	}

	return root;
}



static const char *name2str(long name)
{
	return easy_keyname(name);
}

void easyeda_dump_tree(FILE *f, gdom_node_t *tree)
{
	if (tree != NULL)
		gdom_dump(f, tree, 0, name2str);
	else
		fprintf(f, "<NULL tree>\n");
}

gdom_node_t *easystd_low_parse(FILE *f, int is_sym)
{
	gdom_node_t *tree = easystd_low_pcb_parse(f, is_sym);

	if (conf_io_easyeda.plugins.io_easyeda.debug.dump_dom)
		easyeda_dump_tree(stdout, tree);

	return tree;
}

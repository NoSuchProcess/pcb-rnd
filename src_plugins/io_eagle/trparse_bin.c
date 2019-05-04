/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "error.h"
#include "safe_fs.h"
#include "egb_tree.h"
#include "eagle_bin.h"

#include "trparse.h"
#include "trparse_bin.h"

static int eagle_bin_load(trparse_t *pst, const char *fn)
{
	egb_node_t *root;
	FILE *f;
	int res;

	f = pcb_fopen(NULL, fn, "rb");
	if (f == NULL)
		return -1;

	res = pcb_egle_bin_load(NULL, f, fn, &root);

	fclose(f);

/*
	printf("@@@ eagle_bin_tree @@@\n");
	egb_dump(stdout, root);
*/

	if (res != 0) {
		printf("FAILED TO LOAD: %d\n", res);
		return -1;
	}

	pst->doc = NULL;
	pst->root = root;

	return 0;
}

static int eagle_bin_unload(trparse_t *pst)
{
	egb_node_free(pst->root);
	return 0;
}

static trnode_t *eagle_bin_parent(trparse_t *pst, trnode_t *node)
{
	egb_node_t *nd = (egb_node_t *)node;
	return (trnode_t *)nd->parent;
}

static trnode_t *eagle_bin_children(trparse_t *pst, trnode_t *node)
{
	egb_node_t *nd = (egb_node_t *)node;
	return (trnode_t *)nd->first_child;
}

static trnode_t *eagle_bin_next(trparse_t *pst, trnode_t *node)
{
	egb_node_t *nd = (egb_node_t *)node;
	return (trnode_t *)nd->next;
}

const char *eagle_bin_nodename(trnode_t *node)
{
	egb_node_t *nd = (egb_node_t *)node;
	return (const char *)nd->id_name;
}

static const char *eagle_bin_prop(trparse_t *pst, trnode_t *node, const char *key)
{
	egb_node_t *nd = (egb_node_t *)node;
	return egb_node_prop_get(nd, key);
}

static const char *eagle_bin_text(trparse_t *pst, trnode_t *node)
{
	/*egb_node_t *nd = (egb_node_t *)node;*/
TODO("TODO")
	return NULL;
}

static int eagle_bin_is_text(trparse_t *pst, trnode_t *node)
{
TODO("TODO")
	return 0;
}

static void *eagle_bin_get_user_data(trnode_t *node)
{
	egb_node_t *nd = (egb_node_t *)node;
	return nd->user_data;
}

static void eagle_bin_set_user_data(trnode_t *node, void *data)
{
	egb_node_t *nd = (egb_node_t *)node;
	nd->user_data = data;
}


trparse_calls_t trparse_bin_calls = {
	eagle_bin_load,
	eagle_bin_unload,
	eagle_bin_parent,
	eagle_bin_children,
	eagle_bin_next,
	eagle_bin_nodename,
	eagle_bin_prop,
	eagle_bin_text,
	strcmp,
	eagle_bin_is_text,
	eagle_bin_get_user_data,
	eagle_bin_set_user_data
};

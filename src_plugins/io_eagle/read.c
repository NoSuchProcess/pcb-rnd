/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2017 Erich S. Heinzle
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"

#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "board.h"
#include "read.h"
#include "conf.h"
#include "error.h"

typedef struct read_state_s {
	xmlDoc *doc;
	xmlNode *root;
	pcb_board_t *pcb;

	htip_t layers;
} read_state_t;

typedef struct {
	const char *node_name;
	int (*parser)(read_state_t *st, xmlNode *subtree);
} dispatch_t;

typedef struct {
	const char *name;
	int color;
	int fill;
	int visible;
	int active;

	pcb_layer_id_t ly;
} eagle_layer_t;

/* Search the dispatcher table for subtree->str, execute the parser on match
   with the children ("parameters") of the subtree */
static int eagle_dispatch(read_state_t *st, xmlNode *subtree, const dispatch_t *disp_table)
{
	const dispatch_t *d;
	const xmlChar *name;

	/* do not tolerate empty/NIL node */
	if (subtree->name == NULL)
		return -1;

	if (subtree->type == XML_TEXT_NODE)
		name = (const xmlChar *)"@text";
	else
		name = subtree->name;

	for(d = disp_table; d->node_name != NULL; d++)
		if (xmlStrcmp((xmlChar *)d->node_name, name) == 0)
			return d->parser(st, subtree);

	pcb_message(PCB_MSG_ERROR, "eagle: unknown node: '%s'\n", name);
	/* node name not found in the dispatcher table */
	return -1;
}

/* Take each children of tree and execute them using eagle_dispatch
   Useful for procssing nodes that may host various subtrees of different
   nodes ina  flexible way. Return non-zero if any subtree processor failed. */
static int eagle_foreach_dispatch(read_state_t *st, xmlNode *tree, const dispatch_t *disp_table)
{
	xmlNode *n;

	for(n = tree; n != NULL; n = n->next)
		if (eagle_dispatch(st, n, disp_table) != 0)
			return -1;

	return 0; /* success */
}

/* No-op: ignore the subtree */
static int eagle_read_nop(read_state_t *st, xmlNode *subtree)
{
	return 0;
}



int io_eagle_test_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
{
	char line[1024];
	int found = 0, lineno = 0;

	/* look for <!DOCTYPE.*eagle.dtd in the first 32 lines, not assuming it is in a single line */
	while(fgets(line, sizeof(line), f) != NULL) {
		if ((found == 0) && (strstr(line, "<!DOCTYPE")))
			found = 1;
		if ((found == 1) && (strstr(line, "eagle.dtd")))
			return 1;
		lineno++;
		if (lineno > 32)
			break;
	}
	return 0;
}

/* Return a node attribute value converted to long, or return invalid_val
   for synatx error or if the attribute doesn't exist */
static long eagle_get_attrl(xmlNode *nd, const char *name, long invalid_val)
{
	xmlChar *p = xmlGetProp(nd, (xmlChar *)name);
	char *end;
	long res;

	if (p == NULL)
		return invalid_val;
	res = strtol((char *)p, &end, 10);
	if (*end != '\0')
		return invalid_val;
	return res;
}

/* Return a node attribute value converted to char *, or return invalid_val
   if the attribute doesn't exist */
static const char *eagle_get_attrs(xmlNode *nd, const char *name, const char *invalid_val)
{
	xmlChar *p = xmlGetProp(nd, (xmlChar *)name);
	if (p == NULL)
		return invalid_val;
	return (const char *)p;
}

static int eagle_read_layers(read_state_t *st, xmlNode *subtree)
{
	xmlNode *n;

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"layer") == 0) {
			eagle_layer_t *ly = calloc(sizeof(eagle_layer_t), 1);
			int id;
			unsigned long typ;
			pcb_layergrp_id_t gid;
			pcb_layer_group_t *grp;

			ly->name    = eagle_get_attrs(n, "name", NULL);
			ly->color   = eagle_get_attrl(n, "color", -1);
			ly->fill    = eagle_get_attrl(n, "fill", -1);
			ly->visible = eagle_get_attrl(n, "visible", -1);
			ly->active  = eagle_get_attrl(n, "active", -1);
			ly->ly      = -1;
			id = eagle_get_attrl(n, "number", -1);
			if (id >= 0)
				htip_set(&st->layers, id, ly);

			typ = 0;
			switch(id) {
				case 1: typ = PCB_LYT_COPPER | PCB_LYT_TOP; break;
				case 16: typ = PCB_LYT_COPPER | PCB_LYT_BOTTOM; break;
				case 121: typ = PCB_LYT_SILK | PCB_LYT_TOP; break;
				case 122: typ = PCB_LYT_SILK | PCB_LYT_BOTTOM; break;
				case 199:
					grp = pcb_get_grp_new_intern(st->pcb, -1);
					ly->ly = pcb_layer_create(grp - st->pcb->LayerGroups.grp, ly->name);
					pcb_layergrp_fix_turn_to_outline(grp);
					break;

				default:
					if ((id > 1) && (id < 16)) {
						/* new internal layer */
						grp = pcb_get_grp_new_intern(st->pcb, -1);
						ly->ly = pcb_layer_create(grp - st->pcb->LayerGroups.grp, ly->name);
					}
			}
			if (typ != 0) {
				if (pcb_layergrp_list(st->pcb, typ, &gid, 1) > 0) {
					ly->ly = pcb_layer_create(gid, ly->name);
/*					pcb_layer_add_in_group(st->pcb, lid, gid);*/
				}
			}
		}
	}
	return 0;
}

static eagle_layer_t *eagle_layer_get(read_state_t *st, int id)
{
	return htip_get(&st->layers, id);
}

static int eagle_read_drawing(read_state_t *st, xmlNode *subtree)
{
	static const dispatch_t disp[] = { /* possible children of <drawing> */
		{"settings",  eagle_read_nop},
		{"grid",      eagle_read_nop},
		{"layers",    eagle_read_layers},
		{"board",     eagle_read_nop},
		{"@text",     eagle_read_nop},
		{NULL, NULL}
	};
	return eagle_foreach_dispatch(st, subtree->children, disp);
}

int io_eagle_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	xmlDoc *doc;
	xmlNode *root;
	xmlChar *ver;
	char *end;
	int v1, v2, v3, res;
	read_state_t st;
	htip_entry_t *e;

	static const dispatch_t disp[] = { /* possible children of root */
		{"drawing",        eagle_read_drawing},
		{"compatibility",  eagle_read_nop},
		{"@text",          eagle_read_nop},
		{NULL, NULL}
	};

	doc = xmlReadFile(Filename, NULL, 0);
	if (doc == NULL) {
		pcb_message(PCB_MSG_ERROR, "xml parsing error\n");
		return -1;
	}

	root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (xmlChar *)"eagle") != 0) {
		pcb_message(PCB_MSG_ERROR, "xml error: root is not <eagle>\n");
		goto err;
	}

	ver = xmlGetProp(root, (xmlChar *)"version");
	if (ver == NULL) {
		pcb_message(PCB_MSG_ERROR, "no version attribute in <eagle>\n");
		goto err;
	}

	v1 = strtol((char *)ver, &end, 10);
	if (*end != '.') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [1] in <eagle>\n");
		goto err;
	}
	v2 = strtol((char *)end+1, &end, 10);
	if (*end != '.') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [2] in <eagle>\n");
		goto err;
	}
	v3 = strtol((char *)end+1, &end, 10);
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [3] in <eagle>\n");
		goto err;
	}

	/* version check */
	if (v1 < 6) {
		pcb_message(PCB_MSG_ERROR, "file version too old\n");
		goto err;
	}
	if (v1 > 7) {
		pcb_message(PCB_MSG_ERROR, "file version too new\n");
		goto err;
	}
	pcb_message(PCB_MSG_DEBUG, "Loading eagle board version %d.%d.%d\n", v1, v2, v3);

	st.doc = doc;
	st.root = root;
	st.pcb = pcb;

	htip_init(&st.layers, longhash, longkeyeq);
	pcb_layer_group_setup_default(&pcb->LayerGroups);

	res = eagle_foreach_dispatch(&st, root->children, disp);

	for (e = htip_first(&st.layers); e; e = htip_next(&st.layers, e))
		free(e->value);
	htip_uninit(&st.layers);
	pcb_layergrp_fix_old_outline(pcb);

	pcb_trace("Houston, the Eagle has landed. %d\n", res);

	xmlFreeDoc(doc);
	return 0;
err:;
	xmlFreeDoc(doc);
	return -1;
}


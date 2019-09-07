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
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "error.h"
#include "safe_fs.h"

#include "trparse.h"
#include "trparse_xml.h"

static int eagle_xml_load(trparse_t *pst, const char *fn)
{
	xmlDoc *doc;
	xmlNode *root;
	FILE *f;
	char *efn;

	f = pcb_fopen_fn(NULL, fn, "r", &efn);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open '%s'\n", fn);
		return -1;
	}
	fclose(f);

	doc = xmlReadFile(efn, NULL, 0);
	if (doc == NULL) {
		pcb_message(PCB_MSG_ERROR, "xml parsing error on file %s (%s)\n", fn, efn);
		free(efn);
		return -1;
	}
	free(efn);

	root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (xmlChar *)"eagle") != 0) {
		pcb_message(PCB_MSG_ERROR, "xml error: root is not <eagle>\n");
		xmlFreeDoc(doc);
		return -1;
	}

	pst->doc = doc;
	pst->root = root;

	return 0;
}

static int eagle_xml_unload(trparse_t *pst)
{
	xmlFreeDoc((xmlDoc *)pst->doc);
	return 0;
}

static trnode_t *eagle_xml_children(trparse_t *pst, trnode_t *node)
{
	xmlNode *nd = (xmlNode *)node;
	return (trnode_t *)nd->children;
}

static trnode_t *eagle_xml_parent(trparse_t *pst, trnode_t *node)
{
	xmlNode *nd = (xmlNode *)node;
	return (trnode_t *)nd->parent;
}

static trnode_t *eagle_xml_next(trparse_t *pst, trnode_t *node)
{
	xmlNode *nd = (xmlNode *)node;
	return (trnode_t *)nd->next;
}

const char *eagle_xml_nodename(trnode_t *node)
{
	xmlNode *nd = (xmlNode *)node;
	return (const char *)nd->name;
}

static const char *eagle_xml_prop(trparse_t *pst, trnode_t *node, const char *key)
{
	xmlNode *nd = (xmlNode *)node;
	return (const char *)xmlGetProp(nd, (const xmlChar *)key);
}

static const char *eagle_xml_text(trparse_t *pst, trnode_t *node)
{
	xmlNode *nd = (xmlNode *)node;
	return (const char *)nd->content;
}

static int eagle_xml_strcmp(const char *s1, const char *s2)
{
	return xmlStrcmp((const xmlChar *)s1, (const xmlChar *)s2);
}

static int eagle_xml_is_text(trparse_t *pst, trnode_t *node)
{
	return ((xmlNode *)node)->type == XML_TEXT_NODE;
}

static void *eagle_xml_get_user_data(trnode_t *node)
{
	xmlNode *nd = (xmlNode *)node;
	return nd->_private;
}

static void eagle_xml_set_user_data(trnode_t *node, void *data)
{
	xmlNode *nd = (xmlNode *)node;
	nd->_private = data;
}


trparse_calls_t trparse_xml_calls = {
	eagle_xml_load,
	eagle_xml_unload,
	eagle_xml_parent,
	eagle_xml_children,
	eagle_xml_next,
	eagle_xml_nodename,
	eagle_xml_prop,
	eagle_xml_text,
	eagle_xml_strcmp,
	eagle_xml_is_text,
	eagle_xml_get_user_data,
	eagle_xml_set_user_data
};

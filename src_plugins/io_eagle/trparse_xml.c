/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "error.h"

#include "trparse.h"
#include "trparse_xml.h"

static int eagle_xml_load(trparse_t *pst, const char *fn)
{
	xmlDoc *doc;
	xmlNode *root;

	doc = xmlReadFile(fn, NULL, 0);
	if (doc == NULL) {
		pcb_message(PCB_MSG_ERROR, "xml parsing error\n");
		return -1;
	}

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
}

static trnode_t *eagle_xml_children(trparse_t *pst, trnode_t *node)
{
	xmlNode *nd = (xmlNode *)node;
	return (trnode_t *)nd->children;
}

trparse_calls_t trparse_xml_calls = {
	eagle_xml_load,
	eagle_xml_unload,
	eagle_xml_children
};

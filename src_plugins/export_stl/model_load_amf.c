/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <stdio.h>

#ifdef PCB_HAVE_LIBXML2
#include <libxml/tree.h>
#include <libxml/parser.h>

static void amf_load_material(xmlNode *material)
{
	rnd_trace("amf material\n");
}

static void amf_load_mesh(xmlNode *mesh)
{
	rnd_trace("amf mesh\n");
}

stl_facet_t *amf_solid_fload(rnd_hidlib_t *hl, FILE *f, const char *fn)
{
	xmlDoc *doc;
	xmlNode *root, *n, *m;

	doc = xmlReadFile(fn, NULL, 0);
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "amf xml parsing error on file %s\n", fn);
		return NULL;
	}

	root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (xmlChar *)"amf") != 0) {
		rnd_message(RND_MSG_ERROR, "amf xml error on file %s: root is not <amf>\n", fn);
		xmlFreeDoc(doc);
		return NULL;
	}

	/* read all materials */
	for(n = root->children; n != NULL; n = n->next)
		if (xmlStrcmp(n->name, (xmlChar *)"material") != 0)
			amf_load_material(n);

	/* read all volumes */
	for(n = root->children; n != NULL; n = n->next)
		if (xmlStrcmp(n->name, (xmlChar *)"object") != 0)
			for(m = root->children; m != NULL; m = m->next)
				if (xmlStrcmp(m->name, (xmlChar *)"mesh") != 0)
					amf_load_mesh(n);

	xmlFreeDoc(doc);
	return NULL;
}



#else

/* Fallback: still provide a dummy if libxml is not available */
stl_facet_t *amf_solid_fload(rnd_hidlib_t *hl, FILE *f, const char *fn)
{
	return &stl_format_not_supported;
}

#endif

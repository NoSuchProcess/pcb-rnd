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

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "board.h"
#include "read.h"
#include "conf.h"
#include "error.h"

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

int io_eagle_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	xmlDoc *doc;
	xmlNode *root;
	xmlChar *ver;
	char *end;
	int v1, v2, v3;

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


	pcb_trace("Houston, the Eagle has landed.\n");
	goto err; /* until we really parse */

	xmlFreeDoc(doc);
	return 0;
err:;
	xmlFreeDoc(doc);
	return -1;
}


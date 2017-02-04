/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Specctra .dsn import HID
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gensexpr/gsxl.h>

#include "board.h"
#include "data.h"
#include "polygon.h"

#include "action_helper.h"
#include "hid.h"
#include "plugins.h"

static const char *dsn_cookie = "dsn importer";

typedef enum {
	TYPE_PCB,
	TYPE_SESSION
} dsn_type_t;

static void parse_wire(long int *nlines, pcb_coord_t clear, const gsxl_node_t *wire, dsn_type_t type)
{
	const gsxl_node_t *n, *c;
	for(n = wire->children; n != NULL; n = n->next) {
		if ((type == TYPE_PCB) && (strcmp(n->str, "polyline_path") == 0)) {
			pcb_coord_t x, y, lx, ly, thick;
			long pn;
			const char *slayer = n->children->str;
			const char *sthick = n->children->next->str;
			pcb_bool succ;
			pcb_layer_id_t lid;
			pcb_layer_t *layer;

			thick = pcb_get_value(sthick, "mm", NULL, &succ);
			if (!succ) {
				pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline because thickness is invalid: %s\n", sthick);
				continue;
			}

			lid = pcb_layer_by_name(slayer);
			if (lid < 0) {
				pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline because layer name is invalid: %s\n", slayer);
				continue;
			}
			layer = PCB->Data->Layer+lid;

			/*printf("- %s\n", slayer);*/
			for(pn = 0, c = n->children->next->next; c != NULL; pn++, c = c->next->next) {
				const char *sx = c->str;
				const char *sy = c->next->str;
				x = pcb_get_value(sx, "mm", NULL, &succ);
				if (!succ) {
					pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline segment because x coord is invalid: %s\n", sx);
					continue;
				}
				y = pcb_get_value(sy, "mm", NULL, &succ);
				if (!succ) {
					pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline segment because x coord is invalid: %s\n", sy);
					continue;
				}
				if ((y < PCB_MM_TO_COORD(0.01)) || (x < PCB_MM_TO_COORD(0.01))) /* workaround for broken polyline coords */
					continue;
				(*nlines)++;
				if (pn > 0) {
					pcb_line_t *line = pcb_line_new_merge(layer, lx, PCB->MaxHeight - ly,
						x, PCB->MaxHeight - y, thick, clear, pcb_flag_make(PCB_FLAG_AUTO | PCB_FLAG_CLEARLINE));
/*					pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, layer, line);*/
/*					pcb_printf("LINE: %$mm %$mm .. %$mm %$mm\n", lx, ly, x, y);*/
				}

				lx = x;
				ly = y;
			}
		}
		if ((type == TYPE_SESSION) && (strcmp(n->str, "path") == 0)) {
		
		}
		else if (strcmp(n->str, "net") == 0) {
			/* ignore */
		}
		else if (strcmp(n->str, "type") == 0) {
			/* ignore */
		}
		else if (strcmp(n->str, "clearance_class") == 0) {
			/* ignore */
		}
		else
			pcb_message(PCB_MSG_WARNING, "import_dsn: ignoring unknown wire directive %s\n", n->str);
	}
}

static void parse_via(pcb_coord_t clear, const gsxl_node_t *via)
{
	const gsxl_node_t *c = via->children->next;
	const char *name = via->children->str;
	const char *sx = c->str;
	const char *sy = c->next->str;
	pcb_bool succ;
	pcb_coord_t x, y, dia, drill;

	if (strncmp(name, "via_", 4) != 0) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via with invalid name (prefix): %s\n", name);
		return;
	}

	name += 4;
	if (sscanf(name, "%ld_%ld", &dia, &drill) != 2) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via with invalid name (diameters): %s\n", name);
		return;
	}

	x = pcb_get_value(sx, "mm", NULL, &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via segment because x coord is invalid: %s\n", sx);
		return;
	}
	y = pcb_get_value(sy, "mm", NULL, &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via segment because x coord is invalid: %s\n", sy);
		return;
	}

	pcb_via_new(PCB->Data, x, PCB->MaxHeight - y, dia, clear, 0, drill, 0, pcb_flag_make(PCB_FLAG_AUTO));
}

static const char load_dsn_syntax[] = "LoadDsnFrom(filename)";

static const char load_dsn_help[] = "Loads the specified routed dsn file.";

int pcb_act_LoadDsnFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	pcb_coord_t clear;
	FILE *f;
	gsxl_dom_t dom;
	int c, seek_quote = 1;
	long int nlines = 0, nvias = 0;
	gsx_parse_res_t res;
	gsxl_node_t *wiring, *w, *routes, *nout, *n;
	dsn_type_t type;

	fname = argc ? argv[0] : 0;

	if ((fname == NULL) || (*fname == '\0')) {
		fname = pcb_gui->fileselect(
			"Load a routed dsn file...",
			"Select dsn file to load.\nThe file could be generated using the tool downloaded from freeroute.net\n",
			NULL, /* default file name */
			".dsn", "dsn", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_AFAIL(load_dsn);
	}

	/* load and parse the file into a dom tree */
	f = fopen(fname, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: can't open %s for read\n", fname);
		return 1;
	}
	gsxl_init(&dom, gsxl_node_t);
	dom.parse.line_comment_char = '#';
	do {
		c = fgetc(f);
		res = gsxl_parse_char(&dom, c);

		/* Workaround: skip the unbalanced '"' in string_quote */
		if (seek_quote && (dom.parse.used == 12) && (strncmp(dom.parse.atom, "string_quote", dom.parse.used) == 0)) {
			do {
				c = fgetc(f);
			} while((c != ')') && (c != EOF));
			res = gsxl_parse_char(&dom, ')');
			seek_quote = 0;
		}
	} while(res == GSX_RES_NEXT);
	fclose(f);

	if (res != GSX_RES_EOE) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: parse error in %s at %d:%d\n", fname, dom.parse.line, dom.parse.col);
		goto error;
	}
	gsxl_compact_tree(&dom);


	/* parse the tree: find wiring */
	clear = PCB->RouteStyle.array[0].Clearance * 2;
	if (strcmp(dom.root->str, "PCB") == 0)
		type = TYPE_PCB;
	else if (strcmp(dom.root->str, "session") == 0)
		type = TYPE_SESSION;
	else {
		pcb_message(PCB_MSG_ERROR, "import_dsn: s-expr is not a PCB or session\n");
		goto error;
	}

	switch(type) {
		case TYPE_PCB:
			for(wiring = dom.root->children; wiring != NULL; wiring = wiring->next)
				if (strcmp(wiring->str, "wiring") == 0)
					break;

			if (wiring == NULL) {
				pcb_message(PCB_MSG_ERROR, "import_dsn: s-expr does not have a wiring section\n");
				goto error;
			}

			/* parse wiring */
			for(w = wiring->children; w != NULL; w = w->next) {
				if (strcmp(w->str, "wire") == 0)
					parse_wire(&nlines, clear, w, type);
				if (strcmp(w->str, "via") == 0) {
					parse_via(clear, w);
					nvias++;
				}
			}
			break;
		case TYPE_SESSION:
			for(routes = dom.root->children; routes != NULL; routes = routes->next)
				if (strcmp(routes->str, "routes") == 0)
					break;

			if (routes == NULL) {
				pcb_message(PCB_MSG_ERROR, "import_dsn: s-expr does not have a routes section\n");
				goto error;
			}

			for(nout = routes->children; nout != NULL; nout = nout->next)
				if (strcmp(nout->str, "network_out") == 0)
					break;

			if (nout == NULL) {
				pcb_message(PCB_MSG_ERROR, "import_dsn: s-expr does not have a network_out section\n");
				goto error;
			}

			for(n = nout->children; n != NULL; n = n->next) {
				for(w = n->children; w != NULL; w = w->next) {
					if (strcmp(w->str, "wire") == 0)
						parse_wire(&nlines, clear, w, type);
				}
			}
			break;
	}

	pcb_message(PCB_MSG_INFO, "import_dsn: loaded %ld wires and %ld vias\n", nlines, nvias);

	gsxl_uninit(&dom);
	return 0;

	error:;
	gsxl_uninit(&dom);
	return 0;
}

pcb_hid_action_t dsn_action_list[] = {
	{"LoadDsnFrom", 0, pcb_act_LoadDsnFrom, load_dsn_help, load_dsn_syntax}
};

PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)

static void hid_dsn_uninit()
{

}

#include "dolists.h"
pcb_uninit_t hid_import_dsn_init()
{
	PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)
	return hid_dsn_uninit;
}


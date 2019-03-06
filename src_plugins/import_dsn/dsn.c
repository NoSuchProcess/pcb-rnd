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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "safe_fs.h"

#include "actions.h"
#include "hid.h"
#include "plugins.h"

#include "src_plugins/lib_compat_help/pstk_compat.h"

static const char *dsn_cookie = "dsn importer";

typedef enum {
	TYPE_PCB,
	TYPE_SESSION
} dsn_type_t;

static pcb_layer_id_t ses_layer_by_name(pcb_board_t *pcb, const char *name)
{
	char *end;
	long gid;
	pcb_layergrp_t *grp;

	gid = strtol(name, &end, 10);
	if ((end[0] != '_') || (end[1] != '_'))
		return -1;

	grp = pcb_get_layergrp(pcb, gid);
	if (grp == NULL)
		return -1;

	if (strcmp(end+2, grp->name) != 0) {
		pcb_message(PCB_MSG_ERROR, "layer (group) name mismatch: group %ld should be '%s' but is '%s'\nses file not for this board?\n", gid, grp->name, end+2);
		return -1;
	}

	if (grp->len <= 0) {
		pcb_message(PCB_MSG_ERROR, "layer (group) '%s' has no layers\nses file not for this board?\n", name);
		return -1;
	}

	if (!(grp->ltype & PCB_LYT_COPPER)) {
		pcb_message(PCB_MSG_ERROR, "layer (group) type %s should a copper layer group\nses file not for this board?\n", name);
		return -1;
	}

	return grp->lid[0];
}

static void parse_polyline(long int *nlines, pcb_coord_t clear, const gsxl_node_t *n, const char *unit, int workaround0)
{
	const gsxl_node_t *c;
	pcb_coord_t x, y, lx, ly, thick;
	long pn;
	const char *slayer = n->children->str;
	const char *sthick = n->children->next->str;
	pcb_bool succ;
	pcb_layer_id_t lid;
	pcb_layer_t *layer;

	thick = pcb_get_value(sthick, unit, NULL, &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline because thickness is invalid: %s\n", sthick);
		return;
	}

	lid = ses_layer_by_name(PCB, slayer);
	if (lid < 0) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline because layer name is invalid: %s\n", slayer);
		return;
	}
	layer = PCB->Data->Layer+lid;

	/*printf("- %s\n", slayer);*/
	for(pn = 0, c = n->children->next->next; c != NULL; pn++, c = c->next->next) {
		const char *sx = c->str;
		const char *sy = c->next->str;
		x = pcb_get_value(sx, unit, NULL, &succ);
		if (!succ) {
			pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline segment because x coord is invalid: %s\n", sx);
			return;
		}
		y = pcb_get_value(sy, unit, NULL, &succ);
		if (!succ) {
			pcb_message(PCB_MSG_ERROR, "import_dsn: skipping polyline segment because x coord is invalid: %s\n", sy);
			return;
		}
		if (workaround0 && ((y < PCB_MM_TO_COORD(0.01)) || (x < PCB_MM_TO_COORD(0.01)))) /* workaround for broken polyline coords */
			return;
		(*nlines)++;
		if (pn > 0) {
			/*pcb_line_t *line = */pcb_line_new_merge(layer, lx, PCB->MaxHeight - ly,
				x, PCB->MaxHeight - y, thick, clear, pcb_flag_make(PCB_FLAG_AUTO | PCB_FLAG_CLEARLINE));
/*				pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_LINE, layer, line);*/
/*				pcb_printf("LINE: %$mm %$mm .. %$mm %$mm\n", lx, ly, x, y);*/
		}
		lx = x;
		ly = y;
	}
}

static void parse_wire(long int *nlines, pcb_coord_t clear, const gsxl_node_t *wire, dsn_type_t type)
{
	const gsxl_node_t *n;
	for(n = wire->children; n != NULL; n = n->next) {
		if ((type == TYPE_PCB) && (strcmp(n->str, "polyline_path") == 0))
			parse_polyline(nlines, clear, n, "mm", 1);
		else if ((type == TYPE_SESSION) && (strcmp(n->str, "path") == 0))
			parse_polyline(nlines, clear, n, "nm", 0);
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

static void parse_via(pcb_coord_t clear, const gsxl_node_t *via, dsn_type_t type)
{
	const gsxl_node_t *c = via->children->next;
	const char *name = via->children->str;
	const char *sx = c->str;
	const char *sy = c->next->str;
	pcb_bool succ;
	pcb_coord_t x, y, dia, drill;
	long l1, l2;
	const char *unit = (type == TYPE_PCB) ? "mm" : "nm";

	if (strncmp(name, "via_", 4) != 0) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via with invalid name (prefix): %s\n", name);
		return;
	}

	name += 4;
	if (sscanf(name, "%ld_%ld", &l1, &l2) != 2) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via with invalid name (diameters): %s\n", name);
		return;
	}

	dia = l1;
	drill = l2;

	x = pcb_get_value(sx, unit, NULL, &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via segment because x coord is invalid: %s\n", sx);
		return;
	}
	y = pcb_get_value(sy, unit, NULL, &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: skipping via segment because x coord is invalid: %s\n", sy);
		return;
	}

	{
		pcb_pstk_t *ps = pcb_pstk_new_compat_via(PCB->Data, -1, x, PCB->MaxHeight - y, drill, dia, clear, 0, PCB_PSTK_COMPAT_ROUND, 1);
		PCB_FLAG_SET(PCB_FLAG_AUTO, ps);
	}
}

static const char pcb_acts_LoadDsnFrom[] = "LoadDsnFrom(filename)";
static const char pcb_acth_LoadDsnFrom[] = "Loads the specified routed dsn file.";

fgw_error_t pcb_act_LoadDsnFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	pcb_coord_t clear;
	FILE *f;
	gsxl_dom_t dom;
	int c, seek_quote = 1;
	long int nlines = 0, nvias = 0;
	gsx_parse_res_t rs;
	gsxl_node_t *wiring, *w, *routes, *nout, *n;
	dsn_type_t type;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadDsnFrom, fname = argv[1].val.str);

	if ((fname == NULL) || (*fname == '\0')) {
		fname = pcb_gui->fileselect(
			"Load a routed dsn or ses file...",
			"Select dsn or ses file to load.\nThe file could be generated using the tool downloaded from freeroute.net\n",
			NULL, /* default file name */
			".dsn", NULL, "dsn", PCB_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
	}

	/* load and parse the file into a dom tree */
	f = pcb_fopen(fname, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: can't open %s for read\n", fname);
		return 1;
	}
	gsxl_init(&dom, gsxl_node_t);
	dom.parse.line_comment_char = '#';
	do {
		c = fgetc(f);
		rs = gsxl_parse_char(&dom, c);

		/* Workaround: skip the unbalanced '"' in string_quote */
		if (seek_quote && (dom.parse.used == 12) && (strncmp(dom.parse.atom, "string_quote", dom.parse.used) == 0)) {
			do {
				c = fgetc(f);
			} while((c != ')') && (c != EOF));
			rs = gsxl_parse_char(&dom, ')');
			seek_quote = 0;
		}
	} while(rs == GSX_RES_NEXT);
	fclose(f);

	if (rs != GSX_RES_EOE) {
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
					parse_via(clear, w, type);
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
					if (strcmp(w->str, "via") == 0) {
						parse_via(clear, w, type);
						nvias++;
					}
				}
			}
			break;
	}

	pcb_message(PCB_MSG_INFO, "import_dsn: loaded %ld wires and %ld vias\n", nlines, nvias);

	gsxl_uninit(&dom);
	PCB_ACT_IRES(0);
	return 0;

	error:;
	gsxl_uninit(&dom);

	PCB_ACT_IRES(-1);
	return 0;
}

pcb_action_t dsn_action_list[] = {
	{"LoadDsnFrom", pcb_act_LoadDsnFrom, pcb_acth_LoadDsnFrom, pcb_acts_LoadDsnFrom}
};

PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)

int pplg_check_ver_import_dsn(int ver_needed) { return 0; }

void pplg_uninit_import_dsn(void)
{
	pcb_remove_actions_by_cookie(dsn_cookie);

}

#include "dolists.h"
int pplg_init_import_dsn(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)
	return 0;
}


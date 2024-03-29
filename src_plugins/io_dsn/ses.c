/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Specctra dsn .ses import HID
 *  Copyright (C) 2017,2021 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"
#include "data.h"
#include "polygon.h"
#include <librnd/core/safe_fs.h>

#include <librnd/core/actions.h>
#include <librnd/hid/hid.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>

#include "src_plugins/lib_compat_help/pstk_compat.h"

#include "menu_internal.c"


static const char *dsn_cookie = "dsn importer/ses";

typedef enum {
	TYPE_PCB,
	TYPE_SESSION
} dsn_type_t;

static rnd_layer_id_t ses_layer_by_name(pcb_board_t *pcb, const char *name)
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
		rnd_message(RND_MSG_ERROR, "layer (group) name mismatch: group %ld should be '%s' but is '%s'\nses file not for this board?\n", gid, grp->name, end+2);
		return -1;
	}

	if (grp->len <= 0) {
		rnd_message(RND_MSG_ERROR, "layer (group) '%s' has no layers\nses file not for this board?\n", name);
		return -1;
	}

	if (!(grp->ltype & PCB_LYT_COPPER)) {
		rnd_message(RND_MSG_ERROR, "layer (group) type %s should a copper layer group\nses file not for this board?\n", name);
		return -1;
	}

	return grp->lid[0];
}

static void parse_polyline(long int *nlines, rnd_coord_t clear, const gsxl_node_t *n, const char *unit, int workaround0)
{
	const gsxl_node_t *c;
	rnd_coord_t x, y, lx, ly, thick;
	long pn;
	const char *slayer = n->children->str;
	const char *sthick = n->children->next->str;
	rnd_bool succ;
	rnd_layer_id_t lid;
	pcb_layer_t *layer;

	thick = rnd_get_value(sthick, unit, NULL, &succ);
	if (!succ) {
		rnd_message(RND_MSG_ERROR, "import_dsn: skipping polyline because thickness is invalid: %s\n", sthick);
		return;
	}

	lid = ses_layer_by_name(PCB, slayer);
	if (lid < 0) {
		rnd_message(RND_MSG_ERROR, "import_dsn: skipping polyline because layer name is invalid: %s\n", slayer);
		return;
	}
	layer = PCB->Data->Layer+lid;

	/*printf("- %s\n", slayer);*/
	for(pn = 0, c = n->children->next->next; c != NULL; pn++, c = c->next->next) {
		const char *sx = c->str;
		const char *sy = c->next->str;
		x = rnd_get_value(sx, unit, NULL, &succ);
		if (!succ) {
			rnd_message(RND_MSG_ERROR, "import_dsn: skipping polyline segment because x coord is invalid: %s\n", sx);
			return;
		}
		y = rnd_get_value(sy, unit, NULL, &succ);
		if (!succ) {
			rnd_message(RND_MSG_ERROR, "import_dsn: skipping polyline segment because x coord is invalid: %s\n", sy);
			return;
		}
		if (workaround0 && ((y < RND_MM_TO_COORD(0.01)) || (x < RND_MM_TO_COORD(0.01)))) /* workaround for broken polyline coords */
			return;
		(*nlines)++;
		if (pn > 0) {
			/*pcb_line_t *line = */pcb_line_new_merge(layer, lx, PCB->hidlib.dwg.Y2 - ly,
				x, PCB->hidlib.dwg.Y2 - y, thick, clear, pcb_flag_make(PCB_FLAG_AUTO | PCB_FLAG_CLEARLINE));
/*				pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_LINE, layer, line);*/
/*				rnd_printf("LINE: %$mm %$mm .. %$mm %$mm\n", lx, ly, x, y);*/
		}
		lx = x;
		ly = y;
	}
}

static void parse_wire(long int *nlines, rnd_coord_t clear, const gsxl_node_t *wire, dsn_type_t type)
{
	const gsxl_node_t *n;
	for(n = wire->children; n != NULL; n = n->next) {
		if ((type == TYPE_PCB) && ((strcmp(n->str, "polyline_path") == 0) || (strcmp(n->str, "path") == 0)))
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
			rnd_message(RND_MSG_WARNING, "import_dsn: ignoring unknown wire directive %s\n", n->str);
	}
}

static void parse_via(rnd_coord_t clear, const gsxl_node_t *via, dsn_type_t type)
{
	const gsxl_node_t *c = via->children->next;
	const char *name = via->children->str;
	const char *sx = c->str;
	const char *sy = c->next->str;
	rnd_bool succ;
	rnd_coord_t x, y;
	long l1;
	const char *unit = (type == TYPE_PCB) ? "mm" : "nm";

	if (strncmp(name, "pstk_", 5) != 0) {
		rnd_message(RND_MSG_ERROR, "import_ses: skipping via with invalid name (prefix): %s\n", name);
		return;
	}

	name += 5;
	if (sscanf(name, "%ld", &l1) != 1) {
		rnd_message(RND_MSG_ERROR, "import_ses: skipping via with invalid name (diameters): %s\n", name);
		return;
	}

	x = rnd_get_value(sx, unit, NULL, &succ);
	if (!succ) {
		rnd_message(RND_MSG_ERROR, "import_ses: skipping via segment because x coord is invalid: %s\n", sx);
		return;
	}
	y = rnd_get_value(sy, unit, NULL, &succ);
	if (!succ) {
		rnd_message(RND_MSG_ERROR, "import_ses: skipping via segment because x coord is invalid: %s\n", sy);
		return;
	}

	{
		pcb_pstk_t *ps = pcb_pstk_new(PCB->Data, -1, l1, x, PCB->hidlib.dwg.Y2 - y, clear, pcb_flag_make(PCB_FLAG_CLEARLINE | PCB_FLAG_AUTO));
		if (ps == NULL)
			rnd_message(RND_MSG_ERROR, "import_ses: failed to create via at %$mm;%$mm with prototype %ld\n", x, PCB->hidlib.dwg.Y2 - y, l1);
	}
}

static const char pcb_acts_ImportSes[] = "ImportSes(filename)";
static const char pcb_acth_ImportSes[] = "Loads the specified routed dsn file.";

static fgw_error_t pcb_act_ImportSes(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	rnd_coord_t clear;
	FILE *f;
	gsxl_dom_t dom;
	int c, seek_quote = 1;
	long int nlines = 0, nvias = 0;
	gsx_parse_res_t rs;
	gsxl_node_t *wiring, *w, *routes, *nout, *n;
	dsn_type_t type;

	RND_ACT_MAY_CONVARG(1, FGW_STR, ImportSes, fname = argv[1].val.str);

	if ((fname == NULL) || (*fname == '\0')) {
		fname = rnd_hid_fileselect(rnd_gui,
			"Load a routed dsn or ses file...",
			"Select ses (or dsn) file to load.\nThe file could be generated using external autorouters (e.g. freeroute.net)\n",
			NULL, /* default file name */
			".ses", NULL, "ses", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
	}

	/* load and parse the file into a dom tree */
	f = rnd_fopen(&PCB->hidlib, fname, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "import_dsn: can't open %s for read\n", fname);
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
		rnd_message(RND_MSG_ERROR, "import_dsn: parse error in %s at %d:%d\n", fname, dom.parse.line, dom.parse.col);
		goto error;
	}
	gsxl_compact_tree(&dom);


	/* parse the tree: find wiring */
	clear = conf_core.design.clearance * 2;
	if (rnd_strcasecmp(dom.root->str, "pcb") == 0)
		type = TYPE_PCB;
	else if (strcmp(dom.root->str, "session") == 0)
		type = TYPE_SESSION;
	else {
		rnd_message(RND_MSG_ERROR, "import_dsn: s-expr is not a PCB or session\n");
		goto error;
	}

	switch(type) {
		case TYPE_PCB:
			for(wiring = dom.root->children; wiring != NULL; wiring = wiring->next)
				if (strcmp(wiring->str, "wiring") == 0)
					break;

			if (wiring == NULL) {
				rnd_message(RND_MSG_ERROR, "import_dsn: s-expr does not have a wiring section\n");
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
				rnd_message(RND_MSG_ERROR, "import_dsn: s-expr does not have a routes section\n");
				goto error;
			}

			for(nout = routes->children; nout != NULL; nout = nout->next)
				if (strcmp(nout->str, "network_out") == 0)
					break;

			if (nout == NULL) {
				rnd_message(RND_MSG_ERROR, "import_dsn: s-expr does not have a network_out section\n");
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

	rnd_message(RND_MSG_INFO, "import_dsn: loaded %ld wires and %ld vias\n", nlines, nvias);
	pcb_data_clip_polys(PCB->Data);

	gsxl_uninit(&dom);
	RND_ACT_IRES(0);
	return 0;

	error:;
	gsxl_uninit(&dom);

	RND_ACT_IRES(-1);
	return 0;
}

static const char pcb_acts_LoadDsnFrom[] = "LoadDsnFrom(filename)";
static const char pcb_acth_LoadDsnFrom[] = "LoadDsnFrom() is a legacy action provided for compatibility and will be removed later.\nPlease use ImportSes() instead!\n";
static fgw_error_t pcb_act_LoadDsnFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, pcb_acth_LoadDsnFrom);
	return pcb_act_ImportSes(res, argc, argv);
}

static rnd_action_t dsn_action_list[] = {
	{"LoadDsnFrom", pcb_act_LoadDsnFrom, pcb_acth_LoadDsnFrom, pcb_acts_LoadDsnFrom},
	{"ImportSes", pcb_act_ImportSes, pcb_acth_ImportSes, pcb_acts_ImportSes}
};

void pcb_dsn_ses_uninit(void)
{
	rnd_remove_actions_by_cookie(dsn_cookie);
	rnd_hid_menu_unload(rnd_gui, dsn_cookie);
}

void pcb_dsn_ses_init(void)
{
	RND_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, dsn_cookie, 190, NULL, 0, dsn_menu, "plugin: import_dsn");
}


/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with c-pcb (selective import/export in c-pcb format)
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <gensexpr/gsxl.h>

#include "board.h"
#include "data.h"
#include "plugins.h"
#include "actions.h"

static const char *cpcb_cookie = "cpcb plugin";

static int cpcb_load(pcb_board_t *pcb, FILE *f)
{
	gsx_parse_res_t res;
	gsxl_dom_t dom;
	gsxl_node_t *rn, *n;
	int c;
	pcb_layer_t *copper;

	/* low level s-expression parse */
	gsxl_init(&dom, gsxl_node_t);
	dom.parse.line_comment_char = '#';
	gsxl_parse_char(&dom, '('); /* have to fake the whole file is a single expression */
	do {
		c = fgetc(f);
		if (c == EOF)
			gsxl_parse_char(&dom, ')'); /* have to fake the whole file is a single expression */
	} while((res = gsxl_parse_char(&dom, c)) == GSX_RES_NEXT);
	if (res != GSX_RES_EOE)
		return -1;


	for(rn = gsxl_children(dom.root); rn != NULL; rn = gsxl_next(rn)) {
		int numch = 0;
		gsxl_node_t *p, *nid, *ntr, *nvr, *ngap, *npads, *npaths, *nx, *ny, *nl;
		pcb_coord_t thick, clear;

		for(n = gsxl_children(rn); n != NULL; n = gsxl_next(n)) numch++;
		switch(numch) {
			case 0:
				printf("EOF\n");
				break;
			case 3:
				printf("dim: %s %s ly=%s\n", gsxl_nth(rn, 1)->str, gsxl_nth(rn, 2)->str, gsxl_nth(rn, 3)->str);
				break;
			case 6: /* tracks */
				nid = gsxl_nth(rn, 1);
				ntr = gsxl_next(nid); thick = PCB_MM_TO_COORD(strtod(ntr->str, NULL));
				nvr = gsxl_next(ntr);
				ngap = gsxl_next(nvr); clear = PCB_MM_TO_COORD(strtod(ngap->str, NULL));
				npads = gsxl_next(ngap);
				npaths = gsxl_next(npads);

				for(n = gsxl_children(npaths); n != NULL; n = gsxl_next(n)) { /* iterate over all paths of the track */
					pcb_coord_t lx, ly, x, y;
					int len = 0;

					for(p = gsxl_children(n); p != NULL; p = gsxl_next(p)) { /* iterate over all points of the path */
						pcb_line_t *line;
						nx = gsxl_children(p); x = PCB_MM_TO_COORD(strtod(nx->str, NULL));
						ny = gsxl_next(nx); y = PCB_MM_TO_COORD(strtod(ny->str, NULL));
						nl = gsxl_next(ny);
						if (len > 0)
							line = pcb_line_new(CURRENT, lx, ly, x, y, thick, clear, pcb_flag_make(PCB_FLAG_CLEARLINE));
						lx = x;
						ly = y;
						len++;
					}
				}
				break;
		}
	}

	return 0;
}

static const char pcb_acts_import_cpcb[] = "ImportcpcbFrom(filename)";
static const char pcb_acth_import_cpcb[] = "Loads the auto-routed tracks from the specified c-pcb output.";
fgw_error_t pcb_act_import_cpcb(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fn;
	FILE *f;

	PCB_ACT_CONVARG(1, FGW_STR, import_cpcb, fn = argv[1].val.str);

	f = pcb_fopen(fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not open %s for read\n", fn);
		PCB_ACT_IRES(-1);
		return 0;
	}
	cpcb_load(PCB, f);
	fclose(f);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_export_cpcb[] = "ExportcpcbTo(filename)";
static const char pcb_acth_export_cpcb[] = "Dumps the current board in c-pcb format.";
fgw_error_t pcb_act_export_cpcb(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return -1;
}


static pcb_action_t cpcb_action_list[] = {
	{"ImportcpcbFrom", pcb_act_import_cpcb, pcb_acth_import_cpcb, pcb_acts_import_cpcb},
	{"ExportcpcbTo", pcb_act_export_cpcb, pcb_acth_export_cpcb, pcb_acts_export_cpcb},
};

PCB_REGISTER_ACTIONS(cpcb_action_list, cpcb_cookie)

int pplg_check_ver_ar_cpcb(int ver_needed) { return 0; }

void pplg_uninit_ar_cpcb(void)
{
	pcb_remove_actions_by_cookie(cpcb_cookie);
}


#include "dolists.h"
int pplg_init_ar_cpcb(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(cpcb_action_list, cpcb_cookie)

	return 0;
}

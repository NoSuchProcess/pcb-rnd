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
#include "safe_fs.h"
#include "conf_core.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"

static const char *cpcb_cookie = "cpcb plugin";

typedef struct {
	int maxlayer;
	pcb_layer_t *copper[PCB_MAX_LAYERGRP];
} cpcb_layers_t;

static void cpcb_map_layers(pcb_board_t *pcb, cpcb_layers_t *dst)
{
	int gid;
	pcb_layergrp_t *grp;
	/* map copper layers from top to bottom */
	dst->maxlayer = 0;
	for(gid = 0, grp = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,grp++) {
		if ((grp->ltype & PCB_LYT_COPPER) && (grp->len > 0)) {
			dst->copper[dst->maxlayer] = pcb_get_layer(pcb->Data, grp->lid[0]);
			dst->maxlayer++;
		}
	}
}

static int cpcb_load(pcb_board_t *pcb, FILE *f, cpcb_layers_t *stack)
{
	gsx_parse_res_t res;
	gsxl_dom_t dom;
	gsxl_node_t *rn, *n;
	int c;

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
		pcb_coord_t thick, clear, via_dia;
		char *end;

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
				ntr = gsxl_next(nid); thick = 2*PCB_MM_TO_COORD(strtod(ntr->str, NULL));
				nvr = gsxl_next(ntr); via_dia = 2*PCB_MM_TO_COORD(strtod(nvr->str, NULL));
				ngap = gsxl_next(nvr); clear = PCB_MM_TO_COORD(strtod(ngap->str, NULL));
				npads = gsxl_next(ngap);
				npaths = gsxl_next(npads);

				for(n = gsxl_children(npaths); n != NULL; n = gsxl_next(n)) { /* iterate over all paths of the track */
					pcb_coord_t lx, ly, x, y;
					int len = 0, lidx, llidx;

					for(p = gsxl_children(n); p != NULL; p = gsxl_next(p)) { /* iterate over all points of the path */
						pcb_line_t *line;
						nx = gsxl_children(p); x = PCB_MM_TO_COORD(strtod(nx->str, NULL));
						ny = gsxl_next(nx); y = PCB_MM_TO_COORD(strtod(ny->str, NULL));
						nl = gsxl_next(ny);
						
						lidx = strtol(nl->str, &end, 10);
						if (*end != '\0') {
							pcb_message(PCB_MSG_ERROR, "Ignoring invalid layer index '%s' (not an integer) in line %ld\n", nl->str, (long)nl->line);
							continue;
						}
						if ((lidx < 0) || (lidx >= stack->maxlayer)) {
							pcb_message(PCB_MSG_ERROR, "Ignoring invalid layer index '%s' (out of range) in line %ld\n", nl->str, (long)nl->line);
							continue;
						}

						if (len > 0) {
							if (llidx != lidx) {
								if ((lx == x) && (ly == y)) {
									pcb_pstk_t *ps = pcb_pstk_new_compat_via(pcb->Data, x, y,
										conf_core.design.via_drilling_hole, via_dia, conf_core.design.clearance,
										0, PCB_PSTK_COMPAT_ROUND, pcb_true);
								}
								else
									pcb_message(PCB_MSG_ERROR, "Invalid via: not vertical, in line %ld\n", (long)nl->line);
							}
							else
								line = pcb_line_new(stack->copper[lidx], lx, ly, x, y, thick, clear, pcb_flag_make(PCB_FLAG_CLEARLINE));
						}
						lx = x;
						ly = y;
						llidx = lidx;
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
	cpcb_layers_t stk;

	PCB_ACT_CONVARG(1, FGW_STR, import_cpcb, fn = argv[1].val.str);

	f = pcb_fopen(fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not open %s for read\n", fn);
		PCB_ACT_IRES(-1);
		return 0;
	}

	cpcb_map_layers(PCB, &stk);
	cpcb_load(PCB, f, &stk);

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

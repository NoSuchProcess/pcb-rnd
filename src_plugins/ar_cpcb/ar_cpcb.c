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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <gensexpr/gsxl.h>
#include <genht/htpi.h>

#include "board.h"
#include "data.h"
#include "plugins.h"
#include "actions.h"
#include "safe_fs.h"
#include "conf_core.h"
#include "obj_pstk_inlines.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"
#include "src_plugins/lib_netmap/netmap.h"

static const char *cpcb_cookie = "cpcb plugin";

typedef struct {
	int maxlayer;
	pcb_layer_t *copper[PCB_MAX_LAYERGRP];
} cpcb_layers_t;

typedef struct {
	pcb_netmap_t netmap;

	/* int -> net conversion */
	pcb_lib_menu_t **i2n;
	int maxnets;

	/* net->int conversion */
	htpi_t n2i;
} cpcb_netmap_t;

static void cpcb_map_layers(pcb_board_t *pcb, cpcb_layers_t *dst)
{
	int gid;
	pcb_layergrp_t *grp;
	/* map copper layers from top to bottom */
	dst->maxlayer = 0;
	for(gid = 0, grp = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,grp++) {
		if ((grp->ltype & PCB_LYT_COPPER) && (grp->len > 0) && (grp->vis)) {
			dst->copper[dst->maxlayer] = pcb_get_layer(pcb->Data, grp->lid[0]);
			dst->maxlayer++;
		}
	}
}

static int cpcb_map_nets(pcb_board_t *pcb, cpcb_netmap_t *dst)
{
	htpp_entry_t *e;
	long id;

	if (pcb_netmap_init(&dst->netmap, pcb) != 0)
		return -1;

	dst->maxnets = 0;
	for(e = htpp_first(&dst->netmap.o2n); e != NULL; e = htpp_next(&dst->netmap.o2n, e))
		dst->maxnets++;

	if (dst->maxnets == 0)
		return -1;

	dst->i2n = malloc(sizeof(pcb_lib_menu_t *) * dst->maxnets);
	htpi_init(&dst->n2i, ptrhash, ptrkeyeq);

	id = 0;
	for(e = htpp_first(&dst->netmap.n2o); e != NULL; e = htpp_next(&dst->netmap.n2o, e)) {
		dst->i2n[id] = (pcb_lib_menu_t *)e->key;
		htpi_set(&dst->n2i, e->key, id);
		id++;
	}

	return 0;
}


static void cpcb_free_nets(cpcb_netmap_t *dst)
{
	htpi_uninit(&dst->n2i);
	free(dst->i2n);
	pcb_netmap_uninit(&dst->netmap);
}


static int cpcb_load(pcb_board_t *pcb, FILE *f, cpcb_layers_t *stack, cpcb_netmap_t *nmap)
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
									pcb_pstk_t *ps = pcb_pstk_new_compat_via(pcb->Data, -1, x, y,
										conf_core.design.via_drilling_hole, via_dia, conf_core.design.clearance,
										0, PCB_PSTK_COMPAT_ROUND, pcb_true);
								}
								else
									pcb_message(PCB_MSG_ERROR, "Invalid via: not vertical, in line %ld:%ld\n", (long)nl->line, (long)nl->col);
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

static void cpcb_print_pads(pcb_board_t *pcb, FILE *f, pcb_any_obj_t *o, cpcb_layers_t *stack)
{
	int lidx;

	switch(o->type) {
		case PCB_OBJ_PSTK:
			for(lidx = 0; lidx < stack->maxlayer; lidx++) {
				int n;
				pcb_pstk_t *ps = (pcb_pstk_t *)o;
				pcb_pstk_shape_t *shp = pcb_pstk_shape_at(pcb, ps, stack->copper[lidx]);
				if (shp == NULL)
					continue;
				switch(shp->shape) {
					case PCB_PSSH_LINE:
					case PCB_PSSH_HSHADOW:
						TODO("generate a poly");
						break;
					case PCB_PSSH_POLY:
						pcb_fprintf(f, "(0 %mm (%mm %mm %d) (", conf_core.design.clearance, ps->x, ps->y, lidx);
						for(n = 0; n < shp->data.poly.len; n++)
							pcb_fprintf(f, "(%mm %mm)", shp->data.poly.x[n], shp->data.poly.y[n]);
						fprintf(f, "))");
						break;
					case PCB_PSSH_CIRC:
						pcb_fprintf(f, "(%mm %mm (%mm %mm %d) ())", shp->data.circ.dia/2, conf_core.design.clearance, ps->x, ps->y, lidx);
						break;
				}
			}
			break;
		case PCB_OBJ_LINE:
			break;
		case PCB_OBJ_POLY:
			break;
		case PCB_OBJ_TEXT:
		case PCB_OBJ_ARC:
			break;

TODO("subc-in-subc: subc as terminal")
		case PCB_OBJ_SUBC:
			break;

		/* these can not ever be terminals */
		case PCB_OBJ_VOID:
		case PCB_OBJ_RAT:
		case PCB_OBJ_NET:
		case PCB_OBJ_NET_TERM:
		case PCB_OBJ_LAYER:
		case PCB_OBJ_LAYERGRP:
			break;
	}
}

static int cpcb_save(pcb_board_t *pcb, FILE *f, cpcb_layers_t *stack, cpcb_netmap_t *nmap)
{
	htpp_entry_t *e;

	/* print dims */
	pcb_fprintf(f, "(%d %d %d)\n", (int)(PCB_COORD_TO_MM(pcb->MaxWidth)+0.5), (int)(PCB_COORD_TO_MM(pcb->MaxHeight)+0.5), stack->maxlayer);

	/* print tracks */
	for(e = htpp_first(&nmap->netmap.n2o); e != NULL; e = htpp_next(&nmap->netmap.n2o, e)) {
		pcb_lib_menu_t *net = e->key;
		dyn_obj_t *o, *olist = e->value;
		long id = htpi_get(&nmap->n2i, net);

/*		pcb_fprintf(f, "# %s: %ld\n", net->Name, id);*/
		pcb_fprintf(f, "(%ld %mm %mm %mm\n", id, conf_core.design.line_thickness/2, conf_core.design.via_thickness/2, conf_core.design.clearance);

		/* print pads (terminals) */
		pcb_fprintf(f, "	(");
		for(o = olist; o != NULL; o = o->next)
			if (o->obj->term != NULL)
				cpcb_print_pads(pcb, f, o->obj, stack);
		pcb_fprintf(f, ")\n");

		fprintf(f, "	()\n");

		fprintf(f, ")\n");
	}

	/* print eof marker */
	fprintf(f, "()\n");
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
	cpcb_load(PCB, f, &stk, NULL);

	fclose(f);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_export_cpcb[] = "ExportcpcbTo(filename)";
static const char pcb_acth_export_cpcb[] = "Dumps the current board in c-pcb format.";
fgw_error_t pcb_act_export_cpcb(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fn;
	FILE *f;
	cpcb_layers_t stk;
	cpcb_netmap_t nmap;

	PCB_ACT_CONVARG(1, FGW_STR, export_cpcb, fn = argv[1].val.str);

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not open %s for write\n", fn);
		PCB_ACT_IRES(-1);
		return 0;
	}

	if (cpcb_map_nets(PCB, &nmap) != 0) {
		fclose(f);
		pcb_message(PCB_MSG_ERROR, "Failed to map nets\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	cpcb_map_layers(PCB, &stk);
	cpcb_save(PCB, f, &stk, &nmap);
	cpcb_free_nets(&nmap);

	fclose(f);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_cpcb[] = "cpcb(board|selected, [command])";
static const char pcb_acth_cpcb[] = "Executed external autorouter cpcb to route the board or parts of the board";
fgw_error_t pcb_act_cpcb(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *scope, *cmd = "cpcb", *tmpfn = "cpcb.tmp";
	char *cmdline;
	FILE *f;
	cpcb_layers_t stk;
	cpcb_netmap_t nmap;

	PCB_ACT_CONVARG(1, FGW_STR, cpcb, scope = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, cpcb, cmd = argv[2].val.str);

	if (strcmp(scope, "board") != 0) {
		pcb_message(PCB_MSG_ERROR, "Only board routing is supported at the moment\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	f = pcb_fopen(tmpfn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not open temp file %s for write\n", tmpfn);
		PCB_ACT_IRES(-1);
		return 0;
	}

	if (cpcb_map_nets(PCB, &nmap) != 0) {
		fclose(f);
		pcb_message(PCB_MSG_ERROR, "Failed to map nets\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	cpcb_map_layers(PCB, &stk);
	cpcb_save(PCB, f, &stk, &nmap);
	fclose(f);

	cmdline = pcb_strdup_printf("%s < %s", cmd, tmpfn);
	f = pcb_popen(cmdline, "r");
	if (f != NULL) {
		cpcb_load(PCB, f, &stk, NULL);
		pclose(f);
		PCB_ACT_IRES(0);
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Failed to execute c-pcb\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

/*	pcb_remove(tmpfn);*/
	free(cmdline);
	cpcb_free_nets(&nmap);
	return 0;
}

static pcb_action_t cpcb_action_list[] = {
	{"ImportcpcbFrom", pcb_act_import_cpcb, pcb_acth_import_cpcb, pcb_acts_import_cpcb},
	{"ExportcpcbTo", pcb_act_export_cpcb, pcb_acth_export_cpcb, pcb_acts_export_cpcb},
	{"cpcb", pcb_act_cpcb, pcb_acth_cpcb, pcb_acts_cpcb}
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

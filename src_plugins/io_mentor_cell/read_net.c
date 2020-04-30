/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
 *
 *  This module, io_mentor_cell, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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


static rnd_coord_t net_get_clearance_(hkp_ctx_t *ctx, rnd_layer_id_t lid, const hkp_netclass_t *nc, hkp_clearance_type_t type, node_t *errnode)
{
	if ((lid < 0) || (lid >= PCB_MAX_LAYER)) {
		hkp_error(errnode, "failed to determine clearance, falling back to default value\n");
		return RND_MIL_TO_COORD(12);
	}
	return ctx->nc_dflt.clearance[lid][type];
}

static rnd_coord_t net_get_clearance(hkp_ctx_t *ctx, pcb_layer_t *ly, const hkp_netclass_t *nc, hkp_clearance_type_t type, node_t *errnode)
{
	rnd_layer_id_t lid;

	if (ly == NULL) /* typically non-copper layer: clearance is 0 */
		return 0;

	ly = pcb_layer_get_real(ly);
	if (ly == NULL)
		return 0;
	lid = ly - ctx->pcb->Data->Layer;
	return net_get_clearance_(ctx, lid, nc, type, errnode);
}


static int io_mentor_cell_netclass(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fnc;
	node_t *n, *ncl, *nsch, *ns, *nln, *ndefault = NULL, *ncrs = NULL;
	hkp_tree_t nc_tree; /* no need to keep the tree in ctx, no data is needed after the function returns */

	fnc = rnd_fopen(&ctx->pcb->hidlib, fn, "r");
	if (fnc == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open netclass hkp '%s' for read\n", fn);
		return -1;
	}

	load_hkp(&nc_tree, fnc, fn);
	fclose(fnc);

	/* find the default NET_CLASS_SCHEME */
	for(nsch = nc_tree.root->first_child; nsch != NULL; nsch = nsch->next) {
		if (strcmp(nsch->argv[0], "NET_CLASS_SCHEME") == 0) {
			if (ndefault == NULL)
				ndefault = nsch;
			else if (strcmp(nsch->argv[1], "(Master)") == 0)
				ndefault = nsch;
		}
	}

	/* load default NET_CLASS_SCHEME's children */
	if (ndefault != NULL) {
		for(ncl = ndefault->first_child; ncl != NULL; ncl = ncl->next) {
			if (strcmp(ncl->argv[0], "CLEARANCE_RULE_SET") == 0) {
				if (ncrs == NULL)
					ncrs = ncl;
				else if (strcmp(ncl->argv[1], "(Default Rule)") == 0)
					ncrs = ncl;
			}
		}
	}

	if (ncrs == NULL) {
		tree_destroy(&nc_tree);
		rnd_message(RND_MSG_ERROR, "netclass hkp '%s' does not contain any NET_CLASS_SCHEME/CLEARANCE_RULE_SET section\n", fn);
		return -1;
	}

	/* load default CLEARANCE_RULE_SET's children */
	for(ns = ncrs->first_child; ns != NULL; ns = ns->next) {
		rnd_layergrp_id_t gid;
		pcb_layergrp_t *grp;
		rnd_coord_t val;
		int i;

		if (strcmp(ns->argv[0], "SUBRULE") != 0) continue;
		
		nln = find_nth(ns->first_child, "LAYER_NUM", 0);
		if (nln == NULL) continue; /* layer number is required */

		/* layer number is in copper offset, translate it to layer ID */
		gid = pcb_layergrp_step(ctx->pcb, pcb_layergrp_get_top_copper(), atoi(nln->argv[1])-1, PCB_LYT_COPPER);
		grp = pcb_get_layergrp(ctx->pcb, gid);
		if ((grp == NULL) || (grp->len < 1)) continue;

		for(n = ns->first_child; n != NULL; n = n->next) {
			hkp_clearance_type_t type;

			if (parse_coord(ctx, n->argv[1], &val) != 0) {
				hkp_error(n, "Ignoring invalid clearance value '%s'\n", n->argv[1]);
				continue;
			}

			if (strcmp(n->argv[0], "PLANE_TO_TRACE") == 0) type = HKP_CLR_POLY2TRACE;
			else if (strcmp(n->argv[0], "PLANE_TO_PAD") == 0) type = HKP_CLR_POLY2TERM;
			else if (strcmp(n->argv[0], "PLANE_TO_VIA") == 0) type = HKP_CLR_POLY2VIA;
			else if (strcmp(n->argv[0], "PLANE_TO_PLANE") == 0) type = HKP_CLR_POLY2POLY;
			else continue; /* ignore the rest for now */

			for(i = 0; i < grp->len; i++) {
				rnd_layer_id_t lid = grp->lid[i];
				ctx->nc_dflt.clearance[lid][type] = val;
			}
		}
	}

	return 0;
}

static int io_mentor_cell_netlist(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fnet;
	node_t *p, *nnet, *pinsect;
	hkp_tree_t net_tree; /* no need to keep the tree in ctx, no data is needed after the function returns */

	fnet = rnd_fopen(&ctx->pcb->hidlib, fn, "r");
	if (fnet == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open netprops hkp '%s' for read\n", fn);
		return -1;
	}

	load_hkp(&net_tree, fnet, fn);
	fclose(fnet);

	for(nnet = net_tree.root->first_child; nnet != NULL; nnet = nnet->next) {
		if (strcmp(nnet->argv[0], "NETNAME") == 0) {
			pcb_net_t *net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_INPUT], nnet->argv[1], PCB_NETA_ALLOC);
			if (net == NULL) {
				hkp_error(nnet, "Failed to create net '%s' - netlist will be incomplete\n", nnet->argv[1]);
				continue;
			}
			pinsect = find_nth(nnet->first_child, "PIN_SECTION", 0);
			if (pinsect == NULL)
				continue;
			for(p = pinsect->first_child; p != NULL; p = p->next) {
				pcb_net_term_t *term;
				if (strcmp(p->argv[0], "REF_PINNAME") != 0)
					continue;
				term = pcb_net_term_get_by_pinname(net, p->argv[1], PCB_NETA_ALLOC);
				if (term == NULL)
					hkp_error(p, "Failed to create pin '%s' in net '%s' - netlist will be incomplete\n", p->argv[1], nnet->argv[1]);
			}
		}
	}

	tree_destroy(&net_tree);
	return 0;
}

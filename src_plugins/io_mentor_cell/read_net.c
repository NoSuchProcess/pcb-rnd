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

static int io_mentor_cell_netclass(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fnc;
	node_t *n, *ncl, *nsch, *ns, *ndefault = NULL, *ncrs = NULL;
	hkp_tree_t nc_tree; /* no need to keep the tree in ctx, no data is needed after the function returns */

	fnc = pcb_fopen(&ctx->pcb->hidlib, fn, "r");
	if (fnc == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open netclass hkp '%s' for read\n", fn);
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
		pcb_message(PCB_MSG_ERROR, "netclass hkp '%s' does not contain any NET_CLASS_SCHEME/CLEARANCE_RULE_SET section\n", fn);
		return -1;
	}

	/* load default CLEARANCE_RULE_SET's children */
	for(ns = ncrs->first_child; ns != NULL; ns = ns->next) {
		if (strcmp(ns->argv[0], "SUBRULE") != 0) continue;
		for(n = ns->first_child; n != NULL; n = n->next) {
			hkp_clearance_type_t type;
			if (strcmp(n->argv[0], "PLANE_TO_TRACE") != 0) type = HKP_CLR_POLY2TRACE;
			else if (strcmp(n->argv[0], "PLANE_TO_PAD") != 0) type = HKP_CLR_POLY2TERM;
			else if (strcmp(n->argv[0], "PLANE_TO_VIA") != 0) type = HKP_CLR_POLY2VIA;
			else if (strcmp(n->argv[0], "PLANE_TO_PLANE") != 0) type = HKP_CLR_POLY2POLY;
			else continue; /* ignore the rest for now */
		}
	}

	return 0;
}

static int io_mentor_cell_netlist(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fnet;
	node_t *p, *nnet, *pinsect;
	hkp_tree_t net_tree; /* no need to keep the tree in ctx, no data is needed after the function returns */

	fnet = pcb_fopen(&ctx->pcb->hidlib, fn, "r");
	if (fnet == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open netprops hkp '%s' for read\n", fn);
		return -1;
	}

	load_hkp(&net_tree, fnet, fn);
	fclose(fnet);

	for(nnet = net_tree.root->first_child; nnet != NULL; nnet = nnet->next) {
		if (strcmp(nnet->argv[0], "NETNAME") == 0) {
			pcb_net_t *net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_INPUT], nnet->argv[1], 1);
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
				term = pcb_net_term_get_by_pinname(net, p->argv[1], 1);
				if (net == NULL)
					hkp_error(p, "Failed to create pin '%s' in net '%s' - netlist will be incomplete\n", p->argv[1], nnet->argv[1]);
			}
		}
	}

	tree_destroy(&net_tree);
	return 0;
}

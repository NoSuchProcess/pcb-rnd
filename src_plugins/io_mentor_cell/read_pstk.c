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

static hkp_pstk_t *parse_pstk(hkp_ctx_t *ctx, const char *ps)
{
	const pcb_unit_t *old_unit;
	pcb_coord_t ox = 0, oy = 0;
	node_t *n, *hn, *on;
	hkp_pstk_t *p = htsp_get(&ctx->pstks, ps);

	if (p == NULL)
		return NULL;
	if (p->valid)
		return p;

	n = find_nth(p->subtree->first_child, "TECHNOLOGY", 0);
	if (n == NULL) {
		pcb_message(PCB_MSG_ERROR, "Padstack without technology\n");
		return NULL;
	}

	old_unit = ctx->unit;
	ctx->unit = p->unit;

	/* parse the padstack */
	for(n = n->first_child; n != NULL; n = n->next) {
		hn = find_nth(n, "HOLE_NAME", 0);
		if (hn != NULL) {
			on = find_nth(hn->first_child, "OFFSET", 0);
			if (on != NULL) {
				parse_xy(ctx, on->argv[1], &ox, &oy);
				if (ox != 0) ox = -ox;
				if (oy != 0) oy = -oy;
			}
TODO("find the hole by name hn->argv[1]");
		}
	}
	ctx->unit = old_unit;
	return p;
}


static void parse_pin(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *nd)
{
	node_t *tmp;
	pcb_coord_t px, py;
	hkp_pstk_t *hpstk;

	tmp = find_nth(nd->first_child, "XY", 0);
	if (tmp == NULL) {
		pcb_message(PCB_MSG_ERROR, "Ignoring pin with no coords\n");
		return;
	}
	parse_xy(ctx, tmp->argv[1], &px, &py);

	tmp = find_nth(nd->first_child, "PADSTACK", 0);
	if (tmp == NULL) {
		pcb_message(PCB_MSG_ERROR, "Ignoring pin with no padstack\n");
		return;
	}
	hpstk = parse_pstk(ctx, tmp->argv[1]);
	if (hpstk == NULL) {
		pcb_message(PCB_MSG_ERROR, "Ignoring pin with undefined padstack '%s'\n", tmp->argv[1]);
		return;
	}
}

static int io_mentor_cell_pstks(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fpstk;
	node_t *n;

	fpstk = pcb_fopen(&PCB->hidlib, fn, "r");
	if (fpstk == NULL)
		return -1;

	load_hkp(&ctx->padstacks, fpstk);

	/* build the indices */
	htsp_init(&ctx->shapes, strhash, strkeyeq);
	htsp_init(&ctx->holes, strhash, strkeyeq);
	htsp_init(&ctx->pstks, strhash, strkeyeq);

	for(n = ctx->padstacks.root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "UNITS") == 0) {
			ctx->pstk_unit = parse_units(n->argv[1]);
			if (ctx->pstk_unit == NULL) {
				pcb_message(PCB_MSG_ERROR, "Unknown unit '%s'\n", n->argv[1]);
				return -1;
			}
		}
		else if (strcmp(n->argv[0], "PAD") == 0) {
			if (!htsp_has(&ctx->shapes, n->argv[1])) {
				hkp_shape_t *shp = calloc(sizeof(hkp_shape_t), 1);
				shp->subtree = n;
				shp->unit = ctx->pstk_unit;
				htsp_insert(&ctx->shapes, n->argv[1], shp);
			}
			else {
				pcb_message(PCB_MSG_ERROR, "Duplicate PAD '%s'\n", n->argv[1]);
				return -1;
			}
		}
		else if (strcmp(n->argv[0], "HOLE") == 0) {
			if (!htsp_has(&ctx->holes, n->argv[1])) {
				hkp_shape_t *hole = calloc(sizeof(hkp_hole_t), 1);
				hole->subtree = n;
				hole->unit = ctx->pstk_unit;
				htsp_insert(&ctx->holes, n->argv[1], hole);
			}
			else {
				pcb_message(PCB_MSG_ERROR, "Duplicate HOLE '%s'\n", n->argv[1]);
				return -1;
			}
		}
		else if (strcmp(n->argv[0], "PADSTACK") == 0) {
			if (!htsp_has(&ctx->pstks, n->argv[1])) {
				hkp_shape_t *pstk = calloc(sizeof(hkp_pstk_t), 1);
				pstk->subtree = n;
				pstk->unit = ctx->pstk_unit;
				htsp_insert(&ctx->pstks, n->argv[1], pstk);
			}
			else {
				pcb_message(PCB_MSG_ERROR, "Duplicate PADSTACK '%s'\n", n->argv[1]);
				return -1;
			}
		}
	}

	TODO("parse padstacks");

	fclose(fpstk);
	return 0;
}

static int free_pstks(hkp_ctx_t *ctx)
{
	htsp_entry_t *e;

	for(e = htsp_first(&ctx->shapes); e != NULL; e = htsp_next(&ctx->shapes, e)) {
		hkp_shape_t *shp = e->value;
		free(shp);
	}
	htsp_uninit(&ctx->shapes);

	for(e = htsp_first(&ctx->holes); e != NULL; e = htsp_next(&ctx->holes, e)) {
		hkp_hole_t *hole = e->value;
		free(hole);
	}
	htsp_uninit(&ctx->holes);

	for(e = htsp_first(&ctx->pstks); e != NULL; e = htsp_next(&ctx->pstks, e)) {
		hkp_pstk_t *pstk = e->value;
		free(pstk);
	}
	htsp_uninit(&ctx->pstks);
}

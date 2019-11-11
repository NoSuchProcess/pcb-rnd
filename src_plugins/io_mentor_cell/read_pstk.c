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

#if 0
static void parse_pstk(hkp_ctx_t *ctx, pcb_subc_t *subc, char *ps, pcb_coord_t px, pcb_coord_t py, char *name)
{
	pcb_flag_t flags = pcb_no_flags();
	pcb_coord_t thickness, hole, ms;
	char *curr, *next;
	int sqpad_pending = 0;

	thickness = 0;
	hole = 0;
	ms = 0;

	for(curr = ps; curr != NULL; curr = next) {
		next = strchr(curr, ',');
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		ltrim(curr);
		if (strncmp(curr, "Pad", 3) == 0) {
			curr += 4;
			ltrim(curr);
			if (strncmp(curr, "Square", 6) == 0) {
				curr += 6;
				ltrim(curr);
				flags = pcb_flag_add(flags, PCB_FLAG_SQUARE);
				thickness = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
				sqpad_pending = 1;
			}
			if (strncmp(curr, "Rectangle", 9) == 0) {
				char *hs;
				pcb_coord_t w, h;
				curr += 9;
				ltrim(curr);
				thickness = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
				hs = strchr(curr, 'x');
				if (hs == NULL) {
					pcb_message(PCB_MSG_ERROR, "Broken Rectangle pad: no 'x' in size\n");
					return;
				}
				*hs = '\0';
				hs++;
				w = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
				h = pcb_get_value(hs, ctx->unit->suffix, NULL, NULL);
TODO("padstack: rewrite")
#if 0
				pcb_element_pad_new_rect(elem, px+w/2, py+h/2, px-w/2, py-h/2, cl, ms, name, name, flags);
#else
				(void)w; (void)h;
#endif
			}
			else if (strncmp(curr, "Round", 5) == 0) {
				curr += 5;
				ltrim(curr);
				thickness = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
			}
			else if (strncmp(curr, "Oblong", 6) == 0) {
				pcb_message(PCB_MSG_ERROR, "Ignoring oblong pin - not yet supported\n");
				return;
			}
			else {
				pcb_message(PCB_MSG_ERROR, "Ignoring unknown pad shape: %s\n", curr);
				return;
			}
		}
		else if (strncmp(curr, "Hole Rnd", 8) == 0) {
			curr += 8;
			ltrim(curr);
			hole = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
		}
	}

TODO("padstack: rewrite")
#if 0
	if (hole > 0) {
		pcb_element_pin_new(elem, px, py, thickness, cl, ms, hole, name, name, flags);
		sqpad_pending = 0;
	}

	if (sqpad_pending)
		pcb_element_pad_new_rect(elem, px, py, px+thickness, py+thickness, cl, ms, name, name, flags);
#else
	(void)sqpad_pending; (void)ms; (void)hole; (void)thickness;
#endif
}
#endif

#if 0
static void parse_pin(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *nd)
{
	node_t *tmp;
	pcb_coord_t px, py;

	tmp = find_nth(nd->first_child, "XY", 0);
	if (tmp == NULL) {
		pcb_message(PCB_MSG_ERROR, "Ignoring pin with no coords\n");
		return;
	}
	parse_xy(ctx, tmp->argv[1], &px, &py);

	tmp = find_nth(nd->first_child, "PADSTACK", 0);
	if (tmp != NULL)
		parse_pstk(ctx, subc, tmp->argv[1], px, py, nd->argv[1]);
}
#endif

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

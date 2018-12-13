/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* Follow galvanic connections through overlapping objects */

#include "config.h"
#include "find2.h"

/* trickery: keeping vtp0 is reentrant and is cheaper than keeping lists,
   at least for appending. But as long as only the last item is removed,
   it's also cheap on remove! */

/* Do everything that needs to be done for an object found */
static void pcb_find_found(pcb_find_t *ctx, pcb_any_obj_t *obj)
{
	if (ctx->list_found)
		vtp0_append(&ctx->found, obj);
	ctx->nfound++;
}


static void pcb_find_addobj(pcb_find_t *ctx, pcb_any_obj_t *obj)
{
	PCB_DFLAG_SET(&obj->Flags, ctx->mark);
	vtp0_append(&ctx->open, obj);
}

#define PCB_FIND_CHECK(ctx, curr, obj) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)obj; \
		if (!(PCB_DFLAG_TEST(&(__obj__->Flags), ctx->mark))) { \
			if (pcb_intersect_obj_obj(curr, __obj__)) \
				pcb_find_addobj(ctx, __obj__); \
		} \
	} while(0)

static unsigned long pcb_find_exec(pcb_find_t *ctx)
{
	while(ctx->open.used > 0) {
		pcb_any_obj_t *curr;

		/* pop the last object, without reallocating to smaller, mark it found */
		ctx->open.used--;
		curr = ctx->open.array[ctx->open.used];
		pcb_find_found(ctx, curr);

		{ /* search unmkared connections: iterative approach */
			pcb_rtree_it_t it;
			pcb_box_t *n;
			pcb_rtree_box_t *sb = (pcb_rtree_box_t *)&curr->bbox_naked;
			int li;
			pcb_layer_t *l;

			if (PCB->Data->padstack_tree != NULL) {
				for(n = pcb_rtree_first(&it, PCB->Data->padstack_tree, sb); n != NULL; n = pcb_rtree_next(&it))
					PCB_FIND_CHECK(ctx, curr, n);
				pcb_r_end(&it);
			}

			for(li = 0, l = PCB->Data->Layer; li < PCB->Data->LayerN; li++,l++) {
				if (l->line_tree != NULL) {
					for(n = pcb_rtree_first(&it, l->line_tree, sb); n != NULL; n = pcb_rtree_next(&it))
						PCB_FIND_CHECK(ctx, curr, n);
					pcb_r_end(&it);
				}

				if (l->arc_tree != NULL) {
					for(n = pcb_rtree_first(&it, l->arc_tree, sb); n != NULL; n = pcb_rtree_next(&it))
						PCB_FIND_CHECK(ctx, curr, n);
					pcb_r_end(&it);
				}

				if (l->polygon_tree != NULL) {
					for(n = pcb_rtree_first(&it, l->polygon_tree, sb); n != NULL; n = pcb_rtree_next(&it))
						PCB_FIND_CHECK(ctx, curr, n);
					pcb_r_end(&it);
				}

				if (l->text_tree != NULL) {
					for(n = pcb_rtree_first(&it, l->text_tree, sb); n != NULL; n = pcb_rtree_next(&it))
						PCB_FIND_CHECK(ctx, curr, n);
					pcb_r_end(&it);
				}
			}
		}
	}
	return ctx->nfound;
}

static int pcb_find_init_(pcb_find_t *ctx, pcb_data_t *data)
{
	if (ctx->in_use)
		return -1;
	ctx->in_use = 1;
	ctx->mark = pcb_dynflag_alloc("pcb_find_from_obj");
	ctx->data = data;
	ctx->nfound = 0;

	if (ctx->list_found)
		vtp0_init(&ctx->found);
	vtp0_init(&ctx->open);
	return 0;
}

unsigned long pcb_find_from_obj(pcb_find_t *ctx, pcb_data_t *data, pcb_any_obj_t *from)
{
	if (pcb_find_init_(ctx, data) < 0)
		return -1;

	pcb_data_dynflag_clear(data, ctx->mark);
	pcb_find_addobj(ctx, from);
	return pcb_find_exec(ctx);
}



void pcb_find_free(pcb_find_t *ctx)
{
	if (!ctx->in_use)
		return;
	if (ctx->list_found)
		vtp0_uninit(&ctx->found);
	vtp0_uninit(&ctx->open);
	pcb_dynflag_free(ctx->mark);
	ctx->in_use = 0;
}


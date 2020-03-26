/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include "board.h"
#include "find.h"
#include "drc_net_int.h"

TODO("remove:")
#include "drc.h"

#define PCB dontuse

TODO("find: get rid of this global state")
extern pcb_coord_t Bloat;


/* evaluates to true if obj was marked on list (fa or fb) */
#define IS_FOUND(obj, list) (PCB_DFLAG_TEST(&(obj->Flags), ctx->list.mark))

static int pcb_int_broken_cb(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	pcb_net_int_t *ctx = fctx->user_data;

	if (arrived_from == NULL) /* ingore the starting object - it must be marked as we started from the same object in the first search */
		return 0;

	/* broken if new object is not marked in the shrunk search (fa) but
	   the arrived_from object was marked (so we notify direct breaks only) */
	if (!IS_FOUND(new_obj, fa) && IS_FOUND(arrived_from, fa)) {
		int r = ctx->broken_cb(ctx, new_obj, arrived_from, ctype);
		if (r != 0) return r;
		return ctx->fast; /* if fast is 1, we are aborting the search, returning the first hit only */
	}
	return 0;
}

pcb_bool pcb_net_integrity(pcb_board_t *pcb, pcb_view_list_t *lst, pcb_any_obj_t *from, pcb_coord_t shrink, pcb_coord_t bloat, pcb_int_broken_cb_t *cb, void *cb_data)
{
	pcb_net_int_t ctx;

	ctx.pcb = pcb;
	ctx.fast = 1;
	ctx.data = pcb->Data;
	ctx.lst = lst;
	ctx.bloat = bloat;
	ctx.shrink = shrink;
	ctx.broken_cb = cb;
	ctx.cb_data = cb_data;

	memset(&ctx.fa, 0, sizeof(ctx.fa));
	memset(&ctx.fb, 0, sizeof(ctx.fb));
	ctx.fa.user_data = ctx.fb.user_data = &ctx;
	ctx.fb.found_cb = pcb_int_broken_cb;

	/* Check for minimal overlap: shrink mark all objects on the net in fa;
	   restart the search without shrink in fb: if anything new is found, it did
	   not have enough overlap and shrink disconnected it. */
	if (shrink != 0) {
		ctx.shrunk = 1;
		Bloat = -shrink;
		pcb_find_from_obj(&ctx.fa, pcb->Data, from);
		Bloat = 0;
		pcb_find_from_obj(&ctx.fb, pcb->Data, from);
		pcb_find_free(&ctx.fa);
		pcb_find_free(&ctx.fb);
	}

	/* Check for minimal distance: bloat mark all objects on the net in fa;
	   restart the search without bloat in fb: if anything new is found, it did
	   not have a connection without the bloat. */
	if (bloat != 0) {
		ctx.shrunk = 0;
		Bloat = 0;
		pcb_find_from_obj(&ctx.fa, pcb->Data, from);
		Bloat = bloat;
		pcb_find_from_obj(&ctx.fb, pcb->Data, from);
		pcb_find_free(&ctx.fa);
		pcb_find_free(&ctx.fb);
	}

	Bloat = 0;

	return pcb_false;
}

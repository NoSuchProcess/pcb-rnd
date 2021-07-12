/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2021)
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
#include "pstklib.h"
#include <genht/ht_utils.h>

void pcb_pstklib_init(pcb_pstklib_t *ctx, pcb_board_t *pcb)
{
	memset(&ctx->protos, 0, sizeof(ctx->protos));
	ctx->pcb = pcb;
	ctx->next_id = 1;
	htprp_init(&ctx->protos, pcb_pstk_proto_hash, pcb_pstk_proto_eq);
}

void pcb_pstklib_uninit(pcb_pstklib_t *ctx)
{
	genht_uninit_deep(htprp, &ctx->protos, {
		pcb_pstklib_entry_t *pe = htent->value;
		if (ctx->on_free_entry != NULL)
			ctx->on_free_entry(ctx, pe);
		free(pe);
	});
}

void pcb_pstklib_build_data(pcb_pstklib_t *ctx, pcb_data_t *data)
{
	long n;
	for(n = 0; n < data->ps_protos.used; n++) {
		pcb_pstk_proto_t *proto = &data->ps_protos.array[n];

		if (!proto->in_use) continue;

	}
}


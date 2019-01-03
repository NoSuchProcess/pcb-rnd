/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include "placement.h"

void pcb_placement_init(pcb_placement_t *ctx, pcb_board_t *pcb)
{
	memset(ctx, 0, sizeof(pcb_placement_t));
	htscp_init(&ctx->subcs, pcb_subc_hash, pcb_subc_eq);
	pcb_data_init(&ctx->data);
	pcb_data_bind_board_layers(pcb, &ctx->data, 0);
	ctx->pcb = pcb;
}

void pcb_placement_uninit(pcb_placement_t *ctx)
{
	htscp_uninit(&ctx->subcs);
	pcb_data_uninit(&ctx->data);
}

void pcb_placement_build(pcb_placement_t *ctx, pcb_data_t *data)
{
	PCB_SUBC_LOOP(data) {
		if (!htscp_has(&ctx->subcs, subc)) {
			pcb_host_trans_t tr;
			pcb_subc_t *proto = pcb_subc_dup_at(ctx->pcb, &ctx->data, subc, 0, 0, 1);
			pcb_subc_get_host_trans(subc, &tr, 1);
			pcb_subc_move(proto, tr.ox, tr.oy, 1);
			if (tr.rot != 0) {
				double rr = tr.rot / PCB_RAD_TO_DEG;
				pcb_subc_rotate(proto, 0, 0, cos(rr), sin(rr), tr.rot);
			}
			if (tr.on_bottom) {
				int n;
				pcb_data_mirror(proto->data, 0, PCB_TXM_SIDE, 1);
				for(n = 0; n < proto->data->LayerN; n++) {
					pcb_layer_t *ly = proto->data->Layer + n;
					ly->meta.bound.type = pcb_layer_mirror_type(ly->meta.bound.type);
					ly->meta.bound.stack_offs = -ly->meta.bound.stack_offs;
				}
			}

			htscp_insert(&ctx->subcs, subc, proto);
		}
	}
	PCB_END_LOOP;
}

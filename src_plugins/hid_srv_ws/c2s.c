/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Needed by libuv to compile in C99 */
#define _POSIX_C_SOURCE 200112

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>

#include "error.h"
#include "server.h"
#include "c2s.h"

typedef struct {
	pcb_c2s_msg_t msg;
} dgram_t;

void hid_ws_recv_msg(hid_srv_ws_t *ctx)
{
	dgram_t dg;
	int len;

	len = recv(ctx->c2s[0], &dg, sizeof(dg), MSG_DONTWAIT);
	if (len != sizeof(dg)) {
		pcb_message(PCB_MSG_ERROR, "websocket [%d]: server msg invliad size %d (expected %d)\n", ctx->pid, len, sizeof(dg));
		return;
	}

	switch(dg.msg) {
		case PCB_C2S_MSG_START:
			if (ctx->pollfds[ctx->listen_idx].fd < 0)
				ctx->pollfds[ctx->listen_idx].fd = -ctx->pollfds[ctx->listen_idx].fd; /* enable listen because the client finished accepting the connection */
			pcb_message(PCB_MSG_INFO, "websocket [%d]: new client conn acked, reenabling listen\n", ctx->pid);
			break;
		default:
			pcb_message(PCB_MSG_ERROR, "websocket [%d]: server msg invliad msg %d\n", ctx->pid, dg.msg);
	}
}

void hid_ws_send_msg(hid_srv_ws_t *ctx, pcb_c2s_msg_t msg)
{
	dgram_t dg;

	dg.msg = msg;

	send(ctx->c2s[1], &dg, sizeof(dg), 0);
}

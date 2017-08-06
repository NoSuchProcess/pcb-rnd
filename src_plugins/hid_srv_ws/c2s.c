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

void hid_ws_recv_msg(hid_srv_ws_t *ctx)
{
	char msg[256];
	int len;

	*msg = 0;
	len = recv(ctx->c2s[0], msg, sizeof(msg), MSG_DONTWAIT);
	msg[len] = '\0';
	pcb_message(PCB_MSG_INFO, "SERVER MSG: %d/'%s'\n", len, msg);
}

void hid_ws_send_msg(hid_srv_ws_t *ctx)
{
	send(ctx->c2s[1], "jajj", 4, 0);
}

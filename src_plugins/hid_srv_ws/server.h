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

#ifndef PCB_HID_SRV_WS_SERER_H
#define PCB_HID_SRV_WS_SERER_H

#include <sys/types.h>
#include <libwebsockets.h>

/* Number of sockets to handle - need one listening socket and one accepted
   because each client is forked; leave some room for retries and whatnot */
#define WS_MAX_FDS 8

typedef struct {
	struct lws_context *context;
	struct lws_pollfd pollfds[WS_MAX_FDS];
	int fd_lookup[1024];
	int count_pollfds;
	int listen_fd;
	pid_t pid;
	int num_clients;

	int c2s[2]; /* Client to server communication; 0 is read by the server, 1 is written by clients */
} hid_srv_ws_t;

#endif

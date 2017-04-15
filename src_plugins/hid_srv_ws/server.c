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

/* Websocket server HID: listen for connections and fork session servers */

#include "config.h"

#include <stdio.h>

#include "plugins.h"

int pplg_check_ver_hid_srv_ws(int ver_needed) { return 0; }

extern int pplg_init_hid_remote(void);
extern void pcb_set_hid_name(const char *new_hid_name);


void pplg_init_hid_srv_ws(void)
{
}

int pplg_init_hid_srv_ws(void)
{
	printf("WS: waiting for connections\n");
	pcb_set_hid_name("remote");

	return 0;
}

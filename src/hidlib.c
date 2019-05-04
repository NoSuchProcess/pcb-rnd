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

#include "conf_core.h"
#include "hidlib_conf.h"
#include "tool.h"
#include "event.h"
#include "hid.h"

static const char *hidlib_cookie = "hidlib";

static void hidlib_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_tool_gui_init();
	pcb_gui->set_mouse_cursor(hidlib, conf_core.editor.mode); /* make sure the mouse cursor is set up now that it is registered */
}


void pcb_hidlib_event_uninit(void)
{
	pcb_event_unbind_allcookie(hidlib_cookie);
}

void pcb_hidlib_event_init(void)
{
	pcb_event_bind(PCB_EVENT_GUI_INIT, hidlib_gui_init_ev, NULL, hidlib_cookie);
}

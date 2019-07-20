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

#include "hidlib_conf.h"
#include "tool.h"
#include "event.h"
#include "hid.h"

static const char *hidlib_cookie = "hidlib";

static void hidlib_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_tool_gui_init();
	pcb_gui->set_mouse_cursor(pcb_gui, pcbhl_conf.editor.mode); /* make sure the mouse cursor is set up now that it is registered */
}

void pcbhl_log_print_uninit_errs(const char *title)
{
	pcb_logline_t *n, *from = pcb_log_find_first_unseen();
	int printed = 0;

	for(n = from; n != NULL; n = n->next) {
		if ((n->level >= PCB_MSG_INFO) || pcbhl_conf.rc.verbose) {
			if (!printed)
				fprintf(stderr, "*** %s:\n", title);
			fprintf(stderr, "%s", n->str);
			printed = 1;
		}
	}
	if (printed)
		fprintf(stderr, "\n\n");
}


void pcb_hidlib_event_uninit(void)
{
	pcb_event_unbind_allcookie(hidlib_cookie);
}

void pcb_hidlib_event_init(void)
{
	pcb_event_bind(PCB_EVENT_GUI_INIT, hidlib_gui_init_ev, NULL, hidlib_cookie);
}

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

#include <librnd/config.h>

#include <librnd/core/hidlib_conf.h>
#include <librnd/core/tool.h>
#include <librnd/core/event.h>
#include <librnd/core/hid.h>

static const char *hidlib_cookie = "hidlib";

void rnd_hidcore_crosshair_move_to(rnd_hidlib_t *hidlib, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot)
{
	if (mouse_mot)
		rnd_event(hidlib, RND_EVENT_STROKE_RECORD, "cc", abs_x, abs_y);
	rnd_hidlib_crosshair_move_to(hidlib, abs_x, abs_y, mouse_mot);
}


static void hidlib_gui_init_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	rnd_tool_gui_init();
	rnd_gui->set_mouse_cursor(rnd_gui, rnd_conf.editor.mode); /* make sure the mouse cursor is set up now that it is registered */
}

void rnd_log_print_uninit_errs(const char *title)
{
	rnd_logline_t *n, *from = rnd_log_find_first_unseen();
	int printed = 0;

	for(n = from; n != NULL; n = n->next) {
		if ((n->level >= RND_MSG_INFO) || rnd_conf.rc.verbose) {
			if (!printed)
				fprintf(stderr, "*** %s:\n", title);
			fprintf(stderr, "%s", n->str);
			printed = 1;
		}
	}
	if (printed)
		fprintf(stderr, "\n\n");
}


void rnd_hidlib_event_uninit(void)
{
	rnd_event_unbind_allcookie(hidlib_cookie);
}

void rnd_hidlib_event_init(void)
{
	rnd_event_bind(RND_EVENT_GUI_INIT, hidlib_gui_init_ev, NULL, hidlib_cookie);
}

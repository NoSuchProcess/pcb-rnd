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

#include <stdlib.h>
#include <locale.h>

#include "hidlib_conf.h"
#include "tool.h"
#include "event.h"
#include "hid.h"
#include "hid_init.h"
#include "compat_misc.h"

static const char *hidlib_cookie = "hidlib";

static void hidlib_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_tool_gui_init();
	pcb_gui->set_mouse_cursor(hidlib, pcbhl_conf.editor.mode); /* make sure the mouse cursor is set up now that it is registered */
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

/* parse arguments using the gui; if fails and fallback is enabled, try the next gui */
int pcb_gui_parse_arguments(int autopick_gui, int *hid_argc, char **hid_argv[])
{
	conf_listitem_t *apg = NULL;

	if ((autopick_gui >= 0) && (pcbhl_conf.rc.hid_fallback)) { /* start from the GUI we are initializing first */
		int n;
		const char *g;

		conf_loop_list_str(&pcbhl_conf.rc.preferred_gui, apg, g, n) {
			if (n == autopick_gui)
				break;
		}
	}

	for(;;) {
		int res;
		if (pcb_gui->get_export_options != NULL)
			pcb_gui->get_export_options(NULL);
		res = pcb_gui->parse_arguments(hid_argc, hid_argv);
		if (res == 0)
			break; /* HID accepted, don't try anything else */
		if (res < 0) {
			pcb_message(PCB_MSG_ERROR, "Failed to initialize HID %s (unrecoverable, have to give up)\n", pcb_gui->name);
			return -1;
		}
		fprintf(stderr, "Failed to initialize HID %s (recoverable)\n", pcb_gui->name);
		if (apg == NULL) {
			if (pcbhl_conf.rc.hid_fallback) {
				ran_out_of_hids:;
				pcb_message(PCB_MSG_ERROR, "Tried all available HIDs, all failed, giving up.\n");
			}
			else
				pcb_message(PCB_MSG_ERROR, "Not trying any other hid as fallback because rc/hid_fallback is disabled.\n");
			return -1;
		}

		/* falling back to the next HID */
		do {
			int n;
			const char *g;

			apg = conf_list_next_str(apg, &g, &n);
			if (apg == NULL)
				goto ran_out_of_hids;
			pcb_gui = pcb_hid_find_gui(g);
		} while(pcb_gui == NULL);
	}
	return 0;
}

void pcb_fix_locale(void)
{
	static const char *lcs[] = { "LANG", "LC_NUMERIC", "LC_ALL", NULL };
	const char **lc;

	/* some Xlib calls tend ot hardwire setenv() to "" or NULL so a simple
	   setlocale() won't do the trick on GUI. Also set all related env var
	   to "C" so a setlocale(LC_ALL, "") will also do the right thing. */
	for(lc = lcs; *lc != NULL; lc++)
		pcb_setenv(*lc, "C", 1);

	setlocale(LC_ALL, "C");
}

int pcb_main_arg_match(const char *in, const char *shrt, const char *lng)
{
	return ((shrt != NULL) && (strcmp(in, shrt) == 0)) || ((lng != NULL) && (strcmp(in, lng) == 0));
}

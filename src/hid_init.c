/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <locale.h>

#include "hid.h"
#include "hid_nogui.h"
#include "event.h"

/* for dlopen() and friends; will also solve all system-dependent includes
   and provides a dl-compat layer on windows. Also solves the opendir related
   includes. */
#include "plugins.h"
#include "actions.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "hid_dad_unit.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "compat_inc.h"
#include "file_loaded.h"
#include "hidlib.h"
#include "hidlib_conf.h"

static const char *flt_any[] = {"*", "*.*", NULL};

const pcb_hid_fsd_filter_t pcb_hid_fsd_filter_any[] = {
	{ "all", NULL, flt_any },
	{ NULL, NULL, NULL }
};

pcb_hid_t **pcb_hid_list = 0;
int pcb_hid_num_hids = 0;

pcb_hid_t *pcb_gui = NULL;
pcb_hid_t *pcb_next_gui = NULL;
pcb_hid_t *pcb_exporter = NULL;

int pcb_pixel_slop = 1;

pcb_plugin_dir_t *pcb_plugin_dir_first = NULL, *pcb_plugin_dir_last = NULL;

void pcb_hid_init()
{
	char *tmp;

	/* Setup a "nogui" default HID */
	pcb_gui = pcb_hid_nogui_get_hid();

TODO("make this configurable - add to conf_board_ignores avoid plugin injection")
	tmp = pcb_concat(pcbhl_conf.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib", PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL);
	pcb_plugin_add_dir(tmp);
	free(tmp);

	tmp = pcb_concat(pcbhl_conf.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib", PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", NULL);
	pcb_plugin_add_dir(tmp);
	free(tmp);

	/* hardwired libdir, just in case exec-prefix goes wrong (e.g. linstall) */
	tmp = pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL);
	pcb_plugin_add_dir(tmp);
	free(tmp);
	tmp = pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S, "plugins", NULL);
	pcb_plugin_add_dir(tmp);
	free(tmp);

	/* pcbhl_conf.rc.path.home is set by the conf_core immediately on startup */
	if (pcbhl_conf.rc.path.home != NULL) {
		tmp = pcb_concat(pcbhl_conf.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL);
		pcb_plugin_add_dir(tmp);
		free(tmp);

		tmp = pcb_concat(pcbhl_conf.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", NULL);
		pcb_plugin_add_dir(tmp);
		free(tmp);
	}

	tmp = pcb_concat("plugins", PCB_DIR_SEPARATOR_S, HOST, NULL);
	pcb_plugin_add_dir(tmp);
	free(tmp);

	pcb_plugin_add_dir("plugins");
}

void pcb_hid_uninit(void)
{
	pcb_plugin_dir_t *pd, *next;

	if (pcb_hid_num_hids > 0) {
		int i;
		for (i = pcb_hid_num_hids-1; i >= 0; i--) {
			if (pcb_hid_list[i]->uninit != NULL)
				pcb_hid_list[i]->uninit(pcb_hid_list[i]);
		}
	}
	free(pcb_hid_list);

	pup_uninit(&pcb_pup);

	pcb_hid_attributes_uninit();

	for(pd = pcb_plugin_dir_first; pd != NULL; pd = next) {
		next = pd->next;
		free(pd->path);
		free(pd);
	}
	pcb_plugin_dir_first = pcb_plugin_dir_last = NULL;
}

void pcb_hid_register_hid(pcb_hid_t * hid)
{
	int i;
	int sz = (pcb_hid_num_hids + 2) * sizeof(pcb_hid_t *);

	if (hid->struct_size != sizeof(pcb_hid_t)) {
		fprintf(stderr, "Warning: hid \"%s\" has an incompatible ABI.\n", hid->name);
		return;
	}

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (hid == pcb_hid_list[i])
			return;

	pcb_hid_num_hids++;
	if (pcb_hid_list)
		pcb_hid_list = (pcb_hid_t **) realloc(pcb_hid_list, sz);
	else
		pcb_hid_list = (pcb_hid_t **) malloc(sz);

	pcb_hid_list[pcb_hid_num_hids - 1] = hid;
	pcb_hid_list[pcb_hid_num_hids] = 0;
}

void pcb_hid_remove_hid(pcb_hid_t * hid)
{
	int i;

	for (i = 0; i < pcb_hid_num_hids; i++) {
		if (hid == pcb_hid_list[i]) {
			pcb_hid_list[i] = pcb_hid_list[pcb_hid_num_hids - 1];
			pcb_hid_list[pcb_hid_num_hids - 1] = 0;
			pcb_hid_num_hids--;
			return;
		}
	}
}


pcb_hid_t *pcb_hid_find_gui(const char *preference)
{
	int i;

	/* ugly hack for historical reasons: some old configs and veteran users are used to the --gui gtk option */
	if ((preference != NULL) && (strcmp(preference, "gtk") == 0)) {
		pcb_hid_t *g;

		g = pcb_hid_find_gui("gtk2_gl");
		if (g != NULL)
			return g;

		g = pcb_hid_find_gui("gtk2_gdk");
		if (g != NULL)
			return g;

		return NULL;
	}

	/* normal search */
	if (preference != NULL) {
		for (i = 0; i < pcb_hid_num_hids; i++)
			if (!pcb_hid_list[i]->printer && !pcb_hid_list[i]->exporter && !strcmp(pcb_hid_list[i]->name, preference))
				return pcb_hid_list[i];
		return NULL;
	}

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (!pcb_hid_list[i]->printer && !pcb_hid_list[i]->exporter)
			return pcb_hid_list[i];

	fprintf(stderr, "Error: No GUI available.\n");
	exit(1);
}

pcb_hid_t *pcb_hid_find_printer()
{
	int i;

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (pcb_hid_list[i]->printer)
			return pcb_hid_list[i];

	return 0;
}

void pcb_hid_print_exporter_list(FILE *f, const char *prefix, const char *suffix)
{
	int i;
	for (i = 0; i < pcb_hid_num_hids; i++)
		if (pcb_hid_list[i]->exporter)
			fprintf(f, "%s%s%s", prefix, pcb_hid_list[i]->name, suffix);
}

pcb_hid_t *pcb_hid_find_exporter(const char *which)
{
	int i;

	if (which == NULL) {
		fprintf(stderr, "Invalid exporter: need an exporter name, one of:");
		goto list;
	}

	if (strcmp(which, "-list-") == 0) {
		pcb_hid_print_exporter_list(stdout, "", "\n");
		return 0;
	}

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (pcb_hid_list[i]->exporter && strcmp(which, pcb_hid_list[i]->name) == 0)
			return pcb_hid_list[i];

	fprintf(stderr, "Invalid exporter %s, available ones:", which);

	list:;
	pcb_hid_print_exporter_list(stderr, " ", "");
	fprintf(stderr, "\n");

	return 0;
}

pcb_hid_t **pcb_hid_enumerate()
{
	return pcb_hid_list;
}

const char *pcb_hid_export_fn(const char *filename)
{
	if (pcbhl_conf.rc.export_basename) {
		const char *outfn = strrchr(filename, PCB_DIR_SEPARATOR_C);
		if (outfn == NULL)
			return filename;
		return outfn + 1;
	}
	else
		return filename;
}

extern void pcb_hid_dlg_uninit(void);
extern void pcb_hid_dlg_init(void);

void pcb_hidlib_init1(void (*conf_core_init)(void))
{
	pcb_events_init();
	pcb_file_loaded_init();
	conf_init();
	conf_core_init();
	pcb_hidlib_conf_init();
	pcb_hidlib_event_init();
	pcb_hid_dlg_init();
	pcb_hid_init();
	pcb_color_init();
}

void pcb_hidlib_init2(const pup_buildin_t *buildins)
{
	pcb_actions_init();

	conf_load_all(NULL, NULL);

	pup_init(&pcb_pup);
	pcb_pup.error_stack_enable = 1;
	pup_buildin_load(&pcb_pup, buildins);
	pup_autoload_dir(&pcb_pup, NULL, NULL);

	conf_load_extra(NULL, NULL);
	pcb_units_init();
}


void pcb_hidlib_uninit(void)
{
	pcb_hidlib_event_uninit();
	pcb_hid_dlg_uninit();

	if (conf_isdirty(CFR_USER))
		conf_save_file(NULL, NULL, NULL, CFR_USER, NULL);

	pcb_hid_uninit();
	pcb_events_uninit();
	conf_uninit();
	pcb_plugin_uninit();
	pcb_actions_uninit();
	pcb_dad_unit_uninit();
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

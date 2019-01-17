/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
 *  pcb-rnd Copyright (C) 2017,2018 Alain Vigne
 *  pcb-rnd Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
#include <signal.h>
#include <genht/hash.h>
#include <genvector/gds_char.h>
#include "conf_core.h"
#include "board.h"
#include "build_run.h"
#include "hid_init.h"
#include "plug_io.h"
#include "compat_misc.h"

extern void pcb_main_uninit(void);

void pcb_quit_app(void)
{
	/* save data if necessary.  It not needed, then don't trigger EmergencySave
	 * via our atexit() registering of pcb_emergency_save().  We presumably wanted to
	 * exit here and thus it is not an emergency. */
	if (PCB->Changed && conf_core.editor.save_in_tmp)
		pcb_emergency_save();
	else
		pcb_disable_emergency_save();

	if (pcb_gui->do_exit == NULL) {
		pcb_main_uninit();
		exit(0);
	}
	else
		pcb_gui->do_exit(pcb_gui);
}

char *pcb_get_info_program(void)
{
	static gds_t info;
	static int first_time = 1;

	if (first_time) {
		first_time = 0;
		gds_init(&info);
		gds_append_str(&info, "This is pcb-rnd " PCB_VERSION " (" PCB_REVISION ")" "\n an interactive ");
		gds_append_str(&info, "printed circuit board editor\n");
	}
	return info.array;
}

char *pcb_get_info_copyright(void)
{
	static gds_t info;
	static int first_time = 1;

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		gds_append_str(&info, "Recent pcb-rnd copyright:\n");
		gds_append_str(&info, "Copyright (C) Tibor Palinkas 2013..2019 (pcb-rnd)\n");
		gds_append_str(&info, "For a more complete list of pcb-rnd authors and\ncontributors, see http://repo.hu/cgi-bin/pcb-rnd-people.cgi\n\n");

		gds_append_str(&info, "Historical copyright:\n");
		gds_append_str(&info, "pcb-rnd was originally forked from gEDA/PCB:\n");
		gds_append_str(&info, "Copyright (C) Thomas Nau 1994..1997\n");
		gds_append_str(&info, "Copyright (C) harry eaton 1998..2007\n");
		gds_append_str(&info, "Copyright (C) C. Scott Ananian 2001\n");
		gds_append_str(&info, "Copyright (C) DJ Delorie 2003..2008\n");
		gds_append_str(&info, "Copyright (C) Dan McMahill 2003..2008\n\n");
	}
	return info.array;
}

char *pcb_get_info_websites(const char **url_out)
{
	static gds_t info;
	static int first_time = 1;
	static const char *URL = "http://pcb-rnd.repo.hu\n";

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		gds_append_str(&info, "For more information see:\n");
		gds_append_str(&info, URL);
	}

	if (url_out != NULL)
		*url_out = URL;

	return info.array;
}

char *pcb_get_info_comments(void)
{
	static gds_t info;
	static int first_time = 1;
	char *tmp;

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		tmp = pcb_get_info_program();
		gds_append_str(&info, tmp);
		tmp = pcb_get_info_websites(NULL);
		gds_append_str(&info, tmp);
	}
	return info.array;
}


char *pcb_get_info_compile_options(void)
{
	pcb_hid_t **hids;
	int i;
	static gds_t info;
	static int first_time = 1;

#define TAB "    "

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		gds_append_str(&info, "----- Run Time Options -----\n");
		gds_append_str(&info, "GUI: ");
		if (pcb_gui != NULL) {
			gds_append_str(&info, pcb_gui->name);
			gds_append_str(&info, "\n");
		}
		else
			gds_append_str(&info, "none\n");

		gds_append_str(&info, "\n----- Compile Time Options -----\n");
		hids = pcb_hid_enumerate();
		gds_append_str(&info, "GUI:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->gui) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}

		gds_append_str(&info, "Exporters:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->exporter) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}

		gds_append_str(&info, "Printers:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->printer) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}
	}
#undef TAB
	return info.array;
}

char *pcb_get_info_license(void)
{
	static gds_t info;
	static int first_time = 1;

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		gds_append_str(&info, "pcb-rnd is licensed under the terms of the GNU\n");
		gds_append_str(&info, "General Public License version 2\n");
		gds_append_str(&info, "See the COPYING file for more information\n\n");
	}
	return info.array;
}

const char *pcb_author(void)
{
	if (conf_core.design.fab_author && conf_core.design.fab_author[0])
		return conf_core.design.fab_author;
	else
		return pcb_get_user_name();
}

/* Catches signals which abort the program. */
void pcb_catch_signal(int Signal)
{
	const char *s;

	switch (Signal) {
#ifdef SIGHUP
	case SIGHUP:
		s = "SIGHUP";
		break;
#endif
	case SIGINT:
		s = "SIGINT";
		break;
#ifdef SIGQUIT
	case SIGQUIT:
		s = "SIGQUIT";
		break;
#endif
	case SIGABRT:
		s = "SIGABRT";
		break;
	case SIGTERM:
		s = "SIGTERM";
		break;
	case SIGSEGV:
		s = "SIGSEGV";
		break;
	default:
		s = "unknown";
		break;
	}
	pcb_message(PCB_MSG_ERROR, "aborted by %s signal\n", s);
	exit(1);
}


/*** files loaded ***/

htsp_t pcb_file_loaded;


pcb_file_loaded_t *pcb_file_loaded_category(const char *name, int alloc)
{
	pcb_file_loaded_t *cat = htsp_get(&pcb_file_loaded, name);

	if ((cat == NULL) && (alloc)) {
		cat = calloc(sizeof(pcb_file_loaded_t), 1);
		cat->type = PCB_FLT_CATEGORY;
		cat->name = pcb_strdup(name);
		htsp_init(&cat->data.category.children, strhash, strkeyeq);
		htsp_set(&pcb_file_loaded, cat->name, cat);
	}
	return cat;
}

int pcb_file_loaded_file_free(pcb_file_loaded_t *file)
{
	free(file->data.file.path);
	free(file->data.file.desc);
	free(file->name);
	free(file);
	return 0;
}

int pcb_file_loaded_clear(pcb_file_loaded_t *cat)
{
	htsp_entry_t *e;

	assert(cat->type == PCB_FLT_CATEGORY);

	for (e = htsp_first(&cat->data.category.children); e; e = htsp_next(&cat->data.category.children, e)) {
		pcb_file_loaded_file_free(e->value);
		htsp_delentry(&cat->data.category.children, e);
	}
	return 0;
}

int pcb_file_loaded_clear_at(const char *catname)
{
	pcb_file_loaded_t *cat = pcb_file_loaded_category(catname, 0);
	if (cat != NULL)
		return pcb_file_loaded_clear(cat);
	return 0;
}

int pcb_file_loaded_set(pcb_file_loaded_t *cat, const char *name, const char *path, const char *desc)
{
	pcb_file_loaded_t *file;

	assert(cat->type == PCB_FLT_CATEGORY);
	file = htsp_get(&cat->data.category.children, name);
	if (file != NULL) {
		free(file->data.file.path);
		free(file->data.file.desc);
	}
	else {
		file = malloc(sizeof(pcb_file_loaded_t));
		file->type = PCB_FLT_FILE;
		file->name = pcb_strdup(name);
		htsp_set(&cat->data.category.children, file->name, file);
	}
	if (path != NULL)
		file->data.file.path = pcb_strdup(path);
	else
		file->data.file.path = NULL;

	if (desc != NULL)
		file->data.file.desc = pcb_strdup(desc);
	else
		file->data.file.desc = NULL;
	return 0;
}

int pcb_file_loaded_set_at(const char *catnam, const char *name, const char *path, const char *desc)
{
	pcb_file_loaded_t *cat = pcb_file_loaded_category(catnam, 1);
	return pcb_file_loaded_set(cat, name, path, desc);
}

int pcb_file_loaded_del(pcb_file_loaded_t *cat, const char *name)
{
	pcb_file_loaded_t *file;
	assert(cat->type == PCB_FLT_CATEGORY);
	file = htsp_pop(&cat->data.category.children, name);
	if (file != NULL) {
		if (file->type != PCB_FLT_FILE)
			return -1;
		pcb_file_loaded_file_free(file);
	}
	return 0;
}

int pcb_file_loaded_del_at(const char *catname, const char *name)
{
	pcb_file_loaded_t *cat = pcb_file_loaded_category(catname, 1);
	return pcb_file_loaded_del(cat, name);
}

void pcb_file_loaded_init(void)
{
	htsp_init(&pcb_file_loaded, strhash, strkeyeq);
}

void pcb_file_loaded_uninit(void)
{
	htsp_entry_t *e;

	for (e = htsp_first(&pcb_file_loaded); e; e = htsp_next(&pcb_file_loaded, e)) {
		pcb_file_loaded_t *cat = e->value;
		pcb_file_loaded_clear(cat);
		free(cat->name);
		htsp_uninit(&cat->data.category.children);
		free(cat);
		htsp_delentry(&pcb_file_loaded, e);
	}

	htsp_uninit(&pcb_file_loaded);
}



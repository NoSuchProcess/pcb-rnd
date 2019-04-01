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

/* for opendir - must be the first */
#include "compat_inc.h"

#include <ctype.h>
#include <genvector/vtp0.h>

#include "actions.h"
#include "plugins.h"
#include "hid_dad.h"
#include "safe_fs.h"


#include "live_script.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	char *name;
	char **langs;
	char **lang_engines;
} live_script_t;

static void lvs_free_langs(live_script_t *lvs)
{
	char **s;
	for(s = lvs->langs; *s != NULL; s++) free(*s);
	for(s = lvs->lang_engines; *s != NULL; s++) free(*s);
	free(lvs->langs);
	free(lvs->lang_engines);
}


static void lvs_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	live_script_t *lvs = caller_data;
	PCB_DAD_FREE(lvs->dlg);
	lvs_free_langs(lvs);
	free(lvs);
}

extern const char *pcb_script_pup_paths[];
static int lvs_list_langs(live_script_t *lvs)
{
	const char **path;
	vtp0_t vl, ve;

	vtp0_init(&vl);
	vtp0_init(&ve);

	for(path = pcb_script_pup_paths; *path != NULL; path++) {
		char fn[PCB_PATH_MAX*2], *fn_end;
		int dirlen;
		struct dirent *de;
		DIR *d = opendir(*path);

		if (d == NULL)
			continue;

		dirlen = strlen(*path);
		memcpy(fn, *path, dirlen);
		fn_end = fn + dirlen;
		*fn_end = PCB_DIR_SEPARATOR_C;
		fn_end++;

		while((de = readdir(d)) != NULL) {
			FILE *f;
			int el, len = strlen(de->d_name);
			char *s1, *s2, *eng, *s, *end, line[1024];

			if (len < 5)
				continue;
			end = de->d_name + len -4;
			if ((strcmp(end, ".pup") != 0) || (strncmp(de->d_name, "fungw_", 6) != 0))
				continue;

			strcpy(fn_end, de->d_name);

			f = pcb_fopen(fn, "r");
			if (f == NULL)
				continue;
			while((s = fgets(line, sizeof(line), f)) != NULL) {
				while(isspace(*s)) s++;
				if (strncmp(s, "$desc", 5) != 0)
					continue;
				s += 5;
				if (((s1 = strstr(s, "binding")) == NULL) || ((s2 = strstr(s, "engine")) == NULL))
					continue;
				if (s1 < s2) *s1 = '\0';
				else *s2 = '\0';
				eng = pcb_strdup(de->d_name);
				el = strlen(eng);
				eng[el-4] = '\0';
				vtp0_append(&ve, eng);
				vtp0_append(&vl, pcb_strdup(s));
			}
			fclose(f);
		}
		closedir(d);
	}
	lvs->langs = (char **)vl.array;
	lvs->lang_engines = (char **)ve.array;
	return vl.used;
}

static live_script_t *pcb_dlg_live_script(const char *name)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	char *title;
	live_script_t *lvs = calloc(sizeof(live_script_t), 1);

	lvs_list_langs(lvs);

	name = pcb_strdup(name);
	PCB_DAD_BEGIN_VBOX(lvs->dlg);
		PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_TEXT(lvs->dlg, lvs);

		PCB_DAD_BEGIN_HBOX(lvs->dlg);
			PCB_DAD_BUTTON(lvs->dlg, "re-run");
			PCB_DAD_BUTTON(lvs->dlg, "run");
			PCB_DAD_BUTTON(lvs->dlg, "undo");
			PCB_DAD_BUTTON(lvs->dlg, "save");
			PCB_DAD_BUTTON(lvs->dlg, "load");
			PCB_DAD_BEGIN_HBOX(lvs->dlg);
				PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(lvs->dlg);
			PCB_DAD_ENUM(lvs->dlg, (const char **)lvs->langs);
			PCB_DAD_BEGIN_HBOX(lvs->dlg);
				PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(lvs->dlg);
			PCB_DAD_BUTTON_CLOSES(lvs->dlg, clbtn);
		PCB_DAD_END(lvs->dlg);
	PCB_DAD_END(lvs->dlg);

	title = pcb_concat("Live Scripting: ", name, NULL);
	PCB_DAD_NEW("live_script", lvs->dlg, title, lvs, pcb_false, lvs_close_cb);
	free(title);
	return lvs;
}

const char pcb_acts_LiveScript[] = "LiveScript([name], [new])\n";
const char pcb_acth_LiveScript[] = "Manage a live script";
fgw_error_t pcb_act_LiveScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_live_script("name");
	return 0;
}

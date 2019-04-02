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
#include <genht/htsp.h>
#include <genht/hash.h>

#include "actions.h"
#include "plugins.h"
#include "hid_dad.h"
#include "safe_fs.h"
#include "compat_fs.h"
#include "undo.h"
#include "script.h"

#include "live_script.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	char *name, *longname, *fn;
	char **langs;
	char **lang_engines;
	int wtxt, wrerun, wrun, wstop, wundo, wload, wsave, wpers, wlang;
	uundo_serial_t undo_pre, undo_post; /* undo serials pre-run and post-run */
	unsigned loaded:1;
} live_script_t;

static htsp_t pcb_live_scripts;

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

	htsp_popentry(&pcb_live_scripts, lvs->name);

	if (lvs->loaded)
		pcb_script_unload(lvs->longname, NULL);

	if (pcb_gui != NULL)
		PCB_DAD_FREE(lvs->dlg);
	lvs_free_langs(lvs);
	free(lvs->name);
	free(lvs->longname);
	free(lvs->fn);
	free(lvs);
}

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
				eng = pcb_strdup(de->d_name + 6); /* remove the fungw_ prefix, the low level script runner will insert it */
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

static void lvs_button_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_btn)
{
	live_script_t *lvs = caller_data;
	const char *arg;
	int w = attr_btn - lvs->dlg;


	if (w == lvs->wrerun)     arg = "rerun";
	else if (w == lvs->wrun)  arg = "run";
	else if (w == lvs->wstop) arg = "stop";
	else if (w == lvs->wundo) arg = "undo";
	else if (w == lvs->wload) arg = "load";
	else if (w == lvs->wsave) arg = "save";
	else {
		pcb_message(PCB_MSG_ERROR, "lvs_button_cb(): internal error: unhandled switch case\n");
		return;
	}

	pcb_actionl("livescript", arg, lvs->name, NULL);
}

static live_script_t *pcb_dlg_live_script(const char *name)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	char *title;
	live_script_t *lvs = calloc(sizeof(live_script_t), 1);

	if (lvs_list_langs(lvs) < 1) {
		lvs_free_langs(lvs);
		free(lvs);
		pcb_message(PCB_MSG_ERROR, "live_script: no scripting language engines found\nPlease compile and install fungw from source, then\nreconfigure and recompile pcb-rnd.\n");
		return NULL;
	}

	lvs->name = pcb_strdup(name);
	lvs->longname = pcb_concat("_live_script_", name, NULL);
	PCB_DAD_BEGIN_VBOX(lvs->dlg);
		PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_TEXT(lvs->dlg, lvs);
			PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			lvs->wtxt = PCB_DAD_CURRENT(lvs->dlg);

		PCB_DAD_BEGIN_HBOX(lvs->dlg);
			PCB_DAD_BUTTON(lvs->dlg, "re-run");
				lvs->wrerun = PCB_DAD_CURRENT(lvs->dlg);
				PCB_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				PCB_DAD_HELP(lvs->dlg, "Stop the script if it is running\nundo the changes the script performed if no\nuser modification happened since\nrun the script again");
			PCB_DAD_BUTTON(lvs->dlg, "run");
				lvs->wrun = PCB_DAD_CURRENT(lvs->dlg);
				PCB_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				PCB_DAD_HELP(lvs->dlg, "Run the script:\nonce and unload, if not persistent\nor keep it running in persistent mode");
			PCB_DAD_BUTTON(lvs->dlg, "stop");
				lvs->wstop = PCB_DAD_CURRENT(lvs->dlg);
				PCB_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				PCB_DAD_HELP(lvs->dlg, "Halt and unload the script\nand unregister any action, menu, etc. it registered");
			PCB_DAD_BUTTON(lvs->dlg, "undo");
				lvs->wundo = PCB_DAD_CURRENT(lvs->dlg);
				PCB_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				PCB_DAD_HELP(lvs->dlg, "undo the changes the script performed if no\nuser modification happened since");
			PCB_DAD_BUTTON(lvs->dlg, "save");
				lvs->wsave = PCB_DAD_CURRENT(lvs->dlg);
				PCB_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				PCB_DAD_HELP(lvs->dlg, "Save script source to a file");
			PCB_DAD_BUTTON(lvs->dlg, "load");
				lvs->wload = PCB_DAD_CURRENT(lvs->dlg);
				PCB_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				PCB_DAD_HELP(lvs->dlg, "Load script source from a file");
		PCB_DAD_END(lvs->dlg);
		PCB_DAD_BEGIN_HBOX(lvs->dlg);
			PCB_DAD_BOOL(lvs->dlg, "");
				lvs->wpers = PCB_DAD_CURRENT(lvs->dlg);
			PCB_DAD_LABEL(lvs->dlg, "persistent");
				PCB_DAD_HELP(lvs->dlg, "Persistent mode: keep the script loaded and running\n(useful if the script registers actions)\nNon-persistent mode: run once then unload.");
			PCB_DAD_ENUM(lvs->dlg, (const char **)lvs->langs);
				lvs->wlang = PCB_DAD_CURRENT(lvs->dlg);
			PCB_DAD_BEGIN_HBOX(lvs->dlg);
				PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(lvs->dlg);
			PCB_DAD_BUTTON_CLOSES(lvs->dlg, clbtn);
		PCB_DAD_END(lvs->dlg);
	PCB_DAD_END(lvs->dlg);
	PCB_DAD_DEFSIZE(lvs->dlg, 300, 500);

	title = pcb_concat("Live Scripting: ", name, NULL);
	PCB_DAD_NEW("live_script", lvs->dlg, title, lvs, pcb_false, lvs_close_cb);
	free(title);
	pcb_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wstop, 0);
	return lvs;
}

static int live_stop(live_script_t *lvs)
{
	if (lvs->loaded) {
		pcb_script_unload(lvs->longname, NULL);
		lvs->loaded = 0;
	}

	pcb_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wrun, 1);
	pcb_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wstop, 0);
	return 0;
}

static int live_run(live_script_t *lvs)
{
	pcb_hid_attribute_t *atxt = &lvs->dlg[lvs->wtxt];
	pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;
	FILE *f;
	char *src, *fn, *lang;
	int res = 0;
	long numu;

	fn = pcb_tempfile_name_new("live_script");
	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_tempfile_unlink(fn);
		pcb_message(PCB_MSG_ERROR, "live_script: can't open temp file for write\n");
		return -1;
	}

	src = txt->hid_get_text(atxt, lvs->dlg_hid_ctx);
	fputs(src, f);
	free(src);
	fclose(f);

	lang = lvs->lang_engines[lvs->dlg[lvs->wlang].default_val.int_value];

	live_stop(lvs);

	lvs->undo_pre = pcb_undo_serial();
	numu = pcb_num_undo();

	if (pcb_script_load(lvs->longname, fn, lang) != 0) {
		pcb_message(PCB_MSG_ERROR, "live_script: can't load/parse the script\n");
		res = -1;
	}
	lvs->loaded = 1;
	pcb_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wrun, 0);
	pcb_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wstop, 1);

	if (!lvs->dlg[lvs->wpers].default_val.int_value)
		live_stop(lvs);

	if ((pcb_num_undo() != numu) && (lvs->undo_pre == pcb_undo_serial()))
		pcb_undo_inc_serial();
	lvs->undo_post = pcb_undo_serial();

	pcb_gui->invalidate_all(); /* if the script drew anything, get it displayed */

	pcb_tempfile_unlink(fn);
	return res;
}

static const char *live_default_ext(live_script_t *lvs)
{
	TODO("get this info from fungw for the selected language");
	return NULL;
}

static int live_undo(live_script_t *lvs)
{
	if (lvs->undo_pre == lvs->undo_post)
		return 0; /* the script did nothing */
	if (lvs->undo_post < pcb_undo_serial()) {
		pcb_message(PCB_MSG_WARNING, "Can not undo live script modifications:\nthere was user edit after script executaion.\n");
		return 1;
	}
	pcb_undo_above(lvs->undo_pre);
	pcb_gui->invalidate_all();
	return 0;
}


static int live_load(live_script_t *lvs, const char *fn)
{
	pcb_hid_attribute_t *atxt = &lvs->dlg[lvs->wtxt];
	pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;
	FILE *f;
	gds_t tmp;

	if (fn == NULL) {
		const char *default_ext = live_default_ext(lvs);
		fn = pcb_gui->fileselect(
			"Load live script", "Load the a live script from file",
			lvs->fn, default_ext, pcb_hid_fsd_filter_any, "live_script", PCB_HID_FSD_READ, NULL);
		if (fn == NULL)
			return 0;
	}

	f = pcb_fopen(fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "live_script: failed to open '%s' for read\n", fn);
		return -1;
	}

	gds_init(&tmp);

	while(!feof(f)) {
		int len, ou = tmp.used;
		gds_alloc_append(&tmp, 1024);
		len = fread(tmp.array+ou, 1, 1024, f);
		if (len > 0) {
			tmp.used = ou+len;
			tmp.array[tmp.used] = '\0';
		}
	}

	txt->hid_set_text(atxt, lvs->dlg_hid_ctx, PCB_HID_TEXT_REPLACE, tmp.array);

	gds_uninit(&tmp);
	fclose(f);
	return 0;
}

static int live_save(live_script_t *lvs, const char *fn)
{
	pcb_hid_attribute_t *atxt = &lvs->dlg[lvs->wtxt];
	pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;
	FILE *f;
	char *src;
	int res = 0;

	if (fn == NULL) {
		const char *default_ext = live_default_ext(lvs);

		if (lvs->fn == NULL)
			lvs->fn = pcb_concat(lvs->name, ".", default_ext, NULL);

		fn = pcb_gui->fileselect(
			"Save live script", "Save the source of a live script",
			lvs->fn, default_ext, pcb_hid_fsd_filter_any, "live_script", 0, NULL);
		if (fn == NULL)
			return 0;
	}

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "live_script: failed to open '%s' for write\n", fn);
		return -1;
	}

	src = txt->hid_get_text(atxt, lvs->dlg_hid_ctx);
	if (fwrite(src, strlen(src), 1, f) != 1) {
		pcb_message(PCB_MSG_ERROR, "live_script: failed to write script source to '%s'\n", fn);
		res = -1;
	}
	free(src);

	fclose(f);
	return res;
}


const char pcb_acts_LiveScript[] = 
	"LiveScript([new], [name])\n"
	"LiveScript(load|save, [name], [filame])\n"
	"LiveScript(run|rerun|undo, [name])\n";
const char pcb_acth_LiveScript[] = "Manage a live script";
fgw_error_t pcb_act_LiveScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	live_script_t *lvs;
	const char *cmd = "new", *name = NULL, *arg = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, LiveScript, cmd = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, LiveScript, name = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, LiveScript, arg = argv[3].val.str);

	if (pcb_strcasecmp(cmd, "new") == 0) {
		if (name == NULL) name = "default";
		lvs = htsp_get(&pcb_live_scripts, name);
		if (lvs != NULL) {
			pcb_message(PCB_MSG_ERROR, "live script '%s' is already open\n");
			PCB_ACT_IRES(1);
			return 0;
		}
		lvs = pcb_dlg_live_script(name);
		if (lvs != NULL) {
			htsp_set(&pcb_live_scripts, lvs->name, lvs);
			PCB_ACT_IRES(0);
		}
		else
			PCB_ACT_IRES(1);
		return 0;
	}

	if (name == NULL) {
		pcb_message(PCB_MSG_ERROR, "script name (second argument) required\n");
		PCB_ACT_IRES(1);
		return 0;
	}

	lvs = htsp_get(&pcb_live_scripts, name);
	if (lvs == NULL) {
		pcb_message(PCB_MSG_ERROR, "script '%s' does not exist\n", name);
		PCB_ACT_IRES(1);
		return 0;
	}

	PCB_ACT_IRES(0);
	if (pcb_strcasecmp(cmd, "load") == 0) {
		PCB_ACT_IRES(live_load(lvs, arg));
	}
	else if (pcb_strcasecmp(cmd, "save") == 0) {
		PCB_ACT_IRES(live_save(lvs, arg));
	}
	else if (pcb_strcasecmp(cmd, "undo") == 0) {
		PCB_ACT_IRES(live_undo(lvs));
	}
	else if (pcb_strcasecmp(cmd, "run") == 0) {
		live_run(lvs);
	}
	else if (pcb_strcasecmp(cmd, "stop") == 0) {
		live_stop(lvs);
	}
	else if (pcb_strcasecmp(cmd, "rerun") == 0) {
		live_stop(lvs);
		live_undo(lvs);
		live_run(lvs);
	}

	return 0;
}

void pcb_live_script_init(void)
{
	htsp_init(&pcb_live_scripts, strhash, strkeyeq);
}

void pcb_live_script_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&pcb_live_scripts); e != NULL; e = htsp_next(&pcb_live_scripts, e)) {
		live_script_t *lvs = e->value;
		lvs_close_cb(lvs, PCB_HID_ATTR_EV_CODECLOSE);
	}
	htsp_uninit(&pcb_live_scripts);
}


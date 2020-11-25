/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

#include <ctype.h>
#include <genvector/vtp0.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/safe_fs_dir.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/event.h>
#include <librnd/core/hid_menu.h>
#include "undo.h"
#include "globalconst.h"

#include "script.h"

#include "live_script.h"

#include "menu_internal.c"

static const char *lvs_cookie = "live_script";


typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	rnd_hidlib_t *hidlib;
	char *name, *longname, *fn;
	char **langs;
	char **lang_engines;
	int wtxt, wrerun, wrun, wstop, wundo, wload, wsave, wreload, wpers, wlang;
	uundo_serial_t undo_pre, undo_post; /* undo serials pre-run and post-run */
	unsigned loaded:1;
} live_script_t;

static htsp_t rnd_live_scripts;

static void lvs_free_langs(live_script_t *lvs)
{
	char **s;
	if (lvs->langs != NULL)
		for(s = lvs->langs; *s != NULL; s++) free(*s);
	if (lvs->lang_engines != NULL)
		for(s = lvs->lang_engines; *s != NULL; s++) free(*s);
	free(lvs->langs);
	free(lvs->lang_engines);
}


static void lvs_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	live_script_t *lvs = caller_data;

	htsp_popentry(&rnd_live_scripts, lvs->name);

	if (lvs->loaded)
		rnd_script_unload(lvs->longname, NULL);

	if (rnd_gui != NULL)
		RND_DAD_FREE(lvs->dlg);
	lvs_free_langs(lvs);
	free(lvs->name);
	free(lvs->longname);
	free(lvs->fn);
	free(lvs);
}

#ifdef RND_HAVE_SYS_FUNGW
static int lvs_list_langs(rnd_hidlib_t *hl, live_script_t *lvs)
{
	const char **path;
	vtp0_t vl, ve;

	vtp0_init(&vl);
	vtp0_init(&ve);

	for(path = (const char **)rnd_pup_paths; *path != NULL; path++) {
		char fn[RND_PATH_MAX*2], *fn_end;
		int dirlen;
		struct dirent *de;
		DIR *d = rnd_opendir(hl, *path);

		if (d == NULL)
			continue;

		dirlen = strlen(*path);
		memcpy(fn, *path, dirlen);
		fn_end = fn + dirlen;
		*fn_end = RND_DIR_SEPARATOR_C;
		fn_end++;

		while((de = rnd_readdir(d)) != NULL) {
			FILE *f;
			int el, len = strlen(de->d_name);
			char *s1, *s2, *eng, *s, *end, line[1024];

			if (len < 5)
				continue;
			end = de->d_name + len -4;
			if ((strcmp(end, ".pup") != 0) || (strncmp(de->d_name, "fungw_", 6) != 0))
				continue;

			strcpy(fn_end, de->d_name);

			f = rnd_fopen(hl, fn, "r");
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
				eng = rnd_strdup(de->d_name + 6); /* remove the fungw_ prefix, the low level script runner will insert it */
				el = strlen(eng);
				eng[el-4] = '\0';
				vtp0_append(&ve, eng);
				vtp0_append(&vl, rnd_strdup(s));
			}
			fclose(f);
		}
		rnd_closedir(d);
	}
	lvs->langs = (char **)vl.array;
	lvs->lang_engines = (char **)ve.array;
	return vl.used;
}
#else
static int lvs_list_langs(rnd_hidlib_t *hl, live_script_t *lvs)
{
	vtp0_t vl, ve;

	vtp0_init(&vl);
	vtp0_init(&ve);

	vtp0_append(&vl, rnd_strdup("fawk")); vtp0_append(&ve, rnd_strdup("fawk"));
	vtp0_append(&vl, rnd_strdup("fbas")); vtp0_append(&ve, rnd_strdup("fbas"));
	vtp0_append(&vl, rnd_strdup("fpas")); vtp0_append(&ve, rnd_strdup("fpas"));

	lvs->langs = (char **)vl.array;
	lvs->lang_engines = (char **)ve.array;
	return vl.used;
}

#endif

static void lvs_button_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
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
	else if (w == lvs->wreload) arg = "reload-rerun";
	else {
		rnd_message(RND_MSG_ERROR, "lvs_button_cb(): internal error: unhandled switch case\n");
		return;
	}

	rnd_actionva(lvs->hidlib, "livescript", arg, lvs->name, NULL);
}

static live_script_t *rnd_dlg_live_script(rnd_hidlib_t *hidlib, const char *name)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	char *title;
	live_script_t *lvs = calloc(sizeof(live_script_t), 1);

	if (lvs_list_langs(NULL, lvs) < 1) {
		lvs_free_langs(lvs);
		free(lvs);
		rnd_message(RND_MSG_ERROR, "live_script: no scripting language engines found\nPlease compile and install fungw from source, then\nreconfigure and recompile pcb-rnd.\n");
		return NULL;
	}

	lvs->hidlib = hidlib;
	lvs->name = rnd_strdup(name);
	lvs->longname = rnd_concat("_live_script_", name, NULL);
	RND_DAD_BEGIN_VBOX(lvs->dlg);
		RND_DAD_COMPFLAG(lvs->dlg, RND_HATF_EXPFILL);
		RND_DAD_TEXT(lvs->dlg, lvs);
			RND_DAD_COMPFLAG(lvs->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			lvs->wtxt = RND_DAD_CURRENT(lvs->dlg);

		RND_DAD_BEGIN_HBOX(lvs->dlg);
			RND_DAD_BUTTON(lvs->dlg, "re-run");
				lvs->wrerun = RND_DAD_CURRENT(lvs->dlg);
				RND_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				RND_DAD_HELP(lvs->dlg, "Stop the script if it is running\nundo the changes the script performed if no\nuser modification happened since\nrun the script again");
			RND_DAD_BUTTON(lvs->dlg, "run");
				lvs->wrun = RND_DAD_CURRENT(lvs->dlg);
				RND_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				RND_DAD_HELP(lvs->dlg, "Run the script:\nonce and unload, if not persistent\nor keep it running in persistent mode");
			RND_DAD_BUTTON(lvs->dlg, "stop");
				lvs->wstop = RND_DAD_CURRENT(lvs->dlg);
				RND_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				RND_DAD_HELP(lvs->dlg, "Halt and unload the script\nand unregister any action, menu, etc. it registered");
			RND_DAD_BUTTON(lvs->dlg, "undo");
				lvs->wundo = RND_DAD_CURRENT(lvs->dlg);
				RND_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				RND_DAD_HELP(lvs->dlg, "undo the changes the script performed if no\nuser modification happened since");
			RND_DAD_BUTTON(lvs->dlg, "save");
				lvs->wsave = RND_DAD_CURRENT(lvs->dlg);
				RND_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				RND_DAD_HELP(lvs->dlg, "Save script source to a file");
			RND_DAD_BUTTON(lvs->dlg, "load");
				lvs->wload = RND_DAD_CURRENT(lvs->dlg);
				RND_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				RND_DAD_HELP(lvs->dlg, "Load script source from a file");
			RND_DAD_BUTTON(lvs->dlg, "reload&rerun");
				lvs->wreload = RND_DAD_CURRENT(lvs->dlg);
				RND_DAD_CHANGE_CB(lvs->dlg, lvs_button_cb);
				RND_DAD_HELP(lvs->dlg, "Reload script source and rerun\n(Ideal with external editor)");
		RND_DAD_END(lvs->dlg);
		RND_DAD_BEGIN_HBOX(lvs->dlg);
			RND_DAD_BOOL(lvs->dlg);
				lvs->wpers = RND_DAD_CURRENT(lvs->dlg);
			RND_DAD_LABEL(lvs->dlg, "persistent");
				RND_DAD_HELP(lvs->dlg, "Persistent mode: keep the script loaded and running\n(useful if the script registers actions)\nNon-persistent mode: run once then unload.");
			RND_DAD_ENUM(lvs->dlg, (const char **)lvs->langs);
				lvs->wlang = RND_DAD_CURRENT(lvs->dlg);
			RND_DAD_BEGIN_HBOX(lvs->dlg);
				RND_DAD_COMPFLAG(lvs->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(lvs->dlg);
			RND_DAD_BUTTON_CLOSES(lvs->dlg, clbtn);
		RND_DAD_END(lvs->dlg);
	RND_DAD_END(lvs->dlg);
	RND_DAD_DEFSIZE(lvs->dlg, 300, 500);

	title = rnd_concat("Live Scripting: ", name, NULL);
	RND_DAD_NEW("live_script", lvs->dlg, title, lvs, rnd_false, lvs_close_cb);
	free(title);
	rnd_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wstop, 0);
	return lvs;
}

static int live_stop(live_script_t *lvs)
{
	if (lvs->loaded) {
		rnd_script_unload(lvs->longname, NULL);
		lvs->loaded = 0;
	}

	rnd_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wrun, 1);
	rnd_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wstop, 0);
	return 0;
}

static int live_run(rnd_hidlib_t *hl, live_script_t *lvs)
{
	rnd_hid_attribute_t *atxt = &lvs->dlg[lvs->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	FILE *f;
	char *src, *fn, *lang;
	int res = 0;
	long numu;

	fn = rnd_tempfile_name_new("live_script");
	f = rnd_fopen(hl, fn, "w");
	if (f == NULL) {
		rnd_tempfile_unlink(fn);
		rnd_message(RND_MSG_ERROR, "live_script: can't open temp file for write\n");
		return -1;
	}

	src = txt->hid_get_text(atxt, lvs->dlg_hid_ctx);
	fputs(src, f);
	free(src);
	fclose(f);

	lang = lvs->lang_engines[lvs->dlg[lvs->wlang].val.lng];

	live_stop(lvs);

	lvs->undo_pre = pcb_undo_serial();
	numu = pcb_num_undo();

	if (rnd_script_load(lvs->longname, fn, lang) != 0) {
		rnd_message(RND_MSG_ERROR, "live_script: can't load/parse the script\n");
		res = -1;
	}
	lvs->loaded = 1;
	rnd_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wrun, 0);
	rnd_gui->attr_dlg_widget_state(lvs->dlg_hid_ctx, lvs->wstop, 1);

	if (!lvs->dlg[lvs->wpers].val.lng)
		live_stop(lvs);

	if ((pcb_num_undo() != numu) && (lvs->undo_pre == pcb_undo_serial()))
		pcb_undo_inc_serial();
	lvs->undo_post = pcb_undo_serial();

	rnd_gui->invalidate_all(rnd_gui); /* if the script drew anything, get it displayed */

	rnd_tempfile_unlink(fn);
	return res;
}

static const char *live_default_ext(live_script_t *lvs)
{
	char *lang = lvs->lang_engines[lvs->dlg[lvs->wlang].val.lng];
	fgw_eng_t *eng = htsp_get(&fgw_engines, lang);

	if (eng != NULL)
		return eng->def_ext;
	return NULL;
}

static int live_undo(live_script_t *lvs)
{
	if (lvs->undo_pre == lvs->undo_post)
		return 0; /* the script did nothing */
	if (lvs->undo_post < pcb_undo_serial()) {
		rnd_message(RND_MSG_WARNING, "Can not undo live script modifications:\nthere was user edit after script executaion.\n");
		return 1;
	}
	pcb_undo_above(lvs->undo_pre);
	rnd_gui->invalidate_all(rnd_gui);
	return 0;
}


static int live_load(rnd_hidlib_t *hl, live_script_t *lvs, const char *fn)
{
	rnd_hid_attribute_t *atxt = &lvs->dlg[lvs->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	FILE *f;
	gds_t tmp;

	if (fn == NULL) {
		const char *default_ext = live_default_ext(lvs);
		fn = rnd_gui->fileselect(rnd_gui,
			"Load live script", "Load the a live script from file",
			lvs->fn, default_ext, rnd_hid_fsd_filter_any, "live_script", RND_HID_FSD_READ, NULL);
		if (fn == NULL)
			return 0;
		lvs->fn = rnd_strdup(fn);
	}

	f = rnd_fopen(hl, fn, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "live_script: failed to open '%s' for read\n", fn);
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

	txt->hid_set_text(atxt, lvs->dlg_hid_ctx, RND_HID_TEXT_REPLACE, tmp.array);

	gds_uninit(&tmp);
	fclose(f);
	return 0;
}

static int live_save(rnd_hidlib_t *hl, live_script_t *lvs, const char *fn)
{
	rnd_hid_attribute_t *atxt = &lvs->dlg[lvs->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	FILE *f;
	char *src;
	int res = 0;

	if (fn == NULL) {
		const char *default_ext = live_default_ext(lvs);

		if (lvs->fn == NULL)
			lvs->fn = rnd_concat(lvs->name, ".", default_ext, NULL);

		fn = rnd_gui->fileselect(rnd_gui,
			"Save live script", "Save the source of a live script",
			lvs->fn, default_ext, rnd_hid_fsd_filter_any, "live_script", 0, NULL);
		if (fn == NULL)
			return 0;
	}

	f = rnd_fopen(hl, fn, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "live_script: failed to open '%s' for write\n", fn);
		return -1;
	}

	src = txt->hid_get_text(atxt, lvs->dlg_hid_ctx);
	if (fwrite(src, strlen(src), 1, f) != 1) {
		rnd_message(RND_MSG_ERROR, "live_script: failed to write script source to '%s'\n", fn);
		res = -1;
	}
	free(src);

	fclose(f);
	return res;
}


const char pcb_acts_LiveScript[] = 
	"LiveScript([new], [name])\n"
	"LiveScript(load|save, name, [filame])\n"
	"LiveScript(run|stop|rerun|undo, name)\n";
const char pcb_acth_LiveScript[] = "Manage a live script";
/* DOC: livescript.html */
fgw_error_t pcb_act_LiveScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	live_script_t *lvs;
	const char *cmd = "new", *name = NULL, *arg = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LiveScript, cmd = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, LiveScript, name = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, LiveScript, arg = argv[3].val.str);

	if (rnd_strcasecmp(cmd, "new") == 0) {
		if (name == NULL) name = "default";
		lvs = htsp_get(&rnd_live_scripts, name);
		if (lvs != NULL) {
			rnd_message(RND_MSG_ERROR, "live script '%s' is already open\n", name);
			RND_ACT_IRES(1);
			return 0;
		}
		lvs = rnd_dlg_live_script(RND_ACT_HIDLIB, name);
		if (lvs != NULL) {
			htsp_set(&rnd_live_scripts, lvs->name, lvs);
			RND_ACT_IRES(0);
		}
		else
			RND_ACT_IRES(1);
		return 0;
	}

	if (name == NULL) {
		rnd_message(RND_MSG_ERROR, "script name (second argument) required\n");
		RND_ACT_IRES(1);
		return 0;
	}

	lvs = htsp_get(&rnd_live_scripts, name);
	if (lvs == NULL) {
		rnd_message(RND_MSG_ERROR, "script '%s' does not exist\n", name);
		RND_ACT_IRES(1);
		return 0;
	}

	RND_ACT_IRES(0);
	if (rnd_strcasecmp(cmd, "load") == 0) {
		RND_ACT_IRES(live_load(NULL, lvs, arg));
	}
	else if (rnd_strcasecmp(cmd, "save") == 0) {
		RND_ACT_IRES(live_save(NULL, lvs, arg));
	}
	else if (rnd_strcasecmp(cmd, "undo") == 0) {
		RND_ACT_IRES(live_undo(lvs));
	}
	else if (rnd_strcasecmp(cmd, "run") == 0) {
		live_run(NULL, lvs);
	}
	else if (rnd_strcasecmp(cmd, "stop") == 0) {
		live_stop(lvs);
	}
	else if (rnd_strcasecmp(cmd, "rerun") == 0) {
		live_stop(lvs);
		live_undo(lvs);
		live_run(NULL, lvs);
	}
	if (rnd_strcasecmp(cmd, "reload-rerun") == 0) {
		RND_ACT_IRES(live_load(NULL, lvs, lvs->fn));
		live_stop(lvs);
		live_undo(lvs);
		live_run(NULL, lvs);
	}

	return 0;
}

void rnd_live_script_init(void)
{
	htsp_init(&rnd_live_scripts, strhash, strkeyeq);
	rnd_hid_menu_load(rnd_gui, NULL, lvs_cookie, 110, NULL, 0, script_menu, "plugin: live scripting");
}

void rnd_live_script_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&rnd_live_scripts); e != NULL; e = htsp_next(&rnd_live_scripts, e)) {
		live_script_t *lvs = e->value;
		lvs_close_cb(lvs, RND_HID_ATTR_EV_CODECLOSE);
	}
	htsp_uninit(&rnd_live_scripts);
	rnd_event_unbind_allcookie(lvs_cookie);
	rnd_hid_menu_unload(rnd_gui, lvs_cookie);
}


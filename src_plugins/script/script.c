/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include <stdio.h>
#include <stdlib.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include <puplug/puplug.h>
#include <genregex/regex_se.h>

#include "actions.h"
#include "plugins.h"
#include "error.h"
#include "compat_misc.h"
#include "compat_fs.h"
#include "safe_fs.h"
#include "pcb-printf.h"
#include "globalconst.h"

typedef struct {
	char *id, *fn, *lang;
	pup_plugin_t *pup;
	fgw_obj_t *obj;
} script_t;

static htsp_t scripts; /* ID->script_t */

static pup_context_t script_pup;

#include "c_script.c"
#include "conf_core.h"
#include "compat_fs.h"

/* dir name under dotdir for saving script persistency data */
#define SCRIPT_PERS "script_pers"

/* allow only 1 megabyte of script persistency to be loaded */
#define PERS_MAX_SIZE 1024*1024

const char *pcb_script_pup_paths[] = {
	"/usr/local/lib/puplug",
	"/usr/lib/puplug",
	NULL
};

static int script_save_preunload(script_t *s, const char *data)
{
	FILE *f;
	gds_t fn;

	gds_init(&fn);
	gds_append_str(&fn, conf_core.rc.path.home);
	gds_append(&fn, PCB_DIR_SEPARATOR_C);
	gds_append_str(&fn, DOT_PCB_RND);
	pcb_mkdir(fn.array, 0755);

	gds_append(&fn, PCB_DIR_SEPARATOR_C);
	gds_append_str(&fn, SCRIPT_PERS);
	pcb_mkdir(fn.array, 0750);

	gds_append(&fn, PCB_DIR_SEPARATOR_C);
	gds_append_str(&fn, s->obj->name);

	f = pcb_fopen(fn.array, "w");
	if (f != NULL) {
		gds_uninit(&fn);
		fputs(data, f);
		fclose(f);
		return 0;
	}

	gds_uninit(&fn);
	return -1;
}

/* auto-unregister from any central infra the script may have regitered in using
   the standard script cookie */
static void script_unreg(const char *cookie)
{
	if ((pcb_gui != NULL) && (pcb_gui->remove_menu != NULL))
		pcb_gui->remove_menu(cookie);
}

/* unload a script, free all memory and remove it from all lists/hashes.
   If preunload is not NULL, it's the unload reason the script's own
   preunload() function is called and its return is saved. */
static void script_free(script_t *s, const char *preunload, const char *cookie)
{
	if ((preunload != NULL) && (s->obj != NULL)) {
		fgw_func_t *f;
		fgw_arg_t res, argv[2];

		f = fgw_func_lookup(s->obj, "preunload");
		if (f != NULL) {
			argv[0].type = FGW_FUNC;
			argv[0].val.func = f;
			argv[1].type = FGW_STR;
			argv[1].val.cstr = preunload;
			res.type = FGW_INVALID;
			if (f->func(&res, 2, argv) == 0) {
				if ((fgw_arg_conv(&pcb_fgw, &res, FGW_STR) == 0) && (res.val.str != NULL) && (*res.val.str != '\0'))
					script_save_preunload(s, res.val.str);
			}
			fgw_arg_free(&pcb_fgw, &res);
		}
	}

	if (cookie != NULL)
		script_unreg(cookie);

	if (s->obj != NULL)
		fgw_obj_unreg(&pcb_fgw, s->obj);
	if (s->pup != NULL)
		pup_unload(&script_pup, s->pup, NULL);
	free(s->id);
	free(s->fn);
	free(s);
}

static void script_unload_entry(htsp_entry_t *e, const char *preunload, const char *cookie)
{
	script_t *s = (script_t *)e->value;
	script_free(s, preunload, cookie);
	e->key = NULL;
	htsp_delentry(&scripts, e);
}

static const char *script_persistency_id = NULL;
static int script_persistency(fgw_arg_t *res, const char *cmd)
{
	char *fn;

	if (script_persistency_id == NULL) {
		pcb_message(PCB_MSG_ERROR, "ScriptPersistency may be called only from the init part of a script\n");
		goto err;
	}

	fn = pcb_concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, SCRIPT_PERS, PCB_DIR_SEPARATOR_S, script_persistency_id, NULL);

	if (strcmp(cmd, "remove") == 0) {
		PCB_ACT_IRES(pcb_remove(fn));
		goto succ;
	}

	if (strcmp(cmd, "read") == 0) {
		FILE *f;
		long fsize = pcb_file_size(fn);
		char *data;

		if ((fsize < 0) || (fsize > PERS_MAX_SIZE))
			goto err;

		data = malloc(fsize+1);
		if (data == NULL)
			goto err;

		f = pcb_fopen(fn, "r");
		if (f == NULL) {
			free(data);
			goto err;
		}

		if (fread(data, 1, fsize, f) != fsize) {
			free(data);
			fclose(f);
			goto err;
		}

		fclose(f);
		data[fsize] = '\0';
		res->type = FGW_STR | FGW_DYN;
		res->val.str = data;
		goto succ;
	}

	pcb_message(PCB_MSG_ERROR, "Unknown command for ScriptPersistency\n");

	err:;
	PCB_ACT_IRES(-1);
	return 0;

	succ:
	free(fn);
	return 0; /* assume IRES is set already */
}

static char *script_gen_cookie(const char *force_id)
{
	if (force_id == NULL) {
		if (script_persistency_id == NULL) {
			pcb_message(PCB_MSG_ERROR, "ScriptCookie called from outside of script init, can not generate the cookie\n");
			return NULL;
		}
		force_id = script_persistency_id;
	}
	return pcb_concat("script::fungw::", force_id, NULL);
}

static int script_unload(const char *id, const char *preunload)
{
	char *cookie;
	htsp_entry_t *e = htsp_getentry(&scripts, id);
	if (e == NULL)
		return -1;

	cookie = script_gen_cookie(id);
	script_unload_entry(e, preunload, cookie);
	free(cookie);
	return 0;
}

static int script_load(const char *id, const char *fn, const char *lang)
{
	char name[PCB_PATH_MAX];
	pup_plugin_t *pup;
	script_t *s;
	int st;
	const char *old_id;

	if (htsp_has(&scripts, id)) {
		pcb_message(PCB_MSG_ERROR, "Can not load script %s from file %s: ID already in use\n", id, fn);
		return -1;
	}

	if (lang == NULL) {
TODO(": guess")
		pcb_message(PCB_MSG_ERROR, "Can not load script %s from file %s: failed to guess language from file name\n", id, fn);
		return -1;
	}

	if (strcmp(lang, "c") != 0) {
		pcb_snprintf(name, sizeof(name), "fungw_%s", lang);

		old_id = script_persistency_id;
		script_persistency_id = id;
		pup = pup_load(&script_pup, pcb_script_pup_paths, name, 0, &st);
		script_persistency_id = old_id;
		if (pup == NULL) {
			pcb_message(PCB_MSG_ERROR, "Can not load script engine %s for language %s\n", name, lang);
			return -1;
		}
	}
	else {
		lang = "pcb_rnd_c";
		pup = NULL;
	}

	s = calloc(1, sizeof(script_t));
	s->pup = pup;
	s->id = pcb_strdup(id);
	s->fn = pcb_strdup(fn);
	s->lang = pcb_strdup(lang);

	old_id = script_persistency_id;
	script_persistency_id = id;
	s->obj = fgw_obj_new(&pcb_fgw, s->id, s->lang, s->fn, NULL);
	script_persistency_id = old_id;

	if (s->obj == NULL) {
		script_free(s, NULL, NULL);
		pcb_message(PCB_MSG_ERROR, "Failed to parse/execute %s script from file %s\n", id, fn);
		return -1;
	}

	htsp_set(&scripts, s->id, s);
	return 0;
}

static int script_reload(const char *id)
{
	int ret;
	char *fn, *lang, *cookie;
	script_t *s;
	htsp_entry_t *e = htsp_getentry(&scripts, id);

	if (e == NULL)
		return -1;

	s = e->value;
	fn = pcb_strdup(s->fn);
	lang = pcb_strdup(s->lang);

	cookie = script_gen_cookie(id);
	script_unload_entry(e, "reload", cookie);
	free(cookie);

	ret = script_load(id, fn, lang);
	free(fn);
	free(lang);
	return ret;
}

void script_list(const char *pat)
{
	htsp_entry_t *e;
	re_se_t *r = NULL;

	if ((pat != NULL) && (*pat != '\0'))
		r = re_se_comp(pat);

	for(e = htsp_first(&scripts); e; e = htsp_next(&scripts, e)) {
		script_t *s = (script_t *)e->value;
		if ((r == NULL) || (re_se_exec(r, s->id)) || (re_se_exec(r, s->fn)) || (re_se_exec(r, s->lang)))
			pcb_message(PCB_MSG_INFO, "id=%s fn=%s lang=%s\n", s->id, s->fn, s->lang);
	}
	
	if (r != NULL)
		re_se_free(r);
}

static void oneliner_boilerplate(FILE *f, const char *lang, int pre)
{
	if (strcmp(lang, "mawk") == 0) {
		if (pre)
			fputs("BEGIN {\n", f);
		else
			fputs("}\n", f);
	}
}

int script_oneliner(const char *lang, const char *src)
{
	FILE *f;
	char *fn;
	int res = 0;

	fn = pcb_tempfile_name_new("oneliner");
	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_tempfile_unlink(fn);
		pcb_message(PCB_MSG_ERROR, "script oneliner: can't open temp file for write\n");
		return -1;
	}
	oneliner_boilerplate(f, lang, 1);
	fputs(src, f);
	fputs("\n", f);
	oneliner_boilerplate(f, lang, 0);
	fclose(f);

	if (script_load("__oneliner", fn, lang) != 0) {
		pcb_message(PCB_MSG_ERROR, "script oneliner: can't load/parse the script\n");
		res = -1;
	}
	script_unload("__oneliner", NULL);

	pcb_tempfile_unlink(fn);
	return res;
}

#include "timer.c"
#include "script_act.c"

char *script_cookie = "script plugin";

PCB_REGISTER_ACTIONS(script_action_list, script_cookie)

int pplg_check_ver_script(int ver_needed) { return 0; }

void pplg_uninit_script(void)
{
	htsp_entry_t *e;
	pcb_remove_actions_by_cookie(script_cookie);
	for(e = htsp_first(&scripts); e; e = htsp_next(&scripts, e)) {
		script_t *script = e->value;
		char *cookie = script_gen_cookie(script->id);
		script_unload_entry(e, "unload", cookie);
		free(cookie);
	}

	htsp_uninit(&scripts);
	pup_uninit(&script_pup);
}

#include "dolists.h"
int pplg_init_script(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(script_action_list, script_cookie);
	pcb_c_script_init();
	htsp_init(&scripts, strhash, strkeyeq);
	pup_init(&script_pup);
	return 0;
}

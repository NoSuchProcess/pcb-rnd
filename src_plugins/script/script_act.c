/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include <ctype.h>
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "live_script.h"


TODO("This should be in fungw")
static const char *guess_lang(const char *ext)
{
	const char **s, *tr[] = {
		"awk",         "mawk",
		"ruby",        "mruby",
		"py",          "python",
		"js",          "duktape",
		"javascript",  "duktape",
		"stt",         "estutter",
		NULL, NULL
	};

	/* translate short name to long name */
	for(s = tr; *s != NULL; s += 2) {
		if (strcmp(*s, ext) == 0) {
			s++;
			return *s;
		}
	}
	return ext;
}

/*** dialog box ***/
typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */

	int wslist; /* list of scripts */
	int walist; /* list of actions */
} script_dlg_t;

script_dlg_t script_dlg;

static void script_dlg_s2d_act(script_dlg_t *ctx)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[2];
	htsp_entry_t *e;
	script_t *sc;

	attr = &ctx->dlg[ctx->walist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		pcb_dad_tree_remove(attr, r);

	r = pcb_dad_tree_get_selected(&ctx->dlg[ctx->wslist]);
	if (r == NULL)
		return;

	sc = htsp_get(&scripts, r->cell[0]);
	if (sc == NULL)
		return;

	/* add all actions */
	cell[1] = NULL;
	for(e = htsp_first(&sc->obj->func_tbl); e; e = htsp_next(&sc->obj->func_tbl, e)) {
		cell[0] = pcb_strdup(e->key);
		pcb_dad_tree_append(attr, NULL, cell);
	}
}


static void script_dlg_s2d(script_dlg_t *ctx)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;
	htsp_entry_t *e;

	attr = &ctx->dlg[ctx->wslist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		pcb_dad_tree_remove(attr, r);

	/* add all items */
	cell[3] = NULL;
	for(e = htsp_first(&scripts); e; e = htsp_next(&scripts, e)) {
		script_t *s = (script_t *)e->value;
		cell[0] = pcb_strdup(s->id);
		cell[1] = pcb_strdup(s->lang);
		cell[2] = pcb_strdup(s->fn);
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wslist, &hv);
		free(cursor_path);
	}
	script_dlg_s2d_act(ctx);
}

void script_dlg_update(void)
{
	if (script_dlg.active)
		script_dlg_s2d(&script_dlg);
}

static void script_dlg_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	script_dlg_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(script_dlg_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void btn_unload_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	script_dlg_t *ctx = caller_data;
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&ctx->dlg[ctx->wslist]);
	if (row == NULL)
		return;

	pcb_script_unload(row->cell[0], "unload");
	script_dlg_s2d(ctx);
}

static void btn_reload_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	script_dlg_t *ctx = caller_data;
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&ctx->dlg[ctx->wslist]);
	if (row == NULL)
		return;

	script_reload(row->cell[0]);
	script_dlg_s2d(ctx);
}

static void slist_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	script_dlg_s2d_act((script_dlg_t *)caller_data);
}

static void btn_load_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	script_dlg_t *ctx = caller_data;
	int failed;
	char *tmp, *fn = pcb_gui->fileselect("script to load", "Select a script file to load", NULL, NULL, NULL, "script", PCB_HID_FSD_READ, NULL);
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	typedef struct {
		PCB_DAD_DECL_NOINIT(dlg)
		int wid, wlang;
	} idlang_t;
	idlang_t idlang;

	if (fn == NULL)
		return;

	memset(&idlang, 0, sizeof(idlang));
	PCB_DAD_BEGIN_VBOX(idlang.dlg);
		PCB_DAD_BEGIN_HBOX(idlang.dlg);
			PCB_DAD_LABEL(idlang.dlg, "ID:");
			PCB_DAD_STRING(idlang.dlg);
				idlang.wid = PCB_DAD_CURRENT(idlang.dlg);
				tmp = strrchr(fn, PCB_DIR_SEPARATOR_C);
				if (tmp != NULL) {
					tmp++;
					idlang.dlg[idlang.wid].default_val.str_value = tmp = pcb_strdup(tmp);
					tmp = strchr(tmp, '.');
					if (tmp != NULL)
						*tmp = '\0';
				}
		PCB_DAD_END(idlang.dlg);
		PCB_DAD_BEGIN_HBOX(idlang.dlg);
			PCB_DAD_LABEL(idlang.dlg, "language:");
			PCB_DAD_STRING(idlang.dlg);
				idlang.wlang = PCB_DAD_CURRENT(idlang.dlg);
				tmp = strrchr(fn, '.');
				if (tmp != NULL)
					idlang.dlg[idlang.wlang].default_val.str_value = pcb_strdup(guess_lang(tmp+1));
		PCB_DAD_END(idlang.dlg);
		PCB_DAD_BUTTON_CLOSES(idlang.dlg, clbtn);
	PCB_DAD_END(idlang.dlg);


	PCB_DAD_AUTORUN("script_load", idlang.dlg, "load script", NULL, failed);

	if ((!failed) && (pcb_script_load(idlang.dlg[idlang.wid].default_val.str_value, fn, idlang.dlg[idlang.wlang].default_val.str_value) == 0))
		script_dlg_s2d(ctx);

	PCB_DAD_FREE(idlang.dlg);
}

static void script_dlg_open(void)
{
	static const char *hdr[] = {"ID", "language", "file", NULL};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	if (script_dlg.active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(script_dlg.dlg);
	PCB_DAD_COMPFLAG(script_dlg.dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_HPANE(script_dlg.dlg);
		PCB_DAD_COMPFLAG(script_dlg.dlg, PCB_HATF_EXPFILL);
		/* left side */
		PCB_DAD_BEGIN_VBOX(script_dlg.dlg);
			PCB_DAD_COMPFLAG(script_dlg.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_TREE(script_dlg.dlg, 3, 0, hdr);
				PCB_DAD_COMPFLAG(script_dlg.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
				script_dlg.wslist = PCB_DAD_CURRENT(script_dlg.dlg);
				PCB_DAD_CHANGE_CB(script_dlg.dlg, slist_cb);
			PCB_DAD_BEGIN_HBOX(script_dlg.dlg);
				PCB_DAD_BUTTON(script_dlg.dlg, "Unload");
					PCB_DAD_HELP(script_dlg.dlg, "Unload the currently selected script");
					PCB_DAD_CHANGE_CB(script_dlg.dlg, btn_unload_cb);
				PCB_DAD_BUTTON(script_dlg.dlg, "Reload");
					PCB_DAD_HELP(script_dlg.dlg, "Reload the currently selected script\n(useful if the script has changed)");
					PCB_DAD_CHANGE_CB(script_dlg.dlg, btn_reload_cb);
				PCB_DAD_BUTTON(script_dlg.dlg, "Load...");
					PCB_DAD_HELP(script_dlg.dlg, "Load a new script from disk");
					PCB_DAD_CHANGE_CB(script_dlg.dlg, btn_load_cb);
			PCB_DAD_END(script_dlg.dlg);
		PCB_DAD_END(script_dlg.dlg);

		/* right side */
		PCB_DAD_BEGIN_VBOX(script_dlg.dlg);
			PCB_DAD_COMPFLAG(script_dlg.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_LABEL(script_dlg.dlg, "Actions:");
			PCB_DAD_TREE(script_dlg.dlg, 1, 0, NULL);
				PCB_DAD_COMPFLAG(script_dlg.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
				script_dlg.walist = PCB_DAD_CURRENT(script_dlg.dlg);
		PCB_DAD_END(script_dlg.dlg);
	PCB_DAD_END(script_dlg.dlg);
	PCB_DAD_BUTTON_CLOSES(script_dlg.dlg, clbtn);
	PCB_DAD_END(script_dlg.dlg);

	/* set up the context */
	script_dlg.active = 1;

	PCB_DAD_NEW("scripts", script_dlg.dlg, "pcb-rnd Scripts", &script_dlg, pcb_false, script_dlg_close_cb);
	script_dlg_s2d(&script_dlg);
}

/*** actions ***/

static int script_id_invalid(const char *id)
{
	for(; *id != '\0'; id++)
		if (!isalnum(*id) && (*id != '_'))
			return 1;
	return 0;
}

#define ID_VALIDATE(id, act) \
do { \
	if (script_id_invalid(id)) { \
		pcb_message(PCB_MSG_ERROR, #act ": Invalid script ID '%s' (must contain only alphanumeric characters and underscores)\n", id); \
		return FGW_ERR_ARG_CONV; \
	} \
} while(0)

static const char pcb_acth_LoadScript[] = "Load a fungw script";
static const char pcb_acts_LoadScript[] = "LoadScript(id, filename, [language])";
/* DOC: loadscript.html */
static fgw_error_t pcb_act_LoadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id, *fn, *lang = NULL;
	PCB_ACT_CONVARG(1, FGW_STR, LoadScript, id = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, LoadScript, fn = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, LoadScript, lang = argv[3].val.str);

	ID_VALIDATE(id, LoadScript);

	PCB_ACT_IRES(pcb_script_load(id, fn, lang));
	script_dlg_update();
	return 0;
}

static const char pcb_acth_UnloadScript[] = "Unload a fungw script";
static const char pcb_acts_UnloadScript[] = "UnloadScript(id)";
/* DOC: unloadscript.html */
static fgw_error_t pcb_act_UnloadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id = NULL;
	PCB_ACT_CONVARG(1, FGW_STR, UnloadScript, id = argv[1].val.str);

	ID_VALIDATE(id, UnloadScript);

	PCB_ACT_IRES(pcb_script_unload(id, "unload"));
	return 0;
}

static const char pcb_acth_ReloadScript[] = "Reload a fungw script";
static const char pcb_acts_ReloadScript[] = "ReloadScript(id)";
/* DOC: reloadscript.html */
static fgw_error_t pcb_act_ReloadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id = NULL;
	PCB_ACT_CONVARG(1, FGW_STR, UnloadScript, id = argv[1].val.str);

	ID_VALIDATE(id, ReloadScript);

	PCB_ACT_IRES(script_reload(id));
	script_dlg_update();
	return 0;
}

static const char pcb_acth_ScriptPersistency[] = "Read or remove script persistency data savd on preunload";
static const char pcb_acts_ScriptPersistency[] = "ScriptPersistency(read|remove)";
/* DOC: scriptpersistency.html */
static fgw_error_t pcb_act_ScriptPersistency(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd = NULL;
	PCB_ACT_CONVARG(1, FGW_STR, ScriptPersistency, cmd = argv[1].val.str);
	return script_persistency(res, cmd);
}

static const char pcb_acth_ListScripts[] = "List fungw scripts, optionally filtered wiht regex pat.";
static const char pcb_acts_ListScripts[] = "ListScripts([pat])";
/* DOC: listscripts.html */
static fgw_error_t pcb_act_ListScripts(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *pat = NULL;
	PCB_ACT_MAY_CONVARG(1, FGW_STR, ListScripts, pat = argv[1].val.str);

	script_list(pat);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acth_BrowseScripts[] = "Present a dialog box for browsing scripts";
static const char pcb_acts_BrowseScripts[] = "BrowseScripts()";
static fgw_error_t pcb_act_BrowseScripts(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	script_dlg_open();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acth_ScriptCookie[] = "Return a cookie specific to the current script instance during script initialization";
static const char pcb_acts_ScriptCookie[] = "ScriptCookie()";
/* DOC: scriptcookie.html */
static fgw_error_t pcb_act_ScriptCookie(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	res->type = FGW_STR | FGW_DYN;
	res->val.str = script_gen_cookie(NULL);
	if (res->val.str == NULL)
		return -1;
	return 0;
}

static const char pcb_acth_Oneliner[] = "Execute a script one-liner using a specific language";
static const char pcb_acts_Oneliner[] = "Oneliner(lang, script)";
/* DOC: onliner.html */
static fgw_error_t pcb_act_Oneliner(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *first = NULL, *lang = argv[0].val.func->name, *scr = NULL;

	if (strcmp(lang, "oneliner") == 0) {
		/* call to oneliner(lang, script) */
		PCB_ACT_CONVARG(1, FGW_STR, Oneliner, lang = argv[1].val.str);
		PCB_ACT_CONVARG(2, FGW_STR, Oneliner, scr = argv[2].val.str);
	}
	else if (strcmp(lang, "/exit") == 0) {
		PCB_ACT_IRES(pcb_cli_leave());
		return 0;
	}
	else {
		/* call to lang(script) */
		PCB_ACT_MAY_CONVARG(1, FGW_STR, Oneliner, scr = argv[1].val.str);
	}

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Oneliner, first = argv[1].val.str);
	if (first != NULL) {
		if (*first == '/') {
			if (pcb_strcasecmp(scr, "/exit") == 0) {
				PCB_ACT_IRES(pcb_cli_leave());
				return 0;
			}
			PCB_ACT_IRES(-1); /* ignore /click, /tab and others for now */
			return 0;
		}
	}

	lang = guess_lang(lang);

	if (scr == NULL) {
		PCB_ACT_IRES(pcb_cli_enter(lang, lang));
		return 0;
	}

	if (pcb_strcasecmp(scr, "/exit") == 0) {
		PCB_ACT_IRES(pcb_cli_leave());
		return 0;
	}

	PCB_ACT_IRES(script_oneliner(lang, scr));
	return 0;
}

static pcb_action_t script_action_list[] = {
	{"LoadScript", pcb_act_LoadScript, pcb_acth_LoadScript, pcb_acts_LoadScript},
	{"UnloadScript", pcb_act_UnloadScript, pcb_acth_UnloadScript, pcb_acts_UnloadScript},
	{"ReloadScript", pcb_act_ReloadScript, pcb_acth_ReloadScript, pcb_acts_ReloadScript},
	{"ScriptPersistency", pcb_act_ScriptPersistency, pcb_acth_ScriptPersistency, pcb_acts_ScriptPersistency},
	{"ListScripts", pcb_act_ListScripts, pcb_acth_ListScripts, pcb_acts_ListScripts},
	{"BrowseScripts", pcb_act_BrowseScripts, pcb_acth_BrowseScripts, pcb_acts_BrowseScripts},
	{"AddTimer", pcb_act_AddTimer, pcb_acth_AddTimer, pcb_acts_AddTimer},
	{"ScriptCookie", pcb_act_ScriptCookie, pcb_acth_ScriptCookie, pcb_acts_ScriptCookie},
	{"LiveScript", pcb_act_LiveScript, pcb_acth_LiveScript, pcb_acts_LiveScript},
	
	/* script shorthands */
	{"awk",         pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"mawk",        pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"lua",         pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"tcl",         pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"javascript",  pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"js",          pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"stt",         pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"estutter",    pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"perl",        pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"ruby",        pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"mruby",       pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"py",          pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},
	{"python",      pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner},

	{"Oneliner", pcb_act_Oneliner, pcb_acth_Oneliner, pcb_acts_Oneliner}
};

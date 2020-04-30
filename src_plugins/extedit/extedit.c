/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/plugins.h>

#include "board.h"
#include <librnd/core/compat_fs.h>
#include <librnd/core/compat_misc.h>
#include "conf_core.h"
#include "data.h"
#include "buffer.h"
#include <librnd/core/paths.h>
#include "remove.h"
#include <librnd/core/safe_fs.h>
#include "search.h"
#include "undo.h"
#include "../src_plugins/io_lihata/io_lihata.h"
#include "../src_plugins/io_lihata/write.h"
#include "../src_plugins/io_lihata/read.h"

/* List of all available external edit methods */
typedef enum {
	EEF_LIHATA,
	EEF_max
} extedit_fmt_t;

const char *extedit_fmt_names[EEF_max+1] = {
	"lihata",
	NULL
};

typedef struct extedit_method_s {
	char *name;
	long types;
	extedit_fmt_t fmt;
	char *command;
} extedit_method_t;

static extedit_method_t methods[] = {
	{"pcb-rnd",           PCB_OBJ_SUBC, EEF_LIHATA, "pcb-rnd -C \"design;%c\" \"%f\""},
	{"pcb-rnd (no cfg)",  PCB_OBJ_SUBC, EEF_LIHATA, "pcb-rnd \"%f\""},
	{"editor",            PCB_OBJ_SUBC, EEF_LIHATA, "xterm -e editor \"%f\""},
	{NULL, 0, 0, NULL}
};

/* accept these objects for external editing */
#define EXTEDIT_TYPES (PCB_OBJ_SUBC)

#include "extedit_dad.c"

/* HID-dependent, portable watch of child process */
typedef struct {
	FILE *fc;
	int stay;
	rnd_hidval_t wid;
} extedit_wait_t;

rnd_bool extedit_fd_watch(rnd_hidval_t watch, int fd, unsigned int condition, rnd_hidval_t user_data)
{
	char tmp[128];
	int res;
	extedit_wait_t *ctx = user_data.ptr;

	/* excess callbacks */
	if (!ctx->stay)
		return rnd_true;

	res = fread(tmp, 1, sizeof(tmp), ctx->fc);
	if (res <= 0) {
		ctx->stay = 0;
		return rnd_false; /* also disables/removes the watch */
	}
	return rnd_true;
}

/* Invoke the child process, display a "progress bar" or some other indication
   while it's running and wait for it to exit (preferrably keeping the gui
   refreshed, even if the process is blocking) */
static void invoke(extedit_method_t *mth, const char *fn, const char *fn_cfg)
{
	rnd_build_argfn_t subs;
	char *cmd;
	FILE *fc;

	memset(&subs, 0, sizeof(subs));
	subs.params['f' - 'a'] = fn;
	subs.params['c' - 'a'] = fn_cfg;
	subs.hidlib = &PCB->hidlib;
	cmd = rnd_build_argfn(mth->command, &subs);

	/* Don't use rnd_system() because that blocks the current process and the
	   GUI toolkit won't have a chance to handle expose events */
	fc = rnd_popen(&PCB->hidlib, cmd, "r");

	if (rnd_gui != NULL) {
		int fd = rnd_fileno(fc);
		if (fd > 0) {
			int n = 0;
			rnd_hidval_t hd;
			extedit_wait_t ctx;

			ctx.stay = 1;
			ctx.fc = fc;
			hd.ptr = &ctx;
			ctx.wid = rnd_gui->watch_file(rnd_gui, fd, RND_WATCH_READABLE | RND_WATCH_HANGUP, extedit_fd_watch, hd);
			while(ctx.stay) {
				if (rnd_gui != NULL) {
					n++;
					rnd_hid_progress(50+sin((double)n/10.0)*40, 100, "Invoked external editor. Please edit, save and close there to finish this operation");
				}
				rnd_ms_sleep(50);
			}
		}
		else
			goto old_wait;
	}
	else {
		old_wait:;

		if (rnd_gui != NULL) {
			rnd_hid_progress(50, 100, "Invoked external editor. Please edit, save and close there to finish this operation");
			rnd_ms_sleep(1000); /* ugly hack: give the GUI some time to flush */
		}
		while(!(feof(fc))) {
			char tmp[128];
			fread(tmp, 1, sizeof(tmp), fc);
		}
	}
	pclose(fc);

	if (rnd_gui != NULL)
		rnd_hid_progress(0, 0, NULL);
	free(cmd);
}



static const char pcb_acts_extedit[] = "extedit(object|selected|buffer, [interactive|method])\n";
static const char pcb_acth_extedit[] = "Invoke an external program to edit a specific part of the current board.";
static fgw_error_t pcb_act_extedit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd = NULL, *method = NULL;
	long type;
	void *ptr1, *ptr2, *ptr3;
	extedit_method_t *mth = NULL;
	char *tmp_fn, *tmp_cfg_fn;
	int ret = 1;
	FILE *f;
	int bn = PCB_MAX_BUFFER - 1, load_bn = PCB_MAX_BUFFER - 1;
	int obn = conf_core.editor.buffer_number;
	int paste = 0, del_selected = 0;
	rnd_coord_t pastex = 0, pastey = 0;

	RND_ACT_MAY_CONVARG(1, FGW_STR, extedit, cmd = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, extedit, method = argv[2].val.str);

	/* pick up the object to edit */
	if ((cmd == NULL) || (rnd_strcasecmp(cmd, "object") == 0)) {
		rnd_coord_t x, y;
		rnd_hid_get_coords("Click on object to edit", &x, &y, 0);
		type = pcb_search_screen(x, y, EXTEDIT_TYPES, &ptr1, &ptr2, &ptr3);

		pcb_buffer_set_number(bn);
		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		if (pcb_copy_obj_to_buffer(PCB, pcb_buffers[bn].Data, PCB->Data, type, ptr1, ptr2, ptr3) == NULL) {
			rnd_message(RND_MSG_ERROR, "Failed to copy target objects to temporary paste buffer\n");
			goto quit0;
		}
		paste = 1;
	}
	else if ((argc > 1) && (rnd_strcasecmp(cmd, "selected") == 0)) {
		pcb_buffer_set_number(bn);
		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		pcb_buffer_add_selected(PCB, PCB_PASTEBUFFER, pcb_crosshair.X, pcb_crosshair.Y, rnd_false, rnd_false);
		pastex = pcb_crosshair.X;
		pastey = pcb_crosshair.Y;
		del_selected = 1;
		paste = 1;
		if (pcb_data_is_empty(PCB_PASTEBUFFER->Data)) {
			rnd_message(RND_MSG_WARNING, "Nothing is selected, can't ext-edit selection\n");
			goto quit0;
		}
	}
	else if ((argc > 1) && (rnd_strcasecmp(cmd, "buffer") == 0)) {
		load_bn = bn = conf_core.editor.buffer_number;
		if (pcb_data_is_empty(PCB_PASTEBUFFER->Data)) {
			rnd_message(RND_MSG_WARNING, "Nothing in current buffer, can't ext-edit selection\n");
			goto quit0;
		}
	}
	else {
		rnd_message(RND_MSG_ERROR, "Wrong 1st argument '%s'\n", cmd);
		ret = 1;
		goto quit0;
	}

	/* determine the method */
	if (argc > 2) {
		for(mth = methods; mth->name != NULL; mth++) {
			if (rnd_strcasecmp(mth->name, method) == 0)
				break;
		}
		if (mth->name == NULL) {
			rnd_message(RND_MSG_ERROR, "unknown method '%s'; available methods:\n", method);
			for(mth = methods; mth->name != NULL; mth++) {
				if (mth != methods)
					rnd_message(RND_MSG_ERROR, ", ", mth->name);
				rnd_message(RND_MSG_ERROR, "%s", mth->name);
			}
			rnd_message(RND_MSG_ERROR, "\n");
			ret = 1;
			goto quit0;
		}
	}
	if (mth == NULL) {
		mth = extedit_interactive();
		if (mth == NULL) { /* no method selected */
			ret = 1;
			goto quit0;
		}
	}

	tmp_fn = rnd_tempfile_name_new("extedit");
	tmp_cfg_fn = rnd_tempfile_name_new("extedit_cfg");
	if ((tmp_fn == NULL) || (tmp_cfg_fn == NULL)) {
		rnd_message(RND_MSG_ERROR, "Failed to create temporary file\n");
		ret = 1;
		goto quit1;
	}

	/* export */
	switch(mth->fmt) {
		case EEF_LIHATA:
			{
				int res;
				void *udata;

				rnd_conf_save_file(&PCB->hidlib, NULL, NULL, RND_CFR_DESIGN, tmp_cfg_fn);

				f = rnd_fopen(&PCB->hidlib, tmp_fn, "w");
				if (f == NULL) {
					rnd_message(RND_MSG_ERROR, "Failed to open temporary file\n");
					goto quit1;
				}

				res = io_lihata_write_subcs_head(plug_io_lihata_default, &udata, f, 0, 1);
				if (res == 0) {
					res |= io_lihata_write_subcs_subc(plug_io_lihata_default, &udata, f, pcb_subclist_first(&pcb_buffers[bn].Data->subc));
					res |= io_lihata_write_subcs_tail(plug_io_lihata_default, &udata, f);
				}

				if (res != 0) {
					fclose(f);
					rnd_message(RND_MSG_ERROR, "Failed to export target objects to lihata footprint.\n");
					goto quit1;
				}
				fclose(f);
			}
			break;
		case EEF_max:
			break;
	}

	/* invoke external program */
	invoke(mth, tmp_fn, tmp_cfg_fn);

	/* load the result */
	switch(mth->fmt) {
		case EEF_LIHATA:
			{
				int bn = load_bn;

				pcb_buffer_set_number(bn);
				pcb_buffer_clear(PCB, PCB_PASTEBUFFER);

				if (io_lihata_parse_subc(plug_io_lihata_default, pcb_buffers[bn].Data, tmp_fn, NULL) != 0) {
					rnd_message(RND_MSG_ERROR, "Failed to load the edited footprint. File left at '%s'.\n", tmp_fn);
					ret = 1;
					goto quit1;
				}

				if (del_selected)
					pcb_remove_selected(rnd_true);
				if (paste) {
					pcb_undo_save_serial();
					pcb_buffer_copy_to_layout(PCB, pastex, pastey, rnd_false);
					pcb_undo_restore_serial();
					pcb_remove_object(type, ptr1, ptr2, ptr3);
					pcb_undo_inc_serial();
				}
				ret = 0;
			}
		case EEF_max:
			break;
	}
	if (rnd_gui != NULL)
		rnd_gui->invalidate_all(rnd_gui);

	quit1:;
	if (tmp_fn != NULL)
		rnd_tempfile_unlink(tmp_fn);
	if (tmp_cfg_fn != NULL)
		rnd_tempfile_unlink(tmp_cfg_fn);
	quit0:;
	pcb_buffer_set_number(obn);
	RND_ACT_IRES(ret);
	return 0;
}

static rnd_action_t extedit_action_list[] = {
	{"extedit", pcb_act_extedit, pcb_acts_extedit, pcb_acth_extedit}
};

static const char *extedit_cookie = "extedit plugin";

int pplg_check_ver_extedit(int ver_needed) { return 0; }

void pplg_uninit_extedit(void)
{
	rnd_remove_actions_by_cookie(extedit_cookie);
}

int pplg_init_extedit(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(extedit_action_list, extedit_cookie)
	return 0;
}

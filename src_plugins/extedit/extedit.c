/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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
 */

#include "config.h"

#include "hid.h"
#include "hid_attrib.h"
#include "hid_actions.h"
#include "hid_dad.h"
#include "action_helper.h"

#include "board.h"
#include "compat_fs.h"
#include "compat_misc.h"
#include "const.h"
#include "conf_core.h"
#include "data.h"
#include "buffer.h"
#include "paths.h"
#include "remove.h"
#include "safe_fs.h"
#include "search.h"
#include "../src_plugins/io_lihata/io_lihata.h"
#include "../src_plugins/io_lihata/write.h"
#include "../src_plugins/io_lihata/read.h"

/* List of all available external edit methods */
typedef enum {
	EEF_LIHATA
} extedit_fmt_t;

typedef struct extedit_method_s {
	char *name;
	long types;
	extedit_fmt_t fmt;
	char *command;
} extedit_method_t;

static extedit_method_t methods[] = {
	{"pcb-rnd",         PCB_TYPE_SUBC | PCB_TYPE_ELEMENT, EEF_LIHATA, "pcb-rnd \"%f\""},
	{"editor",          PCB_TYPE_SUBC | PCB_TYPE_ELEMENT, EEF_LIHATA, "xterm -e editor \"%f\""},
	{NULL, 0, 0, NULL}
};

/* accept these objects for external editing */
#define EXTEDIT_TYPES (PCB_TYPE_SUBC | PCB_TYPE_ELEMENT)

#include "extedit_dad.c"

/* HID-dependent, portable watch of child process */
typedef struct {
	FILE *fc;
	int stay;
	pcb_hidval_t wid;
} extedit_wait_t;

void extedit_fd_watch(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data)
{
	char tmp[128];
	int res;
	extedit_wait_t *ctx = user_data.ptr;

	/* excess callbacks */
	if (!ctx->stay)
		return;

	res = fread(tmp, 1, sizeof(tmp), ctx->fc);
	if (res <= 0) {
		pcb_gui->unwatch_file(ctx->wid);
		ctx->stay = 0;
	}
}

/* Invoke the child process, display a "progress bar" or some other indication
   while it's running and wait for it to exit (preferrably keeping the gui
   refreshed, even if the process is blocking) */
static void invoke(extedit_method_t *mth, const char *fn)
{
	pcb_build_argfn_t subs;
	char *cmd;
	FILE *fc;

	memset(&subs, 0, sizeof(subs));
	subs.params['f' - 'a'] = fn;
	cmd = pcb_build_argfn(mth->command, &subs);

	/* Don't use pcb_system() because that blocks the current process and the
	   GUI toolkit won't have a chance to handle expose events */
	fc = pcb_popen(cmd, "r");

	if (pcb_gui != NULL) {
		int fd = pcb_fileno(fc);
		if (fd > 0) {
			int n = 0;
			pcb_hidval_t hd;
			extedit_wait_t ctx;

			ctx.stay = 1;
			ctx.fc = fc;
			hd.ptr = &ctx;
			ctx.wid = pcb_gui->watch_file(fd, PCB_WATCH_READABLE | PCB_WATCH_HANGUP, extedit_fd_watch, hd);
			while(ctx.stay) {
				if (pcb_gui != NULL) {
					n++;
					pcb_gui->progress(50+sin((double)n/10.0)*40, 100, "Invoked external editor. Please edit, save and close there to finish this operation");
				}
				pcb_ms_sleep(50);
			}
		}
		else
			goto old_wait;
	}
	else {
		old_wait:;

		if (pcb_gui != NULL) {
			pcb_gui->progress(50, 100, "Invoked external editor. Please edit, save and close there to finish this operation");
			pcb_ms_sleep(1000); /* ugly hack: give the GUI some time to flush */
		}
		while(!(feof(fc))) {
			char tmp[128];
			fread(tmp, 1, sizeof(tmp), fc);
		}
	}
	fclose(fc);

	if (pcb_gui != NULL)
		pcb_gui->progress(0, 0, NULL);
	free(cmd);
}



static const char pcb_acts_extedit[] = "extedit(object|selected, [interactive|method])\n";
static const char pcb_acth_extedit[] = "Invoke an external program to edit a specific part of the current board.";
static int pcb_act_extedit(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	long type;
	void *ptr1, *ptr2, *ptr3;
	extedit_method_t *mth = NULL;
	char *tmp_fn;
	int ret = 1;
	FILE *f;

	/* pick up the object to edit */
	if ((argc == 0) || (pcb_strcasecmp(argv[0], "object") == 0)) {
		pcb_gui->get_coords("Click on object to edit", &x, &y);
		type = pcb_search_screen(x, y, EXTEDIT_TYPES, &ptr1, &ptr2, &ptr3);
	}
	else if ((argc > 0) && (pcb_strcasecmp(argv[0], "selected") == 0)) {
#warning TODO
		pcb_message(PCB_MSG_ERROR, "\"Selected\" not supported yet\n");
		return 1;
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Wrong 1st argument '%s'\n", argv[0]);
		return 1;
	}

	/* determine the method */
	if (argc > 1) {
		for(mth = methods; mth->name != NULL; mth++) {
			if (pcb_strcasecmp(mth->name, argv[1]) == 0)
				break;
		}
		if (mth->name == NULL) {
			pcb_message(PCB_MSG_ERROR, "unknown method '%s'; available methods:\n", argv[1]);
			for(mth = methods; mth->name != NULL; mth++) {
				if (mth != methods)
					pcb_message(PCB_MSG_ERROR, ", ", mth->name);
				pcb_message(PCB_MSG_ERROR, "%s", mth->name);
			}
			pcb_message(PCB_MSG_ERROR, "\n");
			return 1;
		}
	}
	if (mth == NULL) {
		mth = extedit_interactive();
		if (mth == NULL) /* no method selected */
			return 1;
	}

	tmp_fn = pcb_tempfile_name_new("extedit");
	if (tmp_fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to create temporary file\n");
		return 1;
	}

	/* export */
	switch(mth->fmt) {
		case EEF_LIHATA:
			{
				int bn =PCB_MAX_BUFFER - 1;

				pcb_buffer_set_number(bn);
				pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
				if (pcb_copy_obj_to_buffer(PCB, pcb_buffers[bn].Data, PCB->Data, type, ptr1, ptr2, ptr3) == NULL) {
					pcb_message(PCB_MSG_ERROR, "Failed to copy target objects to temporary paste buffer\n");
					goto quit1;
				}

				f = pcb_fopen(tmp_fn, "w");
				if (f == NULL) {
					pcb_message(PCB_MSG_ERROR, "Failed to open temporary file\n");
					goto quit1;
				}

				if (io_lihata_write_element(&plug_io_lihata_v4, f, pcb_buffers[bn].Data) != 0) {
					fclose(f);
					pcb_message(PCB_MSG_ERROR, "Failed to export target objects to lihata footprint.\n");
					goto quit1;
				}
				fclose(f);
			}
			break;
	}

	/* invoke external program */
	invoke(mth, tmp_fn);

	/* load the result */
	switch(mth->fmt) {
		case EEF_LIHATA:
			{
				int bn =PCB_MAX_BUFFER - 1;

				pcb_buffer_set_number(bn);
				pcb_buffer_clear(PCB, PCB_PASTEBUFFER);

				if (io_lihata_parse_element(&plug_io_lihata_v4, pcb_buffers[bn].Data, tmp_fn) != 0) {
					pcb_message(PCB_MSG_ERROR, "Failed to load the edited footprint. File left at '%s'.\n", tmp_fn);
					goto quit0;
				}

				pcb_buffer_copy_to_layout(PCB, 0, 0);
				pcb_remove_object(type, ptr1, ptr2, ptr3);
				ret = 0;
			}
	}
	if (pcb_gui != NULL)
		pcb_gui->invalidate_all();

	quit1:;
	pcb_tempfile_unlink(tmp_fn);
	quit0:;
	return ret;
}

static pcb_hid_action_t extedit_action_list[] = {
	{"extedit", 0, pcb_act_extedit,
	 pcb_acts_extedit, pcb_acth_extedit}
};

static const char *extedit_cookie = "extedit plugin";

PCB_REGISTER_ACTIONS(extedit_action_list, extedit_cookie)

int pplg_check_ver_extedit(int ver_needed) { return 0; }

void pplg_uninit_extedit(void)
{
	pcb_hid_remove_actions_by_cookie(extedit_cookie);
}

#include "dolists.h"
int pplg_init_extedit(void)
{
	PCB_REGISTER_ACTIONS(extedit_action_list, extedit_cookie)
	return 0;
}

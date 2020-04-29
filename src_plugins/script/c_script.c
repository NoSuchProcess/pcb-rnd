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

/* Fungw engine for loading and runnign "scripts" written in C. The .so files
   are loaded with puplug low level */

#include <libfungw/fungw.h>
#include <puplug/os_dep.h>
#include <stdlib.h>

#include <librnd/core/fptr_cast.h>

static fgw_error_t fgws_c_call_script(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_error_t rv;
	fgw_func_t *fnc = argv[0].val.func;
	rv = fnc->func(res, argc, argv);
	fgw_argv_free(fnc->obj->parent, argc, argv);
	return rv;
}


int fgws_c_load(fgw_obj_t *obj, const char *filename, const char *opts)
{
	pup_plugin_t *library = calloc(sizeof(pup_plugin_t), 1);
	typedef int (*init_t)(fgw_obj_t *obj, const char *opts);
	init_t init;
	

	if (pup_load_lib(&script_pup, library, filename) != 0) {
		rnd_message(RND_MSG_ERROR, "Can't dlopen() %s\n", filename);
		free(library);
		return -1;
	}

	init = (init_t)pcb_cast_d2f(pup_dlsym(library, "pcb_rnd_init"));
	if (init == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't find pcb_rnd_init() in %s - not a pcb-rnd c \"script\".\n", filename);
		free(library);
		return -1;
	}

	if (init(obj, opts) != 0) {
		rnd_message(RND_MSG_ERROR, "%s pcb_rnd_init() returned error\n", filename);
		free(library);
		return -1;
	}

	library->name = rnd_strdup(filename);
	obj->script_data = library;
	return 0;
}

int fgws_c_unload(fgw_obj_t *obj)
{
	pup_plugin_t *library = obj->script_data;
	typedef void (*uninit_t)(fgw_obj_t *obj);
	uninit_t uninit;

	uninit = (uninit_t)pcb_cast_d2f(pup_dlsym(library, "pcb_rnd_uninit"));
	if (uninit != NULL)
		uninit(obj);

	pup_unload_lib(library);

	free(library->name);
	free(library);

	return 0;
}

static fgw_eng_t pcb_fgw_c_eng = {
	"pcb_rnd_c",
	fgws_c_call_script,
	NULL,
	fgws_c_load,
	fgws_c_unload,
	NULL, NULL
};

static void pcb_c_script_init(void)
{
	fgw_eng_reg(&pcb_fgw_c_eng);
}

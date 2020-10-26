/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015,2016,2019,2020 Tibor 'Igor2' Palinkas
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


/* This used to be file.c; some of the code landed in the io_pcb plugin,
   the format-independent parts ended up here. */

/* file save, load, merge ... routines */

/* If emergency format is not configured but emergency file name is, default
   to saving in the native format */
#define DEFAULT_EMERGENCY_FMT "lihata"

#include "config.h"

#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>

#include <locale.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "change.h"
#include <librnd/core/conf.h>
#include "data.h"
#include <librnd/core/error.h>
#include "plug_io.h"
#include "remove.h"
#include <librnd/core/paths.h>
#include "rats_patch.h"
#include <librnd/core/actions.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid_menu.h>
#include "event.h"
#include <librnd/core/compat_misc.h>
#include "route_style.h"
#include <librnd/core/compat_fs.h>
#include <librnd/core/compat_lrealpath.h>
#include "layer_vis.h"
#include <librnd/core/safe_fs.h>
#include "plug_footprint.h"
#include <librnd/core/file_loaded.h>
#include <librnd/core/tool.h>
#include "view.h"

pcb_plug_io_t *pcb_plug_io_chain = NULL;
int pcb_io_err_inhibit = 0;
pcb_view_list_t pcb_io_incompat_lst;
static rnd_bool pcb_io_incompat_lst_enable = rnd_false;

void pcb_plug_io_err(rnd_hidlib_t *hidlib, int res, const char *what, const char *filename)
{
	if (pcb_io_err_inhibit)
		return;
	if (res != 0) {
		const char *reason = "", *comment = "";
		if (pcb_plug_io_chain != NULL) {
			if (filename == NULL) {
				reason = "none of io plugins could successfully write the file";
				filename = "";
			}
			else {
				FILE *f;
				reason = "none of io plugins could successfully read file";
				f = rnd_fopen(hidlib, filename, "r");
				if (f != NULL) {
					fclose(f);
					comment = "(unknown/invalid file format?)";
				}
				else
					comment = "(can not open the file for reading)";
			}
		}
		else {
			reason = "no io plugin loaded, I don't know any file format";
			if (filename == NULL)
				filename = "";
		}
		rnd_message(RND_MSG_ERROR, "IO error during %s: %s %s %s\n", what, reason, filename, comment);
	}
}

static char *last_design_dir = NULL;
void pcb_set_design_dir(const char *fn)
{
	char *end;

	free(last_design_dir);
	last_design_dir = NULL;

	if (fn != NULL)
		last_design_dir = rnd_lrealpath(fn);

	if (last_design_dir == NULL) {
		last_design_dir = rnd_strdup("<invalid>");
		rnd_conf_force_set_str(conf_core.rc.path.design, last_design_dir);
		rnd_conf_ro("rc/path/design");
		return;
	}

	end = strrchr(last_design_dir, RND_DIR_SEPARATOR_C);
	if (end != NULL)
		*end = '\0';

	rnd_conf_force_set_str(conf_core.rc.path.design, last_design_dir);
	rnd_conf_ro("rc/path/design");
}

static int pcb_test_parse_all(FILE *ft, const char *Filename, const char *fmt, pcb_plug_iot_t type, pcb_find_io_t *available, int *accepts, int *accept_total, int maxav, int ignore_missing, int gen_event)
{
	int len, n;

	if (ft == NULL) {
		if (!ignore_missing)
			rnd_message(RND_MSG_ERROR, "Error: can't open %s for reading (format is %s)\n", Filename, fmt);
		return -1;
	}

	if (gen_event)
		rnd_event(&PCB->hidlib, RND_EVENT_LOAD_PRE, "s", Filename);

	len = pcb_find_io(available, maxav, type, 0, fmt);
	if (fmt != NULL) {
		/* explicit format */
		for(n = 0; n < len; n++) {
			void (*f)();
			switch(type) {
				case PCB_IOT_PCB: f = (void (*)())available[n].plug->parse_pcb; break;
				default: assert(!"internal error: pcb_test_parse_all: wrong type"); f = NULL;
			}
			if (f != NULL) {
				accepts[n] = 1; /* force-accept - if it can handle the format, and the user explicitly wanted this format, let's try it */
				(*accept_total)++;
			}
		}

		if (*accept_total <= 0) {
			rnd_message(RND_MSG_ERROR, "can't find a IO_ plugin to load a PCB using format %s\n", fmt);
			return -1;
		}

		if (*accept_total > 1) {
			rnd_message(RND_MSG_INFO, "multiple IO_ plugins can handle format %s - I'm going to try them all, but you may want to be more specific next time; formats found:\n", fmt);
			for(n = 0; n < len; n++)
				rnd_message(RND_MSG_INFO, "    %s\n", available[n].plug->description);
		}
	}
	else {
		/* test-parse with all plugins to see who can handle the syntax */
		if((fgetc(ft) != EOF)) {
			rewind(ft);
			for(n = 0; n < len; n++) {
				if ((available[n].plug->test_parse == NULL) || (available[n].plug->test_parse(available[n].plug, type, Filename, ft))) {
					accepts[n] = 1;
					(*accept_total)++;
				}
				else
					accepts[n] = 0;
				rewind(ft);
			}
		}
	}

	if (*accept_total == 0) {
		rnd_message(RND_MSG_ERROR, "none of the IO_ plugin recognized the file format of %s - it's either not a valid board file or does not match the format specified\n", Filename);
		return -1;
	}
	return len;
}

int pcb_parse_pcb(pcb_board_t *Ptr, const char *Filename, const char *fmt, int load_settings, int ignore_missing)
{
	int res = -1, len, n;
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int accepts[PCB_IO_MAX_FORMATS]; /* test-parse output */
	int accept_total = 0;
	FILE *ft;
	long design_root_cnt = rnd_conf_main_root_replace_cnt[RND_CFR_DESIGN];

	ft = rnd_fopen(&Ptr->hidlib, Filename, "r");
	len = pcb_test_parse_all(ft, Filename, fmt, PCB_IOT_PCB, available, accepts, &accept_total, sizeof(available)/sizeof(available[0]), ignore_missing, load_settings);
	if (ft != NULL)
		fclose(ft);
	if (len < 0)
		return -1;

	Ptr->Data->loader = NULL;

	pcb_layergrp_inhibit_inc();

	/* try all plugins that said it could handle the file */
	for(n = 0; n < len; n++) {
		if ((available[n].plug->parse_pcb == NULL) || (!accepts[n])) /* can't parse or doesn't want to parse this file */
			continue;
		res = available[n].plug->parse_pcb(available[n].plug, Ptr, Filename, load_settings);
		if (res == 0) {
			if (Ptr->Data->loader == NULL) /* if the loader didn't set this (to some more fine grained, e.g. depending on file format version) */
				Ptr->Data->loader = available[n].plug;
			break;
		}
	}

	pcb_layergrp_inhibit_dec();


	if ((res == 0) && (load_settings)) {
		if (design_root_cnt == rnd_conf_main_root_replace_cnt[RND_CFR_DESIGN]) /* the newly loaded board did not bring a design root */
			rnd_conf_reset(RND_CFR_DESIGN, "<pcb_parse_pcb>");
		rnd_conf_load_project(NULL, Filename);
	}

	if (res == 0)
		pcb_set_design_dir(Filename);

	if (load_settings)
		rnd_event(&PCB->hidlib, RND_EVENT_LOAD_POST, "si", Filename, res);
	rnd_event(&PCB->hidlib, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
	rnd_conf_set(RND_CFR_DESIGN, "design/text_font_id", 0, "0", RND_POL_OVERWRITE); /* we have only one font now, make sure it is selected */

	pcb_plug_io_err(&Ptr->hidlib, res, "load pcb", Filename);
	return res;
}

int pcb_parse_footprint(pcb_data_t *Ptr, const char *Filename, const char *fmt)
{
	int res = -1, len, n;
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int accepts[PCB_IO_MAX_FORMATS]; /* test-parse output */
	int accept_total = 0;
	FILE *f;
	pcb_fp_fopen_ctx_t fctx;

	f = pcb_fp_fopen(&conf_core.rc.library_search_paths, Filename, &fctx, Ptr);
	if (f == PCB_FP_FOPEN_IN_DST)
		return 0;
	len = pcb_test_parse_all(f, fctx.filename, fmt, PCB_IOT_FOOTPRINT, available, accepts, &accept_total, sizeof(available)/sizeof(available[0]), 0, 0);
	if (len < 0) {
		pcb_fp_fclose(f, &fctx);
		return -1;
	}

	Ptr->loader = NULL;

	/* try all plugins that said it could handle the file */
	for(n = 0; n < len; n++) {
		pcb_plug_fp_map_t *map = NULL, head = {0};
		int subfpname_reset = 0;
		if ((available[n].plug->parse_footprint == NULL) || (!accepts[n])) /* can't parse or doesn't want to parse this file */
			continue;

		if ((fctx.subfpname == NULL) && (available[n].plug->multi_footprint)) {
			subfpname_reset = 1;
			rewind(f);
			map = available[n].plug->map_footprint(available[n].plug, f, fctx.filename, &head, 0);
			rewind(f);
			if (map == NULL)
				goto skip;
			fctx.subfpname = pcb_fp_map_choose(&PCB->hidlib, map);
			if (fctx.subfpname == NULL) /* cancel */
				goto skip;
		}

		res = available[n].plug->parse_footprint(available[n].plug, Ptr, fctx.filename, fctx.subfpname);
		if (res == 0) {
			if (Ptr->loader == NULL) /* if the loader didn't set this (to some more fine grained, e.g. depending on file format version) */
				Ptr->loader = available[n].plug;
			break;
		}

		skip:;
		if (subfpname_reset) {
			fctx.subfpname = NULL;
			if (map != NULL)
				pcb_io_fp_map_free(map);
		}
	}

	/* remove selected/found flag when loading reusable footprints */
	if (res == 0)
		pcb_data_flag_change(Ptr, PCB_OBJ_CLASS_REAL, PCB_CHGFLG_CLEAR, PCB_FLAG_FOUND | PCB_FLAG_SELECTED);

	pcb_plug_io_err(&PCB->hidlib, res, "load footprint", Filename);
	pcb_fp_fclose(f, &fctx);
	return res;
}

int pcb_parse_font(pcb_font_t *Ptr, const char *Filename)
{
	int res = -1;
	RND_HOOK_CALL(pcb_plug_io_t, pcb_plug_io_chain, parse_font, res, == 0, (self, Ptr, Filename));

	pcb_plug_io_err(&PCB->hidlib, res, "load font", Filename);
	return res;
}

static int find_prio_cmp(const void *p1, const void *p2)
{
	const pcb_find_io_t *f1 = p1, *f2 = p2;
	if (f1->prio < f2->prio)
		return 1;
	return -1;
}

int pcb_find_io(pcb_find_io_t *available, int avail_len, pcb_plug_iot_t typ, int is_wr, const char *fmt)
{
	int len = 0;

#define cb_append(pl, pr) \
	do { \
		if (pr > 0) { \
			assert(len < avail_len ); \
			available[len].plug = pl; \
			if (fmt == NULL) \
				available[len].prio = pl->save_preference_prio; \
			else \
				available[len].prio = pr; \
			len++; \
		} \
	} while(0)

	RND_HOOK_CALL_ALL(pcb_plug_io_t, pcb_plug_io_chain, fmt_support_prio, cb_append, (self, typ, is_wr, (fmt == NULL ? self->default_fmt : fmt)));

	if (len > 0)
		qsort(available, len, sizeof(available[0]), find_prio_cmp);
#undef cb_append

	return len;
}

pcb_plug_io_t *pcb_io_find_writer(pcb_plug_iot_t typ, const char *fmt)
{
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int len;

	if (fmt == NULL) {
		if (PCB->hidlib.filename != NULL) { /* have a file name, guess from extension */
			int fn_len = strlen(PCB->hidlib.filename);
			const char *end = PCB->hidlib.filename + fn_len;
			pcb_plug_io_t *n, *best = NULL;
			int best_score = 0;

			/* the the plugin that has a matching extension and has the highest save_preference_prio value */
			for(n = pcb_plug_io_chain; n != NULL; n = n->next) {
				if (n->default_extension != NULL) {
					int elen = strlen(n->default_extension);
					if ((elen < fn_len) && (strcmp(end-elen, n->default_extension) == 0)) {
						if (n->save_preference_prio > best_score) {
							best_score = n->save_preference_prio;
							best = n;
						}
					}
				}
			}
			if (best != NULL)
				return best;
		}

		/* no file name or format hint, or file name not recognized: choose the ultimate default */
		fmt = conf_core.rc.save_final_fallback_fmt;
		if (fmt == NULL) {
			rnd_message(RND_MSG_WARNING, "Saving a file with unknown format: failed to guess format from file name, no value configured in rc/save_final_fallback_fmt - CAN NOT SAVE FILE, try save as.\n");
			return NULL;
		}
		else {
			if (PCB->hidlib.filename != NULL)
				rnd_message(RND_MSG_WARNING, "Saving a file with unknown format: failed to guess format from file name, falling back to %s as configured in rc/save_final_fallback_fmt\n", fmt);
		}
	}

	len = pcb_find_io(available, sizeof(available)/sizeof(available[0]), typ, 1, fmt);

	if (len == 0)
		return NULL;

	return available[0].plug;
}

static int pcb_write_data_subcs(pcb_plug_io_t *p, FILE *f, pcb_data_t *data, long subc_idx)
{
	long avail = pcb_subclist_length(&data->subc);
	void *udata;
	int res;

	if ((subc_idx >= 0) && (subc_idx >= avail)) {
			rnd_message(RND_MSG_ERROR, "pcb_write_buffer: subc index out of range");
			return -1;
	}
	if (subc_idx < 0) {
		pcb_subc_t *subc;
		gdl_iterator_t sit;

		if (p->write_subcs_head(p, &udata, f, (avail > 1), avail) != 0) {
			rnd_message(RND_MSG_ERROR, "pcb_write_buffer: failed to write head");
			return -1;
		}
		res = 0;
		subclist_foreach(&data->subc, &sit, subc)
			res |= p->write_subcs_subc(p, &udata, f, subc);
		res |= p->write_subcs_tail(p, &udata, f);
		return res;
	}
	else {
		if (p->write_subcs_head(p, &udata, f, 0, 1) != 0) {
			rnd_message(RND_MSG_ERROR, "pcb_write_buffer: failed to write head");
			return -1;
		}
		res = p->write_subcs_subc(p, &udata, f, pcb_subclist_nth(&data->subc, subc_idx));
		res |= p->write_subcs_tail(p, &udata, f);
		return res;
	}
}

static int pcb_write_buffer(FILE *f, pcb_buffer_t *buff, const char *fmt, rnd_bool subc_only, long subc_idx)
{
	int res/*, newfmt = 0*/;
	pcb_plug_io_t *p = pcb_io_find_writer(subc_only ? PCB_IOT_FOOTPRINT : PCB_IOT_BUFFER, fmt);

	if (p != NULL) {
		if (subc_only)
			res = pcb_write_data_subcs(p, f, buff->Data, subc_idx);
		else
			res = p->write_buffer(p, f, buff);
		/*newfmt = 1;*/
	}

/*	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;*/

	pcb_plug_io_err(&PCB->hidlib, res, "write buffer", NULL);
	return res;
}

int pcb_write_padstack(FILE *f, pcb_pstk_proto_t *proto, const char *fmt)
{
	int res = -1/*, newfmt = 0*/;
	pcb_plug_io_t *p = pcb_io_find_writer(PCB_IOT_PADSTACK, fmt);

	if ((p != NULL) && (p->write_padstack != NULL)) {
		res = p->write_padstack(p, f, proto);
		/*newfmt = 1;*/
	}

/*	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;*/

	pcb_plug_io_err(&PCB->hidlib, res, "write padstack", NULL);
	return res;
}

int pcb_load_padstack(rnd_hidlib_t *hidlib, pcb_pstk_proto_t *proto, const char *fn, const char *fmt)
{
	int res = -1, len, n;
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int accepts[PCB_IO_MAX_FORMATS]; /* test-parse output */
	int accept_total = 0;
	FILE *ft;

	ft = rnd_fopen(hidlib, fn, "r");
	len = pcb_test_parse_all(ft, fn, fmt, PCB_IOT_PADSTACK, available, accepts, &accept_total, sizeof(available)/sizeof(available[0]), 0, 0);
	if (ft != NULL)
		fclose(ft);
	if (len < 0)
		return -1;

	/* try all plugins that said it could handle the file */
	for(n = 0; n < len; n++) {
		if ((available[n].plug->parse_padstack == NULL) || (!accepts[n])) /* can't parse or doesn't want to parse this file */
			continue;
		res = available[n].plug->parse_padstack(available[n].plug, proto, fn);
		if (res == 0)
			break;
	}

	pcb_plug_io_err(hidlib, res, "load padstack", fn);
	return res;
}


int pcb_write_footprint_data(FILE *f, pcb_data_t *e, const char *fmt, long subc_idx)
{
	int res, newfmt = 0;
	pcb_plug_io_t *p = e->loader;

	if ((p == NULL) || ((fmt != NULL) && (*fmt != '\0'))) {
		p = pcb_io_find_writer(PCB_IOT_FOOTPRINT, fmt);
		newfmt = 1;
	}

	if ((p != NULL) && (p->write_subcs_subc != NULL))
		res = pcb_write_data_subcs(p, f, e, subc_idx);

	if ((res == 0) && (newfmt))
		e->loader = p;

	pcb_plug_io_err(&PCB->hidlib, res, "write element", NULL);
	return res;
}

int pcb_write_font(pcb_font_t *Ptr, const char *Filename, const char *fmt)
{
	int res/*, newfmt = 0*/;
	pcb_plug_io_t *p = pcb_io_find_writer(PCB_IOT_FONT, fmt);

	if (p != NULL) {
		res = p->write_font(p, Ptr, Filename);
		/*newfmt = 1;*/
	}
	else
		res = -1;

/*	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;*/

	pcb_plug_io_err(&PCB->hidlib, res, "write font", NULL);
	return res;
}


static int pcb_write_pcb(FILE *f, const char *old_filename, const char *new_filename, const char *fmt, rnd_bool emergency)
{
	int res, newfmt = 0;
	pcb_plug_io_t *p = PCB->Data->loader;

	if ((p == NULL) || ((fmt != NULL) && (*fmt != '\0'))) {
		p = pcb_io_find_writer(PCB_IOT_PCB, fmt);
		newfmt = 1;
	}

	if (p != NULL) {
		if (p->write_pcb != NULL) {
			rnd_event(&PCB->hidlib, RND_EVENT_SAVE_PRE, "s", fmt);
			res = p->write_pcb(p, f, old_filename, new_filename, emergency);
			rnd_event(&PCB->hidlib, RND_EVENT_SAVE_POST, "si", fmt, res);
		}
		else {
			rnd_message(RND_MSG_ERROR, "Can't write PCB: internal error: io plugin %s doesn't implement write_pcb()\n", p->description);
			res = -1;
		}
	}

	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;

	if (res == 0)
		pcb_set_design_dir(new_filename);

	pcb_plug_io_err(&PCB->hidlib, res, "write pcb", NULL);
	return res;
}

/* load PCB: parse the file with enabled 'PCB mode' (see parser); if
   successful, update some other stuff. If revert is rnd_true, we pass
   "revert" as a parameter to the pcb changed event. */
static int real_load_pcb(const char *Filename, const char *fmt, rnd_bool revert, rnd_bool require_font, int how)
{
	const char *unit_suffix;
	char *new_filename;
	pcb_board_t *newPCB = pcb_board_new_(rnd_false);
	pcb_board_t *oldPCB;
	rnd_conf_role_t settings_dest;
	int pres;
#ifdef DEBUG
	double elapsed;
	clock_t start, end;

	start = clock();
#endif

	rnd_path_resolve(&PCB->hidlib, Filename, &new_filename, 0, rnd_false);

	oldPCB = PCB;
	PCB = newPCB;

	/* mark the default font invalid to know if the file has one */
	newPCB->fontkit.valid = rnd_false;

	switch(how & 0x0F) {
		case 0: settings_dest = RND_CFR_DESIGN; break;
		case 1: settings_dest = RND_CFR_DEFAULTPCB; break;
		case 2: settings_dest = RND_CFR_invalid; break;
		default: abort();
	}

	pcb_crosshair_move_absolute(PCB, RND_COORD_MAX, RND_COORD_MAX); /* make sure the crosshair is not above any object so ch* plugins release their highlights */
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true); /* pcb_crosshair_move_absolute()'s side effect is leaving the crosshair notify in false state, flush that */

	/* new data isn't added to the undo list */
	rnd_hid_menu_merge_inhibit_inc();
	pres = pcb_parse_pcb(PCB, new_filename, fmt, settings_dest, how & 0x10);
	rnd_hid_menu_merge_inhibit_dec();

	if (!pres) {
		pcb_board_remove(oldPCB);

		pcb_board_new_postproc(PCB, 0);
		if (how == 0) {
			/* update cursor location */
			PCB->hidlib.ch_x = pcb_crosshair.X = PCB->hidlib.size_x/2;
			PCB->hidlib.ch_y = pcb_crosshair.Y = PCB->hidlib.size_y/2;

			/* update cursor confinement and output area (scrollbars) */
			pcb_board_resize(PCB->hidlib.size_x, PCB->hidlib.size_y, 0);
		}

		/* have to be called after pcb_board_resize() so vis update is after a board changed update */
		rnd_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
		pcb_layervis_reset_stack(&PCB->hidlib);

		/* enable default font if necessary */
		if (!PCB->fontkit.valid) {
			if ((require_font) && (!PCB->is_footprint) && (!PCB->suppress_warn_missing_font))
				rnd_message(RND_MSG_WARNING, "File '%s' has no font information, using default font\n", new_filename);
			PCB->fontkit.valid = rnd_true;
		}

		/* footprint edition: let the user directly manipulate subc parts */
		PCB->loose_subc = PCB->is_footprint;

		/* clear 'changed flag' */
		pcb_board_set_changed_flag(PCB, rnd_false);
		PCB->hidlib.filename = new_filename;
		/* just in case a bad file saved file is loaded */

		/* geda/pcb compatibility: use attribute PCB::grid::unit as unit, if present */
		unit_suffix = pcb_attrib_get(PCB, "PCB::grid::unit");
		if (unit_suffix && *unit_suffix) {
			lht_node_t *nat = rnd_conf_lht_get_at(RND_CFR_DESIGN, "editor/grid_unit", 0);
			if (nat == NULL) {
				const rnd_unit_t *new_unit = rnd_get_unit_struct(unit_suffix);
				if (new_unit)
					rnd_conf_set(settings_dest, "editor/grid_unit", -1, unit_suffix, RND_POL_OVERWRITE);
			}
		}

		pcb_ratspatch_make_edited(PCB);

/* set route style to the first one, if the current one doesn't
   happen to match any.  This way, "revert" won't change the route style. */
	{
		extern fgw_error_t pcb_act_GetStyle(fgw_arg_t *res, int argc, fgw_arg_t *argv);
		fgw_arg_t res, argv;
		if (RND_ACT_CALL_C(&PCB->hidlib, pcb_act_GetStyle, &res, 1, &argv) < 0)
			pcb_use_route_style_idx(&PCB->RouteStyle, 0);
	}

		if ((how == 0) || (revert))
			pcb_board_changed(revert);

#ifdef DEBUG
		end = clock();
		elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
		rnd_message(RND_MSG_INFO, "Loading file %s took %f seconds of CPU time\n", new_filename, elapsed);
#endif

		conf_core.temp.rat_warn = rnd_true; /* make sure the first click can remove warnings */
		rnd_tool_select_by_name(&PCB->hidlib, "arrow");

		return 0;
	}

	free(new_filename);
	PCB = oldPCB;
	if (PCB == NULL) {
		/* bozo: we are trying to revert back to a non-existing pcb... create one to avoid a segfault */
		PCB = pcb_board_new_(rnd_false);
		if (PCB == NULL) {
			rnd_message(RND_MSG_ERROR, "FATAL: can't create a new empty pcb!");
			exit(1);
		}
	}

	if (!(how & PCB_INHIBIT_BOARD_CHANGED))
		pcb_board_changed(0);

	/* release unused memory */
	pcb_board_remove(newPCB);

	return 1;
}

/* Write the pcb file, a footprint or a buffer */
static int pcb_write_file(FILE *fp, rnd_bool thePcb, const char *old_path, const char *new_path, const char *fmt, rnd_bool emergency, rnd_bool subc_only, long subc_idx)
{
	if (thePcb) {
		if (PCB->is_footprint)
			return pcb_write_footprint_data(fp, PCB->Data, fmt, subc_idx);
		return pcb_write_pcb(fp, old_path, new_path, fmt, emergency);
	}
	return pcb_write_buffer(fp, PCB_PASTEBUFFER, fmt, subc_only, subc_idx);
}

/* writes to pipe using the command defined by conf_core.rc.save_command
   %f are replaced by the passed filename */
static int pcb_write_pipe(const char *Filename, rnd_bool thePcb, const char *fmt, rnd_bool subc_only, long subc_idx, int askovr)
{
	FILE *fp;
	int result;
	const char *p;
	static gds_t command;
	const char *save_cmd;

	if (RND_EMPTY_STRING_P(conf_core.rc.save_command))
		return pcb_write_pcb_file(Filename, thePcb, fmt, rnd_false, subc_only, subc_idx, askovr);

	save_cmd = conf_core.rc.save_command;
	/* setup commandline */
	gds_truncate(&command,0);
	for (p = save_cmd; *p; p++) {
		/* copy character if not special or add string to command */
		if (!(p[0] == '%' && p[1] == 'f'))
			gds_append(&command, *p);
		else {
			gds_append_str(&command, Filename);

			/* skip the character */
			p++;
		}
	}
	printf("write to pipe \"%s\"\n", command.array);
	if ((fp = rnd_popen(&PCB->hidlib, command.array, "w")) == NULL) {
		rnd_popen_error_message(command.array);
		return (-1);
	}

	result = pcb_write_file(fp, thePcb, NULL, NULL, fmt, rnd_false, subc_only, subc_idx);

	return (rnd_pclose(fp) ? (-1) : result);
}

#if !defined(RND_HAS_ATEXIT)
static char *TMPFilename = NULL;
#endif

#define F2S(OBJ, TYPE) pcb_strflg_f2s((OBJ)->Flags, TYPE)

int pcb_save_buffer_subcs(const char *Filename, const char *fmt, long subc_idx)
{
	int result;

	if (conf_core.editor.show_solder_side)
		pcb_buffers_flip_side(PCB);
	result = pcb_write_pipe(Filename, rnd_false, fmt, rnd_true, subc_idx, 1);
	if (conf_core.editor.show_solder_side)
		pcb_buffers_flip_side(PCB);
	return result;
}

int pcb_save_buffer(const char *Filename, const char *fmt)
{
	return pcb_write_pipe(Filename, rnd_false, fmt, rnd_false, -1, 1);
}

int pcb_save_pcb(const char *file, const char *fmt)
{
	int retcode;

	pcb_io_incompat_lst_enable = rnd_true;
	if (conf_core.editor.io_incomp_popup)
		pcb_view_list_free_fields(&pcb_io_incompat_lst);

	retcode = pcb_write_pipe(file, rnd_true, fmt, rnd_false, -1, 0);

	pcb_io_incompat_lst_enable = rnd_false;
	if (conf_core.editor.io_incomp_popup) {
		long int len = pcb_view_list_length(&pcb_io_incompat_lst);
		if (len > 0) {
			rnd_message(RND_MSG_ERROR, "There were %ld save incompatibility errors.\nData in memory is not affected, but the file created may be slightly broken.\nSee the popup view listing for details.\n", len);
			rnd_actionva(&PCB->hidlib, "IOincompatList", conf_core.editor.io_incomp_style, "auto", NULL);
		}
	}

	return retcode;
}

int pcb_load_pcb(const char *file, const char *fmt, rnd_bool require_font, int how)
{
	int res = real_load_pcb(file, fmt, rnd_false, require_font, how);
	if (res == 0) {
		rnd_file_loaded_set_at("design", "main", file, PCB->is_footprint ? "footprint" : "board");
		if (PCB->is_footprint) {
			rnd_box_t b;
			/* a footprint has no board size set, need to invent one */
			pcb_data_bbox(&b, PCB->Data, 0);
			if ((b.X2 < b.X1) || (b.Y2 < b.Y1)) {
				rnd_message(RND_MSG_ERROR, "Invalid footprint file: can not determine bounding box\n");
				res = -1;
			}
			else
				pcb_board_resize(b.X2*1.5, b.Y2*1.5, 0);
		}
	}
	return res;
}

int pcb_revert_pcb(void)
{
	return real_load_pcb(PCB->hidlib.filename, NULL, rnd_true, rnd_true, 0);
}

int pcb_load_buffer(rnd_hidlib_t *hidlib, pcb_buffer_t *buff, const char *fn, const char *fmt)
{
	int res = -1, len, n;
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int accepts[PCB_IO_MAX_FORMATS]; /* test-parse output */
	int accept_total = 0;
	FILE *ft;

	ft = rnd_fopen(hidlib, fn, "r");
	len = pcb_test_parse_all(ft, fn, fmt, PCB_IOT_BUFFER, available, accepts, &accept_total, sizeof(available)/sizeof(available[0]), 0, 0);
	if (ft != NULL)
		fclose(ft);
	if (len < 0)
		return -1;

	/* try all plugins that said it could handle the file */
	for(n = 0; n < len; n++) {
		if ((available[n].plug->parse_buffer == NULL) || (!accepts[n])) /* can't parse or doesn't want to parse this file */
			continue;
		res = available[n].plug->parse_buffer(available[n].plug, buff, fn);
		if (res == 0)
			break;
	}

	pcb_plug_io_err(hidlib, res, "load buffer", fn);
	return res;

}


void pcb_print_quoted_string_(FILE * FP, const char *S)
{
	const char *start;

	for(start = S; *S != '\0'; S++) {
		if (*S == '"' || *S == '\\') {
			if (start != S)
				fwrite(start, S-start, 1, FP);
			fputc('\\', FP);
			fputc(*S, FP);
			start = S+1;
		}
	}

	if (start != S)
		fwrite(start, S-start, 1, FP);
}

void pcb_print_quoted_string(FILE *FP, const char *S)
{
	fputc('"', FP);
	pcb_print_quoted_string_(FP, S);
	fputc('"', FP);
}

/* this is used for fatal errors and does not call the program specified in
   'saveCommand' for safety reasons */
void pcb_save_in_tmp(void)
{
	char filename[256];

	/* memory might have been released before this function is called */
	if (PCB && PCB->Changed && (conf_core.rc.emergency_name != NULL) && (*conf_core.rc.emergency_name != '\0')) {
		const char *fmt = conf_core.rc.emergency_format == NULL ? DEFAULT_EMERGENCY_FMT : conf_core.rc.emergency_format;
		sprintf(filename, conf_core.rc.emergency_name, (long int)rnd_getpid());
		rnd_message(RND_MSG_INFO, "Trying to save your layout in '%s'\n", filename);
		pcb_write_pcb_file(filename, rnd_true, fmt, rnd_true, rnd_false, -1, 0);
	}
}

/* front-end for pcb_save_in_tmp() to makes sure it is only called once */
static rnd_bool dont_save_any_more = rnd_false;
void pcb_emergency_save(void)
{

	if (!dont_save_any_more) {
		dont_save_any_more = rnd_true;
		pcb_save_in_tmp();
	}
}

void pcb_disable_emergency_save(void)
{
	dont_save_any_more = rnd_true;
}

/*** Autosave ***/

static rnd_hidval_t backup_timer;

/*
 * If the backup interval is > 0 then set another timer.  Otherwise
 * we do nothing and it is up to the GUI to call pcb_enable_autosave()
 * after setting conf_core.rc.backup_interval > 0 again.
 */
static void backup_cb(rnd_hidval_t data)
{
	backup_timer.ptr = NULL;
	pcb_backup();
	if (conf_core.rc.backup_interval > 0 && rnd_gui->add_timer)
		backup_timer = rnd_gui->add_timer(rnd_gui, backup_cb, 1000 * conf_core.rc.backup_interval, data);
}

void pcb_enable_autosave(void)
{
	rnd_hidval_t x;

	x.ptr = NULL;

	/* If we already have a timer going, then cancel it out */
	if (backup_timer.ptr != NULL && rnd_gui->stop_timer)
		rnd_gui->stop_timer(rnd_gui, backup_timer);

	backup_timer.ptr = NULL;
	/* Start up a new timer */
	if (conf_core.rc.backup_interval > 0 && rnd_gui->add_timer)
		backup_timer = rnd_gui->add_timer(rnd_gui, backup_cb, 1000 * conf_core.rc.backup_interval, x);
}

/* Saves the board in a backup file using the name configured in
   conf_core.rc.backup_name */
void pcb_backup(void)
{
	char *filename = NULL;
	const char *fmt = NULL;
	pcb_plug_io_t *orig;

	filename = rnd_build_fn(&PCB->hidlib, conf_core.rc.backup_name);
	if (filename == NULL) {
		fprintf(stderr, "pcb_backup(): can't build file name for a backup\n");
		exit(1);
	}

	if ((conf_core.rc.backup_format != NULL) && (strcmp(conf_core.rc.backup_format, "original") != 0))
		fmt = conf_core.rc.backup_format;

	orig = PCB->Data->loader;
	pcb_write_pcb_file(filename, rnd_true, fmt, rnd_true, rnd_false, -1, 0);
	PCB->Data->loader = orig;

	free(filename);
}

int pcb_write_pcb_file(const char *Filename, rnd_bool thePcb, const char *fmt, rnd_bool emergency, rnd_bool subc_only, long subc_idx, int askovr)
{
	FILE *fp;
	int result, overwrite = 0;
	char *fn_tmp = NULL;

	/* for askovr, do not make a backup copy - if the user explicitly says overwrite, just overwrite */
	if (!askovr)
		overwrite = rnd_file_readable(Filename);

	if (overwrite) {
		int len = strlen(Filename);
		fn_tmp = malloc(len+8);
		memcpy(fn_tmp, Filename, len);
		strcpy(fn_tmp+len, ".old");
		if (rnd_rename(NULL, Filename, fn_tmp) != 0) {
			if (emergency) {
				/* Try an alternative emergency file */
				strcpy(fn_tmp+len, ".emr");
				Filename = fn_tmp;
			}
			else {
				rnd_message(RND_MSG_ERROR, "Can't rename %s to %s before save\n", Filename, fn_tmp);
				return -1;
			}
		}
	}

	if (askovr)
		fp = rnd_fopen_askovr(&PCB->hidlib, Filename, "w", NULL);
	else
		fp = rnd_fopen(&PCB->hidlib, Filename, "w");

	if (fp == NULL) {
		rnd_open_error_message(Filename);
		return (-1);
	}

	result = pcb_write_file(fp, thePcb, fn_tmp, Filename, fmt, emergency, subc_only, subc_idx);

	fclose(fp);
	if (fn_tmp != NULL) {
		if ((result == 0) && (!conf_core.rc.keep_save_backups))
			rnd_unlink(&PCB->hidlib, fn_tmp);
		free(fn_tmp);
	}
	return result;
}

int pcb_io_list(pcb_io_formats_t *out, pcb_plug_iot_t typ, int wr, int do_digest, pcb_io_list_ext_t ext)
{
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int n;

	memset(out, 0, sizeof(pcb_io_formats_t));
	out->len = pcb_find_io(available, sizeof(available)/sizeof(available[0]), typ, 1, NULL);

	if (out->len == 0)
		return 0;

	for(n = 0; n < out->len; n++) {
		out->plug[n] = available[n].plug;
		switch(ext) {
			case PCB_IOL_EXT_NONE:  out->extension[n] = NULL; break;
			case PCB_IOL_EXT_BOARD: out->extension[n] = out->plug[n]->default_extension; break;
			case PCB_IOL_EXT_FP:    out->extension[n] = out->plug[n]->fp_extension; break;
		}
	}

	if (do_digest) {
		for(n = 0; n < out->len; n++)
			out->digest[n] = rnd_strdup_printf("%s (%s)", out->plug[n]->default_fmt, out->plug[n]->description);
		out->digest[n] = NULL;
	}

	return out->len;
}

void pcb_io_list_free(pcb_io_formats_t *out)
{
	int n;

	for(n = 0; n < out->len; n++) {
		if (out->digest[n] != NULL) {
			free(out->digest[n]);
			out->digest[n] = NULL;
		}
	}
}


rnd_cardinal_t pcb_io_incompat_save(pcb_data_t *data, pcb_any_obj_t *obj, const char *type, const char *title, const char *description)
{

	if ((pcb_io_incompat_lst_enable) && (conf_core.editor.io_incomp_popup)) {
		pcb_view_t *violation = pcb_view_new(&PCB->hidlib, type, title, description);
		if ((obj != NULL) && (obj->type & PCB_OBJ_CLASS_REAL)) {
			pcb_view_append_obj(violation, 0, obj);
			pcb_view_set_bbox_by_objs(PCB->Data, violation);
		}
		pcb_view_list_append(&pcb_io_incompat_lst, violation);
	}
	else {
		rnd_message(RND_MSG_ERROR, "save error: %s\n", title);
		if (obj != NULL) {
			rnd_coord_t x = (obj->BoundingBox.X1 + obj->BoundingBox.X2)/2;
			rnd_coord_t y = (obj->BoundingBox.Y1 + obj->BoundingBox.Y2)/2;
			rnd_message(RND_MSG_ERROR, " near %$mm %$mm\n", x, y);
		}
		if (description != NULL)
			rnd_message(RND_MSG_ERROR, " (%s)\n", description);
	}
	return 0;
}

pcb_plug_fp_map_t *pcb_io_map_footprint_file(rnd_hidlib_t *hl, const char *fn, pcb_plug_fp_map_t *head, int need_tags)
{
	FILE *f = rnd_fopen(hl, fn, "r");
	pcb_plug_fp_map_t *res = NULL;
	pcb_plug_io_t *plug;

	if (f == NULL) {
		head->type = PCB_FP_INVALID;
		return head;
	}

	for(plug = pcb_plug_io_chain; plug != NULL; plug = plug->next) {
		if (plug->map_footprint == NULL) continue;

		rewind(f);
		head->type = PCB_FP_INVALID;
		head->libtype = PCB_LIB_FOOTPRINT;
		res = plug->map_footprint(NULL, f, fn, head, need_tags);
		if (res == NULL) continue;
		if (res->type != PCB_FP_INVALID)
			break; /* success */
		vts0_uninit(&res->tags);
	}


	fclose(f);
	if (res == NULL) {
		res = head;
		head->type = PCB_FP_INVALID;
	}
	return res;
}

void pcb_io_fp_map_append(pcb_plug_fp_map_t **tail, pcb_plug_fp_map_t *head, const char *filename, const char *fpname)
{
	pcb_plug_fp_map_t *m;

	switch(head->type) {
		case PCB_FP_INVALID: /* first append */
			(*tail)->type = PCB_FP_FILE;
			(*tail)->name = rnd_strdup(fpname);
			break;
		case PCB_FP_FILE: /* second append */
			/* clone the existing head */
			m = calloc(sizeof(pcb_plug_fp_map_t), 1);
			m->type = PCB_FP_FILE;
			m->libtype = PCB_LIB_FOOTPRINT;
			m->name = head->name;

			head->type = PCB_FP_DIR;
			head->libtype = PCB_LIB_DIR;
			head->name = rnd_strdup(filename);
			head->next = m;

			*tail = m;
			/* fall through adding the second */
		case PCB_FP_DIR: /* third append */
			m = calloc(sizeof(pcb_plug_fp_map_t), 1);
			m->type = PCB_FP_FILE;
			m->libtype = PCB_LIB_FOOTPRINT;
			m->name = rnd_strdup(fpname);
			
			(*tail)->next = m;
			*tail = m;
			break;
		case PCB_FP_FILEDIR:
		case PCB_FP_PARAMETRIC:
			assert(!"broken format plugin: shouldn't do PCB_FP_FILDIR or PCB_FP_PARAMETRIC");
			break;
	}
}

void pcb_io_fp_map_free_fields(pcb_plug_fp_map_t *m)
{
/*
	Do NOTE free tag values, they are allocated by the central fp hash
	long n;
	for(n = 0; n < m->tags.used; n++)
		free(m->tags.array[n]);
*/
	vts0_uninit(&m->tags);
	free(m->name);
	m->name = NULL;
	m->type = 0;
	m->libtype = 0;
}

void pcb_io_fp_map_free(pcb_plug_fp_map_t *head)
{
	pcb_plug_fp_map_t *m, *next;

	pcb_io_fp_map_free_fields(head);

	for(m = head->next; m != NULL; m = next) {
		next = m->next;
		pcb_io_fp_map_free_fields(m);
		free(m);
	}
}


void pcb_io_uninit(void)
{
	pcb_view_list_free_fields(&pcb_io_incompat_lst);
	if (pcb_plug_io_chain != NULL) {
		rnd_message(RND_MSG_ERROR, "pcb_plug_io_chain is not empty; a plugin did not remove itself from the chain. Fix your plugins!\n");
		rnd_message(RND_MSG_ERROR, "head: desc='%s'\n", pcb_plug_io_chain->description);
	}
	free(last_design_dir);
	last_design_dir = NULL;
}

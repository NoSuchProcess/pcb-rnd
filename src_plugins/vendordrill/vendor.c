/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2004, 2007 Dan McMahill
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <genregex/regex_sei.h>

#include "change.h"
#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "undo.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_menu.h>
#include "vendor_conf.h"
#include <librnd/core/compat_misc.h>
#include "obj_pstk_inlines.h"
#include "event.h"
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include "menu_internal.c"

conf_vendor_t conf_vendor;

static void add_to_drills(char *);
static void apply_vendor_map(void);
static void process_skips(lht_node_t *);
static rnd_bool rematch(const char *, const char *);
static void vendor_free_all(void);
rnd_coord_t vendorDrillMap(rnd_coord_t in);


/* list of vendor drills and a count of them */
static rnd_coord_t *vendor_drills = NULL;
static int n_vendor_drills = 0;

static int cached_drill = -1;
static int cached_map = -1;

/* lists of elements to ignore */
static char **ignore_refdes = NULL;
static int n_refdes = 0;
static char **ignore_value = NULL;
static int n_value = 0;
static char **ignore_descr = NULL;
static int n_descr = 0;

/* vendor name */
static char *vendor_name = NULL;

/* resource file to PCB units scale factor */
static double sf;


/* type of drill mapping */
#define CLOSEST 1
#define ROUND_UP 0
static int rounding_method = ROUND_UP;

#define FREE(x) if((x) != NULL) { free (x) ; (x) = NULL; }

/* load a board metadata into conf_core */
static void load_meta_coord(const char *path, rnd_coord_t crd)
{
	char tmp[128];
	rnd_sprintf(tmp, "%$mm", crd);
	rnd_conf_set(RND_CFR_DESIGN, path, -1, tmp, RND_POL_OVERWRITE);
}

static rnd_bool vendorIsSubcMappable(pcb_subc_t *subc);


static const char apply_vendor_syntax[] = "ApplyVendor()";
static const char apply_vendor_help[] = "Applies the currently loaded vendor drill table to the current design.";
/* DOC: applyvendor.html */
fgw_error_t pcb_act_ApplyVendor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_busy(PCB, 1);
	apply_vendor_map();
	rnd_hid_busy(PCB, 0);
	RND_ACT_IRES(0);
	return 0;
}

static const char unload_vendor_syntax[] = "UnloadVendor()";
static const char unload_vendor_help[] = "Unloads the current vendor drill mapping table.";
fgw_error_t pcb_act_UnloadVendor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	cached_drill = -1;

	vendor_free_all();
	RND_ACT_IRES(0);
	return 0;
}


static const char pcb_acts_LoadVendorFrom[] = "LoadVendorFrom(filename)";
static const char pcb_acth_LoadVendorFrom[] = "Loads the specified vendor lihata file.";
/* DOC: loadvendorfrom.html */
fgw_error_t pcb_act_LoadVendorFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;
	const char *sval;
	lht_doc_t *doc;
	lht_node_t *drlres;
	rnd_bool free_fname = rnd_false;

	cached_drill = -1;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadVendorFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui, "Load Vendor Resource File...",
														"Picks a vendor resource file to load.\n"
															"This file can contain drc settings for a\n"
															"particular vendor as well as a list of\n"
															"predefined drills which are allowed.", default_file, ".res", NULL, "vendor", RND_HID_FSD_READ, NULL);
		if (fname == NULL) {
			RND_ACT_IRES(1);
			return 0;
		}

		free_fname = rnd_true;

		free(default_file);
		default_file = NULL;

		if (fname && *fname)
			default_file = rnd_strdup(fname);
	}

	vendor_free_all();

	/* load the resource file */
	doc = rnd_hid_cfg_load_lht(&PCB->hidlib, fname);
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "Could not load vendor resource file \"%s\"\n", fname);
		RND_ACT_IRES(1);
		return 0;
	}

	/* figure out the vendor name, if specified */
	vendor_name = (char *) RND_UNKNOWN(rnd_hid_cfg_text_value(doc, "vendor"));

	/* figure out the units, if specified */
	sval = rnd_hid_cfg_text_value(doc, "/units");
	if (sval == NULL) {
		sf = RND_MIL_TO_COORD(1);
	}
	else if ((RND_NSTRCMP(sval, "mil") == 0) || (RND_NSTRCMP(sval, "mils") == 0)) {
		sf = RND_MIL_TO_COORD(1);
	}
	else if ((RND_NSTRCMP(sval, "inch") == 0) || (RND_NSTRCMP(sval, "inches") == 0)) {
		sf = RND_INCH_TO_COORD(1);
	}
	else if (RND_NSTRCMP(sval, "mm") == 0) {
		sf = RND_MM_TO_COORD(1);
	}
	else {
		rnd_message(RND_MSG_ERROR, "\"%s\" is not a supported units.  Defaulting to inch\n", sval);
		sf = RND_INCH_TO_COORD(1);
	}

	/* default to ROUND_UP */
	rounding_method = ROUND_UP;
	sval = rnd_hid_cfg_text_value(doc, "/round");
	if (sval != NULL) {
		if (RND_NSTRCMP(sval, "up") == 0) {
			rounding_method = ROUND_UP;
		}
		else if (RND_NSTRCMP(sval, "nearest") == 0) {
			rounding_method = CLOSEST;
		}
		else {
			rnd_message(RND_MSG_ERROR, "\"%s\" is not a valid rounding type.  Defaulting to up\n", sval);
			rounding_method = ROUND_UP;
		}
	}

	process_skips(lht_tree_path(doc, "/", "/skips", 1, NULL));

	/* extract the drillmap resource */
	drlres = lht_tree_path(doc, "/", "/drillmap", 1, NULL);
	if (drlres != NULL) {
		if (drlres->type == LHT_LIST) {
			lht_node_t *n;
			for(n = drlres->data.list.first; n != NULL; n = n->next) {
				if (n->type != LHT_TEXT)
					rnd_hid_cfg_error(n, "Broken drillmap: /drillmap should contain text children only\n");
				else
					add_to_drills(n->data.text.value);
			}
		}
		else
			rnd_message(RND_MSG_ERROR, "Broken drillmap: /drillmap should be a list\n");
	}
	else
		rnd_message(RND_MSG_ERROR, "No drillmap resource found\n");

	sval = rnd_hid_cfg_text_value(doc, "/drc/copper_space");
	if (sval != NULL) {
		load_meta_coord("design/bloat", floor(sf * atof(sval) + 0.5));
		rnd_message(RND_MSG_INFO, "Set DRC minimum copper spacing to %ml mils\n", conf_core.design.bloat);
	}

	sval = rnd_hid_cfg_text_value(doc, "/drc/copper_overlap");
	if (sval != NULL) {
		load_meta_coord("design/shrink", floor(sf * atof(sval) + 0.5));
		rnd_message(RND_MSG_INFO, "Set DRC minimum copper overlap to %ml mils\n", conf_core.design.shrink);
	}

	sval = rnd_hid_cfg_text_value(doc, "/drc/copper_width");
	if (sval != NULL) {
		load_meta_coord("design/min_wid", floor(sf * atof(sval) + 0.5));
		rnd_message(RND_MSG_INFO, "Set DRC minimum copper spacing to %ml mils\n", conf_core.design.min_wid);
	}

	sval = rnd_hid_cfg_text_value(doc, "/drc/silk_width");
	if (sval != NULL) {
		load_meta_coord("design/min_slk", floor(sf * atof(sval) + 0.5));
		rnd_message(RND_MSG_INFO, "Set DRC minimum silk width to %ml mils\n", conf_core.design.min_slk);
	}

	sval = rnd_hid_cfg_text_value(doc, "/drc/min_drill");
	if (sval != NULL) {
		load_meta_coord("design/min_drill", floor(sf * atof(sval) + 0.5));
		rnd_message(RND_MSG_INFO, "Set DRC minimum drill diameter to %ml mils\n", conf_core.design.min_drill);
	}

	sval = rnd_hid_cfg_text_value(doc, "/drc/min_ring");
	if (sval != NULL) {
		load_meta_coord("design/min_ring", floor(sf * atof(sval) + 0.5));
		rnd_message(RND_MSG_INFO, "Set DRC minimum annular ring to %ml mils\n", conf_core.design.min_ring);
	}

	rnd_message(RND_MSG_INFO, "Loaded %d vendor drills from %s\n", n_vendor_drills, fname);
	rnd_message(RND_MSG_INFO, "Loaded %d RefDes skips, %d Value skips, %d Descr skips\n", n_refdes, n_value, n_descr);

	rnd_conf_set(RND_CFR_DESIGN, "plugins/vendor/enable", -1, "0", RND_POL_OVERWRITE);

	apply_vendor_map();
	if (free_fname)
		free((char*)fname);
	lht_dom_uninit(doc);
	RND_ACT_IRES(0);
	return 0;
}

static int apply_vendor_pstk1(pcb_pstk_t *pstk, rnd_cardinal_t *tot)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(pstk);
	rnd_coord_t target;
	int res = 0;

	if ((proto == NULL) || (proto->hdia == 0)) return 0;
	(*tot)++;
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, pstk)) return 0;

	target = vendorDrillMap(proto->hdia);
	if (proto->hdia != target) {
		if (pcb_chg_obj_2nd_size(PCB_OBJ_PSTK, pstk, pstk, pstk, target, rnd_true, rnd_false))
			res = 1;
		else {
			rnd_message(RND_MSG_WARNING,
				"Padstack at %ml, %ml not changed.  Possible reasons:\n"
				 "\t- pad size too small\n"
				 "\t- new size would be too large or too small\n", pstk->x, pstk->y);
		}
	}
	return res;
}

static rnd_cardinal_t apply_vendor_pstk(pcb_data_t *data, rnd_cardinal_t *tot)
{
	gdl_iterator_t it;
	pcb_pstk_t *pstk;
	rnd_cardinal_t changed = 0;

	padstacklist_foreach(&data->padstack, &it, pstk)
		if (apply_vendor_pstk1(pstk, tot))
			changed++;
	return changed;
}

static void apply_vendor_map(void)
{
	int i;
	rnd_cardinal_t changed = 0, tot = 0;
	rnd_bool state;

	state = conf_vendor.plugins.vendor.enable;

	/* enable mapping */
	rnd_conf_force_set_bool(conf_vendor.plugins.vendor.enable, 1);

	/* If we have loaded vendor drills, then apply them to the design */
	if (n_vendor_drills > 0) {

		/* global padsatcks (e.g. vias) */
		changed += apply_vendor_pstk(PCB->Data, &tot);

		PCB_SUBC_LOOP(PCB->Data);
		{
			if (vendorIsSubcMappable(subc))
				changed += apply_vendor_pstk(subc->data, &tot);
		}
		PCB_END_LOOP;

		rnd_message(RND_MSG_INFO, "Updated %ld drill sizes out of %ld total\n", (long)changed, (long)tot);

		/* Update the current Via */
		if (conf_core.design.via_drilling_hole != vendorDrillMap(conf_core.design.via_drilling_hole)) {
			changed++;
			rnd_conf_setf(RND_CFR_DESIGN, "design/via_drilling_hole", -1, "%$mm", vendorDrillMap(conf_core.design.via_drilling_hole));
			rnd_message(RND_MSG_INFO, "Adjusted active via hole size to be %ml mils\n", conf_core.design.via_drilling_hole);
		}

		/* and update the vias for the various routing styles */
		for (i = 0; i < vtroutestyle_len(&PCB->RouteStyle); i++) {
			if (PCB->RouteStyle.array[i].Hole != vendorDrillMap(PCB->RouteStyle.array[i].Hole)) {
				changed++;
				PCB->RouteStyle.array[i].Hole = vendorDrillMap(PCB->RouteStyle.array[i].Hole);
				rnd_message(RND_MSG_INFO,
								"Adjusted %s routing style hole size to be %ml mils\n",
								PCB->RouteStyle.array[i].name, PCB->RouteStyle.array[i].Hole);
				if (PCB->RouteStyle.array[i].Diameter < PCB->RouteStyle.array[i].Hole + PCB_MIN_PINORVIACOPPER) {
					PCB->RouteStyle.array[i].Diameter = PCB->RouteStyle.array[i].Hole + PCB_MIN_PINORVIACOPPER;
					rnd_message(RND_MSG_INFO,
									"Increased %s routing style via diameter to %ml mils\n",
									PCB->RouteStyle.array[i].name, PCB->RouteStyle.array[i].Diameter);
				}
			}
		}

		/*
		 * if we've changed anything, indicate that we need to save the
		 * file, redraw things, and make sure we can undo.
		 */
		if (changed) {
			pcb_board_set_changed_flag(PCB, rnd_true);
			rnd_hid_redraw(PCB);
			pcb_undo_inc_serial();
		}
	}

	/* restore mapping on/off */
	rnd_conf_force_set_bool(conf_vendor.plugins.vendor.enable, state);
}

/* for a given drill size, find the closest vendor drill size */
rnd_coord_t vendorDrillMap(rnd_coord_t in)
{
	int i, min, max;

	if (in == cached_drill)
		return cached_map;
	cached_drill = in;

	/* skip the mapping if we don't have a vendor drill table */
	if ((n_vendor_drills == 0) || (vendor_drills == NULL)
			|| (!conf_vendor.plugins.vendor.enable)) {
		cached_map = in;
		return in;
	}

	/* are we smaller than the smallest drill? */
	if (in <= vendor_drills[0]) {
		cached_map = vendor_drills[0];
		return vendor_drills[0];
	}

	/* are we larger than the largest drill? */
	if (in > vendor_drills[n_vendor_drills - 1]) {
		rnd_message(RND_MSG_ERROR, "Vendor drill list does not contain a drill >= %ml mil\n"
							"Using %ml mil instead.\n", in, vendor_drills[n_vendor_drills - 1]);
		cached_map = vendor_drills[n_vendor_drills - 1];
		return vendor_drills[n_vendor_drills - 1];
	}

	/* figure out which 2 drills are closest in size */
	min = 0;
	max = n_vendor_drills - 1;
	while (max - min > 1) {
		i = (max + min) / 2;
		if (in > vendor_drills[i])
			min = i;
		else
			max = i;
	}
	i = max;

	/* now round per the rounding mode */
	if (rounding_method == CLOSEST) {
		/* find the closest drill size */
		if ((in - vendor_drills[i - 1]) > (vendor_drills[i] - in)) {
			cached_map = vendor_drills[i];
			return vendor_drills[i];
		}
		else {
			cached_map = vendor_drills[i - 1];
			return vendor_drills[i - 1];
		}
	}
	else {
		/* always round up */
		cached_map = vendor_drills[i];
		return vendor_drills[i];
	}

}

/* add a drill size to the vendor drill list */
static void add_to_drills(char *sval)
{
	double tmpd;
	int val;
	int k, j;

	/* increment the count and make sure we have memory */
	n_vendor_drills++;
	if ((vendor_drills = (int *) realloc(vendor_drills, n_vendor_drills * sizeof(int))) == NULL) {
		fprintf(stderr, "realloc() failed to allocate %ld bytes\n", (unsigned long) n_vendor_drills * sizeof(int));
		return;
	}

	/* string to a value with the units scale factor in place */
	tmpd = atof(sval);
	val = floor(sf * tmpd + 0.5);

	/*
	 * We keep the array of vendor drills sorted to make it easier to
	 * do the rounding later.  The algorithm used here is not so efficient,
	 * but we're not dealing with much in the way of data.
	 */

	/* figure out where to insert the value to keep the array sorted.  */
	k = 0;
	while ((k < n_vendor_drills - 1) && (vendor_drills[k] < val))
		k++;

	if (k == n_vendor_drills - 1) {
		vendor_drills[n_vendor_drills - 1] = val;
	}
	else {
		/* move up the existing drills to make room */
		for (j = n_vendor_drills - 1; j > k; j--) {
			vendor_drills[j] = vendor_drills[j - 1];
		}

		vendor_drills[k] = val;
	}
}

/* deal with the "skip" subresource */
static void process_skips(lht_node_t *res)
{
	char *sval;
	int *cnt;
	char ***lst = NULL;
	lht_node_t *n;

	if (res == NULL)
		return;

	if (res->type != LHT_LIST)
		rnd_hid_cfg_error(res, "skips must be a list.\n");

	for(n = res->data.list.first; n != NULL; n = n->next) {
		if (n->type == LHT_TEXT) {
			if (RND_NSTRCMP(n->name, "refdes") == 0) {
				cnt = &n_refdes;
				lst = &ignore_refdes;
			}
			else if (RND_NSTRCMP(n->name, "value") == 0) {
				cnt = &n_value;
				lst = &ignore_value;
			}
			else if (RND_NSTRCMP(n->name, "descr") == 0) {
				cnt = &n_descr;
				lst = &ignore_descr;
			}
			else {
				rnd_hid_cfg_error(n, "invalid skip name; must be one of refdes, value, descr");
				continue;
			}
			/* add the entry to the appropriate list */
			sval = n->data.text.value;
			(*cnt)++;
			if ((*lst = (char **) realloc(*lst, (*cnt) * sizeof(char *))) == NULL) {
				fprintf(stderr, "realloc() failed\n");
				exit(-1);
			}
			(*lst)[*cnt - 1] = rnd_strdup(sval);
		}
		else
			rnd_hid_cfg_error(n, "invalid skip type; must be text");
	}
}

static rnd_bool vendorIsSubcMappable(pcb_subc_t *subc)
{
	int i;
	int noskip;

	if (!conf_vendor.plugins.vendor.enable)
		return rnd_false;

TODO(": these 3 loops should be wrapped in a single loop that iterates over attribute keys")
	noskip = 1;
	for (i = 0; i < n_refdes; i++) {
		if ((RND_NSTRCMP(RND_UNKNOWN(subc->refdes), ignore_refdes[i]) == 0)
				|| rematch(ignore_refdes[i], RND_UNKNOWN(subc->refdes))) {
			rnd_message(RND_MSG_INFO, "Vendor mapping skipped because refdes = %s matches %s\n", RND_UNKNOWN(subc->refdes), ignore_refdes[i]);
			noskip = 0;
		}
	}
	if (noskip) {
		const char *vl = pcb_attribute_get(&subc->Attributes, "value");
		for (i = 0; i < n_value; i++) {
			if ((RND_NSTRCMP(RND_UNKNOWN(vl), ignore_value[i]) == 0)
					|| rematch(ignore_value[i], RND_UNKNOWN(vl))) {
				rnd_message(RND_MSG_INFO, "Vendor mapping skipped because value = %s matches %s\n", RND_UNKNOWN(vl), ignore_value[i]);
				noskip = 0;
			}
		}
	}

	if (noskip) {
		const char *fp = pcb_attribute_get(&subc->Attributes, "footprint");
		for (i = 0; i < n_descr; i++) {
			if ((RND_NSTRCMP(RND_UNKNOWN(fp), ignore_descr[i]) == 0)
					|| rematch(ignore_descr[i], RND_UNKNOWN(fp))) {
				rnd_message(RND_MSG_INFO,
								"Vendor mapping skipped because descr = %s matches %s\n",
								RND_UNKNOWN(fp), ignore_descr[i]);
				noskip = 0;
			}
		}
	}

	if (noskip && PCB_FLAG_TEST(PCB_FLAG_LOCK, subc)) {
		rnd_message(RND_MSG_INFO, "Vendor mapping skipped because element %s is locked\n", RND_UNKNOWN(subc->refdes));
		noskip = 0;
	}

	if (noskip)
		return rnd_true;
	else
		return rnd_false;
}

static rnd_bool rematch(const char *re, const char *s)
{
	int result;
	re_sei_t *regex;

	/* compile the regular expression */
	regex = re_sei_comp(re);
	if (re_sei_errno(regex) != 0) {
		rnd_message(RND_MSG_ERROR, "regexp error: %s\n", re_error_str(re_sei_errno(regex)));
		re_sei_free(regex);
		return rnd_false;
	}

	result = re_sei_exec(regex, s);
	re_sei_free(regex);

	if (result != 0)
		return rnd_true;
	else
		return rnd_false;
}

static const char *vendor_cookie = "vendor drill mapping";

rnd_action_t vendor_action_list[] = {
	{"ApplyVendor", pcb_act_ApplyVendor, apply_vendor_help, apply_vendor_syntax},
	{"UnloadVendor", pcb_act_UnloadVendor, unload_vendor_help, unload_vendor_syntax},
	{"LoadVendorFrom", pcb_act_LoadVendorFrom, pcb_acth_LoadVendorFrom, pcb_acts_LoadVendorFrom}
};

static char **vendor_free_vect(char **lst, int *len)
{
	if (lst != NULL) {
		int n;
		for(n = 0; n < *len; n++)
			if (lst[n] != NULL)
				free(lst[n]);
		free(lst);
	}
	*len = 0;
	return NULL;
}

static void vendor_free_all(void)
{
	ignore_refdes = vendor_free_vect(ignore_refdes, &n_refdes);
	ignore_value = vendor_free_vect(ignore_value, &n_value);
	ignore_descr = vendor_free_vect(ignore_descr, &n_descr);
	if (vendor_drills != NULL) {
		free(vendor_drills);
		vendor_drills = NULL;
		n_vendor_drills = 0;
	}
	cached_drill = -1;
}

/* Tune newly placed padstacks */
static void vendor_new_pstk(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_pstk_t *ps;
	rnd_cardinal_t dummy;

	if ((argc < 2) || (argv[1].type != RND_EVARG_PTR))
		return;

	ps = argv[1].d.p;
	apply_vendor_pstk1(ps, &dummy);
}

int pplg_check_ver_vendordrill(int ver_needed) { return 0; }

void pplg_uninit_vendordrill(void)
{
	rnd_event_unbind_allcookie(vendor_cookie);
	rnd_remove_actions_by_cookie(vendor_cookie);
	vendor_free_all();
	rnd_conf_unreg_fields("plugins/vendor/");
	rnd_hid_menu_unload(rnd_gui, vendor_cookie);
}

int pplg_init_vendordrill(void)
{
	RND_API_CHK_VER;
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_vendor, field,isarray,type_name,cpath,cname,desc,flags);
#include "vendor_conf_fields.h"

	rnd_event_bind(PCB_EVENT_NEW_PSTK, vendor_new_pstk, NULL, vendor_cookie);
	RND_REGISTER_ACTIONS(vendor_action_list, vendor_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, vendor_cookie, 110, NULL, 0, vendor_menu, "plugin: vendor drill mapping");
	return 0;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2004, 2007 Dan McMahill
 *  Copyright (C) 2018,2021 Tibor 'Igor2' Palinkas
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

#include <genht/hash.h>
#include <genvector/vtp0.h>
#include <genregex/regex_sei.h>

#include <librnd/core/anyload.h>
#include "change.h"
#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "undo.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/hid/hid_menu.h>
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
static long process_skips(lht_node_t *);
static void vendor_free_all(void);
rnd_coord_t vendorDrillMap(rnd_coord_t in);


/*** skip ***/
static vtp0_t skip_invalid;
#define HT_INVALID_VALUE skip_invalid
#define HT_HAS_CONST_KEY
typedef char *htsv_key_t;
typedef const char *htsv_const_key_t;
typedef vtp0_t htsv_value_t;
#define HT(x) htsv_ ## x
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT

static int skips_inited = 0;
static htsv_t skips; /* key=attribute-name, value = vtp0_t pairs of (precompiled regex list):(string) */

/* compile regex_str and remember it for skipping an attrib */
static void skip_add(const char *attrib, const char *regex_str)
{
	vtp0_t *slot;
	re_sei_t *regex;
	htsv_entry_t *e;

	if (!skips_inited) {
		htsv_init(&skips, strhash, strkeyeq);
		skips_inited = 1;
	}

	e = htsv_getentry(&skips, attrib);
	if (e == NULL) {
		htsv_set(&skips, rnd_strdup(attrib), skip_invalid);
		e = htsv_getentry(&skips, attrib);
	}
	slot = &e->value;

	/* compile the regular expression */
	regex = re_sei_comp(regex_str);
	if (re_sei_errno(regex) != 0) {
		rnd_message(RND_MSG_ERROR, "regexp error: %s\n", re_error_str(re_sei_errno(regex)));
		re_sei_free(regex);
		return;
	}
	vtp0_append(slot, regex);
	vtp0_append(slot, rnd_strdup(regex_str));
}

static void skip_uninit(void)
{
	htsv_entry_t *e;

	for(e = htsv_first(&skips); e != NULL; e = htsv_next(&skips, e)) {
		long n;
		vtp0_t *slot = &e->value;

		for(n = 0; n < slot->used; n+=2) {
			void *rx  = slot->array[n];
			char *str = slot->array[n+1];
			re_sei_free(rx);
			free(str);
			
		}
		vtp0_uninit(slot);
		free(e->key);
	}
	htsv_uninit(&skips);
	skips_inited = 0;
}

/***/

/* list of vendor drills and a count of them */
static rnd_coord_t *vendor_drills = NULL;
static int n_vendor_drills = 0;

static int cached_drill = -1;
static int cached_map = -1;

/* resource file to PCB units scale factor */
static double sf;


/* type of drill mapping */
typedef enum {
	ROUND_UP = 0,
	ROUND_NEAREST,
	ROUND_DOWN
} vendor_round_t;

static vendor_round_t rounding_method = ROUND_UP;

#define FREE(x) if((x) != NULL) { free (x) ; (x) = NULL; }

static rnd_bool vendorIsSubcMappable(pcb_subc_t *subc);


static const char apply_vendor_syntax[] = "ApplyVendor()";
static const char apply_vendor_help[] = "Applies the currently loaded vendor drill table to the current design.";
/* DOC: applyvendor.html */
fgw_error_t pcb_act_ApplyVendor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_busy(&PCB->hidlib, 1);
	apply_vendor_map();
	rnd_hid_busy(&PCB->hidlib, 0);
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

const char *lht_get_htext(lht_node_t *h, const char *name)
{
	lht_node_t *n = lht_dom_hash_get(h, name);

	if ((n == NULL) || (n->type != LHT_TEXT))
		return NULL;

	return n->data.text.value;
}

static int vendor_load_root(const char *fname, lht_node_t *root, rnd_bool pure)
{
	long num_skips;
	lht_node_t *drlres;
	const char *sval;
	int res = 0;

	if (root->type != LHT_HASH) {
		rnd_hid_cfg_error(root, "vendor drill root node must be a hash\n");
		return -1;
	}

	if (!pure)
		vendor_free_all();

	/* figure out the units, if specified */
	sval = lht_get_htext(root, "units");
	if (sval == NULL) {
		if (!pure)
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

	if (!pure)
		rounding_method = ROUND_UP; /* default to ROUND_UP */

	sval = lht_get_htext(root, "round");
	if (sval != NULL) {
		if (RND_NSTRCMP(sval, "up") == 0) {
			rounding_method = ROUND_UP;
		}
		else if (RND_NSTRCMP(sval, "down") == 0) {
			rounding_method = ROUND_DOWN;
		}
		else if (RND_NSTRCMP(sval, "nearest") == 0) {
			rounding_method = ROUND_NEAREST;
		}
		else if (!pure) {
			rnd_message(RND_MSG_ERROR, "\"%s\" is not a valid rounding type.  Defaulting to up\n", sval);
			rounding_method = ROUND_UP;
		}
	}

	num_skips = process_skips(lht_dom_hash_get(root, "skips"));
	if (num_skips < 0)
		res = -1;

	/* extract the drillmap resource */
	drlres = lht_dom_hash_get(root, "drillmap");
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

	if (lht_dom_hash_get(root, "drc") != NULL) {
		rnd_message(RND_MSG_ERROR, "Vendordrill: %s contains a drc subtree. The vendor drill plugin does not support DRC settings anymore\n", fname);
		rnd_message(RND_MSG_ERROR, "Vendordrill: please refer to http://www.repo.hu/projects/pcb-rnd/help/err0003.html\n");
	}

	rnd_message(RND_MSG_INFO, "Loaded %d vendor drills from %s\n", n_vendor_drills, fname);
	rnd_message(RND_MSG_INFO, "Loaded %ld skips for %d different attributes\n", num_skips, skips.used);

	rnd_conf_set(RND_CFR_DESIGN, "plugins/vendor/enable", -1, "0", RND_POL_OVERWRITE);

	if (!pure)
		apply_vendor_map();

	return res;
}

static const char pcb_acts_LoadVendorFrom[] = "LoadVendorFrom(filename, [yes|no])";
static const char pcb_acth_LoadVendorFrom[] = "Loads the specified vendor lihata file. If second argument is \"yes\" or \"pure\", load in pure mode without side effects: do not reset or apply, only incrementally load.";
/* DOC: loadvendorfrom.html */
fgw_error_t pcb_act_LoadVendorFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL, *spure = NULL;
	static char *default_file = NULL;
	int pure = 0;
	lht_doc_t *doc;
	rnd_bool free_fname = rnd_false;
	int r;

	cached_drill = -1;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadVendorFrom, fname = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, LoadVendorFrom, spure = argv[2].val.str);

	if (!fname || !*fname) {
		fname = rnd_hid_fileselect(rnd_gui, "Load Vendor Resource File...",
			"Picks a vendor resource file to load.\n"
			"This file contains a list of\n"
			"predefined drills which are allowed.", default_file, ".res",
			NULL, "vendor", RND_HID_FSD_READ, NULL);
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

	if (spure != NULL) {
		if (strcmp(spure, "pure") == 0)
			pure = 1;
		else
			pure = rnd_istrue(spure);
	}

	/* load the resource file */
	doc = rnd_hid_cfg_load_lht(&PCB->hidlib, fname);
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "Could not load vendor resource file \"%s\"\n", fname);
		RND_ACT_IRES(1);
		return 0;
	}

	r = vendor_load_root(fname, doc->root, pure);

	if (free_fname)
		free((char*)fname);
	lht_dom_uninit(doc);
	RND_ACT_IRES(r);
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
		{
			pcb_pstk_proto_t *proto = pcb_pstk_get_proto_(PCB->Data, conf_core.design.via_proto);
			rnd_coord_t ndia = (proto != NULL) ? vendorDrillMap(proto->hdia) : 0;

			if ((proto != NULL) && (proto->hdia != ndia)) {
				changed++;
				pcb_pstk_proto_change_hole(proto, NULL, &ndia, NULL, NULL);
				rnd_conf_setf(RND_CFR_DESIGN, "design/via_drilling_hole", -1, "%$mm", ndia);
				rnd_message(RND_MSG_INFO, "Adjusted pen via hole size to be %ml mils\n", ndia);
			}
		}

		/* and update the vias for the various routing styles */
		for (i = 0; i < vtroutestyle_len(&PCB->RouteStyle); i++) {
			pcb_pstk_proto_t *proto = pcb_pstk_get_proto_(PCB->Data, PCB->RouteStyle.array[i].via_proto);
			rnd_coord_t ndia;

			if (proto == NULL)
				continue;

			ndia = vendorDrillMap(proto->hdia);
			if (proto->hdia != ndia) {
				changed++;
				pcb_pstk_proto_change_hole(proto, NULL, &ndia, NULL, NULL);
				rnd_message(RND_MSG_INFO,
					"Adjusted %s routing style hole size to be %ml mils\n",
					PCB->RouteStyle.array[i].name, ndia);
			}
		}

		/* if we've changed anything, indicate that we need to save the
		   file, redraw things, and make sure we can undo. */
		if (changed) {
			pcb_board_set_changed_flag(PCB, rnd_true);
			rnd_hid_redraw(&PCB->hidlib);
			pcb_undo_inc_serial();
		}
	}

	/* restore mapping on/off */
	rnd_conf_force_set_bool(conf_vendor.plugins.vendor.enable, state);
}

/* for a given drill size, find the nearest vendor drill size */
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

	/* figure out which 2 drills are nearest in size */
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
	switch(rounding_method) {
		case ROUND_NEAREST:
			if ((in - vendor_drills[i - 1]) > (vendor_drills[i] - in)) {
				cached_map = vendor_drills[i];
				return vendor_drills[i];
			}
			cached_map = vendor_drills[i - 1];
			return vendor_drills[i - 1];

		case ROUND_UP:
			goto round_up;

		case ROUND_DOWN:
			if (in == vendor_drills[max]) {
				cached_map = vendor_drills[max];
				return vendor_drills[max];
			}
			cached_map = vendor_drills[min];
			return vendor_drills[min];
	}

	/* default, for invalid internal state */
	round_up:;
		cached_map = vendor_drills[i];
		return vendor_drills[i];
}

/* add a drill size to the vendor drill list */
static void add_to_drills(char *sval)
{
	double tmpd;
	int val;
	int k, j;

	/* increment the count and make sure we have memory */
	n_vendor_drills++;
	if ((vendor_drills = (rnd_coord_t *) realloc(vendor_drills, n_vendor_drills * sizeof(rnd_coord_t))) == NULL) {
		fprintf(stderr, "realloc() failed to allocate %ld bytes\n", (unsigned long) n_vendor_drills * sizeof(rnd_coord_t));
		return;
	}

	/* string to a value with the units scale factor in place */
	tmpd = atof(sval);
	val = floor(sf * tmpd + 0.5);

	/* We keep the array of vendor drills sorted to make it easier to
	   do the rounding later.  The algorithm used here is not so efficient,
	   but we're not dealing with much in the way of data. */

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

/* deal with the "skip" subtree; returns number of skips loaded */
static long process_skips(lht_node_t *res)
{
	lht_node_t *n;
	const char *attr;
	long cnt = 0;

	if (res == NULL)
		return 0;

	if (res->type != LHT_LIST) {
		rnd_hid_cfg_error(res, "skips must be a list.\n");
		return -1;
	}

	for(n = res->data.list.first; n != NULL; n = n->next) {
		if (n->type == LHT_TEXT) {
			if ((RND_NSTRCMP(n->name, "refdes") == 0) || (RND_NSTRCMP(n->name, "value") == 0)) {
				attr = n->name;
			}
			else if (RND_NSTRCMP(n->name, "descr") == 0) {
				attr = "footprint";
			}
			else if ((n->name[0] == 'a') && (n->name[1] == '.')) {
				attr = n->name+2;
			}
			else {
				rnd_hid_cfg_error(n, "invalid skip name; must be one of refdes, value, descr");
				continue;
			}
			skip_add(attr, n->data.text.value);
			cnt++;
		}
		else
			rnd_hid_cfg_error(n, "invalid skip type; must be text");
	}
	return cnt;
}

static rnd_bool vendorIsSubcMappable(pcb_subc_t *subc)
{
	int noskip = 1;
	htsv_entry_t *e;

	if (!conf_vendor.plugins.vendor.enable)
		return rnd_false;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, subc)) {
		rnd_message(RND_MSG_INFO, "Vendor mapping skipped because element %s is locked\n", RND_UNKNOWN(subc->refdes));
		noskip = 0;
		goto done;
	}

	for(e = htsv_first(&skips); e != NULL; e = htsv_next(&skips, e)) {
		long n;
		vtp0_t *slot = &e->value;
		const char *attr = e->key;
		const char *vl = RND_UNKNOWN(pcb_attribute_get(&subc->Attributes, attr));

		for(n = 0; n < slot->used; n+=2) {
			void *rx  = slot->array[n];
			char *str = slot->array[n+1];

			if (re_sei_exec(rx, vl) != 0) {
				rnd_message(RND_MSG_INFO, "Vendor mapping skipped because %s = %s matches %s\n", attr, vl, str);
				noskip = 0;
				goto done;
			}
		}
	}

	done:;

	if (noskip)
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

static void vendor_free_all(void)
{
	skip_uninit();
	if (vendor_drills != NULL) {
		free(vendor_drills);
		vendor_drills = NULL;
		n_vendor_drills = 0;
	}
	cached_drill = -1;
}

/* Tune newly placed padstacks */
static void vendor_new_pstk(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_pstk_t *ps;
	rnd_cardinal_t dummy;

	if ((argc < 2) || (argv[1].type != RND_EVARG_PTR))
		return;

	ps = argv[1].d.p;
	apply_vendor_pstk1(ps, &dummy);
}

static int vendor_anyload_subtree(const rnd_anyload_t *al, rnd_design_t *hl, lht_node_t *root)
{
	return vendor_load_root(root->file_name, root, 0);
}

static rnd_anyload_t vendor_anyload = {0};

int pplg_check_ver_vendordrill(int ver_needed) { return 0; }

void pplg_uninit_vendordrill(void)
{
	rnd_anyload_unreg_by_cookie(vendor_cookie);
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

	vendor_anyload.load_subtree = vendor_anyload_subtree;
	vendor_anyload.cookie = vendor_cookie;
	rnd_anyload_reg("^vendor_drill_map$", &vendor_anyload);

	return 0;
}

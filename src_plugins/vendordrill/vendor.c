/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2004, 2007 Dan McMahill
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
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
#include "error.h"
#include "set.h"
#include "undo.h"
#include "vendor.h"
#include "stub_vendor.h"
#include "plugins.h"
#include "action_helper.h"
#include "hid_flags.h"
#include "hid_actions.h"
#include "hid_cfg.h"
#include "vendor_conf.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_pinvia.h"
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

conf_vendor_t conf_vendor;

static void add_to_drills(char *);
static void apply_vendor_map(void);
static void process_skips(lht_node_t *);
static pcb_bool rematch(const char *, const char *);
static void vendor_free_all(void);

/* list of vendor drills and a count of them */
static int *vendor_drills = NULL;
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

/* ************************************************************ */

static const char apply_vendor_syntax[] = "ApplyVendor()";

static const char apply_vendor_help[] = "Applies the currently loaded vendor drill table to the current design.";

/* %start-doc actions ApplyVendor
@cindex vendor map
@cindex vendor drill table
@findex ApplyVendor()

This will modify all of your drill holes to match the list of allowed
sizes for your vendor.
%end-doc */

int ActionApplyVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_hid_action("Busy");
	apply_vendor_map();
	return 0;
}

/* ************************************************************ */

static const char unload_vendor_syntax[] = "UnloadVendor()";

static const char unload_vendor_help[] = "Unloads the current vendor drill mapping table.";

/* %start-doc actions UnloadVendor

@cindex vendor map
@cindex vendor drill table
@findex UnloadVendor()

%end-doc */

int ActionUnloadVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	cached_drill = -1;

	vendor_free_all();
	return 0;
}

/* ************************************************************ */

static const char load_vendor_syntax[] = "LoadVendorFrom(filename)";

static const char load_vendor_help[] = "Loads the specified vendor lihata file.";

/* %start-doc actions LoadVendorFrom

@cindex vendor map
@cindex vendor drill table
@findex LoadVendorFrom()

@table @var
@item filename
Name of the vendor lihata file.  If not specified, the user will
be prompted to enter one.
@end table

%end-doc */

int ActionLoadVendorFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;
	const char *sval;
	lht_doc_t *doc;
	lht_node_t *drlres;
	pcb_bool free_fname = pcb_false;

	cached_drill = -1;

	fname = argc ? argv[0] : NULL;

	if (!fname || !*fname) {
		fname = gui->fileselect(_("Load Vendor Resource File..."),
														_("Picks a vendor resource file to load.\n"
															"This file can contain drc settings for a\n"
															"particular vendor as well as a list of\n"
															"predefined drills which are allowed."), default_file, ".res", "vendor", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_AFAIL(load_vendor);

		free_fname = pcb_true;

		free(default_file);
		default_file = NULL;

		if (fname && *fname)
			default_file = pcb_strdup(fname);
	}

	vendor_free_all();

	/* load the resource file */
	doc = pcb_hid_cfg_load_lht(fname);
	if (doc == NULL) {
		pcb_message(PCB_MSG_DEFAULT, _("Could not load vendor resource file \"%s\"\n"), fname);
		return 1;
	}

	/* figure out the vendor name, if specified */
	vendor_name = (char *) UNKNOWN(pcb_hid_cfg_text_value(doc, "vendor"));

	/* figure out the units, if specified */
	sval = pcb_hid_cfg_text_value(doc, "/units");
	if (sval == NULL) {
		sf = PCB_MIL_TO_COORD(1);
	}
	else if ((NSTRCMP(sval, "mil") == 0) || (NSTRCMP(sval, "mils") == 0)) {
		sf = PCB_MIL_TO_COORD(1);
	}
	else if ((NSTRCMP(sval, "inch") == 0) || (NSTRCMP(sval, "inches") == 0)) {
		sf = PCB_INCH_TO_COORD(1);
	}
	else if (NSTRCMP(sval, "mm") == 0) {
		sf = PCB_MM_TO_COORD(1);
	}
	else {
		pcb_message(PCB_MSG_DEFAULT, "\"%s\" is not a supported units.  Defaulting to inch\n", sval);
		sf = PCB_INCH_TO_COORD(1);
	}

	/* default to ROUND_UP */
	rounding_method = ROUND_UP;
	sval = pcb_hid_cfg_text_value(doc, "/round");
	if (sval != NULL) {
		if (NSTRCMP(sval, "up") == 0) {
			rounding_method = ROUND_UP;
		}
		else if (NSTRCMP(sval, "nearest") == 0) {
			rounding_method = CLOSEST;
		}
		else {
			pcb_message(PCB_MSG_DEFAULT, _("\"%s\" is not a valid rounding type.  Defaulting to up\n"), sval);
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
					pcb_hid_cfg_error(n, "Broken drillmap: /drillmap should contain text children only\n");
				else
					add_to_drills(n->data.text.value);
			}
		}
		pcb_message(PCB_MSG_DEFAULT, _("Broken drillmap: /drillmap should be a list\n"));
	}
	else
		pcb_message(PCB_MSG_DEFAULT, _("No drillmap resource found\n"));

	sval = pcb_hid_cfg_text_value(doc, "/drc/copper_space");
	if (sval != NULL) {
		PCB->Bloat = floor(sf * atof(sval) + 0.5);
		pcb_message(PCB_MSG_DEFAULT, _("Set DRC minimum copper spacing to %.2f mils\n"), 0.01 * PCB->Bloat);
	}

	sval = pcb_hid_cfg_text_value(doc, "/drc/copper_overlap");
	if (sval != NULL) {
		PCB->Shrink = floor(sf * atof(sval) + 0.5);
		pcb_message(PCB_MSG_DEFAULT, _("Set DRC minimum copper overlap to %.2f mils\n"), 0.01 * PCB->Shrink);
	}

	sval = pcb_hid_cfg_text_value(doc, "/drc/copper_width");
	if (sval != NULL) {
		PCB->minWid = floor(sf * atof(sval) + 0.5);
		pcb_message(PCB_MSG_DEFAULT, _("Set DRC minimum copper spacing to %.2f mils\n"), 0.01 * PCB->minWid);
	}

	sval = pcb_hid_cfg_text_value(doc, "/drc/silk_width");
	if (sval != NULL) {
		PCB->minSlk = floor(sf * atof(sval) + 0.5);
		pcb_message(PCB_MSG_DEFAULT, _("Set DRC minimum silk width to %.2f mils\n"), 0.01 * PCB->minSlk);
	}

	sval = pcb_hid_cfg_text_value(doc, "/drc/min_drill");
	if (sval != NULL) {
		PCB->minDrill = floor(sf * atof(sval) + 0.5);
		pcb_message(PCB_MSG_DEFAULT, _("Set DRC minimum drill diameter to %.2f mils\n"), 0.01 * PCB->minDrill);
	}

	sval = pcb_hid_cfg_text_value(doc, "/drc/min_ring");
	if (sval != NULL) {
		PCB->minRing = floor(sf * atof(sval) + 0.5);
		pcb_message(PCB_MSG_DEFAULT, _("Set DRC minimum annular ring to %.2f mils\n"), 0.01 * PCB->minRing);
	}

	pcb_message(PCB_MSG_DEFAULT, _("Loaded %d vendor drills from %s\n"), n_vendor_drills, fname);
	pcb_message(PCB_MSG_DEFAULT, _("Loaded %d RefDes skips, %d Value skips, %d Descr skips\n"), n_refdes, n_value, n_descr);

	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, "0", POL_OVERWRITE);

	apply_vendor_map();
	if (free_fname)
		free((char*)fname);
	lht_dom_uninit(doc);
	return 0;
}

static void apply_vendor_map(void)
{
	int changed, tot;
	pcb_bool state;

	state = conf_vendor.plugins.vendor.enable;

	/* enable mapping */
	conf_force_set_bool(conf_vendor.plugins.vendor.enable, 1);

	/* reset our counts */
	changed = 0;
	tot = 0;

	/* If we have loaded vendor drills, then apply them to the design */
	if (n_vendor_drills > 0) {

		/* first all the vias */
		VIA_LOOP(PCB->Data);
		{
			tot++;
			if (via->DrillingHole != vendorDrillMap(via->DrillingHole)) {
				/* only change unlocked vias */
				if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, via)) {
					if (pcb_chg_obj_2nd_size(PCB_TYPE_VIA, via, NULL, NULL, vendorDrillMap(via->DrillingHole), pcb_true, pcb_false))
						changed++;
					else {
						pcb_message(PCB_MSG_DEFAULT, _
										("Via at %.2f, %.2f not changed.  Possible reasons:\n"
										 "\t- pad size too small\n"
										 "\t- new size would be too large or too small\n"), 0.01 * via->X, 0.01 * via->Y);
					}
				}
				else {
					pcb_message(PCB_MSG_DEFAULT, _("Locked via at %.2f, %.2f not changed.\n"), 0.01 * via->X, 0.01 * via->Y);
				}
			}
		}
		END_LOOP;

		/* and now the pins */
		ELEMENT_LOOP(PCB->Data);
		{
			/*
			 * first figure out if this element should be skipped for some
			 * reason
			 */
			if (vendorIsElementMappable(element)) {
				/* the element is ok to modify, so iterate over its pins */
				PIN_LOOP(element);
				{
					tot++;
					if (pin->DrillingHole != vendorDrillMap(pin->DrillingHole)) {
						if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, pin)) {
							if (pcb_chg_obj_2nd_size(PCB_TYPE_PIN, element, pin, NULL, vendorDrillMap(pin->DrillingHole), pcb_true, pcb_false))
								changed++;
							else {
								pcb_message(PCB_MSG_DEFAULT, _
												("Pin %s (%s) at %.2f, %.2f (element %s, %s, %s) not changed.\n"
												 "\tPossible reasons:\n"
												 "\t- pad size too small\n"
												 "\t- new size would be too large or too small\n"),
												UNKNOWN(pin->Number), UNKNOWN(pin->Name),
												0.01 * pin->X, 0.01 * pin->Y,
												UNKNOWN(NAMEONPCB_NAME(element)), UNKNOWN(VALUE_NAME(element)), UNKNOWN(DESCRIPTION_NAME(element)));
							}
						}
						else {
							pcb_message(PCB_MSG_DEFAULT, _("Locked pin at %-6.2f, %-6.2f not changed.\n"), 0.01 * pin->X, 0.01 * pin->Y);
						}
					}
				}
				END_LOOP;
			}
		}
		END_LOOP;

		pcb_message(PCB_MSG_DEFAULT, _("Updated %d drill sizes out of %d total\n"), changed, tot);

#warning TODO: this should not happen; modify some local setting?
#if 0
		/* Update the current Via */
		if (conf_core.design.via_drilling_hole != vendorDrillMap(Settings.ViaDrillingHole)) {
			changed++;
			Settings.ViaDrillingHole = vendorDrillMap(Settings.ViaDrillingHole);
			pcb_message(PCB_MSG_DEFAULT, _("Adjusted active via hole size to be %6.2f mils\n"), 0.01 * Settings.ViaDrillingHole);
		}

		/* and update the vias for the various routing styles */
		for (i = 0; i < NUM_STYLES; i++) {
			if (PCB->RouteStyle[i].Hole != vendorDrillMap(PCB->RouteStyle[i].Hole)) {
				changed++;
				PCB->RouteStyle[i].Hole = vendorDrillMap(PCB->RouteStyle[i].Hole);
				pcb_message(PCB_MSG_DEFAULT, _
								("Adjusted %s routing style via hole size to be %6.2f mils\n"),
								PCB->RouteStyle[i].Name, 0.01 * PCB->RouteStyle[i].Hole);
				if (PCB->RouteStyle[i].Diameter < PCB->RouteStyle[i].Hole + MIN_PINORVIACOPPER) {
					PCB->RouteStyle[i].Diameter = PCB->RouteStyle[i].Hole + MIN_PINORVIACOPPER;
					pcb_message(PCB_MSG_DEFAULT, _
									("Increased %s routing style via diameter to %6.2f mils\n"),
									PCB->RouteStyle[i].Name, 0.01 * PCB->RouteStyle[i].Diameter);
				}
			}
		}
#endif
		/*
		 * if we've changed anything, indicate that we need to save the
		 * file, redraw things, and make sure we can undo.
		 */
		if (changed) {
			SetChangedFlag(pcb_true);
			pcb_redraw();
			IncrementUndoSerialNumber();
		}
	}

	/* restore mapping on/off */
	conf_force_set_bool(conf_vendor.plugins.vendor.enable, state);
}

/* for a given drill size, find the closest vendor drill size */
int vendorDrillMap(int in)
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
		pcb_message(PCB_MSG_DEFAULT, _("Vendor drill list does not contain a drill >= %6.2f mil\n"
							"Using %6.2f mil instead.\n"), 0.01 * in, 0.01 * vendor_drills[n_vendor_drills - 1]);
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
		pcb_hid_cfg_error(res, "skips must be a list.\n");

	for(n = res->data.list.first; n != NULL; n = n->next) {
		if (n->type == LHT_TEXT) {
			if (NSTRCMP(n->name, "refdes") == 0) {
				cnt = &n_refdes;
				lst = &ignore_refdes;
			}
			else if (NSTRCMP(n->name, "value") == 0) {
				cnt = &n_value;
				lst = &ignore_value;
			}
			else if (NSTRCMP(n->name, "descr") == 0) {
				cnt = &n_descr;
				lst = &ignore_descr;
			}
			else {
				pcb_hid_cfg_error(n, "invalid skip name; must be one of refdes, value, descr");
				continue;
			}
			/* add the entry to the appropriate list */
			sval = n->data.text.value;
			(*cnt)++;
			if ((*lst = (char **) realloc(*lst, (*cnt) * sizeof(char *))) == NULL) {
				fprintf(stderr, "realloc() failed\n");
				exit(-1);
			}
			(*lst)[*cnt - 1] = pcb_strdup(sval);
		}
		else
			pcb_hid_cfg_error(n, "invalid skip type; must be text");
	}
}

pcb_bool vendorIsElementMappable(pcb_element_t *element)
{
	int i;
	int noskip;

	if (!conf_vendor.plugins.vendor.enable)
		return pcb_false;

	noskip = 1;
	for (i = 0; i < n_refdes; i++) {
		if ((NSTRCMP(UNKNOWN(NAMEONPCB_NAME(element)), ignore_refdes[i]) == 0)
				|| rematch(ignore_refdes[i], UNKNOWN(NAMEONPCB_NAME(element)))) {
			pcb_message(PCB_MSG_DEFAULT, _("Vendor mapping skipped because refdes = %s matches %s\n"), UNKNOWN(NAMEONPCB_NAME(element)), ignore_refdes[i]);
			noskip = 0;
		}
	}
	if (noskip)
		for (i = 0; i < n_value; i++) {
			if ((NSTRCMP(UNKNOWN(VALUE_NAME(element)), ignore_value[i]) == 0)
					|| rematch(ignore_value[i], UNKNOWN(VALUE_NAME(element)))) {
				pcb_message(PCB_MSG_DEFAULT, _("Vendor mapping skipped because value = %s matches %s\n"), UNKNOWN(VALUE_NAME(element)), ignore_value[i]);
				noskip = 0;
			}
		}

	if (noskip)
		for (i = 0; i < n_descr; i++) {
			if ((NSTRCMP(UNKNOWN(DESCRIPTION_NAME(element)), ignore_descr[i])
					 == 0)
					|| rematch(ignore_descr[i], UNKNOWN(DESCRIPTION_NAME(element)))) {
				pcb_message(PCB_MSG_DEFAULT, _
								("Vendor mapping skipped because descr = %s matches %s\n"),
								UNKNOWN(DESCRIPTION_NAME(element)), ignore_descr[i]);
				noskip = 0;
			}
		}

	if (noskip && PCB_FLAG_TEST(PCB_FLAG_LOCK, element)) {
		pcb_message(PCB_MSG_DEFAULT, _("Vendor mapping skipped because element %s is locked\n"), UNKNOWN(NAMEONPCB_NAME(element)));
		noskip = 0;
	}

	if (noskip)
		return pcb_true;
	else
		return pcb_false;
}

static pcb_bool rematch(const char *re, const char *s)
{
	int result;
	re_sei_t *regex;

	/* compile the regular expression */
	regex = re_sei_comp(re);
	if (re_sei_errno(regex) != 0) {
		pcb_message(PCB_MSG_DEFAULT, _("regexp error: %s\n"), re_error_str(re_sei_errno(regex)));
		re_sei_free(regex);
		return pcb_false;
	}

	result = re_sei_exec(regex, s);
	re_sei_free(regex);

	if (result != 0)
		return pcb_true;
	else
		return pcb_false;
}

static const char *vendor_cookie = "vendor drill mapping";

pcb_hid_action_t vendor_action_list[] = {
	{"ApplyVendor", 0, ActionApplyVendor,
	 apply_vendor_help, apply_vendor_syntax}
	,
	{"UnloadVendor", 0, ActionUnloadVendor,
	 unload_vendor_help, unload_vendor_syntax}
	,
	{"LoadVendorFrom", 0, ActionLoadVendorFrom,
	 load_vendor_help, load_vendor_syntax}
};

PCB_REGISTER_ACTIONS(vendor_action_list, vendor_cookie)

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

static void hid_vendordrill_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(vendor_cookie);
	vendor_free_all();
	conf_unreg_fields("plugins/vendor/");
}

#include "dolists.h"
pcb_uninit_t hid_vendordrill_init(void)
{
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_vendor, field,isarray,type_name,cpath,cname,desc,flags);
#include "vendor_conf_fields.h"

	stub_vendorDrillMap = vendorDrillMap;
	stub_vendorIsElementMappable = vendorIsElementMappable;

	PCB_REGISTER_ACTIONS(vendor_action_list, vendor_cookie)
	return hid_vendordrill_uninit;
}

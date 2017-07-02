 /*
  *                            COPYRIGHT
  *
  *  PCB, interactive printed circuit board design
  *  Copyright (C) 2006 Dan McMahill
  *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/*
 *  Based on the png exporter by Dan McMahill
 */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "plugins.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "plug_io.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_helper.h"
#include "hid_flags.h"


static pcb_hid_t stat_hid;

const char *stat_cookie = "stat HID";

pcb_hid_attribute_t stat_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Output file name",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_statfile 0

	{"board_id", "Short name of the board so it can be identified for updates",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_board_id 1

	{"orig", "This design started its life in pcb-rnd",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_orig 2

	{"lht_built", "This design was already in lihata when real boards got built",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_lht_built 3


	{"built", "how many actual/physical boards got built",
	 PCB_HATT_INTEGER, 0, 1000000, {0, 0, 0}, 0, 0},
#define HA_built 4

	{"first_ver", "the version of pcb-rnd you first used on this board",
	 PCB_HATT_STRING, 0, 0, {0, NULL, 0}, 0, 0},
#define HA_first_ver 5

	{"license", "license of the design",
	 PCB_HATT_STRING, 0, 0, {0, NULL, 0}, 0, 0},
#define HA_license 6

};

#define NUM_OPTIONS (sizeof(stat_attribute_list)/sizeof(stat_attribute_list[0]))

PCB_REGISTER_ATTRIBUTES(stat_attribute_list, stat_cookie)

static pcb_hid_attr_val_t stat_values[NUM_OPTIONS];

static pcb_hid_attribute_t *stat_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".stat.lht";

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &stat_attribute_list[HA_statfile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return stat_attribute_list;
}

typedef struct layer_stat_s {
	pcb_coord_t trace_len;
	double copper_area;
	unsigned long int lines, arcs, polys, elements;
} layer_stat_t;

static void stat_do_export(pcb_hid_attr_val_t * options)
{
	FILE *f;
	const char *filename;
	int i, lid;
	pcb_layergrp_id_t lgid;
	char buff[1024];
	time_t t;
	layer_stat_t ls, *lgs, lgss[PCB_MAX_LAYERGRP];
	int nl, phg, hp, hup, group_not_empty[PCB_MAX_LAYERGRP];
	pcb_cardinal_t num_etop = 0, num_ebottom = 0, num_esmd = 0, num_epads = 0, num_epins = 0;

	memset(lgss, 0, sizeof(lgss));
	memset(group_not_empty, 0, sizeof(group_not_empty));

	if (!options) {
		stat_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			stat_values[i] = stat_attribute_list[i].default_val;
		options = stat_values;
	}

	filename = options[HA_statfile].str_value;
	if (!filename)
		filename = "pcb.stat.lht";

	f = fopen(filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	pcb_board_count_holes(&hp, &hup, NULL);
	pcb_print_utc(buff, sizeof(buff), 0);

	fprintf(f, "ha:pcb-rnd-board-stats-v1 {\n");
	fprintf(f, "	ha:meta {\n");
	fprintf(f, "		date=%s\n", buff);
	fprintf(f, "		built=%d\n", options[HA_built].int_value);
	fprintf(f, "		lht_built=%s\n", (options[HA_lht_built].int_value ? "yes" : "no"));
	fprintf(f, "		orig_rnd=%s\n", (options[HA_orig].int_value ? "yes" : "no"));
	fprintf(f, "		first_ver=%s\n", options[HA_first_ver].str_value);
	fprintf(f, "		curr_ver=%s\n", PCB_VERSION);
#ifdef PCB_REVISION
	fprintf(f, "		curr_rev=%s\n", PCB_REVISION);
#endif

	fprintf(f, "	}\n");

	fprintf(f, "	li:logical_layers {\n");
	for(lid = 0; lid < pcb_max_layer; lid++) {
		pcb_layer_t *l = PCB->Data->Layer+lid;
		int empty = pcb_layer_is_empty_(PCB, l);
		unsigned int lflg = pcb_layer_flags(PCB, lid);

		lgid = pcb_layer_get_group(PCB, lid);
		lgs = lgss + lgid;

		fprintf(f, "		ha:layer_%d {\n", lid);
		fprintf(f, "			name={%s}\n", l->meta.real.name);
		fprintf(f, "			empty=%s\n", empty ? "yes" : "no");
		fprintf(f, "			flags=%x\n", lflg);
		fprintf(f, "			grp=%ld\n", lgid);

		if (lflg & PCB_LYT_COPPER) {
			memset(&ls, 0, sizeof(ls));

			PCB_LINE_LOOP(l) {
				pcb_coord_t v;
				double d;
				lgs->lines++;
				ls.lines++;
				v = pcb_line_length(line);
				ls.trace_len += v;
				lgs->trace_len += v;
				d = pcb_line_area(line);
				ls.copper_area += d;
				lgs->copper_area += d;
				
			}
			PCB_END_LOOP;

			PCB_ARC_LOOP(l) {
				pcb_coord_t v;
				double d;
				lgs->arcs++;
				ls.arcs++;
				v = pcb_arc_length(arc);
				ls.trace_len += v;
				lgs->trace_len += v;
				d = pcb_arc_area(arc);
				ls.copper_area += d;
				lgs->copper_area += d;
			}
			PCB_END_LOOP;

			PCB_POLY_LOOP(l) {
				double v;
				lgs->polys++;
				ls.polys++;
				v = pcb_poly_area(polygon);
				ls.copper_area += v;
				lgs->copper_area += v;
			}
			PCB_END_LOOP;

			if (!empty)
				group_not_empty[lgid] = 1;

			fprintf(f, "			lines=%lu\n", ls.lines);
			fprintf(f, "			arcs=%lu\n",  ls.arcs);
			fprintf(f, "			polys=%lu\n", ls.polys);
			pcb_fprintf(f, "			trace_len=%$mm\n", ls.trace_len);
			fprintf(f, "			copper_area={%f mm^2}\n", (double)ls.copper_area / (double)PCB_MM_TO_COORD(1) / (double)PCB_MM_TO_COORD(1));
		}
		fprintf(f, "		}\n");
	}
	fprintf(f, "	}\n");

	phg = 0;
	fprintf(f, "	li:physical_layers {\n");
	for(lgid = 0; lgid < pcb_max_group(PCB); lgid++) {
		if (group_not_empty[lgid]) {
			phg++;
			fprintf(f, "		ha:layergroup_%ld {\n", lgid);
			fprintf(f, "			lines=%lu\n", lgss[lgid].lines);
			fprintf(f, "			arcs=%lu\n",  lgss[lgid].arcs);
			fprintf(f, "			polys=%lu\n", lgss[lgid].polys);
			pcb_fprintf(f, "			trace_len=%$mm\n", lgss[lgid].trace_len);
			fprintf(f, "			copper_area={%f mm^2}\n", (double)lgss[lgid].copper_area / (double)PCB_MM_TO_COORD(1) / (double)PCB_MM_TO_COORD(1));
			fprintf(f, "		}\n");
		}
	}
	fprintf(f, "	}\n");

	fprintf(f, "	li:netlist {\n");
	for(nl = 0; nl < PCB_NUM_NETLISTS; nl++) {
		pcb_cardinal_t m, terms = 0, best_terms = 0;
		fprintf(f, "		ha:%s {\n", pcb_netlist_names[nl]);
		for(m = 0; m < PCB->NetlistLib[nl].MenuN; m++) {
			pcb_lib_menu_t *menu = &PCB->NetlistLib[nl].Menu[m];
			terms += menu->EntryN;
			if (menu->EntryN > best_terms)
				best_terms = menu->EntryN;
		}
		fprintf(f, "			nets=%ld\n", (long int)PCB->NetlistLib[nl].MenuN);
		fprintf(f, "			terminals=%ld\n", (long int)terms);
		fprintf(f, "			max_term_per_net=%ld\n", (long int)best_terms);
		fprintf(f, "		}\n");
	}
	fprintf(f, "	}\n");

	PCB_ELEMENT_LOOP(PCB->Data) {
		int pal, pil;
		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element))
			num_ebottom++;
		else
			num_etop++;
		pal = padlist_length(&element->Pad);
		pil = pinlist_length(&element->Pin);
		if (pal > pil)
			num_esmd++;
		num_epads += pal;
		num_epins += pil;
	}
	PCB_END_LOOP;


	fprintf(f, "	ha:board {\n");
	fprintf(f, "		id={%s}\n", options[HA_board_id].str_value);
	fprintf(f, "		license={%s}\n", options[HA_license].str_value);
	fprintf(f, "		format={%s}\n", PCB->Data->loader == NULL ? "unknown" : PCB->Data->loader->description);
	pcb_fprintf(f, "		width=%$mm\n", PCB->MaxWidth);
	pcb_fprintf(f, "		height=%$mm\n", PCB->MaxHeight);
	fprintf(f, "		gross_area={%.4f mm^2}\n", (double)PCB_COORD_TO_MM(PCB->MaxWidth) * (double)PCB_COORD_TO_MM(PCB->MaxWidth));
	fprintf(f, "		holes_plated=%d\n", hp);
	fprintf(f, "		holes_unplated=%d\n", hup);
	fprintf(f, "		physical_copper_layers=%d\n", phg);
	fprintf(f, "		ha:elements {\n");
	fprintf(f, "			total=%ld\n", (long int)num_ebottom + num_etop);
	fprintf(f, "			top_side=%ld\n", (long int)num_etop);
	fprintf(f, "			bottom_side=%ld\n", (long int)num_ebottom);
	fprintf(f, "			smd=%ld\n", (long int)num_esmd);
	fprintf(f, "			pads=%ld\n", (long int)num_epads);
	fprintf(f, "			pins=%ld\n", (long int)num_epins);
	fprintf(f, "		}\n");
	fprintf(f, "	}\n");

	fprintf(f, "}\n");
	fclose(f);
}

static void stat_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(stat_attribute_list, sizeof(stat_attribute_list) / sizeof(stat_attribute_list[0]), stat_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

static int stat_usage(const char *topic)
{
	fprintf(stderr, "\nstat exporter command line arguments:\n\n");
	pcb_hid_usage(stat_attribute_list, sizeof(stat_attribute_list) / sizeof(stat_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x stat foo.pcb [stat options]\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_stat(int ver_needed) { return 0; }

void pplg_uninit_export_stat(void)
{
}

int pplg_init_export_stat(void)
{
	memset(&stat_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&stat_hid);
	pcb_dhlp_draw_helpers_init(&stat_hid);

	stat_hid.struct_size = sizeof(pcb_hid_t);
	stat_hid.name = "stat";
	stat_hid.description = "board statistics";
	stat_hid.exporter = 1;

	stat_hid.get_export_options = stat_get_export_options;
	stat_hid.do_export = stat_do_export;
	stat_hid.parse_arguments = stat_parse_arguments;

	stat_hid.usage = stat_usage;

	stat_attribute_list[HA_first_ver].default_val.str_value = pcb_strdup(PCB_VERSION);
	stat_attribute_list[HA_license].default_val.str_value = pcb_strdup("proprietary/private");

	pcb_hid_register_hid(&stat_hid);

	return 0;
}

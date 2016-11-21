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
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_statfile 0

	{"orig_rnd", "This design started its life in pcb-rnd",
	 HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_orig 1

	{"built", "how many actual/physical boards got built",
	 HID_Integer, 0, 1000000, {0, 0, 0}, 0, 0},
#define HA_built 2
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
	double copper_area; /* in mm^2 */
	unsigned long int lines, arcs, polys, elements;
} layer_stat_t;

static void stat_do_export(pcb_hid_attr_val_t * options)
{
	FILE *f;
	const char *filename;
	int i, lid, lgid;
	char buff[1024];
	time_t t;
	layer_stat_t ls, *lgs, lgss[PCB_MAX_LAYER];
	int phg, hp, hup, group_not_empty[PCB_MAX_LAYER];

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
	t = time(NULL);
	strftime(buff, sizeof(buff), "%y-%d-%m %H:%M:%S", localtime(&t));

	fprintf(f, "ha:pcb-rnd-board-stats-v1 {\n");
	fprintf(f, "	ha:meta {\n");
	fprintf(f, "		date=%s\n", buff);
	fprintf(f, "		built=%d\n", stat_values[HA_built].int_value);
	fprintf(f, "		orig_rnd=%s\n", (stat_values[HA_orig].int_value ? "yes" : "no"));
	fprintf(f, "	}\n");

	fprintf(f, "	li:logical_layers {\n");
	for(lid = 0; lid < pcb_max_copper_layer; lid++) {
		pcb_layer_t *l = PCB->Data->Layer+lid;
		int empty = IsLayerEmpty(l);
		lgid = GetLayerGroupNumberByNumber(lid);
		lgs = lgss + lgid;

		fprintf(f, "		ha:layer_%d {\n", lid);
		fprintf(f, "			name={%s}\n", l->Name);
		fprintf(f, "			empty=%s\n", empty ? "yes" : "no");
		fprintf(f, "			grp=%d\n", lgid);

		memset(&ls, 0, sizeof(ls));

		PCB_LINE_LOOP(l) {
			lgs->lines++;
			ls.lines++;
		}
		PCB_END_LOOP;

		PCB_ARC_LOOP(l) {
			lgs->arcs++;
			ls.arcs++;
		}
		PCB_END_LOOP;

		PCB_POLY_LOOP(l) {
			lgs->polys++;
			ls.polys++;
		}
		PCB_END_LOOP;

		if (!empty)
			group_not_empty[lgid] = 1;

		fprintf(f, "			lines=%lu\n", ls.lines);
		fprintf(f, "			arcs=%lu\n",  ls.arcs);
		fprintf(f, "			polys=%lu\n", ls.polys);

		fprintf(f, "		}\n");
	}
	fprintf(f, "	}\n");

	phg = 0;
	fprintf(f, "	li:physical_layers {\n");
	for(lgid = 0; lgid < pcb_max_copper_layer; lgid++) {
		if (group_not_empty[lgid]) {
			phg++;
			fprintf(f, "		ha:layergroup_%d {\n", lgid);
			fprintf(f, "			lines=%lu\n", lgss[lgid].lines);
			fprintf(f, "			arcs=%lu\n",  lgss[lgid].arcs);
			fprintf(f, "			polys=%lu\n", lgss[lgid].polys);
			fprintf(f, "		}\n");
		}
	}
	fprintf(f, "	}\n");

	fprintf(f, "	ha:board {\n");
	pcb_fprintf(f, "		width=%$mm\n", PCB->MaxWidth);
	pcb_fprintf(f, "		height=%$mm\n", PCB->MaxHeight);
	fprintf(f, "		gross_area={%.4f mm^2}\n", (double)PCB_COORD_TO_MM(PCB->MaxWidth) * (double)PCB_COORD_TO_MM(PCB->MaxWidth));
	fprintf(f, "		holes_plated=%d\n", hp);
	fprintf(f, "		holes_unplated=%d\n", hup);
	fprintf(f, "		physical_copper_layers=%d\n", phg);
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

pcb_uninit_t hid_export_stat_init()
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

	pcb_hid_register_hid(&stat_hid);

	return NULL;
}

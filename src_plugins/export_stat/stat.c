/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include <genht/htsi.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "netlist2.h"
#include "plugins.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "plug_io.h"
#include "safe_fs.h"
#include "obj_pstk_inlines.h"

#include "hid.h"
#include "hid_nogui.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_cam.h"


static pcb_hid_t stat_hid;

const char *stat_cookie = "stat HID";

pcb_export_opt_t stat_attribute_list[] = {
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

static pcb_export_opt_t *stat_get_export_options(pcb_hid_t *hid, int *n)
{
	const char *suffix = ".stat.lht";

	if ((PCB != NULL)  && (stat_attribute_list[HA_statfile].default_val.str == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &stat_attribute_list[HA_statfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return stat_attribute_list;
}

typedef struct layer_stat_s {
	pcb_coord_t trace_len;
	double copper_area;
	unsigned long int lines, arcs, polys, elements;
} layer_stat_t;

static void stat_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	FILE *f;
	const char *filename;
	int i, lid;
	pcb_layergrp_id_t lgid;
	char buff[1024];
	layer_stat_t ls, *lgs, lgss[PCB_MAX_LAYERGRP];
	int nl, phg, hp, hup, group_not_empty[PCB_MAX_LAYERGRP];
	pcb_cardinal_t num_etop = 0, num_ebottom = 0, num_esmd = 0, num_epads = 0, num_epins = 0, num_terms = 0, num_slots = 0;
	pcb_coord_t width, height;

	memset(lgss, 0, sizeof(lgss));
	memset(group_not_empty, 0, sizeof(group_not_empty));

	if (!options) {
		stat_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			stat_values[i] = stat_attribute_list[i].default_val;
		options = stat_values;
	}

	filename = options[HA_statfile].str;
	if (!filename)
		filename = "pcb.stat.lht";

	f = pcb_fopen(&PCB->hidlib, filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	pcb_board_count_holes(PCB, &hp, &hup, NULL);
	pcb_print_utc(buff, sizeof(buff), 0);

	fprintf(f, "ha:pcb-rnd-board-stats-v2 {\n");
	fprintf(f, "	ha:meta {\n");
	fprintf(f, "		date={%s}\n", buff);
	fprintf(f, "		built=%ld\n", options[HA_built].lng);
	fprintf(f, "		lht_built=%s\n", (options[HA_lht_built].lng ? "yes" : "no"));
	fprintf(f, "		orig_rnd=%s\n", (options[HA_orig].lng ? "yes" : "no"));
	fprintf(f, "		first_ver=%s\n", options[HA_first_ver].str);
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
		fprintf(f, "			name={%s}\n", l->name);
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
		htsp_entry_t *e;
		pcb_cardinal_t terms = 0, best_terms = 0;
		fprintf(f, "		ha:%s {\n", pcb_netlist_names[nl]);

		for(e = htsp_first(&PCB->netlist[nl]); e != NULL; e = htsp_next(&PCB->netlist[nl], e)) {
			pcb_net_t *net = e->value;
			long numt = pcb_termlist_length(&net->conns);

			terms += numt;
			if (numt > best_terms)
				best_terms = numt;
		}
		fprintf(f, "			nets=%ld\n", (long int)PCB->netlist[nl].used);
		fprintf(f, "			terminals=%ld\n", (long int)terms);
		fprintf(f, "			max_term_per_net=%ld\n", (long int)best_terms);
		fprintf(f, "		}\n");
	}
	fprintf(f, "	}\n");

	PCB_SUBC_LOOP(PCB->Data) {
		int bott;
		pcb_cardinal_t slot = 0, hole = 0, all = 0;
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		htsi_t t;
		htsi_entry_t *e;

		if (pcb_subc_get_side(subc, &bott) == 0) {
			if (bott)
				num_ebottom++;
			else
				num_etop++;
		}

		/* count each terminal ID only once, because of heavy terminals */
		htsi_init(&t, strhash, strkeyeq);
		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			if (o->term != NULL) {
				htsi_set(&t, (char *)o->term, 1);
				if (o->type == PCB_OBJ_PSTK) {
					pcb_pstk_proto_t *proto = pcb_pstk_get_proto((pcb_pstk_t *)o);
					if ((proto != NULL) && (proto->hdia > 0))
						hole++;
					if ((proto != NULL) && (proto->mech_idx >= 0)) {
						hole++;
						slot++;
						num_slots++;
					}
				}
			}
		}
	
		for (e = htsi_first(&t); e != NULL; e = htsi_next(&t, e)) {
			num_terms++;
			all++;
		}

		/* a part is considered smd if it has at most half as many holes as terminals total */
		if ((hole*2 + slot*2) < all)
			num_esmd++;

		htsi_uninit(&t);
	}
	PCB_END_LOOP;

	if (pcb_has_explicit_outline(PCB)) {
		pcb_box_t bb;
		pcb_data_bbox_naked(&bb, PCB->Data, pcb_true);
		width = bb.X2 - bb.X1;
		height = bb.Y2 - bb.Y1;
	}
	else {
		width = PCB->hidlib.size_x;
		height = PCB->hidlib.size_y;
	}

	fprintf(f, "	ha:board {\n");
	fprintf(f, "		id={%s}\n", options[HA_board_id].str == NULL ? "" : options[HA_board_id].str);
	fprintf(f, "		license={%s}\n", options[HA_license].str);
	fprintf(f, "		format={%s}\n", PCB->Data->loader == NULL ? "unknown" : PCB->Data->loader->description);
	pcb_fprintf(f, "		width=%$mm\n", width);
	pcb_fprintf(f, "		height=%$mm\n", height);
	fprintf(f, "		gross_area={%.4f mm^2}\n", (double)PCB_COORD_TO_MM(width) * (double)PCB_COORD_TO_MM(height));
	fprintf(f, "		holes_plated=%d\n", hp);
	fprintf(f, "		holes_unplated=%d\n", hup);
	fprintf(f, "		physical_copper_layers=%d\n", phg);
	fprintf(f, "		ha:subcircuits {\n");
	fprintf(f, "			total=%ld\n", (long int)num_ebottom + num_etop);
	fprintf(f, "			top_side=%ld\n", (long int)num_etop);
	fprintf(f, "			bottom_side=%ld\n", (long int)num_ebottom);
	fprintf(f, "			smd=%ld\n", (long int)num_esmd);
	fprintf(f, "			pads=%ld\n", (long int)num_epads);
	fprintf(f, "			pins=%ld\n", (long int)num_epins);
	fprintf(f, "			terms=%ld\n", (long int)num_terms);
	fprintf(f, "			slots=%ld\n", (long int)num_slots);
	fprintf(f, "		}\n");
	fprintf(f, "	}\n");

	fprintf(f, "}\n");
	fclose(f);
}

static int stat_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_hid_register_attributes(stat_attribute_list, sizeof(stat_attribute_list) / sizeof(stat_attribute_list[0]), stat_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}

static int stat_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nstat exporter command line arguments:\n\n");
	pcb_hid_usage(stat_attribute_list, sizeof(stat_attribute_list) / sizeof(stat_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x stat [stat options] foo.pcb\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_stat(int ver_needed) { return 0; }

void pplg_uninit_export_stat(void)
{
	free((char *)stat_attribute_list[HA_first_ver].default_val.str);
	free((char *)stat_attribute_list[HA_license].default_val.str);
	stat_attribute_list[HA_first_ver].default_val.str = NULL;
	stat_attribute_list[HA_license].default_val.str = NULL;
	pcb_hid_remove_attributes_by_cookie(stat_cookie);
}

int pplg_init_export_stat(void)
{
	PCB_API_CHK_VER;

	memset(&stat_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&stat_hid);

	stat_hid.struct_size = sizeof(pcb_hid_t);
	stat_hid.name = "stat";
	stat_hid.description = "board statistics";
	stat_hid.exporter = 1;

	stat_hid.get_export_options = stat_get_export_options;
	stat_hid.do_export = stat_do_export;
	stat_hid.parse_arguments = stat_parse_arguments;

	stat_hid.usage = stat_usage;

	stat_attribute_list[HA_first_ver].default_val.str = pcb_strdup(PCB_VERSION);
	stat_attribute_list[HA_license].default_val.str = pcb_strdup("proprietary/private");

	pcb_hid_register_hid(&stat_hid);

	return 0;
}

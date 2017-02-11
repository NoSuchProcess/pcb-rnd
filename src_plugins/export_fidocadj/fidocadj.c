 /*
  *                            COPYRIGHT
  *
  *  PCB, interactive printed circuit board design
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


static pcb_hid_t fidocadj_hid;

const char *fidocadj_cookie = "fidocadj HID";

pcb_hid_attribute_t fidocadj_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Output file name",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_fidocadjfile 0
};

#define NUM_OPTIONS (sizeof(fidocadj_attribute_list)/sizeof(fidocadj_attribute_list[0]))

PCB_REGISTER_ATTRIBUTES(fidocadj_attribute_list, fidocadj_cookie)

static pcb_hid_attr_val_t fidocadj_values[NUM_OPTIONS];

static pcb_hid_attribute_t *fidocadj_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".fcd";

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &fidocadj_attribute_list[HA_fidocadjfile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return fidocadj_attribute_list;
}

static long int crd(pcb_coord_t c)
{
	return pcb_round((double)PCB_COORD_TO_MIL(c) * 200.0);
}

static int layer_map(unsigned int lflg, int *fidoly_next, int *warned, const char *lyname)
{
	if (lflg & PCB_LYT_COPPER) {
		if (lflg & PCB_LYT_BOTTOM)
			return 1;
		if (lflg & PCB_LYT_TOP)
			return 2;
	}
	if (lflg & PCB_LYT_SILK)
		return 3;

	(*fidoly_next)++;
	if (*fidoly_next > 15) {
		if (!*warned)
			pcb_message(PCB_MSG_ERROR, "FidoCadJ can't handle this many layers - layer %s is not exported\n", lyname);
		*warned = 1;
		return -1;
	}
	return *fidoly_next;
}



static void fidocadj_do_export(pcb_hid_attr_val_t * options)
{
	FILE *f;
	const char *filename;
	int n, fidoly_next;
	pcb_layer_id_t lid;
	int layer_warned = 0, hole_warned = 0;

	if (!options) {
		fidocadj_get_export_options(0);
		for (n = 0; n < NUM_OPTIONS; n++)
			fidocadj_values[n] = fidocadj_attribute_list[n].default_val;
		options = fidocadj_values;
	}

	filename = options[HA_fidocadjfile].str_value;
	if (!filename)
		filename = "pcb-rnd-default.fcd";

	f = fopen(filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	fprintf(f, "[FIDOCAD]\n");

	fidoly_next = 3;
	for(lid = 0; lid < pcb_max_layer; lid++) {
		pcb_layer_t *ly = PCB->Data->Layer+lid;
		unsigned int lflg = pcb_layer_flags(lid);
		int fidoly = layer_map(lflg, &fidoly_next, &layer_warned, ly->Name);

		if (fidoly < 0)
			continue;

		PCB_LINE_LOOP(ly) {
			fprintf(f, "PL %ld %ld %ld %ld %ld %d\n",
				crd(line->Point1.X), crd(line->Point1.Y),
				crd(line->Point2.X), crd(line->Point2.Y),
				crd(line->Thickness), fidoly);
		}
		PCB_END_LOOP;

		PCB_ARC_LOOP(ly) {
#warning TODO: fprintf() some curve using arc->*
			;
		}
		PCB_END_LOOP;

		PCB_POLY_LOOP(ly) {
			pcb_vnode_t *v;
			pcb_pline_t *pl = polygon->Clipped->contours;

			if (polygon->HoleIndexN > 0) {
				if (!hole_warned)
					pcb_message(PCB_MSG_ERROR, "FidoCadJ can't handle holes in polygons, ignoring holes for this export - some of the polygons will look different\n");
				hole_warned = 1;
			}

			fprintf(f, "PP %ld %ld", crd(pl->head.point[0]), crd(pl->head.point[1]));
			v = pl->head.next;
			do {
				fprintf(f, " %ld %ld", crd(v->point[0]), crd(v->point[1]));
			} while ((v = v->next) != pl->head.next);
			fprintf(f, " %d\n", fidoly);
		}
		PCB_END_LOOP;

		PCB_TEXT_LOOP(ly) {
			char *s;
			fprintf(f, "TY %ld %ld %ld %ld %x 0 %d * ",
				crd(text->X), crd(text->X),
				crd(text->BoundingBox.X2 - text->BoundingBox.X1), crd(text->BoundingBox.Y2 - text->BoundingBox.Y1),
				text->Direction * 360 / 4, fidoly);
			for(s = text->TextString; *s != '\0'; s++) {
				if (*s == ' ')
					fprintf(f, "++");
				else
					fputc(*s, f);
			}
			fputc('\n', f);
		}
		PCB_END_LOOP;


	}
	fclose(f);
}

static void fidocadj_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(fidocadj_attribute_list, sizeof(fidocadj_attribute_list) / sizeof(fidocadj_attribute_list[0]), fidocadj_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

static int fidocadj_usage(const char *topic)
{
	fprintf(stderr, "\nfidocadj exporter command line arguments:\n\n");
	pcb_hid_usage(fidocadj_attribute_list, sizeof(fidocadj_attribute_list) / sizeof(fidocadj_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x fidocadj foo.pcb [fidocadj options]\n\n");
	return 0;
}

#include "dolists.h"

pcb_uninit_t hid_export_fidocadj_init()
{
	memset(&fidocadj_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&fidocadj_hid);
	pcb_dhlp_draw_helpers_init(&fidocadj_hid);

	fidocadj_hid.struct_size = sizeof(pcb_hid_t);
	fidocadj_hid.name = "fidocadj";
	fidocadj_hid.description = "export board in FidoCadJ .fcd format";
	fidocadj_hid.exporter = 1;

	fidocadj_hid.get_export_options = fidocadj_get_export_options;
	fidocadj_hid.do_export = fidocadj_do_export;
	fidocadj_hid.parse_arguments = fidocadj_parse_arguments;

	fidocadj_hid.usage = fidocadj_usage;

	pcb_hid_register_hid(&fidocadj_hid);

	return NULL;
}

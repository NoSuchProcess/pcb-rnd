/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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

#include "genht/htsi.h"

#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "layer_vis.h"
#include "plugins.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "plug_io.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_nogui.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_cam.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"

static pcb_hid_t fidocadj_hid;

const char *fidocadj_cookie = "fidocadj HID";

pcb_export_opt_t fidocadj_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Output file name",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_fidocadjfile 0

	{"libfile", "path to PCB.fcl",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_libfile 1
};

#define NUM_OPTIONS (sizeof(fidocadj_attribute_list)/sizeof(fidocadj_attribute_list[0]))

PCB_REGISTER_ATTRIBUTES(fidocadj_attribute_list, fidocadj_cookie)

static pcb_hid_attr_val_t fidocadj_values[NUM_OPTIONS];

static pcb_export_opt_t *fidocadj_get_export_options(pcb_hid_t *hid, int *n)
{
	const char *suffix = ".fcd";

	if ((PCB != NULL)  && (fidocadj_attribute_list[HA_fidocadjfile].default_val.str_value == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &fidocadj_attribute_list[HA_fidocadjfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return fidocadj_attribute_list;
}

/* index PCB.fcl so we have a list of known fidocadj footprints */
static int load_lib(htsi_t *ht, const char *fn)
{
	FILE *f;
	char line[1024];
	f = pcb_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open fidocadj PCB library file '%s' for read\n", fn);
		return -1;
	}
	*line = '\0';
	fgets(line, sizeof(line), f);
	if (pcb_strncasecmp(line, "[FIDOLIB PCB Footprints]", 24) != 0) {
		pcb_message(PCB_MSG_ERROR, "'%s' doesn't have the fidocadj lib header\n", fn);
		fclose(f);
		return -1;
	}
	while(fgets(line, sizeof(line), f) != NULL) {
		char *end, *s = line;

		if (*line != '[')
			continue;

		s++;
		end = strchr(s, ' ');
		if (end != NULL) {
			*end = '\0';
			htsi_set(ht, pcb_strdup(s), 1);
		}
	}
	fclose(f);
	return 0;
}

static long int crd(pcb_coord_t c)
{
	return pcb_round((double)PCB_COORD_TO_MIL(c) / 5.0);
}

static int layer_map(unsigned int lflg, int *fidoly_next, const char *lyname)
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
		char *msg = pcb_strdup_printf("FidoCadJ can't handle this many layers - layer %s is not exported\n", lyname);
		pcb_io_incompat_save(NULL, NULL, "layer", msg, NULL);
		free(msg);
		return -1;
	}
	return *fidoly_next;
}

static void write_custom_subc(FILE *f, pcb_subc_t *sc)
{
	char *msg = pcb_strdup_printf("Can't export custom footprint for %s yet\n", sc->refdes);
	pcb_io_incompat_save(sc->parent.data, (pcb_any_obj_t *)sc, msg, "subc", "subcircuit omitted - add the footprint type on the footprint list!");
	free(msg);
}

static void fidocadj_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	FILE *f;
	const char *filename, *libfile;
	int n, fidoly_next, have_lib;
	pcb_layer_id_t lid;
	htsi_t lib_names; /* hash of names found in the library, if have_lib is 1 */

	if (!options) {
		fidocadj_get_export_options(hid, 0);
		for (n = 0; n < NUM_OPTIONS; n++)
			fidocadj_values[n] = fidocadj_attribute_list[n].default_val;
		options = fidocadj_values;
	}

	libfile = options[HA_libfile].str_value;
	if ((libfile != NULL) && (*libfile != '\0')) {
		htsi_init(&lib_names, strhash, strkeyeq);
		have_lib = 1;
		if (load_lib(&lib_names, libfile) != 0)
			goto free_lib;
	}
	else
		have_lib = 0;

	filename = options[HA_fidocadjfile].str_value;
	if (!filename)
		filename = "pcb-rnd-default.fcd";

	f = pcb_fopen(&PCB->hidlib, filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	fprintf(f, "[FIDOCAD]\n");

	fidoly_next = 3;
	for(lid = 0; lid < pcb_max_layer; lid++) {
		pcb_layer_t *ly = PCB->Data->Layer+lid;
		unsigned int lflg = pcb_layer_flags(PCB, lid);
		int fidoly = layer_map(lflg, &fidoly_next, ly->name);

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
TODO(": fprintf() some curve using arc->*")
			;
		}
		PCB_END_LOOP;

		PCB_POLY_LOOP(ly) {
			pcb_vnode_t *v;
			pcb_pline_t *pl = polygon->Clipped->contours;

			if (polygon->HoleIndexN > 0) {
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)polygon, "polygon", "FidoCadJ can't handle holes in polygons, ignoring holes for this export", "(some of the polygons will look different unless you remove the holes or split up the polygons)");
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
			/* default_font 'm' = 4189 wide
			 * default_font 'l', starts at y = 1000
			 * default_font 'p','q', finish at y = 6333
			 * default_font stroke thickness = 800
			 * giving sx = (4189+800)/(5333+800) ~= 0.813
			 */
			pcb_coord_t x0 = text->X;
			/*pcb_coord_t sx = text->BoundingBox.X2 - text->BoundingBox.X1; unused */
			pcb_coord_t y0 = text->Y;
			/* pcb_coord_t sy = text->BoundingBox.Y2 - text->BoundingBox.Y1; unused */
			pcb_coord_t glyphy = 5789*(text->Scale); /* (ascent+descent)*Scale */
			pcb_coord_t glyphx = 813*(glyphy/1000); /* based on 'm' glyph dimensions */
			glyphy = 10*glyphy/7;  /* empirically determined */
			glyphx = 10*glyphx/7;  /* scaling voodoo */
			/*switch(text->Direction) {
				case 0:
					x0 += sx;
					break;
				case 1: 
					y0 += sy;
					break;
				case 2: 
					x0 -= sx;
					break;
				case 3: 
					y0 -= sy;
					break;
			}*/
TODO("textrot: can we exprot rotation with %f?")
			fprintf(f, "TY %ld %ld %ld %ld %d 1 %d * ", /* 1 = bold */
				crd(x0), crd(y0), crd(glyphy), crd(glyphx),
				(int)pcb_round(text->rot), fidoly);
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

	PCB_PADSTACK_LOOP(PCB->Data) {
		int oshape;
		pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask;
		pcb_pstk_compshape_t cshape;
		pcb_bool plated;
		if (!pcb_pstk_export_compat_via(padstack, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
			pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "padstack-uniformity", "can't export non-uniform padstacl", "use a simpler padstack - omiting this one from the export");
			continue;
		}
		switch(cshape) {
			case PCB_PSTK_COMPAT_OCTAGON:
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "padstack-shape", "can't export octagonal shape", "use round or square instead; (fallback to round for now)");
				/* fall-through */
			case PCB_PSTK_COMPAT_ROUND:  oshape = 0; break;
			case PCB_PSTK_COMPAT_SQUARE: oshape = 1; break;  /* rectangle with sharp corners */
			default:
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "padstack-shape", "can't export shaped padstack", "use round or square instead; (fallback to round for now)");
				oshape = 0;
		}
		fprintf(f, "pa %ld %ld %ld %ld %ld %d\n", crd(x), crd(y), crd(pad_dia), crd(pad_dia), crd(drill_dia), oshape);
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(PCB->Data) {
		const char *fp = pcb_attribute_get(&subc->Attributes, "footprint");
		if ((fp != NULL) && have_lib && (htsi_get(&lib_names, fp))) {
			pcb_coord_t x, y;
			double rot = 0.0;
			int on_bottom = 0;
			if (pcb_subc_get_origin(subc, &x, &y)) {
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)subc, "subc", "can't figure subcircuit origin", "omitting this part until the proper subc-aux layer objects are added");
				continue;
			}
			pcb_subc_get_rotation(subc, &rot);
			pcb_subc_get_side(subc, &on_bottom);

TODO(": figure how to store rotation")
TODO(": figure how to store side")
			fprintf(f, "MC %ld %ld %d 0 %s\n", crd(x), crd(y), 0, fp);
		}
		else
			write_custom_subc(f, subc);
	}
	PCB_END_LOOP;


	fclose(f);

	free_lib:;
	if (have_lib) {
		htsi_entry_t *e;
		for (e = htsi_first(&lib_names); e; e = htsi_next(&lib_names, e))
			free(e->key);

		htsi_uninit(&lib_names);
	}
}

static int fidocadj_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_hid_register_attributes(fidocadj_attribute_list, sizeof(fidocadj_attribute_list) / sizeof(fidocadj_attribute_list[0]), fidocadj_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}

static int fidocadj_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nfidocadj exporter command line arguments:\n\n");
	pcb_hid_usage(fidocadj_attribute_list, sizeof(fidocadj_attribute_list) / sizeof(fidocadj_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x fidocadj [fidocadj options] foo.pcb\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_fidocadj(int ver_needed) { return 0; }

void pplg_uninit_export_fidocadj(void)
{
	pcb_hid_remove_attributes_by_cookie(fidocadj_cookie);
}

int pplg_init_export_fidocadj(void)
{
	PCB_API_CHK_VER;

	memset(&fidocadj_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&fidocadj_hid);

	fidocadj_hid.struct_size = sizeof(pcb_hid_t);
	fidocadj_hid.name = "fidocadj";
	fidocadj_hid.description = "export board in FidoCadJ .fcd format";
	fidocadj_hid.exporter = 1;

	fidocadj_hid.get_export_options = fidocadj_get_export_options;
	fidocadj_hid.do_export = fidocadj_do_export;
	fidocadj_hid.parse_arguments = fidocadj_parse_arguments;

	fidocadj_hid.usage = fidocadj_usage;

	pcb_hid_register_hid(&fidocadj_hid);

	return 0;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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

#include "conf_core.h"

static void scad_insert_model(htsp_t *models, const char *name, pcb_coord_t x0, pcb_coord_t y0, double rot, int on_bottom, const char *transf, const char *param)
{
	FILE *fin;
	char *ref;

	if (!htsp_has(models, name)) {
		char buff[1024], *full_path;
		fin = pcb_fopen_first(&PCB->hidlib, &conf_core.rc.library_search_paths, name, "r", &full_path, pcb_true);

		if (fin != NULL) {
			char *s, *safe_name = pcb_strdup(name);
			for(s = safe_name; *s != '\0'; s++)
				if (!isalnum(*s))
					*s = '_';

			fprintf(f, "// Model loaded from '%s'\n", full_path);
			free(full_path);

			/* replace the module line */
			while(fgets(buff, sizeof(buff), fin) != NULL) {
				if (strstr(buff, "module") != NULL) {
					fprintf(f, "module pcb_part_%s()", safe_name);
					if (strchr(buff, '{') != NULL)
						fprintf(f, "{\n");
					else
						fprintf(f, "\n");
					break;
				}
				fprintf(f, "%s", buff);
			}
			
			/* copy the rest */
			while(fgets(buff, sizeof(buff), fin) != NULL) {
				fprintf(f, "%s", buff);
			}
			fclose(fin);
			pcb_snprintf(buff, sizeof(buff), "pcb_part_%s", safe_name);
			htsp_set(models, (char *)name, pcb_strdup(buff));
			free(safe_name);
		}
		else {
			htsp_set(models, (char *)name, NULL);
			pcb_message(PCB_MSG_WARNING, "openscad: can't find model file for %s in the footprint library\n", name);
		}
	}
	ref = htsp_get(models, (char *)name);
	if (ref != NULL) {
		char tab[] = "\t\t\t\t\t\t\t\t";
		int ind = 0;
		pcb_append_printf(&model_calls, "	translate([%mm,%mm,%c0.8])\n", x0, y0, on_bottom ? '-' : '+');
		ind++;
		tab[ind] = '\0';

		if (on_bottom) {
			pcb_append_printf(&model_calls, "	%smirror([0,0,1])\n", tab);
			tab[ind] = '\t'; ind++; tab[ind] = '\0';
		}
		if (rot != 0) {
			pcb_append_printf(&model_calls, "	%srotate([0,0,%f])\n", tab, rot);
			tab[ind] = '\t'; ind++; tab[ind] = '\0';
		}
		if (transf != NULL) {
			pcb_append_printf(&model_calls, "	%s%s\n", tab, transf);
			tab[ind] = '\t'; ind++; tab[ind] = '\0';
		}

		
		if (param != NULL)
			pcb_append_printf(&model_calls, "	%s%s(%s);\n", tab, ref, param);
		else
			pcb_append_printf(&model_calls, "	%s%s();\n", tab, ref);
	}
}

static void scad_insert_models(void)
{
	htsp_t models;
	const char *mod, *transf, *param;
	htsp_entry_t *e;

	htsp_init(&models, strhash, strkeyeq);

	PCB_SUBC_LOOP(PCB->Data); {
		mod = pcb_attribute_get(&subc->Attributes, "openscad");
		if (mod != NULL) {
			pcb_coord_t ox, oy;
			double rot = 0;
			int on_bottom = 0;
			
			if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)subc, "subc-place", "Failed to get origin of subcircuit", "fix the missing subc-aux layer");
				continue;
			}
			pcb_subc_get_rotation(subc, &rot);
			pcb_subc_get_side(subc, &on_bottom);

			transf = pcb_attribute_get(&subc->Attributes, "openscad-transformation");
			param = pcb_attribute_get(&subc->Attributes, "openscad-param");
			scad_insert_model(&models, mod, TRX_(ox), TRY_(oy), rot, on_bottom, transf, param);
		}
	} PCB_END_LOOP;

	for (e = htsp_first(&models); e; e = htsp_next(&models, e))
		free(e->value);

	htsp_uninit(&models);
}

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

static void scad_insert_model(htsp_t *models, const char *name, rnd_coord_t x0, rnd_coord_t y0, double rot, int on_bottom, const char *transf, const char *param)
{
	FILE *fin;
	char *ref;

	if (!htsp_has(models, name)) {
		char buff[1024], *full_path;
		fin = rnd_fopen_first(&PCB->hidlib, &conf_core.rc.library_search_paths, name, "r", &full_path, rnd_true);

		if (fin != NULL) {
			char *s, *safe_name = rnd_strdup(name);
			for(s = safe_name; *s != '\0'; s++)
				if (!isalnum(*s))
					*s = '_';

			fprintf(f, "// Model loaded from '%s'\n", full_path);
			free(full_path);

			/* replace the module line */
			while(fgets(buff, sizeof(buff), fin) != NULL) {
				char *mod = strstr(buff, "module"), *par;
				if (mod != NULL) {
					mod += 6;
					*mod = '\0';
					mod++;
					fprintf(f, "%s", buff);
					while(isspace(*mod)) mod++;
					if (!isalpha(*mod) && (*mod != '_'))
						rnd_message(RND_MSG_ERROR, "Openscad model '%s': module name must be in the same line as the module keyword\n", full_path);
					fprintf(f, " pcb_part_%s", safe_name);
					par = strchr(mod, '(');
					if (par != NULL)
						fprintf(f, "%s", par);
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
			rnd_snprintf(buff, sizeof(buff), "pcb_part_%s", safe_name);
			htsp_set(models, (char *)name, rnd_strdup(buff));
			free(safe_name);
		}
		else {
			htsp_set(models, (char *)name, NULL);
			rnd_message(RND_MSG_WARNING, "openscad: can't find model file for %s in the footprint library\n", name);
		}
	}
	ref = htsp_get(models, (char *)name);
	if (ref != NULL) {
		char tab[] = "\t\t\t\t\t\t\t\t";
		int ind = 0;
		rnd_append_printf(&model_calls, "	translate([%mm,%mm,%c0.8])\n", x0, y0, on_bottom ? '-' : '+');
		ind++;
		tab[ind] = '\0';

		if ((rot != 0) || on_bottom) {
			rnd_append_printf(&model_calls, "	%srotate([%d,0,%f])\n", tab, (on_bottom ? 180 : 0), rot);
			tab[ind] = '\t'; ind++; tab[ind] = '\0';
		}

		if (transf != NULL) {
			rnd_append_printf(&model_calls, "	%s%s\n", tab, transf);
			tab[ind] = '\t'; ind++; tab[ind] = '\0';
		}

		
		if (param != NULL)
			rnd_append_printf(&model_calls, "	%s%s(%s);\n", tab, ref, param);
		else
			rnd_append_printf(&model_calls, "	%s%s();\n", tab, ref);
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
			rnd_coord_t ox, oy;
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

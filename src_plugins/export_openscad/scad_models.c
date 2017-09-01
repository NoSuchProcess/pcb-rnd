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
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
  */

static void scad_insert_model(htsp_t *models, const char *name, pcb_coord_t x0, pcb_coord_t y0)
{
	FILE *fin;
	char *ref;

	if (!htsp_has(models, name)) {
		char buff[1024];
		fin = pcb_fopen(name, "r");
		if (fin != NULL) {
			char *s, *safe_name = pcb_strdup(name);
			for(s = safe_name; *s != '\0'; s++)
				if (!isalnum(*s))
					*s = '_';
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
		else
			htsp_set(models, (char *)name, NULL);
	}
	ref = htsp_get(models, (char *)name);
	if (ref != NULL)
		pcb_append_printf(&model_calls, "	translate([%mm,%mm,0.8])\n		%s();\n", x0, y0, ref);
}

static void scad_insert_models(void)
{
	htsp_t models;
	char *mod;
	htsp_entry_t *e;

	htsp_init(&models, strhash, strkeyeq);

	PCB_ELEMENT_LOOP(PCB->Data); {
		mod = pcb_attribute_get(&element->Attributes, "openscad");
		if (mod != NULL)
			scad_insert_model(&models, mod, element->MarkX, element->MarkY);
	} PCB_END_LOOP;

	for (e = htsp_first(&models); e; e = htsp_next(&models, e))
		free(e->value);

	htsp_uninit(&models);
}

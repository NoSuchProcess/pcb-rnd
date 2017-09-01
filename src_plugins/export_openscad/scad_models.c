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

	htsp_init(&models, strhash, strkeyeq);

	PCB_ELEMENT_LOOP(PCB->Data); {
		mod = pcb_attribute_get(&element->Attributes, "openscad");
		if (mod != NULL)
			scad_insert_model(&models, mod, element->MarkX, element->MarkY);
	} PCB_END_LOOP;

#warning TODO: free strings
	htsp_uninit(&models);
}

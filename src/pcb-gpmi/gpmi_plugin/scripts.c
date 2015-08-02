#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gpmi.h>
#include "src/misc.h"
#include "src/event.h"
#include "gpmi_plugin.h"
#include "scripts.h"

script_info_t *script_info = NULL;

int gpmi_hid_scripts_count()
{
	int n;
	script_info_t *i;

	for(i = script_info, n = 0; i != NULL; i = i->next, n++) ;

	return n;
}

static void script_info_add(gpmi_module *module, const char *name, const char *module_name, const char *conffile_name)
{
	script_info_t *i;

	i = malloc(sizeof(script_info_t));
	i->module = module;
	i->name = strdup(name);
	i->module_name = strdup(module_name);
	i->conffile_name = strdup(conffile_name);
	i->next = script_info;
	script_info = i;
}

static char *conf_dir = NULL;

gpmi_module *hid_gpmi_load_module(const char *module_name, const char *params, const char *config_file_name)
{
	gpmi_module *module;

	printf("Loading GPMI module %s with params %s...\n", module_name, params);
	module = gpmi_mod_load(module_name, params);
	if (module == NULL) {
		printf(" Failed. Details:\n");
		gpmi_err_stack_process_str(gpmi_hid_print_error);
	}
	gpmi_err_stack_destroy(NULL);

	script_info_add(module, params, module, config_file_name);

	return module;
}

void hid_gpmi_load_dir(const char *dir)
{
	char line[1024], *module, *params, *s, *pkg, *cfn;
	FILE *f;

	conf_dir = dir;
	cfn = Concat(dir, "/pcb-rnd-gpmi.conf", NULL);
	fprintf(stderr, "pcb-gpmi: opening config: %s\n", cfn);
	f = fopen(cfn, "r");
	if (f == NULL) {
		free(cfn);
		fprintf(stderr, " ...failed\n");
		return;
	}
	gpmi_path_insert(GPMI_PATH_PACKAGES, dir);

	while(!(feof(f))) {
		*line = '\0';
		fgets(line, sizeof(line), f);
		switch(*line) {
			case '\0':
			case '\n':
			case '\r':
			case '#':
				/* Empty line or comment */
				break;
			default:
				module = line;
				params = line + strcspn(line, "\t ");
				while((*params == ' ') || (*params == '\t')) {
					*(params) = '\0';
					params++;
				}
				s = strchr(params, '\n');
				*s = '\0';
				fprintf(stderr, " ...loading %s %s\n", module, params);
				hid_gpmi_load_module(module, params, cfn);
		}
	}

	fclose(f);
	free(cfn);
	conf_dir = NULL;
}

/* Dummy script name generator allows loading from any path */
char *gpmi_hid_asm_scriptname(const void *info, const char *file_name)
{
	char buffer[1024];

	switch(*file_name) {
		case '~':
			file_name += 2;
			if (homedir != NULL) {
				snprintf(buffer, sizeof(buffer), "%s/%s", homedir, file_name);
				fprintf(stderr, "asm_scriptname FN=%s\n", buffer);
				return strdup(buffer);
			}
			else {
				fprintf(stderr, "pcb-gpmi error: can't access $HOME for substituting ~\n");
				printf("FN=%s\n", file_name);
				return strdup(file_name);
			}
		case '/': /* full path */
			return strdup(file_name);
		default: /* relative path - must be relative to the current conf_dir */
			if ((file_name[0] == '.') && (file_name[1] == '/'))
				file_name += 2;
			snprintf(buffer, sizeof(buffer), "%s/%s", conf_dir, file_name);
			printf("FN=%s\n", buffer);
			return strdup(buffer);
	}
	return NULL;
}


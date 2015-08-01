#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gpmi.h>
#include "src/misc.h"
#include "src/event.h"

/* This function is used to print a detailed GPMI error message */
static void print_gpmi_error(gpmi_err_stack_t *entry, char *string)
{
	printf("[GPMI] %s\n", string);
}

static char *conf_dir = NULL;

void hid_gpmi_load_module(const char *module_name, const char *params)
{
	gpmi_module *module;

	printf("Loading GPMI module %s with params %s...\n", module_name, params);
	module = gpmi_mod_load(module_name, params);
	if (module == NULL) {
		printf(" Failed. Details:\n");
		gpmi_err_stack_process_str(print_gpmi_error);
	}
	gpmi_err_stack_destroy(NULL);
}

void hid_gpmi_load_dir(const char *dir)
{
	char line[1024], *module, *params, *s;
	FILE *f;

	fprintf(stderr, "pcb-gpmi: opening config: %s\n", dir);
	conf_dir = dir;
	snprintf(line, sizeof(line), "%s/pcb-gpmi.conf", dir);
	f = fopen(line, "r");
	if (f == NULL) {
		fprintf(stderr, " ...failed\n");
		return;
	}

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
				hid_gpmi_load_module(module, params);
		}
	}

	fclose(f);
	conf_dir = NULL;
}

/* Dummy script name generator allows loading from any path */
static char *asm_scriptname(const void *info, const char *file_name)
{
	char buffer[1024];
	char *home;

	switch(*file_name) {
		case '~':
			file_name += 2;
			home = getenv("HOME");
			if (home != NULL) {
				snprintf(buffer, sizeof(buffer), "%s/%s", home, file_name);
				printf("FN=%s\n", buffer);
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

static void load_cfg(void)
{
	char *home;
	hid_gpmi_load_dir (Concat (PCBLIBDIR, "/plugins/", HOST, NULL));
	hid_gpmi_load_dir (Concat (PCBLIBDIR, "/plugins", NULL));
	home = getenv("HOME");
	if (home != NULL) {
		hid_gpmi_load_dir (Concat (home, "/.pcb/plugins/", HOST, NULL));
		hid_gpmi_load_dir (Concat (home, "/.pcb/plugins", NULL));
	}
	hid_gpmi_load_dir (Concat ("plugins/", HOST, NULL));
	hid_gpmi_load_dir (Concat ("plugins", NULL));
}

static void ev_gui_init(void *user_data, int argc, event_arg_t *argv[])
{
	const char *menu[2];
	menu[0] = "Plugins";
	menu[1] = "GPMI scripting";
	menu[2] = "Scripts";
	menu[3] = NULL;

	gui->create_menu(menu, "gpmi_scripts()", "S", "Alt<Key>g", "Manage GPMI scripts");
}

void pcb_plugin_init()
{
	void **gpmi_asm_scriptname;
	gpmi_package *scripts = NULL;

	printf("pcb-gpmi hid is loaded.\n");
	gpmi_init();

	gpmi_err_stack_enable();
	if (gpmi_pkg_load("gpmi_scripts", 0, NULL, NULL, &scripts))
	{
		gpmi_err_stack_process_str(print_gpmi_error);
		abort();
	}
	gpmi_err_stack_destroy(NULL);


	gpmi_asm_scriptname = gpmi_pkg_resolve(scripts, "gpmi_scripts_asm_scriptname");
	assert(gpmi_asm_scriptname != NULL);
	*gpmi_asm_scriptname = asm_scriptname;

	event_bind(EVENT_GUI_INIT, ev_gui_init, NULL, pcb_plugin_init);
	load_cfg();
}

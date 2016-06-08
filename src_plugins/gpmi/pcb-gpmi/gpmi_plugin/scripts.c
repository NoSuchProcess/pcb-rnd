#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gpmi.h>
#include "src/misc.h"
#include "src/misc_util.h"
#include "src/event.h"
#include "src/error.h"
#include "gpmi_plugin.h"
#include "scripts.h"


#define CONFNAME "pcb-rnd-gpmi.conf"

hid_gpmi_script_info_t *hid_gpmi_script_info = NULL;

int gpmi_hid_scripts_count()
{
	int n;
	hid_gpmi_script_info_t *i;

	for(i = hid_gpmi_script_info, n = 0; i != NULL; i = i->next, n++) ;

	return n;
}

static void hid_gpmi_script_info_free(hid_gpmi_script_info_t *i)
{
	int ev;
	char *ev_args;
	ev = gpmi_event_find("ACTE_unload", &ev_args);
	if (ev >= 0)
		gpmi_event(i->module, ev, i->conffile_name);

	gpmi_mod_unload(i->module);
	free(i->name);
	free(i->module_name);
	free(i->conffile_name);
}

static hid_gpmi_script_info_t *hid_gpmi_script_info_add(hid_gpmi_script_info_t *i, gpmi_module *module, const char *name_, const char *module_name_, const char *conffile_name_)
{
	char *name, *module_name, *conffile_name;
	/* make these copies before the free()'s because of reload calling us with
	   the same pointers... */
	name = strdup(name_);
	module_name = strdup(module_name_);
	if (conffile_name_ != NULL)
		conffile_name = strdup(conffile_name_);
	else
		conffile_name = NULL;

	if (i == NULL) {
		i = malloc(sizeof(hid_gpmi_script_info_t));
		i->next = hid_gpmi_script_info;
		hid_gpmi_script_info = i;
	}
	else
		hid_gpmi_script_info_free(i);

	i->module = module;
	i->name = name;
	i->module_name = module_name;
	i->conffile_name = conffile_name;
	return i;
}

hid_gpmi_script_info_t *hid_gpmi_lookup(const char *name)
{
	hid_gpmi_script_info_t *i;
	if (name == NULL)
		return NULL;
	for(i = hid_gpmi_script_info; i != NULL; i = i->next)
		if (strcmp(name, i->name) == 0)
			return i;
	return NULL;
}

/* Unload a script and remove it from the list */
static void hid_gpmi_script_info_del(hid_gpmi_script_info_t *inf)
{
	hid_gpmi_script_info_t *i, *prev;
	prev = NULL;
	for(i = hid_gpmi_script_info; i != NULL; prev = i, i = i->next) {
		if (i == inf) {
			/* unlink */
			if (prev == NULL)
				hid_gpmi_script_info = inf->next;
			else
				prev->next = inf->next;
			hid_gpmi_script_info_free(inf);
			free(inf);
			return;
		}
	}
}

/* Unload all scripts and remove them from the list */
void hid_gpmi_script_info_uninit(void)
{
	hid_gpmi_script_info_t *i, *next;

	for(i = hid_gpmi_script_info; i != NULL; i = next) {
		next = i->next;
		hid_gpmi_script_info_free(i);
		free(i);
	}
	hid_gpmi_script_info = NULL;
}


static const char *conf_dir = NULL;

hid_gpmi_script_info_t *hid_gpmi_load_module(hid_gpmi_script_info_t *i, const char *module_name, const char *params, const char *config_file_name)
{
	gpmi_module *module;

	Message("Loading GPMI module %s with params %s...\n", module_name, params);
	module = gpmi_mod_load(module_name, params);
	if (module == NULL) {
		Message(" Failed loading the script. Details:\n");
		gpmi_err_stack_process_str(gpmi_hid_print_error);
	}
	gpmi_err_stack_destroy(NULL);

	if (module != NULL) {
		hid_gpmi_script_info_t *ri;
		int ev;

		ri = hid_gpmi_script_info_add(i, module, params, module_name, config_file_name);
		if ((ri != NULL) && (gpmi_hid_gui_inited)) {
			char *ev_args;
			/* If a script is loaded with a GUI already inited, send the event right after the load */
			ev = gpmi_event_find("ACTE_gui_init", &ev_args);
			gpmi_event(ri->module, ev, 0, NULL);
		}
		return ri;
	}

	return NULL;
}

hid_gpmi_script_info_t *hid_gpmi_reload_module(hid_gpmi_script_info_t *i)
{
	hid_gpmi_script_info_t *r;
	const char *old_cd;

	old_cd = conf_dir;

	if (i->conffile_name != NULL) {
		char *end;
		conf_dir = strdup(i->conffile_name);
		end = strrchr(conf_dir, PCB_DIR_SEPARATOR_C);
		if (end == NULL) {
			free((char *)conf_dir);
			conf_dir = NULL;
		}
		else
			*end = '\0';
	}
	else
		conf_dir = NULL;

	r = hid_gpmi_load_module(i, i->module_name, i->name, i->conffile_name);

	if (conf_dir != NULL)
		free((char *)conf_dir);
	conf_dir = old_cd;

	return r;
}

/* Read and parse gpmi config file fin;
   if fout is NULL, take cfn as config name and load all modules, return number of modules loaded;
   else write all lines to fout, but comment out the one whose second token matches cfn, return number of commented lines
   
*/

static int cfgfile(FILE *fin, FILE *fout, char *cfn)
{
char line[1024], *module, *params, *s;
	int found = 0;

	while(!(feof(fin))) {
		*line = '\0';
		fgets(line, sizeof(line), fin);
		switch(*line) {
			case '\0':
			case '\n':
			case '\r':
			case '#':
				/* Empty line or comment */
				if (fout != NULL)
					fprintf(fout, "%s", line);
				break;
			default:
				module = strdup(line);
				params = module + strcspn(module, "\t ");
				while((*params == ' ') || (*params == '\t')) {
					*(params) = '\0';
					params++;
				}
				s = strchr(params, '\n');
				*s = '\0';
				if (fout == NULL) {
					fprintf(stderr, " ...loading %s %s\n", module, params);
					hid_gpmi_load_module(NULL, module, params, cfn);
					found++;
				}
				else {
					if (strcmp(params, cfn) == 0) {
						fprintf(fout, "# removed from pcb-rnd GUI: ");
						found++;
					}
					if (fout != NULL)
						fprintf(fout, "%s", line);
				}
				free(module);
		}
	}

	return found;
}

void hid_gpmi_load_dir(const char *dir, int add_pkg_path)
{
	FILE *f;
	char *cfn;

	conf_dir = dir;
	cfn = Concat(dir, PCB_DIR_SEPARATOR_S, CONFNAME,  NULL);
#ifdef CONFIG_DEBUG
	fprintf(stderr, "pcb-gpmi: opening config: %s\n", cfn);
#endif
	f = fopen(cfn, "r");
	if (f == NULL) {
		free(cfn);
#ifdef CONFIG_DEBUG
		fprintf(stderr, " ...failed\n");
#endif
		return;
	}

	if (add_pkg_path)
		gpmi_path_insert(GPMI_PATH_PACKAGES, dir);

	cfgfile(f, NULL, cfn);

	fclose(f);
	free(cfn);
	conf_dir = NULL;
}

/* Dummy script name generator allows loading from any path */
char *gpmi_hid_asm_scriptname(const void *info, const char *file_name)
{
	char buffer[1024];
	const char *cd;

	switch(*file_name) {
		case '~':
			file_name += 2;
			if (homedir != NULL) {
				snprintf(buffer, sizeof(buffer), "%s%c%s", homedir, PCB_DIR_SEPARATOR_C, file_name);
				fprintf(stderr, "asm_scriptname FN=%s\n", buffer);
				return strdup(buffer);
			}
			else {
				fprintf(stderr, "pcb-gpmi error: can't access $HOME for substituting ~\n");
#ifdef CONFIG_DEBUG
				printf("FN=%s\n", file_name);
#endif
				return strdup(file_name);
			}
		case PCB_DIR_SEPARATOR_C: /* full path */
			return strdup(file_name);
		default: /* relative path - must be relative to the current conf_dir */
			if ((file_name[0] == '.') && (file_name[1] == PCB_DIR_SEPARATOR_C))
				file_name += 2;
			if (conf_dir == NULL)
				cd = ".";
			else
				cd = conf_dir;
			snprintf(buffer, sizeof(buffer), "%s%c%s", cd, PCB_DIR_SEPARATOR_C, file_name);
#ifdef CONFIG_DEBUG
			printf("FN=%s\n", buffer);
#endif
			return strdup(buffer);
	}
	return NULL;
}

int gpmi_hid_script_unload(hid_gpmi_script_info_t *i)
{
	hid_gpmi_script_info_del(i);
	return 0;
}

int gpmi_hid_script_remove(hid_gpmi_script_info_t *i)
{
	FILE *fin, *fout;
	char *tmpfn;
	int res;

	if (i->conffile_name == NULL) {
		Message("gpmi_hid_script_remove(): can't remove script from configs, the script is not loaded from a config.\n");
		return -1;
	}

	fin = fopen(i->conffile_name, "r");
	if (fin == NULL) {
		Message("gpmi_hid_script_remove(): can't remove script from configs, can't open %s for read.\n", i->conffile_name);
		return -1;
	}
	tmpfn = Concat(i->conffile_name, ".tmp", NULL);
	fout = fopen(tmpfn, "w");
	if (fout == NULL) {
		Message("gpmi_hid_script_remove(): can't remove script from configs, can't create %s.\n", tmpfn);
		fclose(fin);
		free(tmpfn);
		return -1;
	}

	res = cfgfile(fin, fout, i->name);

	fclose(fin);
	fclose(fout);

	if (res < 1) {
		Message("gpmi_hid_script_remove(): can't remove script from configs, can't find the correspondign config line in %s\n", i->conffile_name);
		free(tmpfn);
		return -1;
	}

	if (rename(tmpfn, i->conffile_name) != 0) {
		Message("gpmi_hid_script_remove(): can't remove script from configs, can't move %s to %s.\n", tmpfn, i->conffile_name);
		free(tmpfn);
		return -1;
	}

	free(tmpfn);
	return 0;
}

int gpmi_hid_script_addcfg(hid_gpmi_script_info_t *i)
{
	char *fn, *home;
	FILE *f;

	home = getenv ("PCB_RND_GPMI_HOME");
	if (home == NULL)
		home = homedir;

	if (homedir != NULL) {
		fn = Concat(home, PCB_DIR_SEPARATOR_S ".pcb", NULL);
		mkdir(fn, 0755);
		free(fn);

		fn = Concat(home, PCB_DIR_SEPARATOR_S ".pcb" PCB_DIR_SEPARATOR_S "plugins", NULL);
		mkdir(fn, 0755);
		free(fn);
		
		fn = Concat(home, PCB_DIR_SEPARATOR_S ".pcb" PCB_DIR_SEPARATOR_S "plugins" PCB_DIR_SEPARATOR_S, CONFNAME, NULL);
	}
	else
		fn = Concat("plugins" PCB_DIR_SEPARATOR_S, CONFNAME, NULL);

		f = fopen(fn, "a");
	if (f == NULL) {
		Message("gpmi_hid_script_addcfg: can't open %s for write\n", fn);
		return -1;
	}

	fprintf(f, "\n%s\t%s\n", i->module_name, i->name);
	fclose(f);

	free(fn);
	return 0;
}

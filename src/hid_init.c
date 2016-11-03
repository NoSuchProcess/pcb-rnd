#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "global.h"
#include "hid.h"
#include "hid_nogui.h"

/* for dlopen() and friends; will also solve all system-dependent includes
   and provides a dl-compat layer on windows. Also solves the opendir related
   includes. */
#include "compat_dl.h"

#include "global.h"
#include "misc.h"
#include "plugins.h"
#include "hid_attrib.h"
#include "hid_flags.h"
#include "misc_util.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "compat_inc.h"
#include "fptr_cast.h"

HID **hid_list = 0;
int hid_num_hids = 0;

HID *gui = NULL;
HID *next_gui = NULL;
HID *exporter = NULL;

int pixel_slop = 1;

static void hid_load_dir(char *dirname)
{
	DIR *dir;
	struct dirent *de;

	dir = opendir(dirname);
	if (!dir) {
		free(dirname);
		return;
	}
	while ((de = readdir(dir)) != NULL) {
		void *sym;
		pcb_uninit_t (*symv) ();
		pcb_uninit_t uninit;
		void *so;
		char *basename, *path, *symname;
		struct stat st;

		basename = pcb_strdup(de->d_name);
		if (strlen(basename) > 3 && strcasecmp(basename + strlen(basename) - 3, ".so") == 0)
			basename[strlen(basename) - 3] = 0;
		else if (strlen(basename) > 4 && strcasecmp(basename + strlen(basename) - 4, ".dll") == 0)
			basename[strlen(basename) - 4] = 0;
		path = Concat(dirname, PCB_DIR_SEPARATOR_S, de->d_name, NULL);

		if (stat(path, &st) == 0 && (
/* mingw and win32 do not support S_IXGRP or S_IXOTH */
#if defined(S_IXGRP)
																	(st.st_mode & S_IXGRP) ||
#endif
#if defined(S_IXOTH)
																	(st.st_mode & S_IXOTH) ||
#endif
																	(st.st_mode & S_IXUSR))
				&& S_ISREG(st.st_mode)) {
			if ((so = dlopen(path, RTLD_NOW | RTLD_GLOBAL)) == NULL) {
				fprintf(stderr, "dl_error: %s\n", dlerror());
			}
			else {
				plugin_info_t *inf = plugin_find(basename);
				if (inf == NULL) {
					symname = Concat("hid_", basename, "_init", NULL);
					if ((sym = dlsym(so, symname)) != NULL) {
						symv = (pcb_uninit_t (*)())pcb_cast_d2f(sym);
						uninit = symv();
					}
					else if ((sym = dlsym(so, "pcb_plugin_init")) != NULL) {
						symv = (pcb_uninit_t (*)()) pcb_cast_d2f(sym);
						uninit = symv();
					}
					else
						uninit = NULL;
					plugin_register(basename, path, so, 1, uninit);
					free(symname);
				}
				else
					Message(PCB_MSG_ERROR, "Can't load %s because it'd provide plugin %s that is already loaded from %s\n", path, basename, (*inf->path == '<' ? "<buildin>" : inf->path));
			}
		}
		free(basename);
		free(path);
	}
	free(dirname);
	closedir(dir);
}

void hid_init()
{
	hid_actions_init();

	/* Setup a "nogui" default HID */
	gui = hid_nogui_get_hid();

#warning TODO: make this configurable
	hid_load_dir(Concat(conf_core.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib",
											PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
	hid_load_dir(Concat(conf_core.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib",
											PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", NULL));

	/* conf_core.rc.path.home is set by the conf_core immediately on startup */
	if (conf_core.rc.path.home != NULL) {
		hid_load_dir(Concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
		hid_load_dir(Concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", NULL));
	}
	hid_load_dir(Concat("plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
	hid_load_dir(Concat("plugins", NULL));
}

void hid_uninit(void)
{
	if (hid_num_hids > 0) {
		int i;
		for (i = hid_num_hids-1; i >= 0; i--) {
			if (hid_list[i]->uninit != NULL)
				hid_list[i]->uninit(hid_list[i]);
		}
	}
	free(hid_list);

	hid_actions_uninit();
	hid_attributes_uninit();
}

void hid_register_hid(HID * hid)
{
	int i;
	int sz = (hid_num_hids + 2) * sizeof(HID *);

	if (hid->struct_size != sizeof(HID)) {
		fprintf(stderr, "Warning: hid \"%s\" has an incompatible ABI.\n", hid->name);
		return;
	}

	for (i = 0; i < hid_num_hids; i++)
		if (hid == hid_list[i])
			return;

	hid_num_hids++;
	if (hid_list)
		hid_list = (HID **) realloc(hid_list, sz);
	else
		hid_list = (HID **) malloc(sz);

	hid_list[hid_num_hids - 1] = hid;
	hid_list[hid_num_hids] = 0;
}

void hid_remove_hid(HID * hid)
{
	int i;

	for (i = 0; i < hid_num_hids; i++) {
		if (hid == hid_list[i]) {
			hid_list[i] = hid_list[hid_num_hids - 1];
			hid_list[hid_num_hids - 1] = 0;
			hid_num_hids--;
			return;
		}
	}
}


HID *hid_find_gui(const char *preference)
{
	int i;

	if (preference != NULL) {
		for (i = 0; i < hid_num_hids; i++)
			if (!hid_list[i]->printer && !hid_list[i]->exporter && !strcmp(hid_list[i]->name, preference))
				return hid_list[i];
		return NULL;
	}

	for (i = 0; i < hid_num_hids; i++)
		if (!hid_list[i]->printer && !hid_list[i]->exporter)
			return hid_list[i];

	fprintf(stderr, "Error: No GUI available.\n");
	exit(1);
}

HID *hid_find_printer()
{
	int i;

	for (i = 0; i < hid_num_hids; i++)
		if (hid_list[i]->printer)
			return hid_list[i];

	return 0;
}

HID *hid_find_exporter(const char *which)
{
	int i;

	for (i = 0; i < hid_num_hids; i++)
		if (hid_list[i]->exporter && strcmp(which, hid_list[i]->name) == 0)
			return hid_list[i];

	fprintf(stderr, "Invalid exporter %s, available ones:", which);
	for (i = 0; i < hid_num_hids; i++)
		if (hid_list[i]->exporter)
			fprintf(stderr, " %s", hid_list[i]->name);
	fprintf(stderr, "\n");

	return 0;
}

HID **hid_enumerate()
{
	return hid_list;
}

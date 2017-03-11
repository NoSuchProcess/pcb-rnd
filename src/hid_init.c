#include "config.h"

#include "hid.h"
#include "hid_nogui.h"

/* for dlopen() and friends; will also solve all system-dependent includes
   and provides a dl-compat layer on windows. Also solves the opendir related
   includes. */
#include "compat_dl.h"

#include "plugins.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "misc_util.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "compat_inc.h"
#include "fptr_cast.h"

pcb_hid_t **pcb_hid_list = 0;
int pcb_hid_num_hids = 0;

pcb_hid_t *pcb_gui = NULL;
pcb_hid_t *pcb_next_gui = NULL;
pcb_hid_t *pcb_exporter = NULL;

int pcb_pixel_slop = 1;

pcb_plugin_dir_t *pcb_plugin_dir_first = NULL, *pcb_plugin_dir_last = NULL;

void hid_append_dir(char *dirname, int count)
{
	pcb_plugin_dir_t *pd;
	pd = malloc(sizeof(pcb_plugin_dir_t));
	pd->path = dirname;
	pd->num_plugins = count;
	pd->next = NULL;
	if (pcb_plugin_dir_first == NULL)
		pcb_plugin_dir_first = pd;
	if (pcb_plugin_dir_last != NULL)
		pcb_plugin_dir_last->next = pd;
	pcb_plugin_dir_last = pd;
}

static unsigned int plugin_hash(const char *fn)
{
	unsigned int h = 0;
	FILE *f = fopen(fn, "r");
	if (f != NULL) {
		char buff[256];
		int len = fread(buff, 1, sizeof(buff), f);
		h ^= jenhash(buff, len);
		fclose(f);
	}
	return h;
}

/* dirname must be strdup()'d on the caller's side! */
static int hid_load_dir(char *dirname)
{
	DIR *dir;
	struct dirent *de;
	int count = 0;

	dir = opendir(dirname);
	if (!dir) {
		hid_append_dir(dirname, 0);
		return 0;
	}
	while ((de = readdir(dir)) != NULL) {
		unsigned int phash;
		void *sym;
		pcb_uninit_t (*symv) ();
		pcb_uninit_t uninit;
		void *so;
		char *basename, *path, *symname;
		struct stat st;

		basename = pcb_strdup(de->d_name);
		if (strlen(basename) > 3 && pcb_strcasecmp(basename + strlen(basename) - 3, ".so") == 0)
			basename[strlen(basename) - 3] = 0;
		else if (strlen(basename) > 4 && pcb_strcasecmp(basename + strlen(basename) - 4, ".dll") == 0)
			basename[strlen(basename) - 4] = 0;
		path = pcb_concat(dirname, PCB_DIR_SEPARATOR_S, de->d_name, NULL);

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
				pcb_plugin_info_t *inf = plugin_find(basename);
				phash = plugin_hash(path);
				if (inf == NULL) {
					symname = pcb_concat("hid_", basename, "_init", NULL);
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
					inf = pcb_plugin_register(basename, path, so, 1, uninit);
					inf->hash = phash;
					count++;
					free(symname);
				}
				else if (phash != inf->hash) /* warn only if this is not the very same file in yet-another-copy */
					pcb_message(PCB_MSG_ERROR, "Can't load %s because it'd provide plugin %s that is already loaded from %s\n", path, basename, (*inf->path == '<' ? "<buildin>" : inf->path));
			}
		}
		free(basename);
		free(path);
	}
	closedir(dir);
	hid_append_dir(dirname, count);
	return count;
}

void pcb_hid_init()
{
	int found;
	pcb_hid_actions_init();

	/* Setup a "nogui" default HID */
	pcb_gui = pcb_hid_nogui_get_hid();

#warning TODO: make this configurable
	found = hid_load_dir(pcb_concat(conf_core.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib",
											PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
	found += hid_load_dir(pcb_concat(conf_core.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib",
											PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", NULL));

	/* hardwired libdir, just in case exec-prefix goes wrong (e.g. linstall) */
	hid_load_dir(pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
	hid_load_dir(pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S, "plugins", NULL));

	/* conf_core.rc.path.home is set by the conf_core immediately on startup */
	if (conf_core.rc.path.home != NULL) {
		hid_load_dir(pcb_concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
		hid_load_dir(pcb_concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", NULL));
	}
	hid_load_dir(pcb_concat("plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
	hid_load_dir(pcb_concat("plugins", NULL));
}

void pcb_hid_uninit(void)
{
	pcb_plugin_dir_t *pd, *next;

	if (pcb_hid_num_hids > 0) {
		int i;
		for (i = pcb_hid_num_hids-1; i >= 0; i--) {
			if (pcb_hid_list[i]->uninit != NULL)
				pcb_hid_list[i]->uninit(pcb_hid_list[i]);
		}
	}
	free(pcb_hid_list);

	pcb_hid_actions_uninit();
	pcb_hid_attributes_uninit();

	for(pd = pcb_plugin_dir_first; pd != NULL; pd = next) {
		next = pd->next;
		free(pd->path);
		free(pd);
	}
	pcb_plugin_dir_first = pcb_plugin_dir_last = NULL;
}

void pcb_hid_register_hid(pcb_hid_t * hid)
{
	int i;
	int sz = (pcb_hid_num_hids + 2) * sizeof(pcb_hid_t *);

	if (hid->struct_size != sizeof(pcb_hid_t)) {
		fprintf(stderr, "Warning: hid \"%s\" has an incompatible ABI.\n", hid->name);
		return;
	}

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (hid == pcb_hid_list[i])
			return;

	pcb_hid_num_hids++;
	if (pcb_hid_list)
		pcb_hid_list = (pcb_hid_t **) realloc(pcb_hid_list, sz);
	else
		pcb_hid_list = (pcb_hid_t **) malloc(sz);

	pcb_hid_list[pcb_hid_num_hids - 1] = hid;
	pcb_hid_list[pcb_hid_num_hids] = 0;
}

void pcb_hid_remove_hid(pcb_hid_t * hid)
{
	int i;

	for (i = 0; i < pcb_hid_num_hids; i++) {
		if (hid == pcb_hid_list[i]) {
			pcb_hid_list[i] = pcb_hid_list[pcb_hid_num_hids - 1];
			pcb_hid_list[pcb_hid_num_hids - 1] = 0;
			pcb_hid_num_hids--;
			return;
		}
	}
}


pcb_hid_t *pcb_hid_find_gui(const char *preference)
{
	int i;

	/* ugly hack for historical reasons: some old configs and veteran users are used to the --gui gtk option */
	if (strcmp(preference, "gtk") == 0) {
		pcb_hid_t *g;

		g = pcb_hid_find_gui("gtk2_gl");
		if (g != NULL)
			return g;

		g = pcb_hid_find_gui("gtk2_gdk");
		if (g != NULL)
			return g;

		return NULL;
	}

	/* normal search */
	if (preference != NULL) {
		for (i = 0; i < pcb_hid_num_hids; i++)
			if (!pcb_hid_list[i]->printer && !pcb_hid_list[i]->exporter && !strcmp(pcb_hid_list[i]->name, preference))
				return pcb_hid_list[i];
		return NULL;
	}

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (!pcb_hid_list[i]->printer && !pcb_hid_list[i]->exporter)
			return pcb_hid_list[i];

	fprintf(stderr, "Error: No GUI available.\n");
	exit(1);
}

pcb_hid_t *pcb_hid_find_printer()
{
	int i;

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (pcb_hid_list[i]->printer)
			return pcb_hid_list[i];

	return 0;
}

pcb_hid_t *pcb_hid_find_exporter(const char *which)
{
	int i;

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (pcb_hid_list[i]->exporter && strcmp(which, pcb_hid_list[i]->name) == 0)
			return pcb_hid_list[i];

	fprintf(stderr, "Invalid exporter %s, available ones:", which);
	for (i = 0; i < pcb_hid_num_hids; i++)
		if (pcb_hid_list[i]->exporter)
			fprintf(stderr, " %s", pcb_hid_list[i]->name);
	fprintf(stderr, "\n");

	return 0;
}

pcb_hid_t **pcb_hid_enumerate()
{
	return pcb_hid_list;
}

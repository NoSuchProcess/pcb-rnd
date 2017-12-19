#include "config.h"

#include "hid.h"
#include "hid_nogui.h"

/* for dlopen() and friends; will also solve all system-dependent includes
   and provides a dl-compat layer on windows. Also solves the opendir related
   includes. */
#include "plugins.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "misc_util.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "compat_inc.h"

pcb_hid_t **pcb_hid_list = 0;
int pcb_hid_num_hids = 0;

pcb_hid_t *pcb_gui = NULL;
pcb_hid_t *pcb_next_gui = NULL;
pcb_hid_t *pcb_exporter = NULL;

int pcb_pixel_slop = 1;

pcb_plugin_dir_t *pcb_plugin_dir_first = NULL, *pcb_plugin_dir_last = NULL;

void pcb_hid_init()
{
	char *tmp;
	pcb_hid_actions_init();

	/* Setup a "nogui" default HID */
	pcb_gui = pcb_hid_nogui_get_hid();

#warning TODO: make this configurable - use pcb_conf_cmd_is_safe() to avoid plugin injection
	tmp = pcb_concat(conf_core.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib", PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL);
	pcb_plugin_add_dir(tmp);
	free(tmp);

	tmp = pcb_concat(conf_core.rc.path.exec_prefix, PCB_DIR_SEPARATOR_S, "lib", PCB_DIR_SEPARATOR_S, "pcb-rnd", PCB_DIR_SEPARATOR_S, "plugins", NULL);
	pcb_plugin_add_dir(tmp);
	free(tmp);

	/* hardwired libdir, just in case exec-prefix goes wrong (e.g. linstall) */
	pcb_plugin_add_dir(pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
	pcb_plugin_add_dir(pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S, "plugins", NULL));

	/* conf_core.rc.path.home is set by the conf_core immediately on startup */
	if (conf_core.rc.path.home != NULL) {
		pcb_plugin_add_dir(pcb_concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
		pcb_plugin_add_dir(pcb_concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, DOT_PCB_RND, PCB_DIR_SEPARATOR_S, "plugins", NULL));
	}
	pcb_plugin_add_dir(pcb_concat("plugins", PCB_DIR_SEPARATOR_S, HOST, NULL));
	pcb_plugin_add_dir(pcb_concat("plugins", NULL));
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

	pup_uninit(&pcb_pup);

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
	if ((preference != NULL) && (strcmp(preference, "gtk") == 0)) {
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

	if (which == NULL) {
		fprintf(stderr, "Invalid exporter: need an exporter name, one of:");
		goto list;
	}

	for (i = 0; i < pcb_hid_num_hids; i++)
		if (pcb_hid_list[i]->exporter && strcmp(which, pcb_hid_list[i]->name) == 0)
			return pcb_hid_list[i];

	fprintf(stderr, "Invalid exporter %s, available ones:", which);

	list:;
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

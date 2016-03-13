#include <stdlib.h>
#include <string.h>
#include "plugins.h"

/* for the action */
#include "config.h"
#include "global.h"
#include "data.h"
#include "action.h"
#include "ds.h"

plugin_info_t *plugins = NULL;

void plugin_register(const char *name, const char *path, void *handle, int dynamic_loaded, void (*uninit)(void))
{
	plugin_info_t *i = malloc(sizeof(plugin_info_t));

	i->name = strdup(name);
	i->path = strdup(path);
	i->handle = handle;
	i->dynamic_loaded = dynamic_loaded;
	i->uninit = uninit;

	i->next = plugins;
	plugins = i;
}

void plugins_init(void)
{
}


void plugins_uninit(void)
{
	plugin_info_t *i, *next;
	for(i = plugins; i != NULL; i = next) {
		next = i->next;
		free(i->name);
		free(i->path);
		if (i->uninit != NULL)
			i->uninit();
		free(i);
	}
	plugins = NULL;
}



/* ---------------------------------------------------------------- */
static const char manageplugins_syntax[] = "ManagePlugins()\n";

static const char manageplugins_help[] = "Manage plugins dialog.";

static int ManagePlugins(int argc, char **argv, Coord x, Coord y)
{
	plugin_info_t *i;
	int nump = 0, numb = 0;
	DynamicStringType str;

	memset(&str, 0, sizeof(str));

	for (i = plugins; i != NULL; i = i->next)
		if (i->dynamic_loaded)
			nump++;
		else
			numb++;

	DSAddString(&str, "Plugins loaded:\n");
	if (nump > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (i->dynamic_loaded) {
				DSAddCharacter(&str, ' ');
				DSAddString(&str, i->name);
				DSAddCharacter(&str, ' ');
				DSAddString(&str, i->path);
				DSAddCharacter(&str, '\n');
			}
		}
	}
	else
		DSAddString(&str, " (none)\n");

	DSAddString(&str, "\n\nBuildins:\n");
	if (numb > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (!i->dynamic_loaded) {
				DSAddCharacter(&str, ' ');
				DSAddString(&str, i->name);
				DSAddCharacter(&str, '\n');
			}
		}
	}
	else
		DSAddString(&str, " (none)\n");

	DSAddString(&str, "\n\nNOTE: this is the alpha version, can only list plugins/buildins\n");
	gui->report_dialog("Manage plugins", str.Data);
	free(str.Data);
	return 0;
}


HID_Action plugins_action_list[] = {
	{"ManagePlugins", 0, ManagePlugins,
	 manageplugins_help, manageplugins_syntax}
};

REGISTER_ACTIONS(plugins_action_list)

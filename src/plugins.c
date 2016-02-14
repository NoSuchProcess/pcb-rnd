#include <stdlib.h>
#include <string.h>
#include "plugins.h"

plugin_info_t *plugins = NULL;

void plugin_register(const char *name, const char *path, void *handle, int dynamic)
{
	plugin_info_t *i = malloc(sizeof(plugin_info_t));

	i->name = strdup(name);
	i->path = strdup(path);
	i->handle = handle;
	i->dynamic = dynamic;

	i->next = plugins;
	plugins = i;
}

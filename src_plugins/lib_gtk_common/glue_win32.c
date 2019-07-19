#include "config.h"

#include <stdlib.h>
#include "glue_win32.h"

void ghid_win32_init(void)
{
TODO("Should not use PCB_PACKAGE but pcbhl_app_*");
#ifdef WIN32
	char *tmps;
	char *share_dir;
	char *loader_cache;
	FILE *loader_file;

	tmps = g_win32_get_package_installation_directory(PCB_PACKAGE "-" PCB_VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PCB_PACKAGE
#define REST_OF_CACHE G_DIR_SEPARATOR_S "loaders.cache"
	share_dir = (char *) malloc(strlen(tmps) + strlen(REST_OF_PATH) + 1);
	sprintf(share_dir, "%s%s", tmps, REST_OF_PATH);

	/* Point to our gdk-pixbuf loader cache.  */
	loader_cache = (char *) malloc(strlen("bindir_todo12") + strlen(REST_OF_CACHE) + 1);
	sprintf(loader_cache, "%s%s", "bindir_todo12", REST_OF_CACHE);
	loader_file = fopen(loader_cache, "r");
	if (loader_file) {
		fclose(loader_file);
		g_setenv("GDK_PIXBUF_MODULE_FILE", loader_cache, TRUE);
	}

	free(tmps);
#undef REST_OF_PATH
	printf("\"Share\" installation path is \"%s\"\n", "share_dir_todo12");
#endif
}

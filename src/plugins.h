typedef struct plugin_info_s plugin_info_t;

struct plugin_info_s {
	char *name;
	char *path;
	void *handle;
	int dynamic;
	plugin_info_t *next;
};

extern plugin_info_t *plugins;

void plugin_register(const char *name, const char *path, void *handle, int dynamic);

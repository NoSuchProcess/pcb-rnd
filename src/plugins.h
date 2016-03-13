typedef struct plugin_info_s plugin_info_t;

typedef void (*pcb_uninit_t)(void);

struct plugin_info_s {
	char *name;
	char *path;
	void *handle;
	int dynamic_loaded;
	pcb_uninit_t uninit;
	plugin_info_t *next;
};

extern plugin_info_t *plugins;

/* Init the plugin system */
void plugins_init(void);

/* Uninit each plugin then uninit the plugin system */
void plugins_uninit(void);

/* Register a new plugin (or building) */
void plugin_register(const char *name, const char *path, void *handle, int dynamic, pcb_uninit_t uninit);

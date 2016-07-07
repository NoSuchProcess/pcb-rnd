#ifndef PCB_CONF_HID_H
#define PCB_CONF_HID_H

#include "conf.h"

typedef struct conf_hid_callbacks_s {
	/* Called before/after a value of a config item is changed */
	void (*val_change_pre)(conf_native_t *cfg, int arr_idx);
	void (*val_change_post)(conf_native_t *cfg, int arr_idx);

	/* Called when a new config item is added to the database */
	void (*new_item_post)(conf_native_t *cfg, int arr_idx);

	/* Called during conf_hid_unreg to get hid-data cleaned up */
	void (*unreg_item)(conf_native_t *cfg, int arr_idx);
} conf_hid_callbacks_t;

typedef int conf_hid_id_t;

void *conf_hid_set_data(conf_native_t *cfg, conf_hid_id_t id, void *data);
void *conf_hid_get_data(conf_native_t *cfg, conf_hid_id_t id);

/* register a hid with a cookie; this is necessary only if:
     - the HID wants to store per-config-item hid_data with the above calls
     - the HID wants to get notified about changes in the config tree using callback functions
   NOTE: cookie is taken by pointer, the string value does not matter. One pointer
         can be registered only once.
   cb can be NULL.
   Returns a new HID id that can be used to access hid data, or -1 on error.
*/
conf_hid_id_t conf_hid_reg(const char *cookie, const conf_hid_callbacks_t *cb);

/* Unregister a hid; if unreg_item cb is specified, call it on each config item */
void conf_hid_unreg(const char *cookie);

void conf_hid_uninit(void);
#endif

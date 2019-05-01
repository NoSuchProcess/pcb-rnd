#include "config.h"
#include "conf_hid.h"
#include <genht/hash.h>
#include <genht/htpp.h>
#include "error.h"
#include "hidlib_conf.h"

typedef struct {
	const conf_hid_callbacks_t *cb;
	conf_hid_id_t id;
} conf_hid_t;

void *conf_hid_set_data(conf_native_t *cfg, conf_hid_id_t id, void *data)
{
	void **old = vtp0_get(&cfg->hid_data, id, 0);
	vtp0_set(&cfg->hid_data, id, data);
	return old == NULL ? NULL : *old;
}

void *conf_hid_get_data(conf_native_t *cfg, conf_hid_id_t id)
{
	void **old = vtp0_get(&cfg->hid_data, id, 0);
	return old == NULL ? NULL : *old;
}

const conf_hid_callbacks_t *conf_hid_set_cb(conf_native_t *cfg, conf_hid_id_t id, const conf_hid_callbacks_t *cbs)
{
	void **old;
	assert(id >= 0);
	old = vtp0_get(&cfg->hid_callbacks, id, 0);
	vtp0_set(&cfg->hid_callbacks, id, (void *)cbs);
	return (const conf_hid_callbacks_t *)(old == NULL ? NULL : *old);
}


static int conf_hid_id_next = 0;
static htpp_t *conf_hid_ids = NULL;

static void conf_pcb_hid_init(void)
{
	if (conf_hid_ids == NULL)
		conf_hid_ids = htpp_alloc(ptrhash, ptrkeyeq);
}

void conf_pcb_hid_uninit(void)
{
#ifndef NDEBUG
	if (conf_hid_ids != NULL) {
		htpp_entry_t *e;
		for(e = htpp_first(conf_hid_ids); e != NULL; e = htpp_next(conf_hid_ids, e))
			fprintf(stderr, "ERROR: conf_hid id left registered: '%s'\n", (char *)e->key);
	}
#endif

	if (conf_hid_ids != NULL) {
		htpp_free(conf_hid_ids);
		conf_hid_ids = NULL;
	}
}

conf_hid_id_t conf_hid_reg(const char *cookie, const conf_hid_callbacks_t *cb)
{
	conf_hid_t *h;

	if (cookie == NULL)
		return -1;

	conf_pcb_hid_init();
	if (htpp_getentry(conf_hid_ids, (void *)cookie) != NULL)
		return -1; /* already registered */

	h = malloc(sizeof(conf_hid_t));
	h->cb = cb;
	h->id = conf_hid_id_next++;
	htpp_set(conf_hid_ids, (void *)cookie, h);
	return h->id;
}

void conf_hid_unreg(const char *cookie)
{
	htsp_entry_t *e;
	conf_hid_t *h = htpp_pop(conf_hid_ids, (void *)cookie);

	if (h == NULL)
		return;

	/* remove local callbacks */
	conf_fields_foreach(e) {
		int len;
		conf_native_t *cfg = e->value;
		len = vtp0_len(&cfg->hid_callbacks);

		conf_hid_local_cb(cfg, -1, unreg_item);

		/* truncate the list if there are empty items at the end */
		if (len > h->id) {
			int last;
			cfg->hid_callbacks.array[h->id] = NULL;
			for(last = len-1; last >= 0; last--)
				if (cfg->hid_callbacks.array[last] != NULL)
					break;
			if (last < len)
				vtp0_truncate(&cfg->hid_callbacks, last+1);
		}
	}

	if ((h->cb != NULL) && (h->cb->unreg_item != NULL)) {
		conf_fields_foreach(e) {
			conf_native_t *cfg = e->value;
			h->cb->unreg_item(cfg, -1);
		}
	}

	free(h);
}

typedef void (*cb_t)(conf_native_t *cfg, int arr_idx);
void conf_hid_global_cb_(conf_native_t *item, int arr_idx, int offs)
{
	htpp_entry_t *e;
	if (conf_hid_ids == NULL)
		return;
	for (e = htpp_first(conf_hid_ids); e; e = htpp_next(conf_hid_ids, e)) {
		conf_hid_t *h = e->value;
		const conf_hid_callbacks_t *cbs = h->cb;
		if (cbs != NULL) {
			char *s = (char *)&cbs->val_change_pre;
			cb_t *cb = (cb_t *)(s + offs);
			if ((*cb) != NULL)
				(*cb)(item, arr_idx);
		}
	}
}


void conf_loglevel_props(enum pcb_message_level level, const char **tag, int *popup)
{
	*tag = NULL;
	*popup = 0;
	switch(level) {
		case PCB_MSG_DEBUG:   *tag = pcbhl_conf.appearance.loglevels.debug_tag; *popup = pcbhl_conf.appearance.loglevels.debug_popup; break;
		case PCB_MSG_INFO:    *tag = pcbhl_conf.appearance.loglevels.info_tag; *popup = pcbhl_conf.appearance.loglevels.info_popup; break;
		case PCB_MSG_WARNING: *tag = pcbhl_conf.appearance.loglevels.warning_tag; *popup = pcbhl_conf.appearance.loglevels.warning_popup; break;
		case PCB_MSG_ERROR:   *tag = pcbhl_conf.appearance.loglevels.error_tag; *popup = pcbhl_conf.appearance.loglevels.error_popup; break;
			break;
	}
}


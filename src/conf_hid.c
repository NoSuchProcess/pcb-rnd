#include "conf_hid.h"
#include <genht/hash.h>
#include <genht/htpp.h>

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
	void **old = vtp0_get(&cfg->hid_callbacks, id, 0);
	vtp0_set(&cfg->hid_callbacks, id, (void *)cbs);
	return (const conf_hid_callbacks_t *)(old == NULL ? NULL : *old);
}


static int conf_hid_id_next = 0;
static htpp_t *conf_hid_ids = NULL;

static int keyeq(void *a, void *b) {
	return a == b;
}


static void conf_hid_init(void)
{
	if (conf_hid_ids == NULL)
		conf_hid_ids = htpp_alloc(ptrhash, keyeq);
}

void conf_hid_uninit(void)
{
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

	conf_hid_init();
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

		conf_hid_local_cb(cfg, unreg_item);

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
			h->cb->unreg_item(cfg);
		}
	}

	free(h);
}

void conf_hid_global_cb_(conf_native_t *item, int offs)
{
	htpp_entry_t *e;
	if (conf_hid_ids == NULL)
		return;
	for (e = htpp_first(conf_hid_ids); e; e = htpp_next(conf_hid_ids, e)) {
		conf_hid_t *h = e->value;
		const conf_hid_callbacks_t *cbs = h->cb;
		if (cbs != NULL) {
			void (*cb)(conf_native_t *cfg) = (void (*)(conf_native_t *cfg)) ((char *)cbs) + offs;
			if (cb != NULL)
				cb(item);
		}
	}
}


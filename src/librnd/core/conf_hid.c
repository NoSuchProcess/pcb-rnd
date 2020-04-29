#include <librnd/config.h>
#include <librnd/core/conf_hid.h>
#include <genht/hash.h>
#include <genht/htpp.h>
#include <librnd/core/error.h>
#include <librnd/core/hidlib_conf.h>

typedef struct {
	const rnd_conf_hid_callbacks_t *cb;
	rnd_conf_hid_id_t id;
} conf_hid_t;

void *rnd_conf_hid_set_data(rnd_conf_native_t *cfg, rnd_conf_hid_id_t id, void *data)
{
	void **old = vtp0_get(&cfg->hid_data, id, 0);
	vtp0_set(&cfg->hid_data, id, data);
	return old == NULL ? NULL : *old;
}

void *rnd_conf_hid_get_data(rnd_conf_native_t *cfg, rnd_conf_hid_id_t id)
{
	void **old = vtp0_get(&cfg->hid_data, id, 0);
	return old == NULL ? NULL : *old;
}

const rnd_conf_hid_callbacks_t *rnd_conf_hid_set_cb(rnd_conf_native_t *cfg, rnd_conf_hid_id_t id, const rnd_conf_hid_callbacks_t *cbs)
{
	void **old;
	assert(id >= 0);
	old = vtp0_get(&cfg->hid_callbacks, id, 0);
	vtp0_set(&cfg->hid_callbacks, id, (void *)cbs);
	return (const rnd_conf_hid_callbacks_t *)(old == NULL ? NULL : *old);
}


static int conf_hid_id_next = 0;
static htpp_t *conf_hid_ids = NULL;

static void conf_pcb_hid_init(void)
{
	if (conf_hid_ids == NULL)
		conf_hid_ids = htpp_alloc(ptrhash, ptrkeyeq);
}

void rnd_conf_pcb_hid_uninit(void)
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

rnd_conf_hid_id_t rnd_conf_hid_reg(const char *cookie, const rnd_conf_hid_callbacks_t *cb)
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

void rnd_conf_hid_unreg(const char *cookie)
{
	htsp_entry_t *e;
	conf_hid_t *h = htpp_pop(conf_hid_ids, (void *)cookie);

	if (h == NULL)
		return;

	/* remove local callbacks */
	rnd_conf_fields_foreach(e) {
		int len;
		rnd_conf_native_t *cfg = e->value;
		len = vtp0_len(&cfg->hid_callbacks);

		rnd_conf_hid_local_cb(cfg, -1, unreg_item);

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
		rnd_conf_fields_foreach(e) {
			rnd_conf_native_t *cfg = e->value;
			h->cb->unreg_item(cfg, -1);
		}
	}

	free(h);
}

typedef void (*cbi_t)(rnd_conf_native_t *cfg, int arr_idx);
void rnd_conf_hid_global_cb_int_(rnd_conf_native_t *item, int arr_idx, int offs)
{
	htpp_entry_t *e;
	if (conf_hid_ids == NULL)
		return;
	for (e = htpp_first(conf_hid_ids); e; e = htpp_next(conf_hid_ids, e)) {
		conf_hid_t *h = e->value;
		const rnd_conf_hid_callbacks_t *cbs = h->cb;
		if (cbs != NULL) {
			char *s = (char *)&cbs->val_change_pre;
			cbi_t *cb = (cbi_t *)(s + offs);
			if ((*cb) != NULL)
				(*cb)(item, arr_idx);
		}
	}
}

typedef void (*cbp_t)(rnd_conf_native_t *cfg, void *ptr);
void rnd_conf_hid_global_cb_ptr_(rnd_conf_native_t *item, void *ptr, int offs)
{
	htpp_entry_t *e;
	if (conf_hid_ids == NULL)
		return;
	for (e = htpp_first(conf_hid_ids); e; e = htpp_next(conf_hid_ids, e)) {
		conf_hid_t *h = e->value;
		const rnd_conf_hid_callbacks_t *cbs = h->cb;
		if (cbs != NULL) {
			char *s = (char *)&cbs->val_change_pre;
			cbp_t *cb = (cbp_t *)(s + offs);
			if ((*cb) != NULL)
				(*cb)(item, ptr);
		}
	}
}


void rnd_conf_loglevel_props(rnd_message_level_t level, const char **tag, int *popup)
{
	*tag = NULL;
	*popup = 0;
	switch(level) {
		case RND_MSG_DEBUG:   *tag = pcbhl_conf.appearance.loglevels.debug_tag; *popup = pcbhl_conf.appearance.loglevels.debug_popup; break;
		case RND_MSG_INFO:    *tag = pcbhl_conf.appearance.loglevels.info_tag; *popup = pcbhl_conf.appearance.loglevels.info_popup; break;
		case RND_MSG_WARNING: *tag = pcbhl_conf.appearance.loglevels.warning_tag; *popup = pcbhl_conf.appearance.loglevels.warning_popup; break;
		case RND_MSG_ERROR:   *tag = pcbhl_conf.appearance.loglevels.error_tag; *popup = pcbhl_conf.appearance.loglevels.error_popup; break;
			break;
	}
}


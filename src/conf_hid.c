#include "conf.h"

typedef struct {
	const conf_hid_callbacks_t *cb;
	conf_hid_id_t id;
} conf_hid_t;

void *conf_hid_get_data(conf_native_t *cfg, conf_hid_id_t id, void *data)
{
	void *old = vtsp0_get(&cfg->hid_data, id, 0);
	vtsp0_set(&cfg->hid_data, id, data);
	return old;
}

void *conf_hid_get_data(conf_native_t *cfg, conf_hid_id_t id)
{
	return vtsp0_get(&cfg->hid_data, id, 0);
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

#warning TODO: get this called from conf_uninit()
void conf_hid_uninit(void)
{
	htpp_free(conf_hid_ids);
	conf_hid_ids = NULL;
}

conf_hid_id_t conf_hid_reg(const char *cookie, const conf_hid_callbacks_t *cb)
{
	int id;
	conf_hid_t *h;

	if (cookie == NULL)
		return -1;

	conf_hid_init();
	if (htpp_get_entry(conf_hid_ids, cookie) != NULL)
		return -1; /* already registered */

	h = malloc(sizeof(conf_hid_t));
	h->cb = cb;
	h->id = conf_hid_id_next++;
	htpp_set(conf_hid_ids, cookie, id);
	return id;
}

void conf_hid_unreg(const char *cookie)
{
	conf_hid_t *h = htpp_pop(conf_hid_ids, cookie);
	if (h == NULL)
		return -1;

	if ((h->cb != NULL) && (h->cb.unreg_item != NULL)) {
		htsp_entry_t *e;
		conf_fields_foreach(e) {
			int n;
			conf_native_t *cfg = e->value;
			for(n = 0; n < cfg->used; n++)
				h->cb.unreg_item(cfg, n);
		}
	}

	free(h);
}

#warning TODO: call other callbacks as well

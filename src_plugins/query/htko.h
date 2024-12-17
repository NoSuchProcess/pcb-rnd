#ifndef GENHT_HTKO_H
#define GENHT_HTKO_H

#include <librnd/core/global_typedefs.h>

typedef struct htko_key_s {
	rnd_layer_id_t lid, ko_lid;
} htko_key_t;

typedef char htko_value_t;
#define HT(x) htko_ ## x
#include <genht/ht.h>
#undef HT

int htko_keyeq(htko_key_t a, htko_key_t b);
unsigned htko_keyhash(htko_key_t a);


RND_INLINE htko_key_t htko_mkkey(rnd_layer_id_t lid, rnd_layer_id_t ko_lid)
{
	htko_key_t res;
	res.lid = lid;
	res.ko_lid = ko_lid;
	return res;
}

#endif

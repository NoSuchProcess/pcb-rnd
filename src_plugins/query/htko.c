#include "htko.h"
#define HT(x) htko_ ## x
#include <genht/ht.c>

#include <genht/hash.h>


int htko_keyeq(htko_key_t a, htko_key_t b)
{
	return (a.lid == b.lid) && (a.ko_lid == b.ko_lid);
}

unsigned htko_keyhash(htko_key_t a)
{
	return longhash(a.lid) ^ longhash(((unsigned long)a.ko_lid) << 8);
}

#undef HT

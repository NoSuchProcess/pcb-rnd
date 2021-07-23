#ifndef GENHT_HTIC_H
#define GENHT_HTIC_H

#include <librnd/core/unit.h>

typedef long int htic_key_t;
typedef rnd_coord_t htic_value_t;
#define HT(x) htic_ ## x
#include <genht/ht.h>
#undef HT

int htic_keyeq(htic_key_t a, htic_key_t b);

#endif

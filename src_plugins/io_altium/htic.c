#include "htic.h"
#define HT(x) htic_ ## x
#include <genht/ht.c>

int htic_keyeq(htic_key_t a, htic_key_t b)
{
	return a == b;
}

#undef HT

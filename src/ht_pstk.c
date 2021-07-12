#include <string.h>
#include "config.h"

#include "ht_pstk.h"


#define HT(x) htprp_ ## x
#include <genht/ht.c>
#undef HT



#ifndef PCB_HT_ELEMENT_H
#define PCB_HT_ELEMENT_H

/* hash instance */
typedef ElementType *htep_key_t;
typedef int htep_value_t;
#define HT(x) htep_ ## x
#include <liblihata/genht/ht.h>
#undef HT

#endif

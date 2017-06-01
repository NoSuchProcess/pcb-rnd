#ifndef PCB_HT_SUBC_H
#define PCB_HT_SUBC_H

/* Hash: subcircuit -> pointer */

/* hash instance */
typedef const pcb_subc_t *htscp_key_t;
typedef int htscp_value_t;
#define HT(x) htscp_ ## x
#include <genht/ht.h>
#undef HT

#endif

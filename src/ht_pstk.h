#ifndef PCB_HT_PSTK_H
#define PCB_HT_PSTK_H

#include "obj_pstk.h"

/* Hash: padstack_proto -> pointer */

/* hash instance */
#define HT_HAS_CONST_KEY 1
typedef pcb_pstk_proto_t *htprp_key_t;
typedef const pcb_pstk_proto_t *htprp_const_key_t;
typedef void * htprp_value_t;
#define HT(x) htprp_ ## x
#include <genht/ht.h>
#undef HT

#endif

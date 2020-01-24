/* central, auto-generated enum of core function IDs */

#include <librnd/core/funchash.h>

#define action_entry(x) F_ ## x,
typedef enum {
#include "funchash_core_list.h"
F_END
} pcb_function_id_t;
#undef action_entry

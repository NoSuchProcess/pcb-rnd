/* central, auto-generated enum of core function IDs */

#include "funchash.h"

#define action_entry(x) F_ ## x,
typedef enum {
#include "action_funclist.h"
F_END
} FunctionID;
#undef action_entry

#include <librnd/core/funchash.h>

#define action_entry(x) F_ ## x,
typedef enum {
F_librnd_function_offset = 16384,
#include <librnd/core/funchash_core_list.h>
F_RND_END
} rnd_function_id_t;
#undef action_entry

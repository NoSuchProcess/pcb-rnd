/* Hash: endpoint coords -> vtp0_t */

#include <librnd/core/box.h>

/* hash instance */

static vtp0_t htendp_invalid_val = {-1, -1};

#define HT_HAS_CONST_KEY 1
#define HT_INVALID_VALUE htendp_invalid_val
typedef rnd_cheap_point_t *htendp_key_t;
typedef const rnd_cheap_point_t *htendp_const_key_t;
typedef vtp0_t htendp_value_t;
#define HT(x) htendp_ ## x
#include <genht/ht.h>
#undef HT



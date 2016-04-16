#include "global.h"
#include "gedasymbols.h"
#include "plugins.h"

pcb_uninit_t hid_fp_wget_init(void)
{
	fp_gedasymbols_init();
	return NULL;
}

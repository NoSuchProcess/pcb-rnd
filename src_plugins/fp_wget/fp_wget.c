#include "config.h"
#include "gedasymbols.h"
#include "plugins.h"

void hid_fp_wget_uninit(void)
{
	fp_gedasymbols_uninit();
}

pcb_uninit_t hid_fp_wget_init(void)
{
	fp_gedasymbols_init();
	return hid_fp_wget_uninit;
}

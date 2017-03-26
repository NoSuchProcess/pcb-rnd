#include "config.h"
#include "gedasymbols.h"
#include "edakrill.h"
#include "plugins.h"
#include "fp_wget_conf.h"

conf_fp_wget_t conf_fp_wget;

void hid_fp_wget_uninit(void)
{
	fp_gedasymbols_uninit();
}

pcb_uninit_t hid_fp_wget_init(void)
{
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_fp_wget, field,isarray,type_name,cpath,cname,desc,flags);
#include "fp_wget_conf_fields.h"

	fp_gedasymbols_init();
	fp_edakrill_init();
	return hid_fp_wget_uninit;
}

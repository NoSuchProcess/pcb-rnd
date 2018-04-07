#include "config.h"
#include "gedasymbols.h"
#include "edakrill.h"
#include "plugins.h"
#include "fp_wget_conf.h"

conf_fp_wget_t conf_fp_wget;

int pplg_check_ver_fp_wget(int ver_needed) { return 0; }

void pplg_uninit_fp_wget(void)
{
	fp_gedasymbols_uninit();
	fp_edakrill_uninit();
	conf_unreg_fields("plugins/fp_wget/");
}

int pplg_init_fp_wget(void)
{
	PCB_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_fp_wget, field,isarray,type_name,cpath,cname,desc,flags);
#include "fp_wget_conf_fields.h"

	fp_gedasymbols_init();
	fp_edakrill_init();
	return 0;
}

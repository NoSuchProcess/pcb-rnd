#include "config.h"
#include "gedasymbols.h"
#include "edakrill.h"
#include "plugins.h"
#include "fp_wget_conf.h"
#include "../src_plugins/fp_wget/conf_internal.c"


conf_fp_wget_t conf_fp_wget;

#define FP_WGET_CONF_FN "fp_wget.conf"

int pplg_check_ver_fp_wget(int ver_needed) { return 0; }

void pplg_uninit_fp_wget(void)
{
	pcb_conf_unreg_file(FP_WGET_CONF_FN, fp_wget_conf_internal);
	fp_gedasymbols_uninit();
	fp_edakrill_uninit();
	pcb_conf_unreg_fields("plugins/fp_wget/");
}

int pplg_init_fp_wget(void)
{
	PCB_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_fp_wget, field,isarray,type_name,cpath,cname,desc,flags);
#include "fp_wget_conf_fields.h"

	pcb_conf_reg_file(FP_WGET_CONF_FN, fp_wget_conf_internal);

	fp_gedasymbols_init();
	fp_edakrill_init();
	return 0;
}

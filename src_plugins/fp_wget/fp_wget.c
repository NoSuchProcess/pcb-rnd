#include "config.h"
#include "gedasymbols.h"
#include "edakrill.h"
#include <librnd/core/plugins.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/conf_multi.h>
#include "fp_wget_conf.h"
#include "../src_plugins/fp_wget/conf_internal.c"


conf_fp_wget_t conf_fp_wget;
static const char fp_wget_cookie[] = "fp_wget";

int pplg_check_ver_fp_wget(int ver_needed) { return 0; }

void pplg_uninit_fp_wget(void)
{
	fp_gedasymbols_uninit();
	fp_edakrill_uninit();
	rnd_conf_plug_unreg("plugins/fp_wget/", fp_wget_conf_internal, fp_wget_cookie);
}

int pplg_init_fp_wget(void)
{
	RND_API_CHK_VER;

	rnd_conf_plug_reg(conf_fp_wget, fp_wget_conf_internal, fp_wget_cookie);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_fp_wget, field,isarray,type_name,cpath,cname,desc,flags);
#include "fp_wget_conf_fields.h"

	fp_gedasymbols_init();
	fp_edakrill_init();
	return 0;
}

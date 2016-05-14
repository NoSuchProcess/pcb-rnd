#include "conf.h"
#include "conf_core.h"
conf_core_t conf_core;

void conf_core_init()
{
#define conf_reg(field,isarray,type_name,cpath,cname,desc) \
	conf_reg_field(conf_core, field,isarray,type_name,cpath,cname,desc);
#include "conf_core_fields.h"
}

#include "config.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <genvector/gds_char.h>

TODO("win32: remove this once the w32_ dir workaround is removed")
#include <librnd/core/hid_init.h>

conf_core_t conf_core;

#define conf_clamp_to(type, var, min, max, safe_val) \
do { \
	if ((var < min) || (var > max)) \
		*((type *)(&var)) = safe_val; \
}	while(0)

#define conf_clamp(type, var, min, max) \
do { \
	if (var < min) \
		*((type *)(&var)) = min; \
	else if (var > max) \
		*((type *)(&var)) = max; \
}	while(0)

void pcb_conf_legacy(const char *dst_path, const char *legacy_path)
{
	conf_native_t *nl = pcb_conf_get_field(legacy_path);
	if (nl != NULL) {
		gds_t tmp;

		gds_init(&tmp);
		pcb_conf_print_native_field((conf_pfn)pcb_append_printf, &tmp, 0, &nl->val, nl->type, nl->prop, 0);
		if (tmp.used > 0)
			pcb_conf_set(CFR_INTERNAL, dst_path, -1, tmp.array, POL_OVERWRITE);
		gds_uninit(&tmp);
	}
	else
		pcb_message(PCB_MSG_ERROR, "drc_query: invalid legacy path '%s' for %s\n", legacy_path, dst_path);
}

static void conf_core_postproc(void)
{
	conf_clamp_to(CFT_COORD, conf_core.design.line_thickness, PCB_MIN_THICKNESS, PCB_MAX_THICKNESS, PCB_MIL_TO_COORD(10));
	conf_force_set_bool(conf_core.rc.have_regex, 1);
	pcb_conf_ro("rc/have_regex");

	conf_force_set_str(conf_core.rc.path.prefix, PCB_PREFIX);   pcb_conf_ro("rc/path/prefix");
	conf_force_set_str(conf_core.rc.path.lib, PCBLIBDIR);       pcb_conf_ro("rc/path/lib");
	conf_force_set_str(conf_core.rc.path.bin, BINDIR);          pcb_conf_ro("rc/path/bin");
	conf_force_set_str(conf_core.rc.path.share, PCBSHAREDIR);   pcb_conf_ro("rc/path/share");
}

void conf_core_init()
{
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_core, field,isarray,type_name,cpath,cname,desc,flags);
#include "conf_core_fields.h"
	pcb_conf_core_postproc = conf_core_postproc;
	conf_core_postproc();
}

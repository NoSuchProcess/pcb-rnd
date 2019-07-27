#include "config.h"
#include "conf.h"
#include "conf_core.h"
#include "hidlib_conf.h"
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

static void conf_core_postproc(void)
{
	conf_clamp_to(CFT_COORD, conf_core.design.line_thickness, PCB_MIN_LINESIZE, PCB_MAX_LINESIZE, PCB_MIL_TO_COORD(10));
	conf_clamp_to(CFT_COORD, conf_core.design.via_thickness, PCB_MIN_PINORVIASIZE, PCB_MAX_PINORVIASIZE, PCB_MIL_TO_COORD(40));
	conf_clamp_to(CFT_COORD, conf_core.design.via_drilling_hole, 0, PCB_MAX_COORD, PCB_DEFAULT_DRILLINGHOLE * conf_core.design.via_thickness / 100);
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

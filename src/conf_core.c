#include "conf.h"
#include "conf_core.h"
conf_core_t conf_core;

void conf_core_init()
{
#define conf_reg(field,isarray,type_name,cpath,cname,desc) \
	conf_reg_field(conf_core, field,isarray,type_name,cpath,cname,desc);
#include "conf_core_fields.h"
}

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

static char *get_homedir(void)
{
	char *homedir = getenv("HOME");
	if (homedir == NULL)
		homedir = getenv("USERPROFILE");
	return homedir;
}

void conf_core_postproc()
{
	conf_clamp_to(CFT_COORD, conf_core.design.line_thickness, MIN_LINESIZE, MAX_LINESIZE, MIL_TO_COORD(10));
	conf_clamp_to(CFT_COORD, conf_core.design.via_thickness, MIN_PINORVIASIZE, MAX_PINORVIASIZE, MIL_TO_COORD(40));
	conf_clamp_to(CFT_COORD, conf_core.design.via_drilling_hole, 0, MAX_COORD, DEFAULT_DRILLINGHOLE * conf_core.design.via_thickness / 100);
	conf_clamp(CFT_COORD, conf_core.design.max_width, MIN_SIZE, MAX_COORD);
	conf_clamp(CFT_COORD, conf_core.design.max_height, MIN_SIZE, MAX_COORD);
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
	conf_force_set_bool(conf_core.rc.have_regex, 1);
#else
	conf_force_set_bool(conf_core.rc.have_regex, 0);
#endif

	conf_force_set_str(conf_core.rc.path.prefix, PCB_PREFIX);
	conf_force_set_str(conf_core.rc.path.lib, PCBLIBDIR);
	conf_force_set_str(conf_core.rc.path.bin, BINDIR);
	conf_force_set_str(conf_core.rc.path.share, PCBSHAREDIR);
	conf_force_set_str(conf_core.rc.path.home, get_homedir());
}
